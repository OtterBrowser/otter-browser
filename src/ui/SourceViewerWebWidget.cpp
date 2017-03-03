/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "SourceViewerWebWidget.h"
#include "SourceViewerWidget.h"
#include "../core/Console.h"
#include "../core/HistoryManager.h"
#include "../core/NetworkManager.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/NotesManager.h"
#include "../core/Utils.h"

#include <QtCore/QFile>
#include <QtCore/QTextCodec>
#include <QtGui/QClipboard>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

SourceViewerWebWidget::SourceViewerWebWidget(bool isPrivate, ContentsWidget *parent) : WebWidget(isPrivate, nullptr, parent),
	m_sourceViewer(new SourceViewerWidget(this)),
	m_networkManager(nullptr),
	m_viewSourceReply(nullptr),
	m_isLoading(true),
	m_isPrivate(isPrivate)
{
	QVBoxLayout *layout(new QVBoxLayout(this));
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

void SourceViewerWebWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	if (parameters.contains(QLatin1String("isBounced")))
	{
		return;
	}

	switch (identifier)
	{
		case ActionsManager::SaveAction:
			{
				const QString path(Utils::getSavePath(suggestSaveFileName(SingleHtmlFileSaveFormat)).path);

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
						Console::addMessage(tr("Failed to save file: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, path);

						QMessageBox::warning(nullptr, tr("Error"), tr("Failed to save file."), QMessageBox::Close);
					}
				}
			}

			return;
		case ActionsManager::StopAction:
			if (m_viewSourceReply)
			{
				disconnect(m_viewSourceReply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));

				m_viewSourceReply->abort();
				m_viewSourceReply->deleteLater();
				m_viewSourceReply = nullptr;
			}

			m_isLoading = false;

			emit loadingStateChanged(WindowsManager::FinishedLoadingState);

			updateNavigationActions();

			return;
		case ActionsManager::ReloadAction:
			{
				if (m_sourceViewer->document()->isModified())
				{
					const QMessageBox::StandardButton result(QMessageBox::warning(this, tr("Warning"), tr("The document has been modified.\nDo you want to save your changes or discard them?"), (QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel)));

					if (result == QMessageBox::Cancel)
					{
						return;
					}

					if (result == QMessageBox::Save)
					{
						triggerAction(ActionsManager::SaveAction);
					}
				}

				triggerAction(ActionsManager::StopAction);

				QNetworkRequest request(QUrl(getUrl().toString().mid(12)));
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

				if (!m_networkManager)
				{
					m_networkManager = new NetworkManager(m_isPrivate, this);
				}

				m_viewSourceReply = m_networkManager->get(request);

				connect(m_viewSourceReply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));

				m_isLoading = true;

				emit loadingStateChanged(WindowsManager::OngoingLoadingState);

				updateNavigationActions();
			}

			return;
		case ActionsManager::ReloadOrStopAction:
			if (m_isLoading)
			{
				triggerAction(ActionsManager::StopAction);
			}
			else
			{
				triggerAction(ActionsManager::ReloadAction);
			}

			return;
		case ActionsManager::ReloadAndBypassCacheAction:
			{
				triggerAction(ActionsManager::StopAction);

				QNetworkRequest request(QUrl(getUrl().toString().mid(12)));
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

				m_viewSourceReply = NetworkManagerFactory::getNetworkManager()->get(request);

				connect(m_viewSourceReply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));

				m_isLoading = true;

				emit loadingStateChanged(WindowsManager::OngoingLoadingState);

				updateNavigationActions();
			}

			return;
		case ActionsManager::ContextMenuAction:
			showContextMenu();

			return;
		case ActionsManager::UndoAction:
			m_sourceViewer->undo();

			return;
		case ActionsManager::RedoAction:
			m_sourceViewer->redo();

			return;
		case ActionsManager::CutAction:
			m_sourceViewer->cut();

			return;
		case ActionsManager::CopyAction:
		case ActionsManager::CopyPlainTextAction:
			m_sourceViewer->copy();

			return;
		case ActionsManager::CopyAddressAction:
			QApplication::clipboard()->setText(getUrl().toString());

			return;
		case ActionsManager::CopyToNoteAction:
			{
				BookmarksItem *note(NotesManager::addNote(BookmarksModel::UrlBookmark, getUrl()));
				note->setData(getSelectedText(), BookmarksModel::DescriptionRole);
			}

			return;
		case ActionsManager::PasteAction:
			m_sourceViewer->paste();

			return;
		case ActionsManager::DeleteAction:
			m_sourceViewer->textCursor().removeSelectedText();

			return;
		case ActionsManager::SelectAllAction:
			m_sourceViewer->selectAll();

			return;
		case ActionsManager::UnselectAction:
			m_sourceViewer->textCursor().clearSelection();

			return;
		case ActionsManager::ClearAllAction:
			m_sourceViewer->selectAll();
			m_sourceViewer->textCursor().removeSelectedText();

			return;
		case ActionsManager::ActivateContentAction:
			m_sourceViewer->setFocus();

			return;
	}

	bounceAction(identifier, parameters);
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
		setContents(m_viewSourceReply->readAll(), m_viewSourceReply->header(QNetworkRequest::ContentTypeHeader).toString());

		m_viewSourceReply->deleteLater();
		m_viewSourceReply = nullptr;

		updateNavigationActions();
	}
}

void SourceViewerWebWidget::goToHistoryIndex(int index)
{
	Q_UNUSED(index)
}

void SourceViewerWebWidget::removeHistoryIndex(int index, bool purge)
{
	Q_UNUSED(index)
	Q_UNUSED(purge)
}

void SourceViewerWebWidget::handleZoomChange()
{
	SessionsManager::markSessionModified();
}

void SourceViewerWebWidget::showContextMenu(const QPoint &position)
{
	updateHitTestResult(position);
	updateEditActions();

	QWidget *child(childAt(position.isNull() ? mapFromGlobal(QCursor::pos()) : position));
	QMenu menu;

	if (child && child->metaObject()->className() == QLatin1String("Otter::MarginWidget"))
	{
		QAction *showLineNumbersAction(menu.addAction(tr("Show Line Numbers")));
		showLineNumbersAction->setCheckable(true);
		showLineNumbersAction->setChecked(SettingsManager::getOption(SettingsManager::SourceViewer_ShowLineNumbersOption).toBool());

		connect(showLineNumbersAction, SIGNAL(triggered(bool)), this, SLOT(setShowLineNumbers(bool)));
	}
	else
	{
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
	}

	menu.exec(position.isNull() ? QCursor::pos() : mapToGlobal(position));
}

void SourceViewerWebWidget::setShowLineNumbers(bool show)
{
	SettingsManager::setValue(SettingsManager::SourceViewer_ShowLineNumbersOption, show);
}

void SourceViewerWebWidget::setOption(int identifier, const QVariant &value)
{
	const bool needsReload(identifier == SettingsManager::Content_DefaultCharacterEncodingOption && getOption(identifier).toString() != value.toString());

	WebWidget::setOption(identifier, value);

	if (needsReload)
	{
		triggerAction(ActionsManager::ReloadAction);
	}
}

void SourceViewerWebWidget::setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions)
{
	WebWidget::setOptions(options, excludedOptions);

	if (options.contains(SettingsManager::Content_DefaultCharacterEncodingOption))
	{
		setOption(SettingsManager::Content_DefaultCharacterEncodingOption, options[SettingsManager::Content_DefaultCharacterEncodingOption]);
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

void SourceViewerWebWidget::setContents(const QByteArray &contents, const QString &contentType)
{
	triggerAction(ActionsManager::StopAction);

	QTextCodec *codec(nullptr);

	if (hasOption(SettingsManager::Content_DefaultCharacterEncodingOption))
	{
		const QString encoding(getOption(SettingsManager::Content_DefaultCharacterEncodingOption).toString());

		if (encoding != QLatin1String("auto"))
		{
			codec = QTextCodec::codecForName(encoding.toLatin1());
		}
	}

	if (!codec && !contentType.isEmpty() && contentType.contains(QLatin1String("charset=")))
	{
		codec = QTextCodec::codecForName(contentType.mid(contentType.indexOf(QLatin1String("charset=")) + 8).toLatin1());
	}

	if (!codec)
	{
		codec = QTextCodec::codecForHtml(contents);
	}

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
	m_sourceViewer->document()->setModified(false);
}

WebWidget* SourceViewerWebWidget::clone(bool cloneHistory, bool isPrivate, const QStringList &excludedOptions)
{
	Q_UNUSED(cloneHistory)
	Q_UNUSED(isPrivate)
	Q_UNUSED(excludedOptions)

	return nullptr;
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
		case ActionsManager::FindAction:
		case ActionsManager::FindNextAction:
		case ActionsManager::FindPreviousAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ActivateContentAction:
			break;
		default:
			return nullptr;
	}

	if (identifier == ActionsManager::UndoAction && !getExistingAction(ActionsManager::UndoAction))
	{
		Action *action(WebWidget::getAction(ActionsManager::UndoAction));
		action->setEnabled(m_sourceViewer->document()->isUndoAvailable());

		connect(m_sourceViewer, SIGNAL(undoAvailable(bool)), action, SLOT(setEnabled(bool)));

		return action;
	}

	if (identifier == ActionsManager::RedoAction && !getExistingAction(ActionsManager::RedoAction))
	{
		Action *action(WebWidget::getAction(ActionsManager::RedoAction));
		action->setEnabled(m_sourceViewer->document()->isRedoAvailable());

		connect(m_sourceViewer, SIGNAL(redoAvailable(bool)), action, SLOT(setEnabled(bool)));

		return action;
	}

	return WebWidget::getAction(identifier);
}

QString SourceViewerWebWidget::getTitle() const
{
	return (m_url.isValid() ? tr("Source Viewer: %1").arg(m_url.toDisplayString().mid(12)) : tr("Source Viewer"));
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

WindowHistoryInformation SourceViewerWebWidget::getHistory() const
{
	WindowHistoryEntry entry;
	entry.url = getUrl().toString();
	entry.title = getTitle();
	entry.zoom = getZoom();

	WindowHistoryInformation information;
	information.entries.append(entry);
	information.index = 0;

	return information;
}

WebWidget::HitTestResult SourceViewerWebWidget::getHitTestResult(const QPoint &position)
{
	HitTestResult result;
	result.position = position;
	result.flags = IsContentEditableTest;

	if (m_sourceViewer->document()->isEmpty())
	{
		result.flags |= IsEmptyTest;
	}

	return result;
}

WindowsManager::LoadingState SourceViewerWebWidget::getLoadingState() const
{
	return (m_isLoading ? WindowsManager::OngoingLoadingState : WindowsManager::FinishedLoadingState);
}

int SourceViewerWebWidget::getZoom() const
{
	return m_sourceViewer->getZoom();
}

bool SourceViewerWebWidget::hasSelection() const
{
	return m_sourceViewer->textCursor().hasSelection();
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
