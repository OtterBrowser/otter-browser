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
#include "ContentsWidget.h"
#include "Menu.h"
#include "ReloadTimeDialog.h"
#include "../core/ActionsManager.h"
#include "../core/NotesManager.h"
#include "../core/SearchesManager.h"
#include "../core/SettingsManager.h"

#include <QtCore/QUrl>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>

namespace Otter
{

QMap<int, QPixmap> WebWidget::m_scrollCursors;

WebWidget::WebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent) : QWidget(parent),
	m_backend(backend),
	m_pasteNoteMenu(NULL),
	m_reloadTimeMenu(NULL),
	m_quickSearchMenu(NULL),
	m_scrollMode(NoScroll),
	m_reloadTimer(0),
	m_scrollTimer(0)
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
	else if (event->timerId() == m_scrollTimer)
	{
		const QPoint scrollDelta = (QCursor::pos() - m_beginCursorPosition) / 20;

		ScrollDirections directions = NoDirection;

		if (scrollDelta.x() < 0)
 		{
 			directions |= LeftDirection;
 		}
		else if (scrollDelta.x() > 0)
 		{
 			directions |= RightDirection;
 		}

		if (scrollDelta.y() < 0)
 		{
 			directions |= TopDirection;
 		}
		else if (scrollDelta.y() > 0)
 		{
 			directions |= BottomDirection;
 		}

		if (!m_scrollCursors.contains(directions))
		{
			if (directions == (BottomDirection | LeftDirection))
			{
				m_scrollCursors[directions] = QPixmap(QLatin1String(":/cursors/scroll-bottom-left.png"));
			}
			else if (directions == (BottomDirection | RightDirection))
			{
				m_scrollCursors[directions] = QPixmap(QLatin1String(":/cursors/scroll-bottom-right.png"));
			}
			else if (directions == BottomDirection)
			{
				m_scrollCursors[directions] = QPixmap(QLatin1String(":/cursors/scroll-bottom.png"));
			}
			else if (directions == LeftDirection)
			{
				m_scrollCursors[directions] = QPixmap(QLatin1String(":/cursors/scroll-left.png"));
			}
			else if (directions == RightDirection)
			{
				m_scrollCursors[directions] = QPixmap(QLatin1String(":/cursors/scroll-right.png"));
			}
			else if (directions == (TopDirection | LeftDirection))
			{
				m_scrollCursors[directions] = QPixmap(QLatin1String(":/cursors/scroll-top-left.png"));
			}
			else if (directions == (TopDirection | RightDirection))
			{
				m_scrollCursors[directions] = QPixmap(QLatin1String(":/cursors/scroll-top-right.png"));
			}
			else if (directions == TopDirection)
			{
				m_scrollCursors[directions] = QPixmap(QLatin1String(":/cursors/scroll-top.png"));
			}
			else if (directions == NoDirection)
			{
				m_scrollCursors[directions] = QPixmap(QLatin1String(":/cursors/scroll-vertical.png"));
			}
		}

 		QApplication::changeOverrideCursor(m_scrollCursors[directions]);

		setScrollPosition(getScrollPosition() + scrollDelta);
	}
}

void WebWidget::keyPressEvent(QKeyEvent *event)
{
	QWidget::keyPressEvent(event);

	if (m_scrollMode == MoveScroll)
	{
		triggerAction(ActionsManager::EndScrollAction);
	}
}

void WebWidget::contextMenuEvent(QContextMenuEvent *event)
{
	if (m_scrollMode == MoveScroll)
	{
		triggerAction(ActionsManager::EndScrollAction);
	}

	event->accept();
}

void WebWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (m_scrollMode == DragScroll && event->button() == Qt::LeftButton)
	{
		setScrollPosition(m_beginScrollPosition + m_beginCursorPosition - QCursor::pos());
	}
}

void WebWidget::triggerAction()
{
	Action *action = qobject_cast<Action*>(sender());

	if (action)
	{
		triggerAction(action->getIdentifier(), (action->isCheckable() && action->isChecked()));
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

void WebWidget::showDialog(ContentsDialog *dialog)
{
	ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());

	if (parent)
	{
		parent->showDialog(dialog);
	}
}

void WebWidget::showContextMenu(const QPoint &position, MenuFlags flags)
{
	QMenu menu;

	if (flags & StandardMenu)
	{
		menu.addAction(getAction(ActionsManager::GoBackAction));
		menu.addAction(getAction(ActionsManager::GoForwardAction));
		menu.addAction(getAction(ActionsManager::RewindAction));
		menu.addAction(getAction(ActionsManager::FastForwardAction));
		menu.addSeparator();
		menu.addAction(getAction(ActionsManager::ReloadOrStopAction));
		menu.addAction(getAction(ActionsManager::ScheduleReloadAction));
		menu.addSeparator();
		menu.addAction(getAction(ActionsManager::AddBookmarkAction));
		menu.addAction(getAction(ActionsManager::CopyAddressAction));
		menu.addAction(getAction(ActionsManager::PrintAction));
		menu.addSeparator();

		if (flags & FormMenu)
		{
			menu.addAction(getAction(ActionsManager::CreateSearchAction));
			menu.addSeparator();
		}

		menu.addAction(getAction(ActionsManager::InspectElementAction));
		menu.addAction(getAction(ActionsManager::ViewSourceAction));
		menu.addAction(getAction(ActionsManager::ValidateAction));
		menu.addSeparator();

		if (flags & FrameMenu)
		{
			QMenu *frameMenu = new QMenu(&menu);
			frameMenu->setTitle(tr("Frame"));
			frameMenu->addAction(getAction(ActionsManager::OpenFrameInCurrentTabAction));
			frameMenu->addAction(getAction(ActionsManager::OpenFrameInNewTabAction));
			frameMenu->addAction(getAction(ActionsManager::OpenFrameInNewTabBackgroundAction));
			frameMenu->addSeparator();
			frameMenu->addAction(getAction(ActionsManager::ViewFrameSourceAction));
			frameMenu->addAction(getAction(ActionsManager::ReloadFrameAction));
			frameMenu->addAction(getAction(ActionsManager::CopyFrameLinkToClipboardAction));

			menu.addMenu(frameMenu);
			menu.addSeparator();
		}

		menu.addAction(ActionsManager::getAction(ActionsManager::ContentBlockingAction, this));
		menu.addAction(getAction(ActionsManager::WebsitePreferencesAction));
		menu.addSeparator();
		menu.addAction(ActionsManager::getAction(ActionsManager::FullScreenAction, this));
	}
	else
	{
		if (flags & EditMenu)
		{
			menu.addAction(getAction(ActionsManager::PasteNoteAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::UndoAction));
			menu.addAction(getAction(ActionsManager::RedoAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::CutAction));
			menu.addAction(getAction(ActionsManager::CopyAction));
			menu.addAction(getAction(ActionsManager::PasteAction));
			menu.addAction(getAction(ActionsManager::DeleteAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::CopyToNoteAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::SelectAllAction));
			menu.addAction(getAction(ActionsManager::ClearAllAction));
			menu.addSeparator();

			if (flags & FormMenu)
			{
				menu.addAction(getAction(ActionsManager::CreateSearchAction));
				menu.addSeparator();
			}

			if (flags == EditMenu || flags == (EditMenu | FormMenu))
			{
				menu.addAction(getAction(ActionsManager::InspectElementAction));
				menu.addSeparator();
			}

			menu.addAction(getAction(ActionsManager::CheckSpellingAction));
			menu.addSeparator();
		}

		if (flags & SelectionMenu)
		{
			menu.addAction(getAction(ActionsManager::SearchAction));
			menu.addAction(getAction(ActionsManager::SearchMenuAction));
			menu.addSeparator();

			if (!(flags & EditMenu))
			{
				menu.addAction(getAction(ActionsManager::CopyAction));
				menu.addAction(getAction(ActionsManager::CopyToNoteAction));
				menu.addSeparator();
			}

			menu.addAction(getAction(ActionsManager::OpenSelectionAsLinkAction));
			menu.addSeparator();
		}

		if (flags & MailMenu)
		{
			menu.addAction(getAction(ActionsManager::OpenLinkAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::CopyLinkToClipboardAction));

			if (!(flags & ImageMenu))
			{
				menu.addAction(getAction(ActionsManager::InspectElementAction));
			}

			menu.addSeparator();
		}
		else if (flags & LinkMenu)
		{
			menu.addAction(getAction(ActionsManager::OpenLinkAction));
			menu.addAction(getAction(ActionsManager::OpenLinkInNewTabAction));
			menu.addAction(getAction(ActionsManager::OpenLinkInNewTabBackgroundAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::OpenLinkInNewWindowAction));
			menu.addAction(getAction(ActionsManager::OpenLinkInNewWindowBackgroundAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::BookmarkLinkAction));
			menu.addAction(getAction(ActionsManager::CopyLinkToClipboardAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::SaveLinkToDiskAction));
			menu.addAction(getAction(ActionsManager::SaveLinkToDownloadsAction));

			if (!(flags & ImageMenu))
			{
				menu.addAction(getAction(ActionsManager::InspectElementAction));
			}

			menu.addSeparator();
		}

		if (flags & ImageMenu)
		{
			menu.addAction(getAction(ActionsManager::OpenImageInNewTabAction));
			menu.addAction(getAction(ActionsManager::ReloadImageAction));
			menu.addAction(getAction(ActionsManager::CopyImageUrlToClipboardAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::SaveImageToDiskAction));
			menu.addAction(getAction(ActionsManager::CopyImageToClipboardAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::InspectElementAction));
			menu.addAction(getAction(ActionsManager::ImagePropertiesAction));
			menu.addSeparator();
		}

		if (flags & MediaMenu)
		{
			menu.addAction(getAction(ActionsManager::CopyMediaUrlToClipboardAction));
			menu.addAction(getAction(ActionsManager::SaveMediaToDiskAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::MediaPlayPauseAction));
			menu.addAction(getAction(ActionsManager::MediaMuteAction));
			menu.addAction(getAction(ActionsManager::MediaLoopAction));
			menu.addAction(getAction(ActionsManager::MediaControlsAction));
			menu.addSeparator();
			menu.addAction(getAction(ActionsManager::InspectElementAction));
			menu.addSeparator();
		}
	}

	menu.exec(mapToGlobal(position));
}

void WebWidget::updateQuickSearch()
{
	if (m_quickSearchMenu)
	{
		m_quickSearchMenu->clear();
	}
}

void WebWidget::setAlternateStyleSheets(const QStringList &styleSheets)
{
	m_alternateStyleSheets = styleSheets;
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

QMenu* WebWidget::getPasteNoteMenu()
{
	if (!m_pasteNoteMenu)
	{
		m_pasteNoteMenu = new Menu(Menu::NotesMenuRole, this);

		connect(m_pasteNoteMenu, SIGNAL(triggered(QAction*)), this, SLOT(pasteNote(QAction*)));
	}

	return m_pasteNoteMenu;
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

void WebWidget::setScrollMode(WebWidget::ScrollMode mode)
{
	m_scrollMode = mode;

	switch (mode)
	{
		case MoveScroll:
			if (!m_scrollCursors.contains(NoDirection))
			{
				m_scrollCursors[NoDirection] = QPixmap(QLatin1String(":/cursors/scroll-vertical.png"));
			}

			if (m_scrollTimer == 0)
			{
				m_scrollTimer = startTimer(10);
			}

			grabKeyboard();
			grabMouse();

			QApplication::setOverrideCursor(m_scrollCursors[NoDirection]);

			break;
		case DragScroll:
			QApplication::setOverrideCursor(Qt::ClosedHandCursor);

			break;
		default:
			QApplication::restoreOverrideCursor();

			break;
	}

	if (mode == NoScroll)
	{
		m_beginCursorPosition = QPoint();
		m_beginScrollPosition = QPoint();

		if (m_scrollTimer > 0)
		{
			killTimer(m_scrollTimer);

			m_scrollTimer = 0;

			releaseKeyboard();
			releaseMouse();
		}
	}
	else
	{
		m_beginCursorPosition = QCursor::pos();
		m_beginScrollPosition = getScrollPosition();

	}
}

WebBackend* WebWidget::getBackend()
{
	return m_backend;
}

QMenu* WebWidget::getReloadTimeMenu()
{
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

	return m_reloadTimeMenu;
}

QMenu* WebWidget::getQuickSearchMenu()
{
	if (!m_quickSearchMenu)
	{
		m_quickSearchMenu = new QMenu(this);

		connect(m_quickSearchMenu, SIGNAL(aboutToShow()), this, SLOT(quickSearchMenuAboutToShow()));
		connect(m_quickSearchMenu, SIGNAL(triggered(QAction*)), this, SLOT(quickSearch(QAction*)));
	}

	return m_quickSearchMenu;
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

QStringList WebWidget::getAlternateStyleSheets() const
{
	return m_alternateStyleSheets;
}

QList<FeedUrl> WebWidget::getFeeds() const
{
	return QList<FeedUrl>();
}

QVariantHash WebWidget::getOptions() const
{
	return m_options;
}

WebWidget::ScrollMode WebWidget::getScrollMode() const
{
	return m_scrollMode;
}

bool WebWidget::hasOption(const QString &key) const
{
	return m_options.contains(key);
}

}
