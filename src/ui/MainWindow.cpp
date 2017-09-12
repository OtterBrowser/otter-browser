/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "MainWindow.h"
#include "ClearHistoryDialog.h"
#include "ContentsWidget.h"
#include "WorkspaceWidget.h"
#include "Menu.h"
#include "MenuBarWidget.h"
#include "OpenAddressDialog.h"
#include "OpenBookmarkDialog.h"
#include "PreferencesDialog.h"
#include "SidebarWidget.h"
#include "StatusBarWidget.h"
#include "TabBarWidget.h"
#include "TabSwitcherWidget.h"
#include "ToolBarWidget.h"
#include "Window.h"
#include "preferences/ContentBlockingDialog.h"
#include "../core/ActionsManager.h"
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/GesturesManager.h"
#include "../core/InputInterpreter.h"
#include "../core/SessionModel.h"
#include "../core/SettingsManager.h"
#include "../core/ThemesManager.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"
#include "../core/WebBackend.h"

#include "ui_MainWindow.h"

#include <QtCore/QTimer>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>

namespace Otter
{

quint64 MainWindow::m_identifierCounter(0);

MainWindow::MainWindow(const QVariantMap &parameters, const SessionMainWindow &session, QWidget *parent) : QMainWindow(parent), ActionExecutor(),
	m_tabSwitcher(nullptr),
	m_workspace(new WorkspaceWidget(this)),
	m_tabBar(nullptr),
	m_menuBar(nullptr),
	m_statusBar(nullptr),
	m_currentWindow(nullptr),
	m_identifier(++m_identifierCounter),
	m_mouseTrackerTimer(0),
	m_tabSwitchingOrderIndex(-1),
	m_isAboutToClose(false),
	m_isDraggingToolBar(false),
	m_isPrivate((SessionsManager::isPrivate() || SettingsManager::getOption(SettingsManager::Browser_PrivateModeOption).toBool() || SessionsManager::calculateOpenHints(parameters).testFlag(SessionsManager::PrivateOpen))),
	m_isRestored(false),
	m_hasToolBars(!parameters.value(QLatin1String("noToolBars"), false).toBool()),
	m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);

	setUnifiedTitleAndToolBarOnMac(true);
	updateShortcuts();

	if (m_hasToolBars)
	{
		const QVector<Qt::ToolBarArea> areas({Qt::LeftToolBarArea, Qt::RightToolBarArea, Qt::TopToolBarArea, Qt::BottomToolBarArea});

		for (int i = 0; i < 4; ++i)
		{
			const Qt::ToolBarArea area(areas.at(i));
			QVector<ToolBarsManager::ToolBarDefinition> toolBarDefinitions(ToolBarsManager::getToolBarDefinitions(area));

			std::sort(toolBarDefinitions.begin(), toolBarDefinitions.end(), [&](const ToolBarsManager::ToolBarDefinition &first, const ToolBarsManager::ToolBarDefinition &second)
			{
				return (first.row > second.row);
			});

			for (int j = 0; j < toolBarDefinitions.count(); ++j)
			{
				ToolBarWidget *toolBar(new ToolBarWidget(toolBarDefinitions.at(j).identifier, nullptr, this));

				if (toolBarDefinitions.at(j).identifier == ToolBarsManager::TabBar)
				{
					m_tabBar = toolBar->findChild<TabBarWidget*>();
				}

				if (j > 0)
				{
					addToolBarBreak(area);
				}

				addToolBar(area, toolBar);
			}
		}
	}
	else
	{
		m_tabBar = new TabBarWidget(this);
		m_tabBar->hide();
	}

	setCentralWidget(m_workspace);

	if (m_hasToolBars && ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar).normalVisibility != ToolBarsManager::AlwaysHiddenToolBar)
	{
		m_menuBar = new MenuBarWidget(this);

		setMenuBar(m_menuBar);
	}

	if (m_hasToolBars && ToolBarsManager::getToolBarDefinition(ToolBarsManager::StatusBar).normalVisibility != ToolBarsManager::AlwaysHiddenToolBar)
	{
		m_statusBar = new StatusBarWidget(this);

		setStatusBar(m_statusBar);
	}

	connect(ActionsManager::getInstance(), SIGNAL(shortcutsChanged()), this, SLOT(updateShortcuts()));
	connect(SessionsManager::getInstance(), SIGNAL(requestedRemoveStoredUrl(QString)), this, SLOT(removeStoredUrl(QString)));
	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int)));
	connect(ToolBarsManager::getInstance(), SIGNAL(toolBarAdded(int)), this, SLOT(handleToolBarAdded(int)));
	connect(ToolBarsManager::getInstance(), SIGNAL(toolBarModified(int)), this, SLOT(handleToolBarModified(int)));
	connect(ToolBarsManager::getInstance(), SIGNAL(toolBarMoved(int)), this, SLOT(handleToolBarMoved(int)));
	connect(ToolBarsManager::getInstance(), SIGNAL(toolBarRemoved(int)), this, SLOT(handleToolBarRemoved(int)));
	connect(TransfersManager::getInstance(), SIGNAL(transferStarted(Transfer*)), this, SLOT(handleTransferStarted()));

	if (session.geometry.isEmpty())
	{
		if (Application::getActiveWindow())
		{
			resize(Application::getActiveWindow()->size());
		}
		else
		{
			showMaximized();
		}
	}
	else
	{
		restoreGeometry(session.geometry);
	}

	QTimer::singleShot(0, [=]()
	{
		restore(session);
		updateWindowTitle();
	});

	connect(m_workspace, SIGNAL(actionsStateChanged(QVector<int>)), this, SIGNAL(actionsStateChanged(QVector<int>)));
}

MainWindow::~MainWindow()
{
	delete m_ui;
}

void MainWindow::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_mouseTrackerTimer)
	{
		QVector<Qt::ToolBarArea> areas;
		const QPoint position(mapFromGlobal(QCursor::pos()));

		if (position.x() < 10)
		{
			areas.append(isLeftToRight() ? Qt::LeftToolBarArea : Qt::RightToolBarArea);
		}
		else if (position.x() > (contentsRect().width() - 10))
		{
			areas.append(isLeftToRight() ? Qt::RightToolBarArea : Qt::LeftToolBarArea);
		}

		if (position.y() < 10)
		{
			areas.append(Qt::TopToolBarArea);
		}
		else if (position.y() > (contentsRect().height() - 10))
		{
			areas.append(Qt::BottomToolBarArea);
		}

		for (int i = 0; i < areas.count(); ++i)
		{
			const QVector<ToolBarWidget*> toolBars(getToolBars(areas.at(i)));

			for (int j = 0; j < toolBars.count(); ++j)
			{
				if (toolBars.at(j)->getDefinition().fullScreenVisibility == ToolBarsManager::OnHoverVisibleToolBar)
				{
					toolBars.at(j)->show();
					toolBars.at(j)->installEventFilter(this);
				}
			}
		}
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	const bool isLastWindow(Application::getWindows().count() == 1);

	if (isLastWindow && !Application::canClose())
	{
		event->ignore();

		return;
	}

	m_isAboutToClose = true;

	if (!isLastWindow)
	{
		SessionsManager::storeClosedWindow(this);
	}
	else if (SessionsManager::getCurrentSession() == QLatin1String("default"))
	{
		SessionsManager::saveSession();
	}

	QHash<quint64, Window*>::const_iterator iterator;

	for (iterator = m_windows.constBegin(); iterator != m_windows.constEnd(); ++iterator)
	{
		iterator.value()->requestClose();
	}

	Application::removeWindow(this);

	event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
		case Qt::Key_Backtab:
		case Qt::Key_Tab:
			if (m_windows.count() < 2)
			{
				event->accept();

				return;
			}

			if (SettingsManager::getOption(SettingsManager::TabSwitcher_ShowListOption).toBool())
			{
				if (!m_tabSwitcher)
				{
					m_tabSwitcher = new TabSwitcherWidget(this);
				}

				m_tabSwitcher->raise();
				m_tabSwitcher->resize(size());
				m_tabSwitcher->show(TabSwitcherWidget::KeyboardReason);
				m_tabSwitcher->selectTab(event->key() == Qt::Key_Tab);
			}
			else if (SettingsManager::getOption(SettingsManager::TabSwitcher_OrderByLastActivityOption).toBool())
			{
				if (m_tabSwitchingOrderIndex < 0)
				{
					m_tabSwitchingOrderList = createOrderedWindowList(SettingsManager::getOption(SettingsManager::TabSwitcher_IgnoreMinimizedTabsOption).toBool());
					m_tabSwitchingOrderIndex = (m_tabSwitchingOrderList.count() - 1);
				}

				if (event->key() == Qt::Key_Tab)
				{
					--m_tabSwitchingOrderIndex;

					if (m_tabSwitchingOrderIndex < 0)
					{
						m_tabSwitchingOrderIndex = (m_tabSwitchingOrderList.count() - 1);
					}
				}
				else
				{
					++m_tabSwitchingOrderIndex;

					if (m_tabSwitchingOrderIndex >= m_tabSwitchingOrderList.count())
					{
						m_tabSwitchingOrderIndex = 0;
					}
				}

				setActiveWindowByIdentifier(m_tabSwitchingOrderList.value(m_tabSwitchingOrderIndex), false);
			}
			else
			{
				triggerAction((event->key() == Qt::Key_Tab) ? ActionsManager::ActivateTabOnRightAction : ActionsManager::ActivateTabOnLeftAction);
			}

			event->accept();

			break;
		case Qt::Key_Escape:
			if (m_tabSwitcher && m_tabSwitcher->isVisible())
			{
				m_tabSwitcher->hide();
			}

			break;
		default:
			QMainWindow::keyPressEvent(event);

			break;
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Control && m_tabSwitchingOrderIndex >= 0)
	{
		if (m_tabSwitchingOrderIndex != (m_tabSwitchingOrderList.count() - 1))
		{
			Window *window(m_windows.value(m_tabSwitchingOrderList.value(m_tabSwitchingOrderIndex)));

			if (window)
			{
				window->markAsActive();
			}
		}

		m_tabSwitchingOrderIndex = -1;

		m_tabSwitchingOrderList.clear();
	}

	QMainWindow::keyReleaseEvent(event);
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
	if (m_tabSwitcher && m_tabSwitcher->isVisible())
	{
		return;
	}

	if (event->reason() == QContextMenuEvent::Mouse)
	{
		event->accept();

		return;
	}

	Menu menu(Menu::ToolBarsMenuRole, this);
	menu.exec(event->globalPos());
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::RightButton && m_tabSwitcher && m_tabSwitcher->getReason() == TabSwitcherWidget::WheelReason)
	{
		m_tabSwitcher->accept();
	}

	QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::triggerAction(int identifier, const QVariantMap &parameters)
{
	switch (identifier)
	{
		case ActionsManager::NewTabAction:
			{
				QVariantMap mutableParameters(parameters);
				mutableParameters[QLatin1String("hints")] = SessionsManager::NewTabOpen;

				triggerAction(ActionsManager::OpenUrlAction, mutableParameters);
			}

			return;
		case ActionsManager::NewTabPrivateAction:
			{
				QVariantMap mutableParameters(parameters);
				mutableParameters[QLatin1String("hints")] = QVariant(SessionsManager::NewTabOpen | SessionsManager::PrivateOpen);

				triggerAction(ActionsManager::OpenUrlAction, mutableParameters);
			}

			return;
		case ActionsManager::OpenAction:
			{
				const QString path(Utils::getOpenPaths().value(0));

				if (!path.isEmpty())
				{
					triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl::fromLocalFile(path)}});
				}
			}

			return;
		case ActionsManager::MaximizeTabAction:
		case ActionsManager::MinimizeTabAction:
		case ActionsManager::RestoreTabAction:
		case ActionsManager::AlwaysOnTopTabAction:
		case ActionsManager::MaximizeAllAction:
		case ActionsManager::MinimizeAllAction:
		case ActionsManager::RestoreAllAction:
		case ActionsManager::CascadeAllAction:
		case ActionsManager::TileAllAction:
			m_workspace->triggerAction(identifier, parameters);

			return;
		case ActionsManager::CloseWindowAction:
			close();

			return;
		case ActionsManager::GoToPageAction:
			{
				OpenAddressDialog dialog(this);

				connect(&dialog, SIGNAL(requestedLoadUrl(QUrl,SessionsManager::OpenHints)), this, SLOT(open(QUrl,SessionsManager::OpenHints)));
				connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)), this, SLOT(open(BookmarksItem*,SessionsManager::OpenHints)));
				connect(&dialog, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)), this, SLOT(search(QString,QString,SessionsManager::OpenHints)));

				dialog.exec();
			}

			return;
		case ActionsManager::GoToHomePageAction:
			{
				const QString homePage(SettingsManager::getOption(SettingsManager::Browser_HomePageOption).toString());
				QVariantMap mutableParameters(parameters);
				mutableParameters[QLatin1String("url")] = QUrl(homePage);

				if (!homePage.isEmpty())
				{
					triggerAction(ActionsManager::OpenUrlAction, mutableParameters);
				}
			}

			return;
		case ActionsManager::BookmarksAction:
			{
				const QUrl url(QLatin1String("about:bookmarks"));

				if (!SessionsManager::hasUrl(url, true))
				{
					triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}});
				}
			}

			return;
		case ActionsManager::QuickBookmarkAccessAction:
			{
				OpenBookmarkDialog dialog(this);

				connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*)), this, SLOT(open(BookmarksItem*)));

				dialog.exec();
			}

			return;
		case ActionsManager::ClosePrivateTabsAction:
			for (int i = 0; i < m_privateWindows.count(); ++i)
			{
				m_privateWindows[i]->requestClose();
			}

			emit actionsStateChanged(QVector<int>({ActionsManager::ClosePrivateTabsAction}));

			return;
		case ActionsManager::OpenUrlAction:
			{
				QUrl url;

				if (parameters.contains(QLatin1String("urlPlaceholder")))
				{
					Window *window(parameters.contains(QLatin1String("tab")) ? getWindowByIdentifier(parameters[QLatin1String("tab")].toULongLong()) : m_workspace->getActiveWindow());

					if (window)
					{
						url = QUrl::fromUserInput(window->getContentsWidget()->parseQuery(parameters[QLatin1String("urlPlaceholder")].toString()));
					}
				}
				else if (parameters.contains(QLatin1String("url")))
				{
					url = ((parameters[QLatin1String("url")].type() == QVariant::Url) ? parameters[QLatin1String("url")].toUrl() : QUrl::fromUserInput(parameters[QLatin1String("url")].toString()));
				}

				if (parameters.contains(QLatin1String("application")))
				{
					Utils::runApplication(parameters[QLatin1String("application")].toString(), url);

					return;
				}

				SessionsManager::OpenHints hints(SessionsManager::calculateOpenHints(parameters));
				int index(parameters.value(QLatin1String("index"), -1).toInt());

				if (m_isPrivate)
				{
					hints |= SessionsManager::PrivateOpen;
				}

				QVariantMap mutableParameters(parameters);
				mutableParameters[QLatin1String("hints")] = QVariant(hints);

				if (!parameters.contains(QLatin1String("size")))
				{
					mutableParameters[QLatin1String("size")] = m_workspace->size();
				}

				if (hints.testFlag(SessionsManager::NewWindowOpen))
				{
					Application::createWindow(mutableParameters);

					return;
				}

				if (index >= 0)
				{
					Window *window(new Window(mutableParameters, nullptr, this));

					addWindow(window, hints, index);

					window->setUrl(((url.isEmpty() && SettingsManager::getOption(SettingsManager::StartPage_EnableStartPageOption).toBool()) ? QUrl(QLatin1String("about:start")) : url), false);

					return;
				}

				Window *activeWindow(m_workspace->getActiveWindow());
				const bool isUrlEmpty(activeWindow && activeWindow->getLoadingState() == WebWidget::FinishedLoadingState && Utils::isUrlEmpty(activeWindow->getUrl()));

				if (hints == SessionsManager::NewTabOpen && !url.isEmpty() && isUrlEmpty)
				{
					hints = SessionsManager::CurrentTabOpen;
				}
				else if (hints == SessionsManager::DefaultOpen && url.scheme() == QLatin1String("about") && !url.path().isEmpty() && url.path() != QLatin1String("blank") && url.path() != QLatin1String("start") && (!activeWindow || !Utils::isUrlEmpty(activeWindow->getUrl())))
				{
					hints = SessionsManager::NewTabOpen;
				}
				else if (hints == SessionsManager::DefaultOpen && url.scheme() != QLatin1String("javascript") && (isUrlEmpty || SettingsManager::getOption(SettingsManager::Browser_ReuseCurrentTabOption).toBool()))
				{
					hints = SessionsManager::CurrentTabOpen;
				}

				const bool isReplacing(hints.testFlag(SessionsManager::CurrentTabOpen) && activeWindow);
				const WindowState windowState(isReplacing ? activeWindow->getWindowState() : WindowState());
				const bool isAlwaysOnTop(isReplacing ? activeWindow->getSession().isAlwaysOnTop : false);

				mutableParameters[QLatin1String("hints")] = QVariant(hints);

				if (isReplacing)
				{
					if (activeWindow->getType() == QLatin1String("web") && activeWindow->getWebWidget() && !parameters.contains(QLatin1String("webBackend")))
					{
						mutableParameters[QLatin1String("webBackend")] = activeWindow->getWebWidget()->getBackend()->getName();
					}

					activeWindow->requestClose();
				}

				Window *window(new Window(mutableParameters, nullptr, this));

				addWindow(window, hints, index, windowState, isAlwaysOnTop);

				window->setUrl(((url.isEmpty() && SettingsManager::getOption(SettingsManager::StartPage_EnableStartPageOption).toBool()) ? QUrl(QLatin1String("about:start")) : url), false);
			}

			return;
		case ActionsManager::ReopenTabAction:
			restore(parameters.value(QLatin1String("index"), 0).toInt());

			return;
		case ActionsManager::StopAllAction:
			{
				QHash<quint64, Window*>::const_iterator iterator;

				for (iterator = m_windows.constBegin(); iterator != m_windows.constEnd(); ++iterator)
				{
					iterator.value()->triggerAction(ActionsManager::StopAction);
				}
			}

			return;
		case ActionsManager::ReloadAllAction:
			{
				QHash<quint64, Window*>::const_iterator iterator;

				for (iterator = m_windows.constBegin(); iterator != m_windows.constEnd(); ++iterator)
				{
					iterator.value()->triggerAction(ActionsManager::ReloadAction);
				}
			}

			return;
		case ActionsManager::ActivatePreviouslyUsedTabAction:
		case ActionsManager::ActivateLeastRecentlyUsedTabAction:
			{
				const QVector<quint64> windows(createOrderedWindowList(parameters.contains(QLatin1String("includeMinimized"))));

				if (windows.count() > 1)
				{
					setActiveWindowByIdentifier((identifier == ActionsManager::ActivatePreviouslyUsedTabAction) ? windows.at(windows.count() - 2) : windows.first());
				}
			}

			return;
		case ActionsManager::ActivateTabOnLeftAction:
			setActiveWindowByIndex((getCurrentWindowIndex() > 0) ? (getCurrentWindowIndex() - 1) : (m_windows.count() - 1));

			return;
		case ActionsManager::ActivateTabOnRightAction:
			setActiveWindowByIndex(((getCurrentWindowIndex() + 1) < m_windows.count()) ? (getCurrentWindowIndex() + 1) : 0);

			return;
		case ActionsManager::BookmarkAllOpenPagesAction:
			{
				const MainWindowSessionItem *mainWindowItem(SessionsManager::getModel()->getMainWindowItem(this));

				if (mainWindowItem)
				{
					for (int i = 0; i < mainWindowItem->rowCount(); ++i)
					{
						const WindowSessionItem *windowItem(static_cast<WindowSessionItem*>(mainWindowItem->child(i, 0)));

						if (windowItem && !Utils::isUrlEmpty(windowItem->getActiveWindow()->getUrl()))
						{
							BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, {{BookmarksModel::UrlRole, windowItem->getActiveWindow()->getUrl()}, {BookmarksModel::TitleRole, windowItem->getActiveWindow()->getTitle()}}, (parameters.contains(QLatin1String("folder")) ? BookmarksManager::getBookmark(parameters[QLatin1String("folder")].toULongLong()) : nullptr));
						}
					}
				}
			}

			return;
		case ActionsManager::OpenBookmarkAction:
			if (parameters.contains(QLatin1String("bookmark")))
			{
				const BookmarksItem *bookmark(BookmarksManager::getBookmark(parameters[QLatin1String("bookmark")].toULongLong()));

				if (!bookmark)
				{
					return;
				}

				QVariantMap mutableParameters(parameters);
				mutableParameters.remove(QLatin1String("bookmark"));

				switch (static_cast<BookmarksModel::BookmarkType>(bookmark->data(BookmarksModel::TypeRole).toInt()))
				{
					case BookmarksModel::UrlBookmark:
						mutableParameters[QLatin1String("url")] = bookmark->data(BookmarksModel::UrlRole).toUrl();

						triggerAction(ActionsManager::OpenUrlAction, mutableParameters);

						break;
					case BookmarksModel::RootBookmark:
					case BookmarksModel::FolderBookmark:
						{
							const QVector<QUrl> urls(bookmark->getUrls());
							bool canOpen(true);

							if (urls.count() > 1 && SettingsManager::getOption(SettingsManager::Choices_WarnOpenBookmarkFolderOption).toBool() && !parameters.value(QLatin1String("ignoreWarning"), false).toBool())
							{
								QMessageBox messageBox;
								messageBox.setWindowTitle(tr("Question"));
								messageBox.setText(tr("You are about to open %n bookmark(s).", "", urls.count()));
								messageBox.setInformativeText(tr("Do you want to continue?"));
								messageBox.setIcon(QMessageBox::Question);
								messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
								messageBox.setDefaultButton(QMessageBox::Yes);
								messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

								if (messageBox.exec() == QMessageBox::Cancel)
								{
									canOpen = false;
								}

								SettingsManager::setOption(SettingsManager::Choices_WarnOpenBookmarkFolderOption, !messageBox.checkBox()->isChecked());
							}

							if (urls.isEmpty() || !canOpen)
							{
								break;
							}

							const SessionsManager::OpenHints hints(SessionsManager::calculateOpenHints(parameters));
							int index(parameters.value(QLatin1String("index"), -1).toInt());

							if (index < 0)
							{
								index = ((!hints.testFlag(SessionsManager::EndOpen) && SettingsManager::getOption(SettingsManager::TabBar_OpenNextToActiveOption).toBool()) ? (getCurrentWindowIndex() + 1) : (m_windows.count() - 1));
							}

							mutableParameters[QLatin1String("url")] = urls.at(0);
							mutableParameters[QLatin1String("hints")] = QVariant(hints);
							mutableParameters[QLatin1String("index")] = index;

							triggerAction(ActionsManager::OpenUrlAction, mutableParameters);

							mutableParameters[QLatin1String("hints")] = QVariant((hints == SessionsManager::DefaultOpen || hints.testFlag(SessionsManager::CurrentTabOpen)) ? SessionsManager::NewTabOpen : hints);

							for (int i = 1; i < urls.count(); ++i)
							{
								mutableParameters[QLatin1String("url")] = urls.at(i);
								mutableParameters[QLatin1String("index")] = (index + i);

								triggerAction(ActionsManager::OpenUrlAction, mutableParameters);
							}
						}

						break;
					default:
						break;
				}
			}

			return;
		case ActionsManager::ShowTabSwitcherAction:
			if (!m_tabSwitcher)
			{
				m_tabSwitcher = new TabSwitcherWidget(this);
			}

			m_tabSwitcher->raise();
			m_tabSwitcher->resize(size());
			m_tabSwitcher->show(TabSwitcherWidget::ActionReason);

			return;
		case ActionsManager::ShowToolBarAction:
			if (parameters.contains(QLatin1String("toolBar")))
			{
				ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition((parameters[QLatin1String("toolBar")].type() == QVariant::String) ? ToolBarsManager::getToolBarIdentifier(parameters[QLatin1String("toolBar")].toString()) : parameters[QLatin1String("toolBar")].toInt()));
				definition.normalVisibility = (parameters.value(QLatin1String("isChecked"), !getActionState(identifier, parameters).isChecked).toBool() ? ToolBarsManager::AlwaysVisibleToolBar : ToolBarsManager::AlwaysHiddenToolBar);

				ToolBarsManager::setToolBar(definition);
			}

			return;
		case ActionsManager::ShowMenuBarAction:
			{
				ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar));
				definition.normalVisibility = (parameters.value(QLatin1String("isChecked"), !getActionState(identifier, parameters).isChecked).toBool() ? ToolBarsManager::AlwaysVisibleToolBar : ToolBarsManager::AlwaysHiddenToolBar);

				ToolBarsManager::setToolBar(definition);
			}

			return;
		case ActionsManager::ShowTabBarAction:
			{
				ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(ToolBarsManager::TabBar));
				definition.normalVisibility = (parameters.value(QLatin1String("isChecked"), !getActionState(identifier, parameters).isChecked).toBool() ? ToolBarsManager::AlwaysVisibleToolBar : ToolBarsManager::AlwaysHiddenToolBar);

				ToolBarsManager::setToolBar(definition);
			}

			return;
		case ActionsManager::ShowSidebarAction:
			{
				ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(parameters.value(QLatin1String("sidebar"), ToolBarsManager::SideBar).toInt()));

				if (definition.isValid())
				{
					const bool isFullScreen(windowState().testFlag(Qt::WindowFullScreen));
					ToolBarsManager::ToolBarVisibility visibility(isFullScreen ? definition.fullScreenVisibility : definition.normalVisibility);
					const bool fallback(!parameters.value(QLatin1String("panel")).toString().isEmpty() || !(visibility == ToolBarsManager::AlwaysVisibleToolBar));
					const bool isChecked(parameters.contains(QLatin1String("sidebar")) ? parameters.value(QLatin1String("isChecked"), fallback).toBool() : fallback);

					visibility = (isChecked ? ToolBarsManager::AlwaysVisibleToolBar : ToolBarsManager::AlwaysHiddenToolBar);

					if (isFullScreen)
					{
						definition.fullScreenVisibility = visibility;
					}
					else
					{
						definition.normalVisibility = visibility;
					}

					if (parameters.contains(QLatin1String("panel")))
					{
						definition.currentPanel = parameters.value(QLatin1String("panel")).toString();
					}

					ToolBarsManager::setToolBar(definition);
				}
			}

			return;
		case ActionsManager::ShowErrorConsoleAction:
			{
				ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(ToolBarsManager::ErrorConsoleBar));
				definition.normalVisibility = (parameters.value(QLatin1String("isChecked"), !getActionState(identifier, parameters).isChecked).toBool() ? ToolBarsManager::AlwaysVisibleToolBar : ToolBarsManager::AlwaysHiddenToolBar);

				ToolBarsManager::setToolBar(definition);
			}

			return;
		case ActionsManager::OpenPanelAction:
			{
				ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(parameters.value(QLatin1String("sidebar"), ToolBarsManager::SideBar).toInt()));

				if (definition.isValid() && !definition.currentPanel.isEmpty())
				{
					triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), SidebarWidget::getPanelUrl(definition.currentPanel)}, {QLatin1String("hints"), SessionsManager::NewTabOpen}});
				}
			}

			return;
		case ActionsManager::ClosePanelAction:
			{
				ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(parameters.value(QLatin1String("sidebar"), ToolBarsManager::SideBar).toInt()));
				definition.currentPanel = QString();

				if (definition.isValid())
				{
					ToolBarsManager::setToolBar(definition);
				}
			}

			return;
		case ActionsManager::ContentBlockingAction:
			{
				ContentBlockingDialog dialog(this);
				dialog.exec();
			}

			return;
		case ActionsManager::HistoryAction:
			{
				const QUrl url(QLatin1String("about:history"));

				if (!SessionsManager::hasUrl(url, true))
				{
					triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}});
				}
			}

			return;
		case ActionsManager::ClearHistoryAction:
			{
				ClearHistoryDialog dialog(SettingsManager::getOption(SettingsManager::History_ManualClearOptionsOption).toStringList(), false, this);
				dialog.exec();
			}

			return;
		case ActionsManager::AddonsAction:
			{
				const QUrl url(QLatin1String("about:addons"));

				if (!SessionsManager::hasUrl(url, true))
				{
					triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}});
				}
			}

			return;
		case ActionsManager::NotesAction:
			{
				const QUrl url(QLatin1String("about:notes"));

				if (!SessionsManager::hasUrl(url, true))
				{
					triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}});
				}
			}

			return;
		case ActionsManager::PasswordsAction:
			{
				const QUrl url(QLatin1String("about:passwords"));

				if (!SessionsManager::hasUrl(url, true))
				{
					triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}});
				}
			}

			return;
		case ActionsManager::TransfersAction:
			{
				const QUrl url(QLatin1String("about:transfers"));

				if (!SessionsManager::hasUrl(url, true))
				{
					triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}});
				}
			}

			return;
		case ActionsManager::CookiesAction:
			{
				const QUrl url(QLatin1String("about:cookies"));

				if (!SessionsManager::hasUrl(url, true))
				{
					triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}});
				}
			}

			return;
		case ActionsManager::FullScreenAction:
			{
				if (isFullScreen())
				{
					restoreWindowState();
				}
				else
				{
					storeWindowState();
					showFullScreen();
				}

				QHash<quint64, Window*>::const_iterator iterator;

				for (iterator = m_windows.constBegin(); iterator != m_windows.constEnd(); ++iterator)
				{
					iterator.value()->triggerAction(identifier, parameters);
				}
			}

			return;
		case ActionsManager::PreferencesAction:
			{
				PreferencesDialog dialog(QLatin1String("general"), this);
				dialog.exec();
			}

			return;
		default:
			break;
	}

	Window *window(nullptr);

	if (parameters.contains(QLatin1String("tab")))
	{
		window = getWindowByIdentifier(parameters[QLatin1String("tab")].toULongLong());
	}
	else
	{
		window = m_workspace->getActiveWindow();
	}

	switch (identifier)
	{
		case ActionsManager::CloseOtherTabsAction:
			if (window)
			{
				QHash<quint64, Window*>::const_iterator iterator;

				for (iterator = m_windows.constBegin(); iterator != m_windows.constEnd(); ++iterator)
				{
					if (iterator.value() != window && !iterator.value()->isPinned())
					{
						iterator.value()->requestClose();
					}
				}
			}

			break;
		case ActionsManager::ActivateTabAction:
			if (window)
			{
				setActiveWindowByIdentifier(window->getIdentifier());
			}

			break;
		default:
			if (identifier == ActionsManager::PasteAndGoAction && (!window || window->getType() != QLatin1String("web")))
			{
				QVariantMap mutableParameters(parameters);

				if (m_isPrivate)
				{
					mutableParameters[QLatin1String("hints")] = QVariant(SessionsManager::calculateOpenHints(parameters) | SessionsManager::PrivateOpen);
				}

				window = new Window(mutableParameters, nullptr, this);

				addWindow(window, SessionsManager::NewTabOpen);

				window->triggerAction(ActionsManager::PasteAndGoAction);
			}
			else if (window)
			{
				window->triggerAction(identifier, parameters);
			}

			break;
	}
}

void MainWindow::open(const QUrl &url, SessionsManager::OpenHints hints)
{
	triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(hints)}});
}

void MainWindow::open(BookmarksItem *bookmark, SessionsManager::OpenHints hints)
{
	if (bookmark)
	{
		triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark->data(BookmarksModel::IdentifierRole)}, {QLatin1String("hints"), QVariant(hints)}});
	}
}

void MainWindow::openUrl(const QString &text, bool isPrivate)
{
	SessionsManager::OpenHints hints(isPrivate ? SessionsManager::PrivateOpen : SessionsManager::DefaultOpen);

	if (text.isEmpty())
	{
		triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("hints"), (hints | ActionsManager::NewTabAction)}});

		return;
	}

	InputInterpreter *interpreter(new InputInterpreter(this));

	connect(interpreter, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)), this, SLOT(open(BookmarksItem*,SessionsManager::OpenHints)));
	connect(interpreter, SIGNAL(requestedOpenUrl(QUrl,SessionsManager::OpenHints)), this, SLOT(open(QUrl,SessionsManager::OpenHints)));
	connect(interpreter, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)), this, SLOT(search(QString,QString,SessionsManager::OpenHints)));

	const Window *window(m_workspace->getActiveWindow());

	if (!window || (window->getLoadingState() == WebWidget::FinishedLoadingState && Utils::isUrlEmpty(window->getUrl())))
	{
		hints |= SessionsManager::CurrentTabOpen;
	}
	else
	{
		hints |= SessionsManager::NewTabOpen;
	}

	interpreter->interpret(text, hints);
}

void MainWindow::search(const QString &query, const QString &searchEngine, SessionsManager::OpenHints hints)
{
	Window *window(m_workspace->getActiveWindow());
	const bool isUrlEmpty(window && window->getLoadingState() == WebWidget::FinishedLoadingState && Utils::isUrlEmpty(window->getUrl()));

	if ((hints == SessionsManager::NewTabOpen && isUrlEmpty) || (hints == SessionsManager::DefaultOpen && (isUrlEmpty || SettingsManager::getOption(SettingsManager::Browser_ReuseCurrentTabOption).toBool())))
	{
		hints = SessionsManager::CurrentTabOpen;
	}

	if (window && hints.testFlag(SessionsManager::CurrentTabOpen) && window->getType() == QLatin1String("web"))
	{
		window->search(query, searchEngine);

		return;
	}

	if (window && window->canClone())
	{
		window = window->clone(false, this);

		addWindow(window, hints);
	}
	else
	{
		triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("hints"), QVariant(hints)}});

		window = m_workspace->getActiveWindow();
	}

	if (window)
	{
		window->search(query, searchEngine);
	}
}

void MainWindow::restore(const SessionMainWindow &session)
{
	disconnect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(setActiveWindowByIndex(int)));

	int index(session.index);

	if (index >= session.windows.count())
	{
		index = -1;
	}

	if (session.windows.isEmpty())
	{
		m_isRestored = true;

		if (SettingsManager::getOption(SettingsManager::Interface_LastTabClosingActionOption).toString() != QLatin1String("doNothing"))
		{
			triggerAction(ActionsManager::NewTabAction);
		}
		else
		{
			setCurrentWindow(nullptr);
		}
	}
	else
	{
		for (int i = 0; i < session.windows.count(); ++i)
		{
			QVariantMap parameters({{QLatin1String("size"), ((session.windows.at(i).state.state == Qt::WindowMaximized || !session.windows.at(i).state.geometry.isValid()) ? m_workspace->size() : session.windows.at(i).state.geometry.size())}});

			if (m_isPrivate)
			{
				parameters[QLatin1String("hints")] = SessionsManager::PrivateOpen;
			}

			Window *window(new Window(parameters, nullptr, this));
			window->setSession(session.windows.at(i), SettingsManager::getOption(SettingsManager::Sessions_DeferTabsLoadingOption).toBool());

			if (index < 0 && session.windows.at(i).state.state != Qt::WindowMinimized)
			{
				index = i;
			}

			addWindow(window, SessionsManager::DefaultOpen, -1, session.windows.at(i).state, session.windows.at(i).isAlwaysOnTop);
		}
	}

	m_isRestored = true;

	connect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(setActiveWindowByIndex(int)));

	setActiveWindowByIndex(index);

	m_workspace->markAsRestored();
}

void MainWindow::restore(int index)
{
	if (index < 0 || index >= m_closedWindows.count())
	{
		return;
	}

	const ClosedWindow closedWindow(m_closedWindows.at(index));
	int windowIndex(-1);

	if (closedWindow.previousWindow == 0)
	{
		windowIndex = 0;
	}
	else if (closedWindow.nextWindow == 0)
	{
		windowIndex = m_windows.count();
	}
	else
	{
		const int previousIndex(getWindowIndex(closedWindow.previousWindow));

		if (previousIndex >= 0)
		{
			windowIndex = (previousIndex + 1);
		}
		else
		{
			const int nextIndex(getWindowIndex(closedWindow.nextWindow));

			if (nextIndex >= 0)
			{
				windowIndex = qMax(0, (nextIndex - 1));
			}
		}
	}

	QVariantMap parameters({{QLatin1String("size"), ((closedWindow.window.state.state == Qt::WindowMaximized || !closedWindow.window.state.geometry.isValid()) ? m_workspace->size() : closedWindow.window.state.geometry.size())}});

	if (m_isPrivate || closedWindow.isPrivate)
	{
		parameters[QLatin1String("hints")] = SessionsManager::PrivateOpen;
	}

	Window *window(new Window(parameters, nullptr, this));
	window->setSession(closedWindow.window, false);

	m_closedWindows.removeAt(index);

	if (m_closedWindows.isEmpty() && SessionsManager::getClosedWindows().isEmpty())
	{
		emit closedWindowsAvailableChanged(false);
	}

	addWindow(window, SessionsManager::DefaultOpen, windowIndex, closedWindow.window.state, closedWindow.window.isAlwaysOnTop);
}

void MainWindow::clearClosedWindows()
{
	m_closedWindows.clear();

	if (SessionsManager::getClosedWindows().isEmpty())
	{
		emit closedWindowsAvailableChanged(false);
	}
}

void MainWindow::addWindow(Window *window, SessionsManager::OpenHints hints, int index, const WindowState &state, bool isAlwaysOnTop)
{
	if (!window)
	{
		return;
	}

	m_windows[window->getIdentifier()] = window;

	if (!m_isPrivate && window->isPrivate())
	{
		m_privateWindows.append(window);

		emit actionsStateChanged(QVector<int>({ActionsManager::ClosePrivateTabsAction}));
	}

	if (index < 0)
	{
		index = ((!hints.testFlag(SessionsManager::EndOpen) && SettingsManager::getOption(SettingsManager::TabBar_OpenNextToActiveOption).toBool()) ? (getCurrentWindowIndex() + 1) : (m_windows.count() - 1));
	}

	if (m_isRestored && SettingsManager::getOption(SettingsManager::TabBar_PrependPinnedTabOption).toBool() && !window->isPinned())
	{
		for (int i = 0; i < m_windows.count(); ++i)
		{
			const Window *window(getWindowByIndex(i));

			if (!window || !window->isPinned())
			{
				if (index < i)
				{
					index = i;
				}

				break;
			}
		}
	}

	m_tabBar->addTab(index, window);
	m_workspace->addWindow(window, state, isAlwaysOnTop);

	if (m_tabSwitchingOrderIndex >= 0)
	{
		m_tabSwitchingOrderList.append(window->getIdentifier());
	}

	if (!hints.testFlag(SessionsManager::BackgroundOpen) || m_windows.count() < 2)
	{
		m_tabBar->setCurrentIndex(index);

		if (m_isRestored)
		{
			setActiveWindowByIndex(index);
		}
	}

	if (m_isRestored)
	{
		const QString newTabOpeningAction(SettingsManager::getOption(SettingsManager::Interface_NewTabOpeningActionOption).toString());

		if (newTabOpeningAction == QLatin1String("cascadeAll"))
		{
			triggerAction(ActionsManager::CascadeAllAction);
		}
		else if (newTabOpeningAction == QLatin1String("tileAll"))
		{
			triggerAction(ActionsManager::TileAllAction);
		}
	}

	connect(window, &Window::needsAttention, [&]()
	{
		QApplication::alert(this);
	});
	connect(window, SIGNAL(titleChanged(QString)), this, SLOT(updateWindowTitle()));
	connect(window, SIGNAL(requestedOpenUrl(QUrl,SessionsManager::OpenHints)), this, SLOT(open(QUrl,SessionsManager::OpenHints)));
	connect(window, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)), this, SLOT(open(BookmarksItem*,SessionsManager::OpenHints)));
	connect(window, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)), this, SLOT(search(QString,QString,SessionsManager::OpenHints)));
	connect(window, SIGNAL(requestedNewWindow(ContentsWidget*,SessionsManager::OpenHints)), this, SLOT(openWindow(ContentsWidget*,SessionsManager::OpenHints)));
	connect(window, SIGNAL(requestedCloseWindow(Window*)), this, SLOT(handleWindowClose(Window*)));
	connect(window, SIGNAL(isPinnedChanged(bool)), this, SLOT(handleWindowIsPinnedChanged(bool)));

	emit windowAdded(window->getIdentifier());
}

void MainWindow::moveWindow(Window *window, MainWindow *mainWindow, int index)
{
	Window *newWindow(nullptr);
	SessionsManager::OpenHints hints(mainWindow ? SessionsManager::DefaultOpen : SessionsManager::NewWindowOpen);

	if (window->isPrivate())
	{
		hints |= SessionsManager::PrivateOpen;
	}

	window->getContentsWidget()->setParent(nullptr);

	if (mainWindow)
	{
		newWindow = mainWindow->openWindow(window->getContentsWidget(), hints, index);
	}
	else
	{
		newWindow = openWindow(window->getContentsWidget(), hints);
	}

	if (newWindow && window->isPinned())
	{
		newWindow->setPinned(true);
	}

	m_tabBar->removeTab(getWindowIndex(window->getIdentifier()));

	m_windows.remove(window->getIdentifier());

	if (!m_isPrivate && window->isPrivate())
	{
		m_privateWindows.removeAll(window);
	}

	if (mainWindow && m_windows.isEmpty())
	{
		close();
	}
	else
	{
		if (m_tabSwitchingOrderIndex >= 0)
		{
			m_tabSwitchingOrderList.removeAll(window->getIdentifier());
		}

		emit actionsStateChanged(QVector<int>({ActionsManager::ClosePrivateTabsAction}));
		emit windowRemoved(window->getIdentifier());
	}
}

void MainWindow::storeWindowState()
{
	m_previousState = windowState();
}

void MainWindow::restoreWindowState()
{
	setWindowState(m_previousState);
}

void MainWindow::raiseWindow()
{
	setWindowState(m_previousRaisedState);
}

void MainWindow::removeStoredUrl(const QString &url)
{
	for (int i = (m_closedWindows.count() - 1); i >= 0; --i)
	{
		if (url == m_closedWindows.at(i).window.getUrl())
		{
			m_closedWindows.removeAt(i);

			break;
		}
	}

	if (m_closedWindows.isEmpty())
	{
		emit closedWindowsAvailableChanged(false);
	}
}

void MainWindow::beginToolBarDragging(bool isSidebar)
{
	if (m_isDraggingToolBar || !QGuiApplication::mouseButtons().testFlag(Qt::LeftButton))
	{
		return;
	}

	m_isDraggingToolBar = true;

	const QList<ToolBarWidget*> toolBars(findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly));

	for (int i = 0; i < toolBars.count(); ++i)
	{
		if (toolBars.at(i)->isVisible() && (!isSidebar || toolBars.at(i)->getArea() == Qt::LeftToolBarArea || toolBars.at(i)->getArea() == Qt::RightToolBarArea))
		{
			insertToolBar(toolBars.at(i), new ToolBarDropZoneWidget(this));
			insertToolBarBreak(toolBars.at(i));
		}
	}

	const QVector<Qt::ToolBarArea> areas({Qt::LeftToolBarArea, Qt::RightToolBarArea, Qt::TopToolBarArea, Qt::BottomToolBarArea});

	for (int i = 0; i < 4; ++i)
	{
		if (!isSidebar || areas.at(i) == Qt::LeftToolBarArea || areas.at(i) == Qt::RightToolBarArea)
		{
			addToolBarBreak(areas.at(i));
			addToolBar(areas.at(i), new ToolBarDropZoneWidget(this));
		}
	}
}

void MainWindow::endToolBarDragging()
{
	const QList<ToolBarDropZoneWidget*> toolBars(findChildren<ToolBarDropZoneWidget*>());

	for (int i = 0; i < toolBars.count(); ++i)
	{
		removeToolBarBreak(toolBars.at(i));
		removeToolBar(toolBars.at(i));

		toolBars.at(i)->deleteLater();
	}

	m_isDraggingToolBar = false;
}

void MainWindow::saveToolBarPositions()
{
	const QList<Qt::ToolBarArea> areas({Qt::LeftToolBarArea, Qt::RightToolBarArea, Qt::TopToolBarArea, Qt::BottomToolBarArea});

	for (int i = 0; i < 4; ++i)
	{
		const QVector<ToolBarWidget*> toolBars(getToolBars(areas.at(i)));

		for (int j = 0; j < toolBars.count(); ++j)
		{
			ToolBarsManager::ToolBarDefinition definition(toolBars.at(j)->getDefinition());
			definition.location = areas.at(i);
			definition.row = j;

			ToolBarsManager::setToolBar(definition);
		}
	}
}

void MainWindow::handleOptionChanged(int identifier)
{
	if (identifier == SettingsManager::Browser_HomePageOption)
	{
		emit actionsStateChanged(QVector<int>({ActionsManager::GoToHomePageAction}));
	}
}

void MainWindow::handleWindowClose(Window *window)
{
	const int index(window ? getWindowIndex(window->getIdentifier()) : -1);

	if (index < 0)
	{
		return;
	}

	if (!m_isAboutToClose && (!window->isPrivate() || SettingsManager::getOption(SettingsManager::History_RememberClosedPrivateTabsOption).toBool()))
	{
		const WindowHistoryInformation history(window->getHistory());

		if (!history.isEmpty())
		{
			const Window *nextWindow(getWindowByIndex(index + 1));
			const Window *previousWindow((index > 0) ? getWindowByIndex(index - 1) : nullptr);
			ClosedWindow closedWindow;
			closedWindow.window = window->getSession();
			closedWindow.icon = window->getIcon();
			closedWindow.nextWindow = (nextWindow ? nextWindow->getIdentifier() : 0);
			closedWindow.previousWindow = (previousWindow ? previousWindow->getIdentifier() : 0);
			closedWindow.isPrivate = window->isPrivate();

			if (window->getType() != QLatin1String("web"))
			{
				removeStoredUrl(closedWindow.window.getUrl());
			}

			m_closedWindows.prepend(closedWindow);

			emit closedWindowsAvailableChanged(true);
		}
	}

	const QString lastTabClosingAction(SettingsManager::getOption(SettingsManager::Interface_LastTabClosingActionOption).toString());

	if (m_windows.count() == 1)
	{
		if (lastTabClosingAction == QLatin1String("closeWindow") || (lastTabClosingAction == QLatin1String("closeWindowIfNotLast") && Application::getWindows().count() > 1))
		{
			triggerAction(ActionsManager::CloseWindowAction);

			return;
		}

		if (lastTabClosingAction == QLatin1String("openTab"))
		{
			window = getWindowByIndex(0);

			if (window)
			{
				window->clear();

				return;
			}
		}
		else
		{
			setCurrentWindow(nullptr);

			m_workspace->setActiveWindow(nullptr);

			emit titleChanged(QString());
		}
	}

	m_tabBar->removeTab(index);

	if (m_tabSwitchingOrderIndex >= 0)
	{
		m_tabSwitchingOrderList.removeAll(window->getIdentifier());
	}

	emit windowRemoved(window->getIdentifier());

	m_windows.remove(window->getIdentifier());

	if (!m_isPrivate && window->isPrivate())
	{
		m_privateWindows.removeAll(window);
	}

	if (m_windows.isEmpty() && lastTabClosingAction == QLatin1String("openTab"))
	{
		triggerAction(ActionsManager::NewTabAction);
	}

	emit actionsStateChanged(QVector<int>({ActionsManager::ClosePrivateTabsAction}));
}

void MainWindow::handleWindowIsPinnedChanged(bool isPinned)
{
	const Window *modifiedWindow(qobject_cast<Window*>(sender()));

	if (!modifiedWindow || !m_isRestored || !SettingsManager::getOption(SettingsManager::TabBar_PrependPinnedTabOption).toBool())
	{
		return;
	}

	int amountOfLeadingPinnedTabs(0);
	int index(-1);

	for (int i = 0; i < m_windows.count(); ++i)
	{
		const Window *window(getWindowByIndex(i));

		if ((window && window->isPinned()) || (!isPinned && window == modifiedWindow))
		{
			++amountOfLeadingPinnedTabs;
		}
		else
		{
			break;
		}
	}

	for (int i = 0; i < m_windows.count(); ++i)
	{
		if (getWindowByIndex(i) == modifiedWindow)
		{
			index = i;

			break;
		}
	}

	if (index < 0)
	{
		return;
	}

	if (!isPinned && index < amountOfLeadingPinnedTabs)
	{
		--amountOfLeadingPinnedTabs;
	}

	if ((isPinned && index > amountOfLeadingPinnedTabs) || (!isPinned && index < amountOfLeadingPinnedTabs))
	{
		m_tabBar->moveTab(index, amountOfLeadingPinnedTabs);
	}
}

void MainWindow::handleToolBarAdded(int identifier)
{
	const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(identifier));
	QVector<ToolBarWidget*> toolBars(getToolBars(definition.location));
	ToolBarWidget *toolBar(new ToolBarWidget(identifier, nullptr, this));

	if (toolBars.isEmpty() || definition.row < 0)
	{
		addToolBar(definition.location, toolBar);
	}
	else
	{
		for (int i = 0; i < toolBars.count(); ++i)
		{
			removeToolBar(toolBars.at(i));
		}

		toolBars.append(toolBar);

		std::sort(toolBars.begin(), toolBars.end(), [&](ToolBarWidget *first, ToolBarWidget *second)
		{
			return (first->getDefinition().row > second->getDefinition().row);
		});

		const bool isFullScreen(windowState().testFlag(Qt::WindowFullScreen));

		for (int i = 0; i < toolBars.count(); ++i)
		{
			if (i > 0)
			{
				addToolBarBreak(definition.location);
			}

			addToolBar(definition.location, toolBars.at(i));

			toolBars.at(i)->setVisible(toolBars.at(i)->shouldBeVisible(isFullScreen));
		}
	}
}

void MainWindow::handleToolBarModified(int identifier)
{
	switch (identifier)
	{
		case ToolBarsManager::MenuBar:
			{
				const bool showMenuBar(ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar).normalVisibility != ToolBarsManager::AlwaysHiddenToolBar);

				if (showMenuBar)
				{
					if (!m_menuBar)
					{
						m_menuBar = new MenuBarWidget(this);

						setMenuBar(m_menuBar);
					}

					m_menuBar->show();
				}
				else if (!showMenuBar && m_menuBar)
				{
					m_menuBar->hide();
				}

				emit actionsStateChanged(QVector<int>({ActionsManager::ShowToolBarAction, ActionsManager::ShowMenuBarAction}));
			}

			break;
		case ToolBarsManager::StatusBar:
			{
				const bool showStatusBar(ToolBarsManager::getToolBarDefinition(ToolBarsManager::StatusBar).normalVisibility != ToolBarsManager::AlwaysHiddenToolBar);

				if (m_statusBar && !showStatusBar)
				{
					m_statusBar->deleteLater();
					m_statusBar = nullptr;

					setStatusBar(nullptr);
				}
				else if (!m_statusBar && showStatusBar)
				{
					m_statusBar = new StatusBarWidget(this);

					setStatusBar(m_statusBar);
				}

				emit actionsStateChanged(QVector<int>({ActionsManager::ShowToolBarAction}));
			}

			break;
		default:
			break;
	}
}

void MainWindow::handleToolBarMoved(int identifier)
{
	const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(identifier));
	ToolBarWidget *toolBar(getToolBars(definition.location).value(definition.row));

	if (toolBar && toolBar->getIdentifier() == identifier)
	{
		return;
	}

	QVector<ToolBarWidget*> toolBars(findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly).toVector());

	for (int i = 0; i < toolBars.count(); ++i)
	{
		if (toolBars.at(i)->getIdentifier() == identifier)
		{
			toolBar = toolBars.at(i);

			removeToolBar(toolBar);
			removeToolBarBreak(toolBar);
		}
	}

	toolBars = getToolBars(definition.location);

	for (int i = 0; i < toolBars.count(); ++i)
	{
		removeToolBar(toolBars.at(i));
	}

	toolBars.append(toolBar);

	std::sort(toolBars.begin(), toolBars.end(), [&](ToolBarWidget *first, ToolBarWidget *second)
	{
		return (first->getDefinition().row > second->getDefinition().row);
	});

	const bool isFullScreen(windowState().testFlag(Qt::WindowFullScreen));

	for (int i = 0; i < toolBars.count(); ++i)
	{
		if (i > 0)
		{
			addToolBarBreak(definition.location);
		}

		addToolBar(definition.location, toolBars.at(i));

		toolBars.at(i)->setVisible(toolBars.at(i)->shouldBeVisible(isFullScreen));
	}
}

void MainWindow::handleToolBarRemoved(int identifier)
{
	const QList<ToolBarWidget*> toolBars(findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly));

	for (int i = 0; i < toolBars.count(); ++i)
	{
		if (toolBars.at(i)->getIdentifier() == identifier)
		{
			removeToolBarBreak(toolBars.at(i));
			removeToolBar(toolBars.at(i));

			toolBars.at(i)->deleteLater();

			break;
		}
	}
}

void MainWindow::handleTransferStarted()
{
	const QString action(SettingsManager::getOption(SettingsManager::Browser_TransferStartingActionOption).toString());

	if (action == QLatin1String("openTab"))
	{
		triggerAction(ActionsManager::TransfersAction);
	}
	else if (action == QLatin1String("openBackgroundTab"))
	{
		const QUrl url(QLatin1String("about:transfers"));

		if (!SessionsManager::hasUrl(url, false))
		{
			triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen)}});
		}
	}
	else if (action == QLatin1String("openPanel"))
	{
		triggerAction(ActionsManager::ShowSidebarAction, {{QLatin1String("isChecked"), true}, {QLatin1String("sidebar"), ToolBarsManager::SideBar}, {QLatin1String("panel"), QLatin1String("transfers")}});
	}
}

void MainWindow::updateWindowTitle()
{
	m_windowTitle = getTitle();

	setWindowTitle(m_windowTitle);
}

void MainWindow::updateShortcuts()
{
	qDeleteAll(m_shortcuts);

	m_shortcuts.clear();

	const QVector<KeyboardProfile::Action> definitions(ActionsManager::getShortcutDefinitions());
	const QList<QKeySequence> standardShortcuts({QKeySequence(QKeySequence::Copy), QKeySequence(QKeySequence::Cut), QKeySequence(QKeySequence::Delete), QKeySequence(QKeySequence::Paste), QKeySequence(QKeySequence::Redo), QKeySequence(QKeySequence::SelectAll), QKeySequence(QKeySequence::Undo)});

	for (int i = 0; i < definitions.count(); ++i)
	{
		for (int j = 0; j < definitions[i].shortcuts.count(); ++j)
		{
			if (!standardShortcuts.contains(definitions[i].shortcuts[j]))
			{
				m_shortcuts.append(new Shortcut(definitions[i].action, definitions[i].shortcuts[j], definitions[i].parameters, this));
			}
		}
	}
}

void MainWindow::setOption(int identifier, const QVariant &value)
{
	Window *window(m_workspace->getActiveWindow());

	if (window)
	{
		window->setOption(identifier, value);
	}
}

void MainWindow::setActiveWindowByIndex(int index, bool updateLastActivity)
{
	if (!m_isRestored || index >= m_windows.count())
	{
		return;
	}

	if (index != getCurrentWindowIndex())
	{
		m_tabBar->setCurrentIndex(index);

		return;
	}

	Window *window(m_workspace->getActiveWindow());

	if (window)
	{
		disconnect(window, SIGNAL(statusMessageChanged(QString)), this, SLOT(setStatusMessage(QString)));
	}

	setStatusMessage(QString());

	window = getWindowByIndex(index);

	setCurrentWindow(window);

	if (window)
	{
		m_workspace->setActiveWindow(window);

		window->setFocus();
		window->markAsActive(updateLastActivity);

		setStatusMessage(window->getContentsWidget()->getStatusMessage());

		emit titleChanged(window->getTitle());

		connect(window, SIGNAL(statusMessageChanged(QString)), this, SLOT(setStatusMessage(QString)));
	}

	updateWindowTitle();

	emit currentWindowChanged(window ? window->getIdentifier() : 0);
	emit actionsStateChanged();
}

void MainWindow::setActiveWindowByIdentifier(quint64 identifier, bool updateLastActivity)
{
	for (int i = 0; i < m_windows.count(); ++i)
	{
		const Window *window(getWindowByIndex(i));

		if (window && window->getIdentifier() == identifier)
		{
			setActiveWindowByIndex(i, updateLastActivity);

			break;
		}
	}
}

void MainWindow::setCurrentWindow(Window *window)
{
	Window *previousWindow((m_currentWindow && m_currentWindow->isAboutToClose()) ? nullptr : m_currentWindow);

	m_currentWindow = window;

	if (previousWindow)
	{
		disconnect(previousWindow, SIGNAL(actionsStateChanged(QVector<int>)), this, SIGNAL(actionsStateChanged(QVector<int>)));
		disconnect(previousWindow, SIGNAL(actionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)), this, SIGNAL(actionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)));
	}

	if (window)
	{
		connect(window, SIGNAL(actionsStateChanged(QVector<int>)), this, SIGNAL(actionsStateChanged(QVector<int>)));
		connect(window, SIGNAL(actionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)), this, SIGNAL(actionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)));
	}
}

void MainWindow::setStatusMessage(const QString &message)
{
	QStatusTipEvent event(message);

	QApplication::sendEvent(this, &event);
}

MainWindow* MainWindow::findMainWindow(QObject *parent)
{
	if (!parent)
	{
		return nullptr;
	}

	if (parent->metaObject()->className() == QLatin1String("Otter::MainWindow"))
	{
		return qobject_cast<MainWindow*>(parent);
	}

	MainWindow *window(nullptr);
	const QWidget *widget(qobject_cast<QWidget*>(parent));

	if (widget && widget->window())
	{
		parent = widget->window();
	}

	while (parent)
	{
		if (parent->metaObject()->className() == QLatin1String("Otter::MainWindow"))
		{
			window = qobject_cast<MainWindow*>(parent);

			break;
		}

		parent = parent->parent();
	}

	if (!window)
	{
		window = Application::getActiveWindow();
	}

	return window;
}

TabBarWidget* MainWindow::getTabBar() const
{
	return m_tabBar;
}

Window* MainWindow::getActiveWindow() const
{
	return m_workspace->getActiveWindow();
}

Window* MainWindow::getWindowByIndex(int index) const
{
	return m_tabBar->getWindow(index);
}

Window* MainWindow::getWindowByIdentifier(quint64 identifier) const
{
	return (m_windows.contains(identifier) ? m_windows[identifier] : nullptr);
}

Window* MainWindow::openWindow(ContentsWidget *widget, SessionsManager::OpenHints hints, int index)
{
	if (!widget)
	{
		return nullptr;
	}

	Window *window(nullptr);

	if (widget->isPrivate())
	{
		hints |= SessionsManager::PrivateOpen;
	}

	if (hints.testFlag(SessionsManager::NewWindowOpen))
	{
		MainWindow *mainWindow(Application::createWindow({{QLatin1String("hints"), QVariant(hints)}}));

		window = mainWindow->openWindow(widget);

		mainWindow->triggerAction(ActionsManager::CloseOtherTabsAction);
	}
	else
	{
		window = new Window({{QLatin1String("hints"), QVariant(hints)}, {QLatin1String("size"), ((SettingsManager::getOption(SettingsManager::Interface_NewTabOpeningActionOption).toString() == QLatin1String("maximizeTab")) ? m_workspace->size() : QSize(800, 600))}}, widget, this);

		addWindow(window, hints, index);
	}

	return window;
}

QVariant MainWindow::getOption(int identifier) const
{
	const Window *window(m_workspace->getActiveWindow());

	return (window ? window->getOption(identifier) : QVariant());
}

QString MainWindow::getTitle() const
{
	const Window *window(m_workspace->getActiveWindow());

	return (window ? window->getTitle() : tr("Empty"));
}

QUrl MainWindow::getUrl() const
{
	const Window *window(m_workspace->getActiveWindow());

	return (window ? window->getUrl() : QUrl());
}

ActionsManager::ActionDefinition::State MainWindow::getActionState(int identifier, const QVariantMap &parameters) const
{
	const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(identifier));
	ActionsManager::ActionDefinition::State state(definition.getDefaultState());

	switch (definition.scope)
	{
		case ActionsManager::ActionDefinition::WindowScope:
			if (m_workspace->getActiveWindow())
			{
				return m_workspace->getActiveWindow()->getActionState(identifier, parameters);
			}

			state.isEnabled = false;

			return state;
		case ActionsManager::ActionDefinition::MainWindowScope:
			break;
		case ActionsManager::ActionDefinition::ApplicationScope:
			return Application::getInstance()->getActionState(identifier, parameters);
		default:
			return definition.getDefaultState();
	}

	switch (identifier)
	{
		case ActionsManager::ClosePrivateTabsAction:
			state.isEnabled = ((m_isPrivate && m_windows.count() > 0) || m_privateWindows.count() > 0);

			break;
		case ActionsManager::OpenUrlAction:
			state.isEnabled = !parameters.isEmpty();

			break;
		case ActionsManager::ReopenTabAction:
			if (!m_closedWindows.isEmpty())
			{
				if (parameters.contains(QLatin1String("index")))
				{
					const int index(parameters[QLatin1String("index")].toInt());

					state.isEnabled = (index >= 0 && index < m_closedWindows.count());
				}
				else
				{
					state.isEnabled = true;
				}
			}

			break;
		case ActionsManager::MaximizeAllAction:
			state.isEnabled = (m_workspace->getWindowCount(Qt::WindowMaximized) != m_windows.count());

			break;
		case ActionsManager::MinimizeAllAction:
			state.isEnabled = (m_workspace->getWindowCount(Qt::WindowMinimized) != m_windows.count());

			break;
		case ActionsManager::RestoreAllAction:
			state.isEnabled = (m_workspace->getWindowCount(Qt::WindowNoState) != m_windows.count());

			break;
		case ActionsManager::GoToHomePageAction:
			state.isEnabled = !SettingsManager::getOption(SettingsManager::Browser_HomePageOption).toString().isEmpty();

			break;
		case ActionsManager::CascadeAllAction:
		case ActionsManager::TileAllAction:
		case ActionsManager::StopAllAction:
		case ActionsManager::ReloadAllAction:
			state.isEnabled = !m_windows.isEmpty();

			break;
		case ActionsManager::CloseOtherTabsAction:
		case ActionsManager::ActivatePreviouslyUsedTabAction:
		case ActionsManager::ActivateLeastRecentlyUsedTabAction:
		case ActionsManager::ActivateTabOnLeftAction:
		case ActionsManager::ActivateTabOnRightAction:
		case ActionsManager::ShowTabSwitcherAction:
			state.isEnabled = (m_windows.count() > 1);

			break;
		case ActionsManager::ActivateTabAction:
			state.isEnabled = m_windows.contains(parameters.value(QLatin1String("tab"), 0).toULongLong());

			break;
		case ActionsManager::OpenBookmarkAction:
			{
				const BookmarksItem *bookmark(BookmarksManager::getBookmark(parameters[QLatin1String("bookmark")].toULongLong()));

				if (bookmark)
				{
					state.text = bookmark->data(Qt::DisplayRole).toString();
					state.icon = bookmark->data(Qt::DecorationRole).value<QIcon>();
					state.isEnabled = true;
				}
				else
				{
					state.isEnabled = false;
				}
			}

			break;
		case ActionsManager::FullScreenAction:
			state.icon = ThemesManager::createIcon(isFullScreen() ? QLatin1String("view-restore") : QLatin1String("view-fullscreen"));

			break;
		case ActionsManager::ShowToolBarAction:
			if (parameters.contains(QLatin1String("toolBar")))
			{
				const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition((parameters[QLatin1String("toolBar")].type() == QVariant::String) ? ToolBarsManager::getToolBarIdentifier(parameters[QLatin1String("toolBar")].toString()) : parameters[QLatin1String("toolBar")].toInt()));

				state.text = definition.getTitle();
				state.isChecked = (definition.normalVisibility == ToolBarsManager::AlwaysVisibleToolBar);
				state.isEnabled = true;
			}

			break;
		case ActionsManager::ShowMenuBarAction:
			state.isChecked = (ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar).normalVisibility == ToolBarsManager::AlwaysVisibleToolBar);

			break;
		case ActionsManager::ShowTabBarAction:
			state.isChecked = (ToolBarsManager::getToolBarDefinition(ToolBarsManager::TabBar).normalVisibility == ToolBarsManager::AlwaysVisibleToolBar);

			break;
		case ActionsManager::ShowSidebarAction:
			state.isChecked = (ToolBarsManager::getToolBarDefinition(ToolBarsManager::SideBar).normalVisibility == ToolBarsManager::AlwaysVisibleToolBar);

			break;
		case ActionsManager::ShowErrorConsoleAction:
			state.isChecked = (ToolBarsManager::getToolBarDefinition(ToolBarsManager::ErrorConsoleBar).normalVisibility == ToolBarsManager::AlwaysVisibleToolBar);

			break;
		default:
			break;
	}

	return state;
}

SessionMainWindow MainWindow::getSession() const
{
	SessionMainWindow session;
	session.geometry = saveGeometry();
	session.index = getCurrentWindowIndex();

	for (int i = 0; i < m_windows.count(); ++i)
	{
		const Window *window(getWindowByIndex(i));

		if (window && !window->isPrivate())
		{
			session.windows.append(window->getSession());
		}
		else if (i < session.index)
		{
			--session.index;
		}
	}

	return session;
}

QVector<ToolBarWidget*> MainWindow::getToolBars(Qt::ToolBarArea area) const
{
	QVector<ToolBarWidget*> toolBars(findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly).toVector());

	for (int i = (toolBars.count() - 1); i >= 0; --i)
	{
		if (toolBarArea(toolBars.at(i)) != area)
		{
			toolBars.removeAt(i);
		}
	}

	std::sort(toolBars.begin(), toolBars.end(), [&](ToolBarWidget *first, ToolBarWidget *second)
	{
		const QPoint firstPosition(first->mapToGlobal(first->rect().topLeft()));
		const QPoint secondPosition(second->mapToGlobal(second->rect().topLeft()));

		switch (area)
		{
			case Qt::BottomToolBarArea:
				return (firstPosition.y() < secondPosition.y());
			case Qt::LeftToolBarArea:
				return (firstPosition.x() > secondPosition.x());
			case Qt::RightToolBarArea:
				return (firstPosition.x() < secondPosition.x());
			default:
				return (firstPosition.y() > secondPosition.y());
			}

		return true;
	});

	return toolBars;
}

QVector<ClosedWindow> MainWindow::getClosedWindows() const
{
	return m_closedWindows;
}

QVector<quint64> MainWindow::createOrderedWindowList(bool includeMinimized) const
{
	QHash<quint64, Window*>::const_iterator iterator;
	QMultiMap<qint64, quint64> map;

	for (iterator = m_windows.constBegin(); iterator != m_windows.constEnd(); ++iterator)
	{
		if (includeMinimized || !iterator.value()->isMinimized())
		{
			map.insert(iterator.value()->getLastActivity().toMSecsSinceEpoch(), iterator.value()->getIdentifier());
		}
	}

	return map.values().toVector();
}

quint64 MainWindow::getIdentifier() const
{
	return m_identifier;
}

int MainWindow::getCurrentWindowIndex() const
{
	return m_tabBar->currentIndex();
}

int MainWindow::getWindowCount() const
{
	return m_windows.count();
}

int MainWindow::getWindowIndex(quint64 identifier) const
{
	for (int i = 0; i < m_windows.count(); ++i)
	{
		const Window *window(getWindowByIndex(i));

		if (window && window->getIdentifier() == identifier)
		{
			return i;
		}
	}

	return -1;
}

bool MainWindow::hasUrl(const QUrl &url, bool activate)
{
	for (int i = 0; i < m_windows.count(); ++i)
	{
		const Window *window(getWindowByIndex(i));

		if (window && window->getUrl() == url)
		{
			if (activate)
			{
				setActiveWindowByIndex(i);
			}

			return true;
		}
	}

	return false;
}

bool MainWindow::isAboutToClose() const
{
	return m_isAboutToClose;
}

bool MainWindow::isPrivate() const
{
	return m_isPrivate;
}

bool MainWindow::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			updateWindowTitle();

			break;
		case QEvent::Move:
			SessionsManager::markSessionModified();

			break;
		case QEvent::Resize:
			if (m_tabSwitcher && m_tabSwitcher->isVisible())
			{
				m_tabSwitcher->resize(size());
			}

			SessionsManager::markSessionModified();

			break;
		case QEvent::StatusTip:
			{
				QStatusTipEvent *statusTipEvent(static_cast<QStatusTipEvent*>(event));

				if (statusTipEvent)
				{
					emit statusMessageChanged(statusTipEvent->tip());
				}
			}

			break;
		case QEvent::WindowStateChange:
			{
				QWindowStateChangeEvent *stateChangeEvent(static_cast<QWindowStateChangeEvent*>(event));

				SessionsManager::markSessionModified();

				if (stateChangeEvent && windowState().testFlag(Qt::WindowFullScreen) != stateChangeEvent->oldState().testFlag(Qt::WindowFullScreen))
				{
					if (isFullScreen())
					{
						if (m_menuBar && ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar).fullScreenVisibility != ToolBarsManager::AlwaysVisibleToolBar)
						{
							m_menuBar->hide();
						}

						if (m_statusBar && ToolBarsManager::getToolBarDefinition(ToolBarsManager::StatusBar).fullScreenVisibility != ToolBarsManager::AlwaysVisibleToolBar)
						{
							m_statusBar->hide();
						}

						emit actionsStateChanged(QVector<int>({ActionsManager::FullScreenAction}));
					}
					else
					{
						if (m_menuBar && ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar).normalVisibility == ToolBarsManager::AlwaysVisibleToolBar)
						{
							m_menuBar->show();
						}

						if (m_statusBar && ToolBarsManager::getToolBarDefinition(ToolBarsManager::StatusBar).normalVisibility == ToolBarsManager::AlwaysVisibleToolBar)
						{
							m_statusBar->show();
						}

						emit actionsStateChanged(QVector<int>({ActionsManager::FullScreenAction}));
					}

					if (!windowState().testFlag(Qt::WindowFullScreen))
					{
						killTimer(m_mouseTrackerTimer);

						m_mouseTrackerTimer = 0;
					}
					else
					{
						m_mouseTrackerTimer = startTimer(250);
					}

					const QList<ToolBarWidget*> toolBars(findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly));
					const bool isFullScreen(windowState().testFlag(Qt::WindowFullScreen));

					for (int i = 0; i < toolBars.count(); ++i)
					{
						if (toolBars.at(i)->shouldBeVisible(isFullScreen))
						{
							toolBars.at(i)->removeEventFilter(this);
							toolBars.at(i)->show();
						}
						else
						{
							toolBars.at(i)->hide();
						}
					}

					emit areToolBarsVisibleChanged(!isFullScreen);
				}

				if (!windowState().testFlag(Qt::WindowMinimized))
				{
					m_previousRaisedState = windowState();
				}
			}

			break;
		case QEvent::WindowTitleChange:
			if (m_windowTitle != windowTitle())
			{
				updateWindowTitle();
			}

			break;
		case QEvent::WindowActivate:
			SessionsManager::markSessionModified();

			emit activated(this);

			break;
		default:
			break;
	}

	if (!GesturesManager::isTracking() && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::Wheel))
	{
		GesturesManager::startGesture(this, event);
	}

	return QMainWindow::event(event);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::Leave && isFullScreen())
	{
		ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(object));

		if (toolBar)
		{
			const QVector<ToolBarWidget*> toolBars(getToolBars(toolBar->getArea()));
			bool isUnderMouse(false);

			for (int i = 0; i < toolBars.count(); ++i)
			{
				if (toolBars.at(i)->underMouse())
				{
					isUnderMouse = true;

					break;
				}
			}

			if (!isUnderMouse)
			{
				for (int i = 0; i < toolBars.count(); ++i)
				{
					if (toolBars.at(i)->getDefinition().fullScreenVisibility == ToolBarsManager::OnHoverVisibleToolBar)
					{
						toolBars.at(i)->hide();
					}
				}
			}
		}
	}

	return QMainWindow::eventFilter(object, event);
}

Shortcut::Shortcut(int identifier, const QKeySequence &sequence, const QVariantMap &parameters, MainWindow *parent) : QShortcut(sequence, parent),
	m_mainWindow(parent),
	m_parameters(parameters),
	m_identifier(identifier)
{
	connect(this, &Shortcut::activated, this, &Shortcut::triggerAction);
}

void Shortcut::triggerAction()
{
	const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(m_identifier));
	QVariantMap parameters(m_parameters);

	if (definition.isValid() && definition.flags.testFlag(ActionsManager::ActionDefinition::IsCheckableFlag))
	{
		parameters[QLatin1String("isChecked")] = !m_mainWindow->getActionState(m_identifier, parameters).isChecked;
	}

	if (definition.scope == ActionsManager::ActionDefinition::ApplicationScope)
	{
		Application::getInstance()->triggerAction(m_identifier, parameters);
	}
	else
	{
		m_mainWindow->triggerAction(m_identifier, parameters);
	}
}

}
