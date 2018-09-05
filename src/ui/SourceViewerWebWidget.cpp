/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Menu.h"
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
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

SourceViewerWebWidget::SourceViewerWebWidget(bool isPrivate, ContentsWidget *parent) : WebWidget({}, nullptr, parent),
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

	connect(this, &SourceViewerWebWidget::customContextMenuRequested, this, &SourceViewerWebWidget::showContextMenu);
	connect(m_sourceViewer, &SourceViewerWidget::zoomChanged, this, &SourceViewerWebWidget::zoomChanged);
	connect(m_sourceViewer, &SourceViewerWidget::zoomChanged, this, &SourceViewerWebWidget::handleZoomChanged);
	connect(m_sourceViewer, &SourceViewerWidget::redoAvailable, this, &SourceViewerWebWidget::notifyRedoActionStateChanged);
	connect(m_sourceViewer, &SourceViewerWidget::undoAvailable, this, &SourceViewerWebWidget::notifyUndoActionStateChanged);
	connect(m_sourceViewer, &SourceViewerWidget::copyAvailable, this, &SourceViewerWebWidget::notifyEditingActionsStateChanged);
	connect(m_sourceViewer, &SourceViewerWidget::cursorPositionChanged, this, &SourceViewerWebWidget::notifyEditingActionsStateChanged);
}

void SourceViewerWebWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	switch (identifier)
	{
		case ActionsManager::SaveAction:
			{
				const QString path(Utils::getSavePath(suggestSaveFileName(SingleFileSaveFormat)).path);

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

			break;
		case ActionsManager::StopAction:
			if (m_viewSourceReply)
			{
				disconnect(m_viewSourceReply, &QNetworkReply::finished, this, &SourceViewerWebWidget::handleViewSourceReplyFinished);

				m_viewSourceReply->abort();
				m_viewSourceReply->deleteLater();
				m_viewSourceReply = nullptr;
			}

			m_isLoading = false;

			emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::NavigationCategory});
			emit loadingStateChanged(WebWidget::FinishedLoadingState);

			break;
		case ActionsManager::ReloadAction:
			{
				if (m_sourceViewer->document()->isModified())
				{
					const QMessageBox::StandardButton result(QMessageBox::warning(this, tr("Warning"), tr("The document has been modified.\nDo you want to save your changes or discard them?"), (QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel)));

					if (result == QMessageBox::Cancel)
					{
						break;
					}

					if (result == QMessageBox::Save)
					{
						triggerAction(ActionsManager::SaveAction, {}, trigger);
					}
				}

				triggerAction(ActionsManager::StopAction, {}, trigger);

				QNetworkRequest request(QUrl(getUrl().toString().mid(12)));
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
				request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());

				if (!m_networkManager)
				{
					m_networkManager = new NetworkManager(m_isPrivate, this);
				}

				m_viewSourceReply = m_networkManager->get(request);

				connect(m_viewSourceReply, &QNetworkReply::finished, this, &SourceViewerWebWidget::handleViewSourceReplyFinished);

				m_isLoading = true;

				emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::NavigationCategory});
				emit loadingStateChanged(WebWidget::OngoingLoadingState);
			}

			break;
		case ActionsManager::ReloadOrStopAction:
			if (m_isLoading)
			{
				triggerAction(ActionsManager::StopAction, {}, trigger);
			}
			else
			{
				triggerAction(ActionsManager::ReloadAction, {}, trigger);
			}

			break;
		case ActionsManager::ReloadAndBypassCacheAction:
			{
				triggerAction(ActionsManager::StopAction, {}, trigger);

				QNetworkRequest request(QUrl(getUrl().toString().mid(12)));
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
				request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());

				if (!m_networkManager)
				{
					m_networkManager = new NetworkManager(m_isPrivate, this);
				}

				m_viewSourceReply = m_networkManager->get(request);

				connect(m_viewSourceReply, &QNetworkReply::finished, this, &SourceViewerWebWidget::handleViewSourceReplyFinished);

				m_isLoading = true;

				emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::NavigationCategory});
				emit loadingStateChanged(WebWidget::OngoingLoadingState);
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
			NotesManager::addNote(BookmarksModel::UrlBookmark, {{BookmarksModel::UrlRole, getUrl()}, {BookmarksModel::DescriptionRole, getSelectedText()}});

			break;
		case ActionsManager::PasteAction:
			if (parameters.contains(QLatin1String("note")))
			{
				const BookmarksModel::Bookmark *bookmark(NotesManager::getModel()->getBookmark(parameters[QLatin1String("note")].toULongLong()));

				if (bookmark)
				{
					triggerAction(ActionsManager::PasteAction, {{QLatin1String("text"), bookmark->getDescription()}}, trigger);
				}
			}
			else if (parameters.contains(QLatin1String("text")))
			{
				m_sourceViewer->textCursor().insertText(parameters[QLatin1String("text")].toString());
			}
			else
			{
				m_sourceViewer->paste();
			}

			break;
		case ActionsManager::DeleteAction:
			m_sourceViewer->textCursor().removeSelectedText();

			break;
		case ActionsManager::SelectAllAction:
			m_sourceViewer->selectAll();

			break;
		case ActionsManager::UnselectAction:
			m_sourceViewer->textCursor().clearSelection();

			break;
		case ActionsManager::ClearAllAction:
			m_sourceViewer->selectAll();
			m_sourceViewer->textCursor().removeSelectedText();

			break;
		case ActionsManager::ActivateContentAction:
			m_sourceViewer->setFocus();

			break;
		default:
			break;
	}
}

void SourceViewerWebWidget::print(QPrinter *printer)
{
	m_sourceViewer->print(printer);
}

void SourceViewerWebWidget::handleViewSourceReplyFinished()
{
	if (m_viewSourceReply)
	{
		setContents(m_viewSourceReply->readAll(), m_viewSourceReply->header(QNetworkRequest::ContentTypeHeader).toString());

		m_viewSourceReply->deleteLater();
		m_viewSourceReply = nullptr;

		emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::NavigationCategory});
	}
}

void SourceViewerWebWidget::handleZoomChanged()
{
	SessionsManager::markSessionAsModified();
}

void SourceViewerWebWidget::notifyEditingActionsStateChanged()
{
	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::EditingCategory});
}

void SourceViewerWebWidget::showContextMenu(const QPoint &position)
{
	updateHitTestResult(position);

	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::EditingCategory});

	const QWidget *child(childAt(position.isNull() ? mapFromGlobal(QCursor::pos()) : position));
	const QPoint menuPosition(position.isNull() ? QCursor::pos() : mapToGlobal(position));

	if (child && child->metaObject()->className() == QLatin1String("Otter::MarginWidget"))
	{
		QMenu menu(this);
		QAction *showLineNumbersAction(menu.addAction(tr("Show Line Numbers"), [&](bool show)
		{
			SettingsManager::setOption(SettingsManager::SourceViewer_ShowLineNumbersOption, show);
		}));
		showLineNumbersAction->setCheckable(true);
		showLineNumbersAction->setChecked(SettingsManager::getOption(SettingsManager::SourceViewer_ShowLineNumbersOption).toBool());

		menu.exec(menuPosition);
	}
	else
	{
		Menu menu(Menu::UnknownMenu, this);
		menu.load(QLatin1String("menu/webWidget.json"), {QLatin1String("edit"), QLatin1String("source")}, ActionExecutor::Object(this, this));
		menu.exec(menuPosition);
	}
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
	m_sourceViewer->horizontalScrollBar()->setValue(position.x());
	m_sourceViewer->verticalScrollBar()->setValue(position.y());
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

		SessionsManager::markSessionAsModified();

		emit zoomChanged(zoom);
	}
}

void SourceViewerWebWidget::setUrl(const QUrl &url, bool isTyped)
{
	triggerAction(ActionsManager::StopAction);

	m_url = url;

	if (isTyped)
	{
		triggerAction(ActionsManager::ReloadAction);
	}
}

void SourceViewerWebWidget::setContents(const QByteArray &contents, const QString &contentType)
{
	triggerAction(ActionsManager::StopAction);

	const QTextCodec *codec(nullptr);

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

	if (codec)
	{
		m_sourceViewer->setPlainText(codec->toUnicode(contents));
	}
	else
	{
		m_sourceViewer->setPlainText(QString(contents));
	}

	m_sourceViewer->document()->setModified(false);
}

WebWidget* SourceViewerWebWidget::clone(bool cloneHistory, bool isPrivate, const QStringList &excludedOptions) const
{
	Q_UNUSED(cloneHistory)
	Q_UNUSED(isPrivate)
	Q_UNUSED(excludedOptions)

	return nullptr;
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

QPoint SourceViewerWebWidget::getScrollPosition() const
{
	return {m_sourceViewer->horizontalScrollBar()->value(), m_sourceViewer->verticalScrollBar()->value()};
}

ActionsManager::ActionDefinition::State SourceViewerWebWidget::getActionState(int identifier, const QVariantMap &parameters) const
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
			return WebWidget::getActionState(identifier, parameters);
		default:
			break;
	}

	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());
	state.isEnabled = false;

	return state;
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
	result.hitPosition = position;
	result.flags = HitTestResult::IsContentEditableTest;

	if (m_sourceViewer->document()->isEmpty())
	{
		result.flags |= HitTestResult::IsEmptyTest;
	}

	return result;
}

WebWidget::LoadingState SourceViewerWebWidget::getLoadingState() const
{
	return (m_isLoading ? WebWidget::OngoingLoadingState : WebWidget::FinishedLoadingState);
}

int SourceViewerWebWidget::getZoom() const
{
	return m_sourceViewer->getZoom();
}

int SourceViewerWebWidget::findInPage(const QString &text, WebWidget::FindFlags flags)
{
	return m_sourceViewer->findText(text, flags);
}

bool SourceViewerWebWidget::canRedo() const
{
	return m_sourceViewer->document()->isRedoAvailable();
}

bool SourceViewerWebWidget::canUndo() const
{
	return m_sourceViewer->document()->isUndoAvailable();
}

bool SourceViewerWebWidget::canViewSource() const
{
	return false;
}

bool SourceViewerWebWidget::hasSelection() const
{
	return m_sourceViewer->textCursor().hasSelection();
}

bool SourceViewerWebWidget::isPrivate() const
{
	return m_isPrivate;
}

}
