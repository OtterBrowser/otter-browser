/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#include "WebWidget.h"
#include "ContentsDialog.h"
#include "ContentsWidget.h"
#include "Menu.h"
#include "ReloadTimeDialog.h"
#include "TransferDialog.h"
#include "../core/ActionsManager.h"
#include "../core/BookmarksManager.h"
#include "../core/GesturesManager.h"
#include "../core/NotesManager.h"
#include "../core/SearchesManager.h"
#include "../core/SettingsManager.h"
#include "../core/Transfer.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"

#include <QtCore/QDir>
#include <QtCore/QMimeData>
#include <QtCore/QMimeDatabase>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>

namespace Otter
{

WebWidget::WebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent) : QWidget(parent),
	m_backend(backend),
	m_pasteNoteMenu(NULL),
	m_linkApplicationsMenu(NULL),
	m_frameApplicationsMenu(NULL),
	m_pageApplicationsMenu(NULL),
	m_reloadTimeMenu(NULL),
	m_quickSearchMenu(NULL),
	m_contextMenuReason(QContextMenuEvent::Other),
	m_reloadTimer(0),
	m_ignoreContextMenu(false),
	m_ignoreContextMenuNextTime(false),
	m_isUsingRockerNavigation(false)
{
	Q_UNUSED(isPrivate)

	connect(SearchesManager::getInstance(), SIGNAL(searchEnginesModified()), this, SLOT(updateQuickSearch()));
}

void WebWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_reloadTimer)
	{
		killTimer(m_reloadTimer);

		m_reloadTimer = 0;

		if (!isLoading())
		{
			triggerAction(ActionsManager::ReloadAction);
		}
	}
}

void WebWidget::bounceAction(int identifier, QVariantMap parameters)
{
	Window *window = NULL;
	QObject *parent = parentWidget();

	while (parent)
	{
		if (parent->metaObject()->className() == QLatin1String("Otter::Window"))
		{
			window = qobject_cast<Window*>(parent);

			break;
		}

		parent = parent->parent();
	}

	if (window)
	{
		parameters[QLatin1String("isBounced")] = true;

		window->triggerAction(identifier, parameters);
	}
}

void WebWidget::triggerAction()
{
	Action *action = qobject_cast<Action*>(sender());

	if (action)
	{
		QVariantMap parameters;

		if (action->isCheckable())
		{
			parameters[QLatin1String("isChecked")] = action->isChecked();
		}

		triggerAction(action->getIdentifier(), parameters);
	}
}

void WebWidget::search(const QString &query, const QString &engine)
{
	Q_UNUSED(query)
	Q_UNUSED(engine)
}

void WebWidget::startReloadTimer()
{
	const int reloadTime = getOption(QLatin1String("Content/PageReloadTime")).toInt();

	if (reloadTime >= 0)
	{
		triggerAction(ActionsManager::StopScheduledReloadAction);

		if (reloadTime > 0)
		{
			m_reloadTimer = startTimer(reloadTime * 1000);
		}
	}
}

void WebWidget::startTransfer(Transfer *transfer)
{
	if (transfer->getState() == Transfer::CancelledState)
	{
		transfer->deleteLater();

		return;
	}

	TransferDialog *transferDialog = new TransferDialog(transfer, this);
	ContentsDialog dialog(Utils::getIcon(QLatin1String("download")), transferDialog->windowTitle(), QString(), QString(), QDialogButtonBox::NoButton, transferDialog, this);

	connect(transferDialog, SIGNAL(finished(int)), &dialog, SLOT(close()));

	showDialog(&dialog);
}

void WebWidget::pasteNote(QAction *action)
{
	if (action && action->data().isValid())
	{
		BookmarksItem *note = NotesManager::getModel()->bookmarkFromIndex(action->data().toModelIndex());

		if (note)
		{
			pasteText(note->data(BookmarksModel::DescriptionRole).toString());
		}
	}
}

void WebWidget::reloadTimeMenuAboutToShow()
{
	switch (getOption(QLatin1String("Content/PageReloadTime")).toInt())
	{
		case 1800:
			m_reloadTimeMenu->actions().at(0)->setChecked(true);

			break;
		case 3600:
			m_reloadTimeMenu->actions().at(1)->setChecked(true);

			break;
		case 7200:
			m_reloadTimeMenu->actions().at(2)->setChecked(true);

			break;
		case 21600:
			m_reloadTimeMenu->actions().at(3)->setChecked(true);

			break;
		case 0:
			m_reloadTimeMenu->actions().at(4)->setChecked(true);

			break;
		case -1:
			m_reloadTimeMenu->actions().at(7)->setChecked(true);

			break;
		default:
			m_reloadTimeMenu->actions().at(5)->setChecked(true);

			break;
	}
}

void WebWidget::openInApplication(QAction *action)
{
	QObject *menu = sender();

	if (menu == m_pageApplicationsMenu)
	{
		Utils::runApplication(action->data().toString(), getUrl());
	}
	else if (menu == m_linkApplicationsMenu && m_hitResult.linkUrl.isValid())
	{
		Utils::runApplication(action->data().toString(), m_hitResult.linkUrl);
	}
	else if (menu == m_frameApplicationsMenu && m_hitResult.frameUrl.isValid())
	{
		Utils::runApplication(action->data().toString(), m_hitResult.frameUrl);
	}
}

void WebWidget::openInApplicationMenuAboutToShow()
{
	QMenu *menu = qobject_cast<QMenu*>(sender());

	if (!menu || !menu->actions().isEmpty())
	{
		return;
	}

	const QList<ApplicationInformation> applications = Utils::getApplicationsForMimeType(QMimeDatabase().mimeTypeForName(QLatin1String("text/html")));

	if (applications.isEmpty())
	{
		menu->addAction(tr("Default Application"));
	}
	else
	{
		for (int i = 0; i < applications.count(); ++i)
		{
			menu->addAction(applications.at(i).icon, ((applications.at(i).name.isEmpty()) ? tr("Unknown") : applications.at(i).name))->setData(applications.at(i).command);

			if (i == 0)
			{
				menu->addSeparator();
			}
		}
	}

	connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(openInApplication(QAction*)));
}

void WebWidget::quickSearch(QAction *action)
{
	const SearchInformation engine = SearchesManager::getSearchEngine((!action || action->data().type() != QVariant::String) ? QString() : action->data().toString());

	if (engine.identifier.isEmpty())
	{
		return;
	}

	if (engine.identifier != m_options.value(QLatin1String("Search/DefaultQuickSearchEngine")).toString())
	{
		setOption(QLatin1String("Search/DefaultQuickSearchEngine"), engine.identifier);
	}

	const OpenHints hints = WindowsManager::calculateOpenHints();

	if (hints == CurrentTabOpen)
	{
		search(getSelectedText(), engine.identifier);
	}
	else
	{
		emit requestedSearch(getSelectedText(), engine.identifier, hints);
	}
}

void WebWidget::quickSearchMenuAboutToShow()
{
	if (m_quickSearchMenu && m_quickSearchMenu->isEmpty())
	{
		const QStringList engines = SearchesManager::getSearchEngines();

		for (int i = 0; i < engines.count(); ++i)
		{
			const SearchInformation engine = SearchesManager::getSearchEngine(engines.at(i));

			if (!engine.identifier.isEmpty())
			{
				QAction *action = m_quickSearchMenu->addAction(engine.icon, engine.title);
				action->setData(engine.identifier);
				action->setToolTip(engine.description);
			}
		}
	}
}

void WebWidget::clearOptions()
{
	m_options.clear();
}

void WebWidget::openUrl(const QUrl &url, OpenHints hints)
{
	WebWidget *widget = clone(false);
	widget->setRequestedUrl(url, false);

	emit requestedNewWindow(widget, hints);
}

void WebWidget::handleToolTipEvent(QHelpEvent *event, QWidget *widget)
{
	const HitTestResult hitResult = getHitTestResult(event->pos());
	const QString toolTipsMode = SettingsManager::getValue(QLatin1String("Browser/ToolTipsMode")).toString();
	const QString link = (hitResult.linkUrl.isValid() ? hitResult.linkUrl : hitResult.formUrl).toString();
	QString text;

	if (toolTipsMode != QLatin1String("disabled"))
	{
		const QString title = QString(hitResult.title).replace(QLatin1Char('&'), QLatin1String("&amp;")).replace(QLatin1Char('<'), QLatin1String("&lt;")).replace(QLatin1Char('>'), QLatin1String("&gt;"));

		if (toolTipsMode == QLatin1String("extended"))
		{
			if (!link.isEmpty())
			{
				text = (title.isEmpty() ? QString() : tr("Title: %1").arg(title) + QLatin1String("<br>")) + tr("Address: %1").arg(link);
			}
			else if (!title.isEmpty())
			{
				text = title;
			}
		}
		else
		{
			text = title;
		}
	}

	setStatusMessage((link.isEmpty() ? hitResult.title : link), true);

	if (!text.isEmpty())
	{
		QToolTip::showText(event->globalPos(), QStringLiteral("<div style=\"white-space:pre-line;\">%1</div>").arg(text), widget);
	}

	event->accept();
}

void WebWidget::updateHitTestResult(const QPoint &position)
{
	m_hitResult = getHitTestResult(position);
}

void WebWidget::showDialog(ContentsDialog *dialog)
{
	ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());

	if (parent)
	{
		parent->showDialog(dialog);
	}
}

void WebWidget::showContextMenu(const QPoint &position)
{
	m_contextMenuReason = QContextMenuEvent::Other;

	const bool hasSelection = (this->hasSelection() && !getSelectedText().trimmed().isEmpty());

	if (m_ignoreContextMenu || (position.isNull() && (!hasSelection || m_clickPosition.isNull())))
	{
		return;
	}

	const QPoint hitPosition = (position.isNull() ? m_clickPosition : position);

	if (isScrollBar(hitPosition) || (SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanDisableContextMenu")).toBool() && !canShowContextMenu(hitPosition)))
	{
		return;
	}

	updateHitTestResult(hitPosition);
	updateEditActions();

	QStringList flags;

	if (m_hitResult.flags.testFlag(IsFormTest))
	{
		flags.append(QLatin1String("form"));
	}

	if (!m_hitResult.imageUrl.isValid() && m_hitResult.flags.testFlag(IsSelectedTest) && hasSelection)
	{
		flags.append(QLatin1String("selection"));
	}

	if (m_hitResult.linkUrl.isValid())
	{
		if (m_hitResult.linkUrl.scheme() == QLatin1String("mailto"))
		{
			flags.append(QLatin1String("mail"));
		}
		else
		{
			flags.append(QLatin1String("link"));
		}
	}

	if (!m_hitResult.imageUrl.isEmpty())
	{
		flags.append(QLatin1String("image"));
	}

	if (m_hitResult.mediaUrl.isValid())
	{
		flags.append(QLatin1String("media"));
	}

	if (m_hitResult.flags.testFlag(IsContentEditableTest))
	{
		flags.append(QLatin1String("edit"));
	}

	if (flags.isEmpty() || (flags.size() == 1 && flags.first() == QLatin1String("form")))
	{
		flags.append(QLatin1String("standard"));

		if (m_hitResult.frameUrl.isValid())
		{
			flags.append(QLatin1String("frame"));
		}
	}

	if (flags.isEmpty())
	{
		return;
	}

	Menu menu(Menu::NoMenuRole, this);
	menu.load(QLatin1String("menu/webWidget.json"), flags);
	menu.exec(mapToGlobal(hitPosition));
}

void WebWidget::updateQuickSearch()
{
	if (m_quickSearchMenu)
	{
		m_quickSearchMenu->clear();
	}
}

void WebWidget::updatePageActions(const QUrl &url)
{
	if (m_actions.contains(ActionsManager::AddBookmarkAction))
	{
		m_actions[ActionsManager::AddBookmarkAction]->setOverrideText(BookmarksManager::hasBookmark(url) ? QT_TRANSLATE_NOOP("actions", "Edit Bookmark…") : QT_TRANSLATE_NOOP("actions", "Add Bookmark…"));
	}

	if (m_actions.contains(ActionsManager::WebsitePreferencesAction))
	{
		m_actions[ActionsManager::WebsitePreferencesAction]->setEnabled(!url.isEmpty() && url.scheme() != QLatin1String("about"));
	}
}

void WebWidget::updateNavigationActions()
{
	if (m_actions.contains(ActionsManager::GoBackAction))
	{
		m_actions[ActionsManager::GoBackAction]->setEnabled(canGoBack());
	}

	if (m_actions.contains(ActionsManager::GoForwardAction))
	{
		m_actions[ActionsManager::GoForwardAction]->setEnabled(canGoForward());
	}

	if (m_actions.contains(ActionsManager::RewindAction))
	{
		m_actions[ActionsManager::RewindAction]->setEnabled(canGoBack());
	}

	if (m_actions.contains(ActionsManager::FastForwardAction))
	{
		m_actions[ActionsManager::FastForwardAction]->setEnabled(canGoForward());
	}

	if (m_actions.contains(ActionsManager::StopAction))
	{
		m_actions[ActionsManager::StopAction]->setEnabled(isLoading());
	}

	if (m_actions.contains(ActionsManager::ReloadAction))
	{
		m_actions[ActionsManager::ReloadAction]->setEnabled(!isLoading());
	}

	if (m_actions.contains(ActionsManager::ReloadOrStopAction))
	{
		m_actions[ActionsManager::ReloadOrStopAction]->setup(isLoading() ? getAction(ActionsManager::StopAction) : getAction(ActionsManager::ReloadAction));
	}

	if (m_actions.contains(ActionsManager::LoadPluginsAction))
	{
		m_actions[ActionsManager::LoadPluginsAction]->setEnabled(getAmountOfNotLoadedPlugins() > 0);
	}

	if (m_actions.contains(ActionsManager::ViewSourceAction))
	{
		m_actions[ActionsManager::ViewSourceAction]->setEnabled(canViewSource());
	}
}

void WebWidget::updateEditActions()
{
	const bool canPaste = (QApplication::clipboard()->mimeData() && QApplication::clipboard()->mimeData()->hasText());
	const bool hasSelection = (this->hasSelection() && !getSelectedText().trimmed().isEmpty());

	if (m_actions.contains(ActionsManager::CutAction))
	{
		m_actions[ActionsManager::CutAction]->setEnabled(hasSelection && m_hitResult.flags.testFlag(IsContentEditableTest));
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
		m_actions[ActionsManager::PasteAction]->setEnabled(canPaste && m_hitResult.flags.testFlag(IsContentEditableTest));
	}

	if (m_actions.contains(ActionsManager::PasteAndGoAction))
	{
		m_actions[ActionsManager::PasteAndGoAction]->setEnabled(canPaste);
	}

	if (m_actions.contains(ActionsManager::PasteNoteAction))
	{
		m_actions[ActionsManager::PasteNoteAction]->setEnabled(canPaste && m_hitResult.flags.testFlag(IsContentEditableTest) && NotesManager::getModel()->getRootItem()->hasChildren());
	}

	if (m_actions.contains(ActionsManager::DeleteAction))
	{
		m_actions[ActionsManager::DeleteAction]->setEnabled(m_hitResult.flags.testFlag(IsContentEditableTest) && !m_hitResult.flags.testFlag(IsEmptyTest));
	}

	if (m_actions.contains(ActionsManager::SelectAllAction))
	{
		m_actions[ActionsManager::SelectAllAction]->setEnabled(!m_hitResult.flags.testFlag(IsEmptyTest));
	}

	if (m_actions.contains(ActionsManager::ClearAllAction))
	{
		m_actions[ActionsManager::ClearAllAction]->setEnabled(m_hitResult.flags.testFlag(IsContentEditableTest) && !m_hitResult.flags.testFlag(IsEmptyTest));
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

	updateLinkActions();
	updateFrameActions();
	updateImageActions();
	updateMediaActions();
}

void WebWidget::updateLinkActions()
{
	const bool isLink = m_hitResult.linkUrl.isValid();

	if (m_actions.contains(ActionsManager::OpenLinkAction))
	{
		m_actions[ActionsManager::OpenLinkAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInCurrentTabAction))
	{
		m_actions[ActionsManager::OpenLinkInCurrentTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewTabAction))
	{
		m_actions[ActionsManager::OpenLinkInNewTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewTabBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewWindowAction))
	{
		m_actions[ActionsManager::OpenLinkInNewWindowAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewWindowBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewWindowBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateTabAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateTabBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateWindowAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateWindowAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInApplicationAction))
	{
		m_actions[ActionsManager::OpenLinkInApplicationAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::CopyLinkToClipboardAction))
	{
		m_actions[ActionsManager::CopyLinkToClipboardAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::BookmarkLinkAction))
	{
		m_actions[ActionsManager::BookmarkLinkAction]->setOverrideText(BookmarksManager::hasBookmark(m_hitResult.linkUrl) ? QT_TRANSLATE_NOOP("actions", "Edit Link Bookmark…") : QT_TRANSLATE_NOOP("actions", "Bookmark Link…"));
		m_actions[ActionsManager::BookmarkLinkAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::SaveLinkToDiskAction))
	{
		m_actions[ActionsManager::SaveLinkToDiskAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::SaveLinkToDownloadsAction))
	{
		m_actions[ActionsManager::SaveLinkToDownloadsAction]->setEnabled(isLink);
	}
}

void WebWidget::updateFrameActions()
{
	const bool isFrame = (m_hitResult.frameUrl.isValid());

	if (m_actions.contains(ActionsManager::OpenFrameInCurrentTabAction))
	{
		m_actions[ActionsManager::OpenFrameInCurrentTabAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::OpenFrameInNewTabAction))
	{
		m_actions[ActionsManager::OpenFrameInNewTabAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::OpenFrameInNewTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenFrameInNewTabBackgroundAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::OpenFrameInApplicationAction))
	{
		m_actions[ActionsManager::OpenFrameInApplicationAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::CopyFrameLinkToClipboardAction))
	{
		m_actions[ActionsManager::CopyFrameLinkToClipboardAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::ReloadFrameAction))
	{
		m_actions[ActionsManager::ReloadFrameAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::ViewFrameSourceAction))
	{
		m_actions[ActionsManager::ViewFrameSourceAction]->setEnabled(isFrame);
	}
}

void WebWidget::updateImageActions()
{
	const bool isImage = !m_hitResult.imageUrl.isEmpty();
	const bool isOpened = getUrl().matches(m_hitResult.imageUrl, (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash));
	const QString fileName = fontMetrics().elidedText(m_hitResult.imageUrl.fileName(), Qt::ElideMiddle, 256);

	if (m_actions.contains(ActionsManager::OpenImageInNewTabAction))
	{
		m_actions[ActionsManager::OpenImageInNewTabAction]->setOverrideText(isImage ? (fileName.isEmpty() || m_hitResult.imageUrl.scheme() == QLatin1String("data")) ? tr("Open Image in New Tab (Untitled)") : tr("Open Image in New Tab (%1)").arg(fileName) : QT_TRANSLATE_NOOP("actions", "Open Image in New Tab"));
		m_actions[ActionsManager::OpenImageInNewTabAction]->setEnabled(isImage && !isOpened);
	}

	if (m_actions.contains(ActionsManager::OpenImageInNewTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenImageInNewTabBackgroundAction]->setOverrideText(isImage ? (fileName.isEmpty() || m_hitResult.imageUrl.scheme() == QLatin1String("data")) ? tr("Open Image in New Background Tab (Untitled)") : tr("Open Image in New Background Tab (%1)").arg(fileName) : QT_TRANSLATE_NOOP("actions", "Open Image in New Background Tab"));
		m_actions[ActionsManager::OpenImageInNewTabBackgroundAction]->setEnabled(isImage && !isOpened);
	}

	if (m_actions.contains(ActionsManager::SaveImageToDiskAction))
	{
		m_actions[ActionsManager::SaveImageToDiskAction]->setEnabled(isImage);
	}

	if (m_actions.contains(ActionsManager::CopyImageToClipboardAction))
	{
		m_actions[ActionsManager::CopyImageToClipboardAction]->setEnabled(isImage);
	}

	if (m_actions.contains(ActionsManager::CopyImageUrlToClipboardAction))
	{
		m_actions[ActionsManager::CopyImageUrlToClipboardAction]->setEnabled(isImage);
	}

	if (m_actions.contains(ActionsManager::ReloadImageAction))
	{
		m_actions[ActionsManager::ReloadImageAction]->setEnabled(isImage);
	}

	if (m_actions.contains(ActionsManager::ImagePropertiesAction))
	{
		m_actions[ActionsManager::ImagePropertiesAction]->setEnabled(isImage);
	}
}

void WebWidget::updateMediaActions()
{
	const bool isMedia = m_hitResult.mediaUrl.isValid();
	const bool isVideo = (m_hitResult.tagName == QLatin1String("video"));
	const bool isPaused = m_hitResult.flags.testFlag(MediaIsPausedTest);
	const bool isMuted = m_hitResult.flags.testFlag(MediaIsMutedTest);

	if (m_actions.contains(ActionsManager::SaveMediaToDiskAction))
	{
		m_actions[ActionsManager::SaveMediaToDiskAction]->setOverrideText(isVideo ? QT_TRANSLATE_NOOP("actions", "Save Video…") : QT_TRANSLATE_NOOP("actions", "Save Audio…"));
		m_actions[ActionsManager::SaveMediaToDiskAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::CopyMediaUrlToClipboardAction))
	{
		m_actions[ActionsManager::CopyMediaUrlToClipboardAction]->setOverrideText(isVideo ? QT_TRANSLATE_NOOP("actions", "Copy Video Link to Clipboard") : QT_TRANSLATE_NOOP("actions", "Copy Audio Link to Clipboard"));
		m_actions[ActionsManager::CopyMediaUrlToClipboardAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaControlsAction))
	{
		m_actions[ActionsManager::MediaControlsAction]->setChecked(m_hitResult.flags.testFlag(MediaHasControlsTest));
		m_actions[ActionsManager::MediaControlsAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaLoopAction))
	{
		m_actions[ActionsManager::MediaLoopAction]->setChecked(m_hitResult.flags.testFlag(MediaIsLoopedTest));
		m_actions[ActionsManager::MediaLoopAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaPlayPauseAction))
	{
		m_actions[ActionsManager::MediaPlayPauseAction]->setOverrideText(isPaused ? QT_TRANSLATE_NOOP("actions", "Play") : QT_TRANSLATE_NOOP("actions", "Pause"));
		m_actions[ActionsManager::MediaPlayPauseAction]->setIcon(Utils::getIcon(isPaused ? QLatin1String("media-playback-start") : QLatin1String("media-playback-pause")));
		m_actions[ActionsManager::MediaPlayPauseAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaMuteAction))
	{
		m_actions[ActionsManager::MediaMuteAction]->setOverrideText(isMuted ? QT_TRANSLATE_NOOP("actions", "Unmute") : QT_TRANSLATE_NOOP("actions", "Mute"));
		m_actions[ActionsManager::MediaMuteAction]->setIcon(Utils::getIcon(isMuted ? QLatin1String("audio-volume-medium") : QLatin1String("audio-volume-muted")));
		m_actions[ActionsManager::MediaMuteAction]->setEnabled(isMedia);
	}
}

void WebWidget::updateBookmarkActions()
{
	updatePageActions(getUrl());
	updateLinkActions();
}

void WebWidget::setAlternateStyleSheets(const QStringList &styleSheets)
{
	m_alternateStyleSheets = styleSheets;
}

void WebWidget::setClickPosition(const QPoint &position)
{
	m_clickPosition = position;
}

void WebWidget::setStatusMessage(const QString &message, bool override)
{
	const QString oldMessage = getStatusMessage();

	if (override)
	{
		m_overridingStatusMessage = message;
	}
	else
	{
		m_javaScriptStatusMessage = message;
	}

	const QString newMessage = getStatusMessage();

	if (newMessage != oldMessage)
	{
		emit statusMessageChanged(newMessage);
	}
}

void WebWidget::setPermission(const QString &key, const QUrl &url, PermissionPolicies policies)
{
	if (policies.testFlag(RememberPermission))
	{
		if (key == QLatin1String("Browser/EnableMediaCaptureAudioVideo"))
		{
			SettingsManager::setValue(QLatin1String("Browser/EnableMediaAudioVideo"), policies.testFlag(GrantedPermission), url);
			SettingsManager::setValue(QLatin1String("Browser/EnableMediaCaptureVideo"), policies.testFlag(GrantedPermission), url);
		}
		else
		{
			SettingsManager::setValue(key, (policies.testFlag(GrantedPermission) ? QLatin1String("allow") : QLatin1String("disallow")), url);
		}
	}
}

void WebWidget::setOption(const QString &key, const QVariant &value)
{
	if (key == QLatin1String("Search/DefaultQuickSearchEngine"))
	{
		const QString quickSearchEngine = value.toString();

		if (quickSearchEngine != m_options.value(QLatin1String("Search/DefaultQuickSearchEngine")).toString())
		{
			if (value.isNull())
			{
				m_options.remove(key);
			}
			else
			{
				m_options[key] = value;
			}

			updateQuickSearch();
		}

		return;
	}

	if (key == QLatin1String("Content/PageReloadTime"))
	{
		const int reloadTime = value.toInt();

		if (reloadTime == m_options.value(QLatin1String("Content/PageReloadTime"), -1).toInt())
		{
			return;
		}

		if (m_reloadTimer != 0)
		{
			killTimer(m_reloadTimer);

			m_reloadTimer = 0;
		}

		if (reloadTime >= 0)
		{
			triggerAction(ActionsManager::StopScheduledReloadAction);

			if (reloadTime > 0)
			{
				m_reloadTimer = startTimer(reloadTime * 1000);
			}
		}
	}

	if (value.isNull())
	{
		m_options.remove(key);
	}
	else
	{
		m_options[key] = value;
	}
}

void WebWidget::setOptions(const QVariantHash &options)
{
	m_options = options;
}

void WebWidget::setRequestedUrl(const QUrl &url, bool typed, bool onlyUpdate)
{
	m_requestedUrl = url;

	if (!onlyUpdate)
	{
		setUrl(url, typed);
	}
}

void WebWidget::setReloadTime(QAction *action)
{
	const int reloadTime = action->data().toInt();

	if (reloadTime == -2)
	{
		ReloadTimeDialog dialog(qMax(0, getOption(QLatin1String("Content/PageReloadTime")).toInt()), this);

		if (dialog.exec() == QDialog::Accepted)
		{
			setOption(QLatin1String("Content/PageReloadTime"), dialog.getReloadTime());
		}
	}
	else
	{
		setOption(QLatin1String("Content/PageReloadTime"), reloadTime);
	}
}

Action* WebWidget::getAction(int identifier)
{
	if (identifier < 0)
	{
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
		case ActionsManager::CheckSpellingAction:
			action->setEnabled(false);

			break;
		case ActionsManager::AddBookmarkAction:
		case ActionsManager::WebsitePreferencesAction:
			updatePageActions(getUrl());

			break;
		case ActionsManager::GoBackAction:
		case ActionsManager::RewindAction:
			action->setEnabled(canGoBack());

			break;
		case ActionsManager::GoForwardAction:
		case ActionsManager::FastForwardAction:
			action->setEnabled(canGoForward());

			break;
		case ActionsManager::PasteNoteAction:
			if (!m_pasteNoteMenu)
			{
				m_pasteNoteMenu = new Menu(Menu::NotesMenuRole, this);

				connect(m_pasteNoteMenu, SIGNAL(triggered(QAction*)), this, SLOT(pasteNote(QAction*)));
			}

			action->setMenu(m_pasteNoteMenu);

			updateEditActions();

			break;
		case ActionsManager::StopAction:
			action->setEnabled(isLoading());

			break;

		case ActionsManager::ReloadAction:
			action->setEnabled(!isLoading());

			break;
		case ActionsManager::ReloadOrStopAction:
			action->setup(isLoading() ? getAction(ActionsManager::StopAction) : getAction(ActionsManager::ReloadAction));

			break;
		case ActionsManager::ScheduleReloadAction:
			if (!m_reloadTimeMenu)
			{
				m_reloadTimeMenu = new QMenu(this);
				m_reloadTimeMenu->addAction(tr("30 Minutes"))->setData(1800);
				m_reloadTimeMenu->addAction(tr("1 Hour"))->setData(3600);
				m_reloadTimeMenu->addAction(tr("2 Hours"))->setData(7200);
				m_reloadTimeMenu->addAction(tr("6 Hours"))->setData(21600);
				m_reloadTimeMenu->addAction(tr("Never"))->setData(0);
				m_reloadTimeMenu->addAction(tr("Custom…"))->setData(-2);
				m_reloadTimeMenu->addSeparator();
				m_reloadTimeMenu->addAction(tr("Page Default"))->setData(-1);

				QActionGroup *actionGroup = new QActionGroup(m_reloadTimeMenu);
				actionGroup->setExclusive(true);

				for (int i = 0; i < m_reloadTimeMenu->actions().count(); ++i)
				{
					m_reloadTimeMenu->actions().at(i)->setCheckable(true);

					actionGroup->addAction(m_reloadTimeMenu->actions().at(i));
				}

				connect(m_reloadTimeMenu, SIGNAL(aboutToShow()), this, SLOT(reloadTimeMenuAboutToShow()));
				connect(m_reloadTimeMenu, SIGNAL(triggered(QAction*)), this, SLOT(setReloadTime(QAction*)));
			}

			action->setMenu(m_reloadTimeMenu);

			break;
		case ActionsManager::LoadPluginsAction:
			action->setEnabled(getAmountOfNotLoadedPlugins() > 0);

			break;
		case ActionsManager::ValidateAction:
			action->setEnabled(false);
			action->setMenu(new QMenu(this));

			break;
		case ActionsManager::SearchMenuAction:
			if (!m_quickSearchMenu)
			{
				m_quickSearchMenu = new QMenu(this);

				connect(m_quickSearchMenu, SIGNAL(aboutToShow()), this, SLOT(quickSearchMenuAboutToShow()));
				connect(m_quickSearchMenu, SIGNAL(triggered(QAction*)), this, SLOT(quickSearch(QAction*)));
			}

			action->setMenu(m_quickSearchMenu);

		case ActionsManager::UndoAction:
		case ActionsManager::RedoAction:
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
		case ActionsManager::OpenLinkInApplicationAction:
			if (!m_linkApplicationsMenu)
			{
				m_linkApplicationsMenu = new QMenu(this);

				action->setMenu(m_linkApplicationsMenu);

				connect(m_linkApplicationsMenu, SIGNAL(aboutToShow()), this, SLOT(openInApplicationMenuAboutToShow()));
			}

		case ActionsManager::OpenLinkAction:
		case ActionsManager::OpenLinkInCurrentTabAction:
		case ActionsManager::OpenLinkInNewTabAction:
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
		case ActionsManager::OpenLinkInNewWindowAction:
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
		case ActionsManager::OpenLinkInNewPrivateTabAction:
		case ActionsManager::OpenLinkInNewPrivateTabBackgroundAction:
		case ActionsManager::OpenLinkInNewPrivateWindowAction:
		case ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction:
		case ActionsManager::CopyLinkToClipboardAction:
		case ActionsManager::BookmarkLinkAction:
		case ActionsManager::SaveLinkToDiskAction:
		case ActionsManager::SaveLinkToDownloadsAction:
			updateLinkActions();

			break;
		case ActionsManager::OpenFrameInApplicationAction:
			if (!m_frameApplicationsMenu)
			{
				m_frameApplicationsMenu = new QMenu(this);

				action->setMenu(m_frameApplicationsMenu);

				connect(m_frameApplicationsMenu, SIGNAL(aboutToShow()), this, SLOT(openInApplicationMenuAboutToShow()));
			}

		case ActionsManager::OpenFrameInCurrentTabAction:
		case ActionsManager::OpenFrameInNewTabAction:
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
		case ActionsManager::CopyFrameLinkToClipboardAction:
		case ActionsManager::ReloadFrameAction:
		case ActionsManager::ViewFrameSourceAction:
			updateFrameActions();

			break;
		case ActionsManager::OpenImageInNewTabAction:
		case ActionsManager::OpenImageInNewTabBackgroundAction:
		case ActionsManager::SaveImageToDiskAction:
		case ActionsManager::CopyImageToClipboardAction:
		case ActionsManager::CopyImageUrlToClipboardAction:
		case ActionsManager::ReloadImageAction:
		case ActionsManager::ImagePropertiesAction:
			updateImageActions();

			break;
		case ActionsManager::SaveMediaToDiskAction:
		case ActionsManager::CopyMediaUrlToClipboardAction:
		case ActionsManager::MediaControlsAction:
		case ActionsManager::MediaLoopAction:
		case ActionsManager::MediaPlayPauseAction:
		case ActionsManager::MediaMuteAction:
			updateMediaActions();

			break;
		case ActionsManager::OpenPageInApplicationAction:
			if (!m_pageApplicationsMenu)
			{
				m_pageApplicationsMenu = new QMenu(this);

				action->setMenu(m_pageApplicationsMenu);

				connect(m_pageApplicationsMenu, SIGNAL(aboutToShow()), this, SLOT(openInApplicationMenuAboutToShow()));
			}

			break;
		default:
			break;
	}

	return action;
}

Action* WebWidget::getExistingAction(int identifier)
{
	return (m_actions.contains(identifier) ? m_actions[identifier] : NULL);
}

WebBackend* WebWidget::getBackend()
{
	return m_backend;
}

QString WebWidget::suggestSaveFileName() const
{
	const QUrl url = getUrl();
	QString fileName = url.fileName();

	if (fileName.isEmpty() && !url.path().isEmpty() && url.path() != QLatin1String("/"))
	{
		fileName = QDir(url.path()).dirName();
	}

	if (fileName.isEmpty() && !url.host().isEmpty())
	{
		fileName = url.host() + QLatin1String(".html");
	}

	if (fileName.isEmpty())
	{
		fileName = QLatin1String("file.html");
	}

	if (!fileName.contains(QLatin1Char('.')))
	{
		fileName.append(QLatin1String(".html"));
	}

	return fileName;
}

QString WebWidget::getSelectedText() const
{
	return QString();
}

QString WebWidget::getStatusMessage() const
{
	return (m_overridingStatusMessage.isEmpty() ? m_javaScriptStatusMessage : m_overridingStatusMessage);
}

QVariant WebWidget::getOption(const QString &key, const QUrl &url) const
{
	if (m_options.contains(key))
	{
		return m_options[key];
	}

	return SettingsManager::getValue(key, (url.isEmpty() ? getUrl() : url));
}

QUrl WebWidget::getRequestedUrl() const
{
	return ((getUrl().isEmpty() || isLoading()) ? m_requestedUrl : getUrl());
}

QPoint WebWidget::getClickPosition() const
{
	return m_clickPosition;
}

QStringList WebWidget::getAlternateStyleSheets() const
{
	return m_alternateStyleSheets;
}

QList<LinkUrl> WebWidget::getFeeds() const
{
	return QList<LinkUrl>();
}

QList<LinkUrl> WebWidget::getSearchEngines() const
{
	return QList<LinkUrl>();
}

QVariantHash WebWidget::getOptions() const
{
	return m_options;
}

QVariantHash WebWidget::getStatistics() const
{
	return QVariantHash();
}

QHash<QByteArray, QByteArray> WebWidget::getHeaders() const
{
	return QHash<QByteArray, QByteArray>();
}

WebWidget::HitTestResult WebWidget::getCurrentHitTestResult() const
{
	return m_hitResult;
}

WebWidget::HitTestResult WebWidget::getHitTestResult(const QPoint &position)
{
	Q_UNUSED(position)

	return HitTestResult();
}

QContextMenuEvent::Reason WebWidget::getContextMenuReason() const
{
	return m_contextMenuReason;
}

int WebWidget::getAmountOfNotLoadedPlugins() const
{
	return 0;
}

bool WebWidget::handleContextMenuEvent(QContextMenuEvent *event, bool canPropagate, QObject *sender)
{
	Q_UNUSED(sender)

	m_contextMenuReason = event->reason();
	m_ignoreContextMenu = (event->reason() == QContextMenuEvent::Mouse);

	if (event->reason() == QContextMenuEvent::Keyboard)
	{
		triggerAction(ActionsManager::ContextMenuAction);
	}

	if (canPropagate)
	{
		WebWidget::contextMenuEvent(event);
	}

	return false;
}

bool WebWidget::handleMousePressEvent(QMouseEvent *event, bool canPropagate, QObject *sender)
{
	if (event->button() == Qt::BackButton)
	{
		triggerAction(ActionsManager::GoBackAction);

		event->accept();

		return true;
	}

	if (event->button() == Qt::ForwardButton)
	{
		triggerAction(ActionsManager::GoForwardAction);

		event->accept();

		return true;
	}

	if ((event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) && !isScrollBar(event->pos()))
	{
		if (event->button() == Qt::LeftButton && event->buttons().testFlag(Qt::RightButton))
		{
			m_isUsingRockerNavigation = true;

			triggerAction(ActionsManager::GoBackAction);

			return true;
		}

		updateHitTestResult(event->pos());

		if (m_hitResult.linkUrl.isValid())
		{
			if (event->button() == Qt::LeftButton && event->modifiers() == Qt::NoModifier)
			{
				return false;
			}

			openUrl(m_hitResult.linkUrl, WindowsManager::calculateOpenHints(event->modifiers(), event->button(), CurrentTabOpen));

			event->accept();

			return true;
		}

		if (event->button() == Qt::MiddleButton)
		{
			if (!m_hitResult.linkUrl.isValid() && m_hitResult .tagName != QLatin1String("textarea") && m_hitResult .tagName != QLatin1String("input"))
			{
				//TODO improve forwarding
				ActionsManager::triggerAction(ActionsManager::StartMoveScrollAction, this);

				return true;
			}
		}
	}
	else if (event->button() == Qt::RightButton)
	{
		updateHitTestResult(event->pos());

		if (event->buttons().testFlag(Qt::LeftButton))
		{
			triggerAction(ActionsManager::GoForwardAction);

			event->ignore();
		}
		else
		{
			event->accept();

			if (m_hitResult.linkUrl.isValid())
			{
				m_clickPosition = event->pos();
			}

			GesturesManager::startGesture((m_hitResult.linkUrl.isValid() ? GesturesManager::LinkGesturesContext : GesturesManager::GenericGesturesContext), sender, event);
		}

		return true;
	}

	if (canPropagate)
	{
		mousePressEvent(event);
	}

	return false;
}

bool WebWidget::handleMouseReleaseEvent(QMouseEvent *event, bool canPropagate, QObject *sender)
{
	if (event->button() == Qt::RightButton && !event->buttons().testFlag(Qt::LeftButton))
	{
		if (m_isUsingRockerNavigation)
		{
			m_isUsingRockerNavigation = false;
			m_ignoreContextMenuNextTime = true;

			if (sender)
			{
				QMouseEvent mousePressEvent(QEvent::MouseButtonPress, QPointF(event->pos()), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
				QMouseEvent mouseReleaseEvent(QEvent::MouseButtonRelease, QPointF(event->pos()), Qt::RightButton, Qt::RightButton, Qt::NoModifier);

				QCoreApplication::sendEvent(sender, &mousePressEvent);
				QCoreApplication::sendEvent(sender, &mouseReleaseEvent);
			}
		}
		else
		{
			m_ignoreContextMenu = false;

			if (!GesturesManager::endGesture(sender, event))
			{
				if (m_ignoreContextMenuNextTime)
				{
					m_ignoreContextMenuNextTime = false;

					event->ignore();

					return false;
				}

				ContentsWidget *contentsWidget = qobject_cast<ContentsWidget*>(parentWidget());

				m_clickPosition = event->pos();

				if (contentsWidget)
				{
					contentsWidget->triggerAction(ActionsManager::ContextMenuAction);
				}
				else
				{
					showContextMenu(event->pos());
				}
			}
		}

		return true;
	}

	if (canPropagate)
	{
		mouseReleaseEvent(event);
	}

	return false;
}

bool WebWidget::handleMouseDoubleClickEvent(QMouseEvent *event, bool canPropagate, QObject *sender)
{
	Q_UNUSED(sender)

	if (SettingsManager::getValue(QLatin1String("Browser/ShowSelectionContextMenuOnDoubleClick")).toBool() && event->button() == Qt::LeftButton)
	{
		updateHitTestResult(event->pos());

		if (!m_hitResult.flags.testFlag(IsContentEditableTest) && m_hitResult.tagName != QLatin1String("textarea") && m_hitResult.tagName!= QLatin1String("select") && m_hitResult.tagName != QLatin1String("input"))
		{
			m_clickPosition = event->pos();

			QTimer::singleShot(250, this, SLOT(showContextMenu()));
		}
	}

	if (canPropagate)
	{
		mouseDoubleClickEvent(event);
	}

	return false;
}

bool WebWidget::handleWheelEvent(QWheelEvent *event, bool canPropagate, QObject *sender)
{
	Q_UNUSED(sender)

	{
		if (event->buttons() == Qt::RightButton)
		{
			m_ignoreContextMenuNextTime = true;

			event->ignore();

			return false;
		}

		if (event->modifiers().testFlag(Qt::ControlModifier))
		{
			setZoom(getZoom() + (event->delta() / 16));

			event->accept();

			return true;
		}
	}

	if (canPropagate)
	{
		wheelEvent(event);
	}

	return false;
}

bool WebWidget::canGoBack() const
{
	return false;
}

bool WebWidget::canGoForward() const
{
	return false;
}

bool WebWidget::canShowContextMenu(const QPoint &position) const
{
	Q_UNUSED(position)

	return true;
}

bool WebWidget::canViewSource() const
{
	return true;
}

bool WebWidget::isScrollBar(const QPoint &position) const
{
	Q_UNUSED(position)

	return false;
}

bool WebWidget::hasOption(const QString &key) const
{
	return m_options.contains(key);
}

bool WebWidget::hasSelection() const
{
	return false;
}

}
