/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "SourceViewerWebWidget.h"
#include "SourceViewerWidget.h"
#include "../core/Console.h"
#include "../core/HistoryManager.h"
#include "../core/NetworkManager.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/NotesManager.h"
#include "../core/SearchesManager.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"

#include <QtCore/QFile>
#include <QtCore/QTextCodec>
#include <QtGui/QClipboard>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

SourceViewerWebWidget::SourceViewerWebWidget(bool isPrivate, ContentsWidget *parent) : WebWidget(isPrivate, NULL, parent),
	m_sourceViewer(new SourceViewerWidget(this)),
	m_viewSourceReply(NULL),
	m_isLoading(true),
	m_isPrivate(isPrivate)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(m_sourceViewer);

	m_sourceViewer->setContextMenuPolicy(Qt::NoContextMenu);

	setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_sourceViewer, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
	connect(m_sourceViewer, SIGNAL(zoomChanged(int)), this, SLOT(handleZoomChange()));
	connect(m_sourceViewer, SIGNAL(copyAvailable(bool)), this, SLOT(updateEditActions()));
}

void SourceViewerWebWidget::triggerAction(int identifier, bool checked)
{
	Q_UNUSED(checked)

	switch (identifier)
	{
		case ActionsManager::SaveAction:
			{
				const QString path = TransfersManager::getSavePath(suggestSaveFileName());

				if (!path.isEmpty())
				{
					QFile file(path);

					if (file.open(QIODevice::WriteOnly))
					{
						QTextStream stream(&file);
						stream << m_sourceViewer->toPlainText();

						file.close();
					}
					else
					{
						Console::addMessage(tr("Failed to save file: %1").arg(file.errorString()), OtherMessageCategory, ErrorMessageLevel, path);

						QMessageBox::warning(NULL, tr("Error"), tr("Failed to save file."), QMessageBox::Close);
					}
				}
			}

			break;
		case ActionsManager::StopAction:
			if (m_viewSourceReply)
			{
				disconnect(m_viewSourceReply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));

				m_viewSourceReply->abort();
				m_viewSourceReply->deleteLater();
				m_viewSourceReply = NULL;

				m_isLoading = false;

				emit loadingChanged(false);
			}

			updateNavigationActions();

			break;
		case ActionsManager::ReloadAction:
			{
				triggerAction(ActionsManager::StopAction);

				QNetworkRequest request(QUrl(getUrl().toString().mid(12)));
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

				m_viewSourceReply = NetworkManagerFactory::getNetworkManager()->get(request);

				connect(m_viewSourceReply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));

				m_isLoading = true;

				emit loadingChanged(true);

				updateNavigationActions();
			}

			break;
		case ActionsManager::ReloadOrStopAction:
			if (m_isLoading)
			{
				triggerAction(ActionsManager::StopAction);
			}
			else
			{
				triggerAction(ActionsManager::ReloadAction);
			}

			break;
		case ActionsManager::ReloadAndBypassCacheAction:
			{
				triggerAction(ActionsManager::StopAction);

				QNetworkRequest request(QUrl(getUrl().toString().mid(12)));
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

				m_viewSourceReply = NetworkManagerFactory::getNetworkManager()->get(request);

				connect(m_viewSourceReply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));

				m_isLoading = true;

				emit loadingChanged(true);

				updateNavigationActions();
			}

			break;
		case ActionsManager::ContextMenuAction:
			showContextMenu();

			break;
		case ActionsManager::UndoAction:
			m_sourceViewer->undo();

			break;
		case ActionsManager::RedoAction:
			m_sourceViewer->redo();

			break;
		case ActionsManager::CutAction:
			m_sourceViewer->cut();

			break;
		case ActionsManager::CopyAction:
		case ActionsManager::CopyPlainTextAction:
			m_sourceViewer->copy();

			break;
		case ActionsManager::CopyAddressAction:
			QApplication::clipboard()->setText(getUrl().toString());

			break;
		case ActionsManager::CopyToNoteAction:
			{
				BookmarksItem *note = NotesManager::addNote(BookmarksModel::UrlBookmark, getUrl());
				note->setData(getSelectedText(), BookmarksModel::DescriptionRole);
			}

			break;
		case ActionsManager::PasteAction:
			m_sourceViewer->paste();

			break;
		case ActionsManager::DeleteAction:
			m_sourceViewer->textCursor().removeSelectedText();

			break;
		case ActionsManager::SelectAllAction:
			m_sourceViewer->selectAll();

			break;
		case ActionsManager::ClearAllAction:
			m_sourceViewer->selectAll();
			m_sourceViewer->textCursor().removeSelectedText();

			break;
		case ActionsManager::SearchAction:
			quickSearch(getAction(ActionsManager::SearchAction));

			break;
		case ActionsManager::ActivateContentAction:
			m_sourceViewer->setFocus();

			break;
	}
}

void SourceViewerWebWidget::print(QPrinter *printer)
{
	m_sourceViewer->print(printer);
}

void SourceViewerWebWidget::pasteText(const QString &text)
{
	Q_UNUSED(text)
}

void SourceViewerWebWidget::viewSourceReplyFinished()
{
	if (m_viewSourceReply)
	{
		setContents(m_viewSourceReply->readAll());

		m_viewSourceReply->deleteLater();
		m_viewSourceReply = NULL;

		updateNavigationActions();
	}
}

void SourceViewerWebWidget::clearSelection()
{
	m_sourceViewer->textCursor().clearSelection();
}

void SourceViewerWebWidget::goToHistoryIndex(int index)
{
	Q_UNUSED(index)
}

void SourceViewerWebWidget::handleZoomChange()
{
	SessionsManager::markSessionModified();
}

void SourceViewerWebWidget::updateNavigationActions()
{
	if (m_actions.contains(ActionsManager::StopAction))
	{
		m_actions[ActionsManager::StopAction]->setEnabled(m_isLoading);
	}

	if (m_actions.contains(ActionsManager::ReloadAction))
	{
		m_actions[ActionsManager::ReloadAction]->setEnabled(!m_isLoading);
	}

	if (m_actions.contains(ActionsManager::ReloadOrStopAction))
	{
		m_actions[ActionsManager::ReloadOrStopAction]->setup(m_isLoading ? getAction(ActionsManager::StopAction) : getAction(ActionsManager::ReloadAction));
	}
}

void SourceViewerWebWidget::showContextMenu(const QPoint &position)
{
	QMenu menu;
	menu.addAction(getAction(ActionsManager::UndoAction));
	menu.addAction(getAction(ActionsManager::RedoAction));
	menu.addSeparator();
	menu.addAction(getAction(ActionsManager::CutAction));
	menu.addAction(getAction(ActionsManager::CopyAction));
	menu.addAction(getAction(ActionsManager::PasteAction));
	menu.addAction(getAction(ActionsManager::DeleteAction));
	menu.addSeparator();
	menu.addAction(getAction(ActionsManager::SelectAllAction));
	menu.addAction(getAction(ActionsManager::ClearAllAction));
	menu.addSeparator();
	menu.exec(position.isNull() ? QCursor::pos() : mapToGlobal(position));
}

void SourceViewerWebWidget::updateEditActions()
{
	const bool hasSelection = !getSelectedText().isEmpty();

	if (m_actions.contains(ActionsManager::CutAction))
	{
		m_actions[ActionsManager::CutAction]->setEnabled(hasSelection);
	}

	if (m_actions.contains(ActionsManager::CopyAction))
	{
		m_actions[ActionsManager::CopyAction]->setEnabled(hasSelection);
	}

	if (m_actions.contains(ActionsManager::CopyPlainTextAction))
	{
		m_actions[ActionsManager::CopyPlainTextAction]->setEnabled(hasSelection);
	}

	if (m_actions.contains(ActionsManager::CopyToNoteAction))
	{
		m_actions[ActionsManager::CopyToNoteAction]->setEnabled(hasSelection);
	}

	if (m_actions.contains(ActionsManager::PasteAction))
	{
		m_actions[ActionsManager::PasteAction]->setEnabled(m_sourceViewer->canPaste());
	}

	if (m_actions.contains(ActionsManager::PasteNoteAction))
	{
		m_actions[ActionsManager::PasteNoteAction]->setEnabled(m_sourceViewer->canPaste() && NotesManager::getModel()->getRootItem()->hasChildren());
	}

	if (m_actions.contains(ActionsManager::DeleteAction))
	{
		m_actions[ActionsManager::DeleteAction]->setEnabled(hasSelection);
	}

	if (m_actions.contains(ActionsManager::SelectAllAction))
	{
		m_actions[ActionsManager::SelectAllAction]->setEnabled(m_sourceViewer->document()->characterCount() > 0);
	}

	if (m_actions.contains(ActionsManager::ClearAllAction))
	{
		m_actions[ActionsManager::ClearAllAction]->setEnabled(m_sourceViewer->document()->characterCount() > 0);
	}

	if (m_actions.contains(ActionsManager::SearchAction))
	{
		const SearchInformation engine = SearchesManager::getSearchEngine(getOption(QLatin1String("Search/DefaultQuickSearchEngine")).toString());
		const bool isValid = !engine.identifier.isEmpty();

		m_actions[ActionsManager::SearchAction]->setEnabled(isValid);
		m_actions[ActionsManager::SearchAction]->setData(isValid ? engine.identifier : QVariant());
		m_actions[ActionsManager::SearchAction]->setIcon((!isValid || engine.icon.isNull()) ? Utils::getIcon(QLatin1String("edit-find")) : engine.icon);
		m_actions[ActionsManager::SearchAction]->setOverrideText(isValid ? engine.title : QT_TRANSLATE_NOOP("actions", "Search"));
		m_actions[ActionsManager::SearchAction]->setToolTip(isValid ? engine.description : tr("No search engines defined"));
	}

	if (m_actions.contains(ActionsManager::SearchMenuAction))
	{
		m_actions[ActionsManager::SearchMenuAction]->setEnabled(SearchesManager::getSearchEngines().count() > 1);
	}
}

void SourceViewerWebWidget::setScrollPosition(const QPoint &position)
{
	Q_UNUSED(position)
}

void SourceViewerWebWidget::setHistory(const WindowHistoryInformation &history)
{
	Q_UNUSED(history)
}

void SourceViewerWebWidget::setZoom(int zoom)
{
	if (zoom != m_sourceViewer->getZoom())
	{
		m_sourceViewer->setZoom(zoom);

		SessionsManager::markSessionModified();

		emit zoomChanged(zoom);
	}
}

void SourceViewerWebWidget::setUrl(const QUrl &url, bool typed)
{
	triggerAction(ActionsManager::StopAction);

	m_url = url;

	if (typed)
	{
		triggerAction(ActionsManager::ReloadAction);
	}
}

void SourceViewerWebWidget::setContents(const QByteArray &contents)
{
	QTextCodec *codec = QTextCodec::codecForHtml(contents);
	QString text;

	if (codec)
	{
		text = codec->toUnicode(contents);
	}
	else
	{
		text = QString(contents);
	}

	m_sourceViewer->setPlainText(text);

	m_isLoading = false;

	emit loadingChanged(false);
}

WebWidget* SourceViewerWebWidget::clone(bool cloneHistory)
{
	Q_UNUSED(cloneHistory)

	return NULL;
}

Action* SourceViewerWebWidget::getAction(int identifier)
{
	switch (identifier)
	{
		case ActionsManager::SaveAction:
		case ActionsManager::StopAction:
		case ActionsManager::ReloadAction:
		case ActionsManager::ReloadOrStopAction:
		case ActionsManager::ReloadAndBypassCacheAction:
		case ActionsManager::ContextMenuAction:
		case ActionsManager::UndoAction:
		case ActionsManager::RedoAction:
		case ActionsManager::CutAction:
		case ActionsManager::CopyAction:
		case ActionsManager::CopyPlainTextAction:
		case ActionsManager::CopyAddressAction:
		case ActionsManager::CopyToNoteAction:
		case ActionsManager::PasteAction:
		case ActionsManager::DeleteAction:
		case ActionsManager::SelectAllAction:
		case ActionsManager::ClearAllAction:
		case ActionsManager::ZoomInAction:
		case ActionsManager::ZoomOutAction:
		case ActionsManager::ZoomOriginalAction:
		case ActionsManager::SearchAction:
		case ActionsManager::SearchMenuAction:
		case ActionsManager::FindAction:
		case ActionsManager::FindNextAction:
		case ActionsManager::FindPreviousAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ActivateContentAction:
			break;
		default:
			return NULL;
	}

	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	Action *action = new Action(identifier, this);

	m_actions[identifier] = action;

	connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

	switch (identifier)
	{
		case ActionsManager::StopAction:
			action->setEnabled(m_isLoading);

			break;

		case ActionsManager::ReloadAction:
			action->setEnabled(!m_isLoading);

			break;
		case ActionsManager::ReloadOrStopAction:
			action->setup(m_isLoading ? getAction(ActionsManager::StopAction) : getAction(ActionsManager::ReloadAction));

			break;
		case ActionsManager::PasteNoteAction:
			action->setMenu(getPasteNoteMenu());

			updateEditActions();

			break;
		case ActionsManager::UndoAction:
			action->setEnabled(false);

			connect(m_sourceViewer, SIGNAL(undoAvailable(bool)), action, SLOT(setEnabled(bool)));

			break;
		case ActionsManager::RedoAction:
			action->setEnabled(false);

			connect(m_sourceViewer, SIGNAL(redoAvailable(bool)), action, SLOT(setEnabled(bool)));

			break;
		case ActionsManager::SearchMenuAction:
			action->setMenu(getQuickSearchMenu());

		case ActionsManager::CutAction:
		case ActionsManager::CopyAction:
		case ActionsManager::CopyPlainTextAction:
		case ActionsManager::CopyToNoteAction:
		case ActionsManager::PasteAction:
		case ActionsManager::PasteAndGoAction:
		case ActionsManager::DeleteAction:
		case ActionsManager::ClearAllAction:
		case ActionsManager::SearchAction:
			updateEditActions();

			break;
		default:
			break;
	}

	return action;
}

QString SourceViewerWebWidget::getTitle() const
{
	return tr("Source Viewer");
}

QString SourceViewerWebWidget::getSelectedText() const
{
	return m_sourceViewer->textCursor().selectedText();
}

QUrl SourceViewerWebWidget::getUrl() const
{
	return m_url;
}

QIcon SourceViewerWebWidget::getIcon() const
{
	return HistoryManager::getIcon(getRequestedUrl());
}

QPixmap SourceViewerWebWidget::getThumbnail()
{
	return QPixmap();
}

QPoint SourceViewerWebWidget::getScrollPosition() const
{
	return QPoint();
}

QRect SourceViewerWebWidget::getProgressBarGeometry() const
{
	return QRect();
}

WindowHistoryInformation SourceViewerWebWidget::getHistory() const
{
	WindowHistoryEntry entry;
	entry.url = getUrl().toString();
	entry.title = getTitle();
	entry.zoom  = getZoom();

	WindowHistoryInformation information;
	information.entries.append(entry);
	information.index = 0;

	return information;
}

int SourceViewerWebWidget::getZoom() const
{
	return m_sourceViewer->getZoom();
}

bool SourceViewerWebWidget::isLoading() const
{
	return m_isLoading;
}

bool SourceViewerWebWidget::isPrivate() const
{
	return m_isPrivate;
}

bool SourceViewerWebWidget::findInPage(const QString &text, WebWidget::FindFlags flags)
{
	return m_sourceViewer->findText(text, flags);
}

}
