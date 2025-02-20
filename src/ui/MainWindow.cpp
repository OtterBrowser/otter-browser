/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Menu.h"
#include "MenuBarWidget.h"
#include "OpenAddressDialog.h"
#include "OpenBookmarkDialog.h"
#include "PreferencesDialog.h"
#include "SidebarWidget.h"
#include "StatusBarWidget.h"
#include "TabBarWidget.h"
#include "TabSwitcherWidget.h"
#include "ToolBarDropZoneWidget.h"
#include "ToolBarWidget.h"
#include "WidgetFactory.h"
#include "Window.h"
#include "WorkspaceWidget.h"
#include "../core/ActionsManager.h"
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/FeedsManager.h"
#include "../core/InputInterpreter.h"
#include "../core/ItemModel.h"
#include "../core/SessionModel.h"
#include "../core/SettingsManager.h"
#include "../core/ThemesManager.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"
#include "../core/WebBackend.h"
#include "../modules/widgets/address/AddressWidget.h"
#include "../modules/widgets/search/SearchWidget.h"

#include "ui_MainWindow.h"

#include <QtCore/QTimer>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>

namespace Otter
{

quint64 MainWindow::m_identifierCounter(0);

MainWindow::MainWindow(const QVariantMap &parameters, const Session::MainWindow &session, QWidget *parent) : QMainWindow(parent),
	m_tabSwitcher(nullptr),
	m_workspace(new WorkspaceWidget(this)),
	m_tabBar(new TabBarWidget(this)),
	m_menuBar(nullptr),
	m_statusBar(nullptr),
	m_activeWindow(nullptr),
	m_identifier(++m_identifierCounter),
	m_mouseTrackerTimer(0),
	m_tabSwitchingOrderIndex(-1),
	m_isAboutToClose(false),
	m_isDraggingToolBar(false),
	m_isPrivate((SessionsManager::isPrivate() || SettingsManager::getOption(SettingsManager::Browser_PrivateModeOption).toBool() || SessionsManager::calculateOpenHints(parameters).testFlag(SessionsManager::PrivateOpen))),
	m_isSessionRestored(false),
	m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);

	installGesturesFilter(this, this);
	setUnifiedTitleAndToolBarOnMac(true);

	m_tabBar->hide();

	const QVector<Qt::ToolBarArea> areas({Qt::LeftToolBarArea, Qt::RightToolBarArea, Qt::TopToolBarArea, Qt::BottomToolBarArea, Qt::NoToolBarArea});
	QMap<Qt::ToolBarArea, QVector<Session::MainWindow::ToolBarState> > toolBarStates;

	if (!session.hasToolBarsState)
	{
		for (int i = 0; i < 5; ++i)
		{
			const QVector<ToolBarsManager::ToolBarDefinition> definitions(ToolBarsManager::getToolBarDefinitions(areas.at(i)));

			if (definitions.isEmpty())
			{
				continue;
			}

			QVector<Session::MainWindow::ToolBarState> states;
			states.reserve(definitions.length());

			for (int j = 0; j < definitions.count(); ++j)
			{
				const int identifier(definitions.at(j).identifier);

				states.append(Session::MainWindow::ToolBarState(identifier, ToolBarsManager::getToolBarDefinition(identifier)));
			}

			toolBarStates[areas.at(i)] = states;
		}
	}
	else if (!session.toolBars.isEmpty())
	{
		for (int i = 0; i < session.toolBars.count(); ++i)
		{
			const Session::MainWindow::ToolBarState state(session.toolBars.at(i));

			if (!toolBarStates.contains(state.location))
			{
				toolBarStates[state.location] = {};
			}

			toolBarStates[state.location].append(state);
		}
	}

	if (toolBarStates.contains(Qt::NoToolBarArea))
	{
		const QVector<Session::MainWindow::ToolBarState> states(toolBarStates[Qt::NoToolBarArea]);

		for (int i = 0; i < states.count(); ++i)
		{
			m_toolBarStates[states.at(i).identifier] = states.at(i);
		}
	}

	for (int i = 0; i < 4; ++i)
	{
		const Qt::ToolBarArea area(areas.at(i));
		QVector<Session::MainWindow::ToolBarState> states(toolBarStates.value(area));
		int row(0);

		std::sort(states.begin(), states.end(), [&](const Session::MainWindow::ToolBarState &first, const Session::MainWindow::ToolBarState &second)
		{
			return (first.row > second.row);
		});

		for (int j = 0; j < states.count(); ++j)
		{
			const Session::MainWindow::ToolBarState state(states.at(j));

			if (state.identifier < 0)
			{
				continue;
			}

			ToolBarWidget *toolBar(WidgetFactory::createToolBar(state.identifier, nullptr, this));
			toolBar->setArea(area);
			toolBar->setState(state);

			if (row > 0)
			{
				addToolBarBreak(area);
			}

			++row;

			addToolBar(area, toolBar);

			m_toolBars[states.at(j).identifier] = toolBar;
		}
	}

	m_splitters = session.splitters;

	if (getActionState(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::MenuBar}}).isChecked)
	{
		m_menuBar = new MenuBarWidget(this);

		setMenuBar(m_menuBar);
	}

	if (getActionState(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::StatusBar}}).isChecked)
	{
		m_statusBar = new StatusBarWidget(this);

		setStatusBar(m_statusBar);
	}

	setCentralWidget(m_workspace);

	connect(ActionsManager::getInstance(), &ActionsManager::shortcutsChanged, this, &MainWindow::updateShortcuts);
	connect(SessionsManager::getInstance(), &SessionsManager::requestedRemoveStoredUrl, this, &MainWindow::removeStoredUrl);
	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, [&](int identifier)
	{
		if (identifier == SettingsManager::Browser_HomePageOption)
		{
			emit arbitraryActionsStateChanged({ActionsManager::GoToHomePageAction});
		}
	});
	connect(ToolBarsManager::getInstance(), &ToolBarsManager::toolBarAdded, this, &MainWindow::handleToolBarAdded);
	connect(ToolBarsManager::getInstance(), &ToolBarsManager::toolBarRemoved, this, &MainWindow::handleToolBarRemoved);
	connect(TransfersManager::getInstance(), &TransfersManager::transferStarted, this, &MainWindow::handleTransferStarted);
	connect(m_workspace, &WorkspaceWidget::arbitraryActionsStateChanged, this, &MainWindow::arbitraryActionsStateChanged);

	if (session.geometry.isEmpty())
	{
		MainWindow *activeWindow(Application::getActiveWindow());

		if (activeWindow)
		{
			resize(activeWindow->size());
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

	if (parameters.value(QLatin1String("noTabs"), false).toBool())
	{
		m_isSessionRestored = true;
	}
	else
	{
		QTimer::singleShot(0, this, [=]()
		{
			restoreSession(session);
			updateWindowTitle();
		});
	}

	QTimer::singleShot(100, this, [=]()
	{
		updateShortcuts();
	});
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
				ToolBarWidget *toolBar(toolBars.at(j));

				if (toolBar->getDefinition().fullScreenVisibility == ToolBarsManager::OnHoverVisibleToolBar)
				{
					toolBar->show();
					toolBar->installEventFilter(this);
				}
			}
		}
	}

	QMainWindow::timerEvent(event);
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

	if (!SessionsManager::isPrivate())
	{
		if (!isLastWindow)
		{
			SessionsManager::storeClosedWindow(this);
		}
		else if (SessionsManager::getCurrentSession() == QLatin1String("default"))
		{
			SessionsManager::saveSession();
		}
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

	Menu menu(Menu::ToolBarsMenu, this);
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

void MainWindow::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	if (m_editorExecutor.isValid() && m_editorExecutor.getObject() == Application::getFocusObject(true) && ActionsManager::getActionDefinition(identifier).scope == ActionsManager::ActionDefinition::EditorScope)
	{
		m_editorExecutor.triggerAction(identifier, parameters);

		return;
	}

	switch (identifier)
	{
		case ActionsManager::NewTabAction:
			{
				QVariantMap mutableParameters(parameters);
				mutableParameters[QLatin1String("hints")] = SessionsManager::NewTabOpen;

				triggerAction(ActionsManager::OpenUrlAction, mutableParameters, trigger);
			}

			return;
		case ActionsManager::NewTabPrivateAction:
			{
				QVariantMap mutableParameters(parameters);
				mutableParameters[QLatin1String("hints")] = QVariant(SessionsManager::NewTabOpen | SessionsManager::PrivateOpen);

				triggerAction(ActionsManager::OpenUrlAction, mutableParameters, trigger);
			}

			return;
		case ActionsManager::OpenAction:
			{
				const QString path(Utils::getOpenPaths().value(0));

				if (!path.isEmpty())
				{
					triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl::fromLocalFile(path)}}, trigger);
				}
			}

			return;
		case ActionsManager::PeekTabAction:
		case ActionsManager::MaximizeTabAction:
		case ActionsManager::MinimizeTabAction:
		case ActionsManager::RestoreTabAction:
		case ActionsManager::AlwaysOnTopTabAction:
		case ActionsManager::MaximizeAllAction:
		case ActionsManager::MinimizeAllAction:
		case ActionsManager::RestoreAllAction:
		case ActionsManager::CascadeAllAction:
		case ActionsManager::TileAllAction:
			m_workspace->triggerAction(identifier, parameters, trigger);

			return;
		case ActionsManager::CloseWindowAction:
			close();

			return;
		case ActionsManager::ActivateAddressFieldAction:
		case ActionsManager::ActivateSearchFieldAction:
			{
				AddressWidget *addressWidget(findAddressField());
				SearchWidget *searchWidget(findSearchField());

				if (identifier == ActionsManager::ActivateSearchFieldAction && searchWidget)
				{
					searchWidget->activate(Qt::ShortcutFocusReason);
				}
				else if (addressWidget)
				{
					if (identifier == ActionsManager::ActivateAddressFieldAction)
					{
						addressWidget->activate(Qt::ShortcutFocusReason);

						if (parameters.value(QLatin1String("showTypedHistoryDropdown")).toBool())
						{
							addressWidget->showCompletion(true);
						}
					}
					else
					{
						addressWidget->setText(QLatin1String("? "));
						addressWidget->activate(Qt::OtherFocusReason);
					}
				}
				else
				{
					OpenAddressDialog dialog(ActionExecutor::Object(this, this), this);

					if (identifier == ActionsManager::ActivateSearchFieldAction)
					{
						dialog.setText(QLatin1String("? "));
					}

					if (dialog.exec() == QDialog::Accepted && dialog.getResult().type == InputInterpreter::InterpreterResult::SearchType)
					{
						search(dialog.getResult().searchQuery, dialog.getResult().searchEngine, SessionsManager::calculateOpenHints(SessionsManager::CurrentTabOpen));
					}
				}
			}

			break;
		case ActionsManager::GoAction:
			{
				AddressWidget *addressWidget(findAddressField());

				if (addressWidget)
				{
					addressWidget->handleUserInput(addressWidget->text(), SessionsManager::CurrentTabOpen);
				}
			}

			return;
		case ActionsManager::GoToPageAction:
			{
				OpenAddressDialog dialog(ActionExecutor::Object(this, this), this);

				if (dialog.exec() == QDialog::Accepted && dialog.getResult().type == InputInterpreter::InterpreterResult::SearchType)
				{
					search(dialog.getResult().searchQuery, dialog.getResult().searchEngine, SessionsManager::calculateOpenHints(SessionsManager::CurrentTabOpen));
				}
			}

			return;
		case ActionsManager::GoToHomePageAction:
			{
				const QString homePage(SettingsManager::getOption(SettingsManager::Browser_HomePageOption).toString());
				QVariantMap mutableParameters(parameters);
				mutableParameters[QLatin1String("url")] = QUrl(homePage);

				if (!homePage.isEmpty())
				{
					triggerAction(ActionsManager::OpenUrlAction, mutableParameters, trigger);
				}
			}

			return;
		case ActionsManager::BookmarksAction:
			triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl(QLatin1String("about:bookmarks"))}}, trigger);

			return;
		case ActionsManager::QuickBookmarkAccessAction:
			{
				OpenBookmarkDialog dialog(ActionExecutor::Object(this, this), this);
				dialog.exec();
			}

			return;
		case ActionsManager::ClosePrivateTabsAction:
			for (int i = 0; i < m_privateWindows.count(); ++i)
			{
				m_privateWindows.at(i)->requestClose();
			}

			emit arbitraryActionsStateChanged({ActionsManager::ClosePrivateTabsAction});

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
					if (parameters.value(QLatin1String("needsInterpretation"), false).toBool())
					{
						const InputInterpreter::InterpreterResult result(InputInterpreter::interpret(parameters[QLatin1String("url")].toString(), InputInterpreter::NoBookmarkKeywordsFlag));

						if (!result.isValid())
						{
							return;
						}

						switch (result.type)
						{
							case InputInterpreter::InterpreterResult::BookmarkType:
								{
									QVariantMap mutableParameters(parameters);
									mutableParameters.remove(QLatin1String("needsInterpretation"));
									mutableParameters[QLatin1String("bookmark")] = result.bookmark->getIdentifier();

									triggerAction(ActionsManager::OpenBookmarkAction, mutableParameters, trigger);
								}

								return;
							case InputInterpreter::InterpreterResult::UrlType:
								url = result.url;

								break;
							case InputInterpreter::InterpreterResult::SearchType:
								search(result.searchQuery, result.searchEngine, SessionsManager::calculateOpenHints(parameters, (trigger == ActionsManager::KeyboardTrigger || trigger == ActionsManager::MouseTrigger)));

								return;
							default:
								return;
						}
					}
					else
					{
						url = ((parameters[QLatin1String("url")].type() == QVariant::Url) ? parameters[QLatin1String("url")].toUrl() : QUrl::fromUserInput(parameters[QLatin1String("url")].toString()));
					}
				}

				if (parameters.contains(QLatin1String("application")))
				{
					Utils::runApplication(parameters[QLatin1String("application")].toString(), url);

					return;
				}

				if (AddonsManager::isSpecialPage(url) && (AddonsManager::getSpecialPage(url.path()).isSingleton && SessionsManager::hasUrl(url, true)))
				{
					return;
				}

				SessionsManager::OpenHints hints(SessionsManager::calculateOpenHints(parameters, (trigger == ActionsManager::KeyboardTrigger || trigger == ActionsManager::MouseTrigger)));
				const int index(parameters.value(QLatin1String("index"), -1).toInt());

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
					Session::MainWindow session;
					session.toolBars = getSession().toolBars;
					session.hasToolBarsState = true;

					Application::createWindow(mutableParameters, session);

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

				if (hints == SessionsManager::NewTabOpen && isUrlEmpty && !url.isEmpty() && url != QUrl(QLatin1String("about:start")))
				{
					hints = SessionsManager::CurrentTabOpen;
				}
				else if (hints == SessionsManager::DefaultOpen && !isUrlEmpty && url.scheme() == QLatin1String("about") && !url.path().isEmpty() && url.path() != QLatin1String("blank") && url.path() != QLatin1String("start"))
				{
					hints = SessionsManager::NewTabOpen;
				}
				else if (hints == SessionsManager::DefaultOpen && (isUrlEmpty || SettingsManager::getOption(SettingsManager::Browser_ReuseCurrentTabOption).toBool()) && url.scheme() != QLatin1String("javascript"))
				{
					hints = SessionsManager::CurrentTabOpen;
				}

				Window *window(nullptr);

				if (hints.testFlag(SessionsManager::CurrentTabOpen) && activeWindow)
				{
					window = activeWindow;
				}
				else
				{
					mutableParameters[QLatin1String("hints")] = QVariant(hints);

					window = new Window(mutableParameters, nullptr, this);

					addWindow(window, hints, index);
				}

				window->setUrl((url.isEmpty() && SettingsManager::getOption(SettingsManager::StartPage_EnableStartPageOption).toBool()) ? QUrl(QLatin1String("about:start")) : url);
			}

			return;
		case ActionsManager::ReopenTabAction:
			restoreClosedWindow(parameters.value(QLatin1String("index"), 0).toInt());

			return;
		case ActionsManager::StopAllAction:
			{
				QHash<quint64, Window*>::const_iterator iterator;

				for (iterator = m_windows.constBegin(); iterator != m_windows.constEnd(); ++iterator)
				{
					if (iterator.value()->getLoadingState() != WebWidget::DeferredLoadingState)
					{
						iterator.value()->triggerAction(ActionsManager::StopAction, {}, trigger);
					}
				}
			}

			return;
		case ActionsManager::ReloadAllAction:
			{
				QHash<quint64, Window*>::const_iterator iterator;

				for (iterator = m_windows.constBegin(); iterator != m_windows.constEnd(); ++iterator)
				{
					if (iterator.value()->getLoadingState() != WebWidget::DeferredLoadingState)
					{
						iterator.value()->triggerAction(ActionsManager::ReloadAction, {}, trigger);
					}
				}
			}

			return;
		case ActionsManager::ActivatePreviouslyUsedTabAction:
		case ActionsManager::ActivateLeastRecentlyUsedTabAction:
			{
				const QVector<quint64> windows(createOrderedWindowList(parameters.contains(QLatin1String("includeMinimized"))));

				if (windows.count() > 1)
				{
					setActiveWindowByIdentifier((identifier == ActionsManager::ActivatePreviouslyUsedTabAction) ? windows.at(windows.count() - 2) : windows.value(0));
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
			{
				BookmarksModel::Bookmark *bookmark(nullptr);

				if (parameters.contains(QLatin1String("bookmark")))
				{
					const QVariant bookmarkData(parameters[QLatin1String("bookmark")]);

					if (bookmarkData.type() == QVariant::String)
					{
						bookmark = BookmarksManager::getBookmark(bookmarkData.toString());
					}
					else
					{
						bookmark = BookmarksManager::getBookmark(bookmarkData.toULongLong());
					}
				}
				else if (parameters.contains(QLatin1String("startPageTile")))
				{
					const QString bookmarkPath(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString());
					const BookmarksModel::Bookmark *startPageBookmark(BookmarksManager::getModel()->getBookmarkByPath(bookmarkPath));

					if (startPageBookmark)
					{
						bookmark = startPageBookmark->getChild(parameters.value(QLatin1String("startPageTile")).toInt() - 1);
					}
				}

				if (!bookmark)
				{
					return;
				}

				QVariantMap mutableParameters(parameters);
				mutableParameters.remove(QLatin1String("bookmark"));
				mutableParameters.remove(QLatin1String("startPageTile"));

				switch (bookmark->getType())
				{
					case BookmarksModel::UrlBookmark:
						mutableParameters[QLatin1String("url")] = bookmark->getUrl();

						triggerAction(ActionsManager::OpenUrlAction, mutableParameters, trigger);

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

							if (!canOpen || urls.isEmpty())
							{
								break;
							}

							SessionsManager::OpenHints hints(SessionsManager::calculateOpenHints(parameters));
							int index(parameters.value(QLatin1String("index"), -1).toInt());

							if (index < 0)
							{
								if (!hints.testFlag(SessionsManager::EndOpen) && SettingsManager::getOption(SettingsManager::TabBar_OpenNextToActiveOption).toBool())
								{
									index = (getCurrentWindowIndex() + 1);
								}
								else
								{
									index = (m_windows.count() - 1);
								}
							}

							mutableParameters[QLatin1String("url")] = urls.at(0);
							mutableParameters[QLatin1String("hints")] = QVariant(hints);
							mutableParameters[QLatin1String("index")] = index;

							triggerAction(ActionsManager::OpenUrlAction, mutableParameters, trigger);

							if (hints == SessionsManager::DefaultOpen || hints.testFlag(SessionsManager::CurrentTabOpen))
							{
								hints = SessionsManager::NewTabOpen;
							}

							mutableParameters[QLatin1String("hints")] = QVariant(hints);

							for (int i = 1; i < urls.count(); ++i)
							{
								mutableParameters[QLatin1String("url")] = urls.at(i);
								mutableParameters[QLatin1String("index")] = (index + i);

								triggerAction(ActionsManager::OpenUrlAction, mutableParameters, trigger);
							}
						}

						break;
					default:
						break;
				}
			}

			return;
		case ActionsManager::OpenFeedAction:
			{
				QVariantMap mutableParameters(parameters);
				mutableParameters.remove(QLatin1String("entry"));

				if (parameters.contains(QLatin1String("entry")))
				{
					const FeedsModel::Entry *entry(FeedsManager::getModel()->getEntry(parameters[QLatin1String("entry")].toULongLong()));

					if (!entry || entry->getType() != FeedsModel::FeedEntry || !entry->getFeed())
					{
						return;
					}

					mutableParameters[QLatin1String("url")] = FeedsManager::createFeedReaderUrl(entry->getFeed()->getUrl());
				}
				else if (parameters.contains(QLatin1String("url")))
				{
					mutableParameters[QLatin1String("url")] = FeedsManager::createFeedReaderUrl(parameters[QLatin1String("url")].toUrl());
				}
				else
				{
					return;
				}

				triggerAction(ActionsManager::OpenUrlAction, mutableParameters, trigger);
			}

			return;
		case ActionsManager::ShowTabSwitcherAction:
			if (m_tabSwitcher && m_tabSwitcher->isVisible())
			{
				m_tabSwitcher->hide();
			}
			else
			{
				if (!m_tabSwitcher)
				{
					m_tabSwitcher = new TabSwitcherWidget(this);
				}

				m_tabSwitcher->raise();
				m_tabSwitcher->resize(size());
				m_tabSwitcher->show(TabSwitcherWidget::ActionReason);
			}

			return;
		case ActionsManager::ShowToolBarAction:
			if (parameters.contains(QLatin1String("toolBar")))
			{
				const int toolBarIdentifier((parameters[QLatin1String("toolBar")].type() == QVariant::String) ? ToolBarsManager::getToolBarIdentifier(parameters[QLatin1String("toolBar")].toString()) : parameters[QLatin1String("toolBar")].toInt());
				const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(toolBarIdentifier));

				if (!definition.isValid())
				{
					return;
				}

				const bool isChecked(parameters.value(QLatin1String("isChecked"), !getActionState(toolBarIdentifier, parameters).isChecked).toBool());
				Session::MainWindow::ToolBarState state(getToolBarState(toolBarIdentifier));
				state.setVisibility((windowState().testFlag(Qt::WindowFullScreen) ? ToolBarsManager::FullScreenMode : ToolBarsManager::NormalMode), (isChecked ? Session::MainWindow::ToolBarState::AlwaysVisibleToolBar : Session::MainWindow::ToolBarState::AlwaysHiddenToolBar));

				if (definition.location == Qt::NoToolBarArea)
				{
					m_toolBarStates[toolBarIdentifier] = state;
				}
				else
				{
					if (!m_toolBars.contains(toolBarIdentifier))
					{
						handleToolBarAdded(toolBarIdentifier);
					}

					if (m_toolBars.contains(toolBarIdentifier))
					{
						m_toolBars[toolBarIdentifier]->setState(state);
					}
				}

				switch (toolBarIdentifier)
				{
					case ToolBarsManager::MenuBar:
						if (isChecked && (!m_menuBar || !m_menuBar->isVisible()))
						{
							if (!m_menuBar)
							{
								m_menuBar = new MenuBarWidget(this);

								setMenuBar(m_menuBar);
							}

							m_menuBar->show();
						}
						else if (!isChecked && (m_menuBar && m_menuBar->isVisible()))
						{
							m_menuBar->hide();
						}

						break;
					case ToolBarsManager::StatusBar:
						if (isChecked && !m_statusBar)
						{
							m_statusBar = new StatusBarWidget(this);

							setStatusBar(m_statusBar);
						}
						else if (!isChecked && m_statusBar)
						{
							m_statusBar->deleteLater();
							m_statusBar = nullptr;

							setStatusBar(nullptr);
						}

						break;
					default:
						break;
				}

				SessionsManager::markSessionAsModified();

				emit arbitraryActionsStateChanged({ActionsManager::ShowToolBarAction});
				emit toolBarStateChanged(toolBarIdentifier, getToolBarState(toolBarIdentifier));
			}

			return;
		case ActionsManager::ShowMenuBarAction:
			triggerAction(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::MenuBar}, {QLatin1String("isChecked"), parameters.value(QLatin1String("isChecked"), !getActionState(identifier).isChecked)}}, trigger);

			return;
		case ActionsManager::ShowTabBarAction:
			triggerAction(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::TabBar}, {QLatin1String("isChecked"), parameters.value(QLatin1String("isChecked"), !getActionState(identifier).isChecked)}}, trigger);

			return;
		case ActionsManager::ShowSidebarAction:
			triggerAction(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::SideBar}, {QLatin1String("isChecked"), parameters.value(QLatin1String("isChecked"), !getActionState(identifier).isChecked)}}, trigger);

			return;
		case ActionsManager::ShowErrorConsoleAction:
			triggerAction(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::ErrorConsoleBar}, {QLatin1String("isChecked"), parameters.value(QLatin1String("isChecked"), !getActionState(identifier).isChecked)}}, trigger);

			return;
		case ActionsManager::ShowPanelAction:
			{
				const int toolBarIdentifier(parameters.value(QLatin1String("sidebar"), ToolBarsManager::SideBar).toInt());
				const QString panel(parameters.value(QLatin1String("panel")).toString());
				ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(toolBarIdentifier));

				if (definition.isValid())
				{
					definition.currentPanel = ((definition.currentPanel == panel) ? QString() : panel);

					ToolBarsManager::setToolBar(definition);

					triggerAction(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), toolBarIdentifier}, {QLatin1String("isChecked"), true}}, trigger);
				}
			}

			return;
		case ActionsManager::OpenPanelAction:
			{
				const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(parameters.value(QLatin1String("sidebar"), ToolBarsManager::SideBar).toInt()));

				if (definition.isValid() && !definition.currentPanel.isEmpty())
				{
					triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), SidebarWidget::getPanelUrl(definition.currentPanel)}, {QLatin1String("hints"), SessionsManager::NewTabOpen}}, trigger);
				}
			}

			return;
		case ActionsManager::ContentBlockingAction:
			triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl(QLatin1String("about:content-filters"))}}, trigger);

			return;
		case ActionsManager::HistoryAction:
			triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl(QLatin1String("about:history"))}}, trigger);

			return;
		case ActionsManager::ClearHistoryAction:
			{
				ClearHistoryDialog dialog(SettingsManager::getOption(SettingsManager::History_ManualClearOptionsOption).toStringList(), false, this);
				dialog.exec();
			}

			return;
		case ActionsManager::AddonsAction:
			triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl(QLatin1String("about:addons"))}}, trigger);

			return;
		case ActionsManager::NotesAction:
			triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl(QLatin1String("about:notes"))}}, trigger);

			return;
		case ActionsManager::PasswordsAction:
			triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl(QLatin1String("about:passwords"))}}, trigger);

			return;
		case ActionsManager::TransfersAction:
			triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl(QLatin1String("about:transfers"))}}, trigger);

			return;
		case ActionsManager::CookiesAction:
			triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl(QLatin1String("about:cookies"))}}, trigger);

			return;
		case ActionsManager::FullScreenAction:
			{
				if (isFullScreen())
				{
					showNormal();
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
					iterator.value()->triggerAction(identifier, parameters, trigger);
				}
			}

			return;
		case ActionsManager::PreferencesAction:
			{
				PreferencesDialog dialog(parameters.value(QLatin1String("page"), QLatin1String("general")).toString(), this);
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

				window->triggerAction(ActionsManager::PasteAndGoAction, {}, trigger);
			}
			else if (window)
			{
				window->triggerAction(identifier, parameters, trigger);
			}

			break;
	}
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

void MainWindow::restoreSession(const Session::MainWindow &session)
{
	int index(session.index);

	if (index >= session.windows.count())
	{
		index = -1;
	}

	if (session.windows.isEmpty())
	{
		m_isSessionRestored = true;

		if (SettingsManager::getOption(SettingsManager::Interface_LastTabClosingActionOption).toString() != QLatin1String("doNothing"))
		{
			triggerAction(ActionsManager::NewTabAction);
		}
		else
		{
			setActiveWindow(nullptr);
		}
	}
	else
	{
		for (int i = 0; i < session.windows.count(); ++i)
		{
			const Session::Window sessionWindow(session.windows.at(i));
			QVariantMap parameters({{QLatin1String("size"), ((sessionWindow.state.state == Qt::WindowMaximized || !sessionWindow.state.geometry.isValid()) ? m_workspace->size() : sessionWindow.state.geometry.size())}});

			if (m_isPrivate)
			{
				parameters[QLatin1String("hints")] = SessionsManager::PrivateOpen;
			}

			Window *window(new Window(parameters, nullptr, this));
			window->setSession(sessionWindow, SettingsManager::getOption(SettingsManager::Sessions_DeferTabsLoadingOption).toBool());

			if (index < 0 && sessionWindow.state.state != Qt::WindowMinimized)
			{
				index = i;
			}

			addWindow(window, SessionsManager::DefaultOpen, -1, sessionWindow.state, sessionWindow.isAlwaysOnTop);
		}

		emit arbitraryActionsStateChanged({ActionsManager::MaximizeAllAction, ActionsManager::MinimizeAllAction, ActionsManager::RestoreAllAction, ActionsManager::CascadeAllAction, ActionsManager::TileAllAction});
	}

	m_isSessionRestored = true;

	setActiveWindowByIndex(index);

	m_workspace->markAsRestored();

	emit sessionRestored();
}

void MainWindow::restoreClosedWindow(int index)
{
	if (index < 0 || index >= m_closedWindows.count())
	{
		return;
	}

	const Session::ClosedWindow closedWindow(m_closedWindows.takeAt(index));

	if (closedWindow.previousWindow == 0)
	{
		index = 0;
	}
	else if (closedWindow.nextWindow == 0)
	{
		index = m_windows.count();
	}
	else
	{
		const int previousIndex(getWindowIndex(closedWindow.previousWindow));

		if (previousIndex >= 0)
		{
			index = (previousIndex + 1);
		}
		else
		{
			const int nextIndex(getWindowIndex(closedWindow.nextWindow));

			if (nextIndex >= 0)
			{
				index = qMax(0, (nextIndex - 1));
			}
			else
			{
				index = -1;
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

	if (m_closedWindows.isEmpty() && SessionsManager::getClosedWindows().isEmpty())
	{
		emit closedWindowsAvailableChanged(false);
	}

	addWindow(window, SessionsManager::DefaultOpen, index, closedWindow.window.state, closedWindow.window.isAlwaysOnTop);
}

void MainWindow::clearClosedWindows()
{
	if (!m_closedWindows.isEmpty())
	{
		m_closedWindows.clear();

		if (SessionsManager::getClosedWindows().isEmpty())
		{
			emit closedWindowsAvailableChanged(false);
		}
	}
}

void MainWindow::addWindow(Window *window, SessionsManager::OpenHints hints, int index, const Session::Window::State &state, bool isAlwaysOnTop)
{
	if (!window)
	{
		return;
	}

	m_windows[window->getIdentifier()] = window;

	if (!m_isPrivate && window->isPrivate())
	{
		m_privateWindows.append(window);

		emit arbitraryActionsStateChanged({ActionsManager::ClosePrivateTabsAction});
	}

	if (index < 0)
	{
		index = ((!hints.testFlag(SessionsManager::EndOpen) && SettingsManager::getOption(SettingsManager::TabBar_OpenNextToActiveOption).toBool()) ? (getCurrentWindowIndex() + 1) : (m_windows.count() - 1));
	}

	if (m_isSessionRestored && SettingsManager::getOption(SettingsManager::TabBar_PrependPinnedTabOption).toBool() && !window->isPinned())
	{
		const MainWindowSessionItem *mainWindowItem(SessionsManager::getModel()->getMainWindowItem(this));

		if (mainWindowItem)
		{
			for (int i = 0; i < mainWindowItem->rowCount(); ++i)
			{
				if (!ItemModel::getItemData(mainWindowItem->child(i, 0), SessionModel::IsPinnedRole).toBool())
				{
					if (index < i)
					{
						index = i;
					}

					break;
				}
			}
		}
	}

	m_workspace->addWindow(window, state, isAlwaysOnTop);
	m_tabBar->addTab(index, window);

	if (m_tabSwitchingOrderIndex >= 0)
	{
		m_tabSwitchingOrderList.append(window->getIdentifier());
	}

	if (!hints.testFlag(SessionsManager::BackgroundOpen) || m_windows.count() < 2)
	{
		m_tabBar->setCurrentIndex(index);

		if (m_isSessionRestored)
		{
			setActiveWindowByIndex(index);
		}
	}

	if (m_isSessionRestored)
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

	connect(window, &Window::needsAttention, this, [&]()
	{
		QApplication::alert(this);
	});
	connect(window, &Window::titleChanged, this, &MainWindow::updateWindowTitle);
	connect(window, &Window::requestedSearch, this, &MainWindow::search);
	connect(window, &Window::requestedCloseWindow, this, &MainWindow::handleRequestedCloseWindow);
	connect(window, &Window::isPinnedChanged, this, &MainWindow::handleWindowIsPinnedChanged);
	connect(window, &Window::requestedNewWindow, this, &MainWindow::openWindow);

	emit windowAdded(window->getIdentifier());
}

void MainWindow::moveWindow(Window *window, MainWindow *mainWindow, const QVariantMap &parameters)
{
	Window *newWindow(nullptr);
	SessionsManager::OpenHints hints(SessionsManager::DefaultOpen);

	if (window->isPrivate())
	{
		hints |= SessionsManager::PrivateOpen;
	}

	window->getContentsWidget()->setParent(nullptr);

	if (mainWindow)
	{
		newWindow = mainWindow->openWindow(window->getContentsWidget(), hints, parameters);
	}
	else
	{
		newWindow = openWindow(window->getContentsWidget(), (hints | SessionsManager::NewWindowOpen), parameters);
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

	if (m_windows.isEmpty())
	{
		close();
	}
	else
	{
		if (m_tabSwitchingOrderIndex >= 0)
		{
			m_tabSwitchingOrderList.removeAll(window->getIdentifier());
		}

		emit arbitraryActionsStateChanged({ActionsManager::ClosePrivateTabsAction});
	}

	emit windowRemoved(window->getIdentifier());
}

void MainWindow::setActiveEditorExecutor(const ActionExecutor::Object &executor)
{
	const QMetaMethod actionsStateChangedMethod(metaObject()->method(metaObject()->indexOfMethod("actionsStateChanged()")));
	const QMetaMethod arbitraryActionsStateChangedMethod(metaObject()->method(metaObject()->indexOfMethod("arbitraryActionsStateChanged(QVector<int>)")));
	const QMetaMethod categorizedActionsStateChangedMethod(metaObject()->method(metaObject()->indexOfMethod("categorizedActionsStateChanged(QVector<int>)")));

	if (m_editorExecutor.isValid())
	{
		m_editorExecutor.disconnectSignals(this, &actionsStateChangedMethod, &arbitraryActionsStateChangedMethod, &categorizedActionsStateChangedMethod);
	}

	m_editorExecutor = executor;

	if (executor.isValid())
	{
		m_editorExecutor.connectSignals(this, &actionsStateChangedMethod, &arbitraryActionsStateChangedMethod, &categorizedActionsStateChangedMethod);
	}

	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::EditingCategory});
}

void MainWindow::setSplitterSizes(const QString &identifier, const QVector<int> &sizes)
{
	if (sizes != m_splitters.value(identifier))
	{
		m_splitters[identifier] = sizes;

		SessionsManager::markSessionAsModified();
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
		ToolBarWidget *toolBar(toolBars.at(i));
		const Qt::ToolBarArea area(toolBarArea(toolBar));

		if (toolBar->isVisible() && (!isSidebar || area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea))
		{
			insertToolBar(toolBar, new ToolBarDropZoneWidget(this));
			insertToolBarBreak(toolBar);
		}
	}

	const QVector<Qt::ToolBarArea> areas({Qt::LeftToolBarArea, Qt::RightToolBarArea, Qt::TopToolBarArea, Qt::BottomToolBarArea});

	for (int i = 0; i < 4; ++i)
	{
		const Qt::ToolBarArea area(areas.at(i));

		if (!isSidebar || area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea)
		{
			addToolBarBreak(area);
			addToolBar(area, new ToolBarDropZoneWidget(this));
		}
	}
}

void MainWindow::endToolBarDragging()
{
	const QList<ToolBarDropZoneWidget*> dropZones(findChildren<ToolBarDropZoneWidget*>());

	for (int i = 0; i < dropZones.count(); ++i)
	{
		ToolBarDropZoneWidget *dropZone(dropZones.at(i));

		removeToolBarBreak(dropZone);
		removeToolBar(dropZone);

		dropZone->deleteLater();
	}

	m_isDraggingToolBar = false;
}

void MainWindow::handleRequestedCloseWindow(Window *window)
{
	const int index(window ? getWindowIndex(window->getIdentifier()) : -1);

	if (index < 0)
	{
		return;
	}

	if (!m_isAboutToClose && (!window->isPrivate() || SettingsManager::getOption(SettingsManager::History_RememberClosedPrivateTabsOption).toBool()))
	{
		const Session::Window::History history(window->getHistory());

		if (!history.isEmpty())
		{
			const Window *nextWindow(getWindowByIndex(index + 1));
			const Window *previousWindow((index > 0) ? getWindowByIndex(index - 1) : nullptr);
			const int limit(SettingsManager::getOption(SettingsManager::History_ClosedTabsLimitAmountOption).toInt());
			Session::ClosedWindow closedWindow;
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

			if (m_closedWindows.count() > limit)
			{
				m_closedWindows.resize(limit);
				m_closedWindows.squeeze();
			}

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

				if (SettingsManager::getOption(SettingsManager::StartPage_EnableStartPageOption).toBool())
				{
					window->setUrl(QUrl(QLatin1String("about:start")));
				}

				return;
			}
		}
		else
		{
			setActiveWindow(nullptr);

			m_workspace->setActiveWindow(nullptr);

			emit titleChanged({});
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

	emit arbitraryActionsStateChanged({ActionsManager::CloseOtherTabsAction, ActionsManager::ClosePrivateTabsAction});
}

void MainWindow::handleWindowIsPinnedChanged(bool isPinned)
{
	const Window *modifiedWindow(qobject_cast<Window*>(sender()));

	if (!modifiedWindow || !m_isSessionRestored || !SettingsManager::getOption(SettingsManager::TabBar_PrependPinnedTabOption).toBool())
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
	ToolBarWidget *newToolBar(WidgetFactory::createToolBar(identifier, nullptr, this));

	if (toolBars.isEmpty() || definition.row < 0)
	{
		if (!toolBars.isEmpty())
		{
			addToolBarBreak(definition.location);
		}

		addToolBar(definition.location, newToolBar);
	}
	else
	{
		for (int i = 0; i < toolBars.count(); ++i)
		{
			removeToolBar(toolBars.at(i));
		}

		toolBars.append(newToolBar);

		std::sort(toolBars.begin(), toolBars.end(), [&](ToolBarWidget *first, ToolBarWidget *second)
		{
			return (first->getDefinition().row > second->getDefinition().row);
		});

		const ToolBarsManager::ToolBarsMode mode(windowState().testFlag(Qt::WindowFullScreen) ? ToolBarsManager::FullScreenMode : ToolBarsManager::NormalMode);

		for (int i = 0; i < toolBars.count(); ++i)
		{
			ToolBarWidget *toolBar(toolBars.at(i));

			if (i > 0)
			{
				addToolBarBreak(definition.location);
			}

			addToolBar(definition.location, toolBar);

			toolBar->setVisible(toolBar->shouldBeVisible(mode));
		}
	}

	m_toolBars[identifier] = newToolBar;

	SessionsManager::markSessionAsModified();

	emit arbitraryActionsStateChanged({ActionsManager::ShowToolBarAction});
}

void MainWindow::handleToolBarRemoved(int identifier)
{
	if (!m_toolBars.contains(identifier))
	{
		return;
	}

	ToolBarWidget *toolBar(m_toolBars.take(identifier));

	removeToolBarBreak(toolBar);
	removeToolBar(toolBar);

	toolBar->deleteLater();

	SessionsManager::markSessionAsModified();

	emit arbitraryActionsStateChanged({ActionsManager::ShowToolBarAction});
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

		if (!SessionsManager::hasUrl(url))
		{
			triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen)}});
		}
	}
	else if (action == QLatin1String("openPanel"))
	{
		triggerAction(ActionsManager::ShowPanelAction, {{QLatin1String("sidebar"), ToolBarsManager::SideBar}, {QLatin1String("panel"), QLatin1String("transfers")}});
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

	m_shortcuts.reserve(definitions.count() * 2);

	for (int i = 0; i < definitions.count(); ++i)
	{
		const KeyboardProfile::Action &definition(definitions.at(i));

		for (int j = 0; j < definition.shortcuts.count(); ++j)
		{
			const QKeySequence shortcut(definition.shortcuts.at(j));

			if (ActionsManager::isShortcutAllowed(shortcut))
			{
				m_shortcuts.append(new Shortcut(definition.action, shortcut, definition.parameters, this));
			}
		}
	}

	m_shortcuts.squeeze();
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
	if (!m_isSessionRestored || index >= m_windows.count())
	{
		return;
	}

	if (index != getCurrentWindowIndex())
	{
		m_tabBar->setCurrentIndex(index);

		return;
	}

	const Window *activeWindow(m_workspace->getActiveWindow());
	Window *window(getWindowByIndex(index));

	if (activeWindow == window)
	{
		return;
	}

	if (activeWindow)
	{
		disconnect(activeWindow, &Window::statusMessageChanged, this, &MainWindow::setStatusMessage);
	}

	setStatusMessage({});
	setActiveWindow(window);

	if (window)
	{
		m_workspace->setActiveWindow(window);

		window->setFocus();
		window->markAsActive(updateLastActivity);

		setStatusMessage(window->getContentsWidget()->getStatusMessage());

		emit titleChanged(window->getTitle());

		connect(window, &Window::statusMessageChanged, this, &MainWindow::setStatusMessage);
	}

	updateWindowTitle();

	emit activeWindowChanged(window ? window->getIdentifier() : 0);
	emit actionsStateChanged();
}

void MainWindow::setActiveWindowByIdentifier(quint64 identifier, bool updateLastActivity)
{
	if (identifier == 0)
	{
		return;
	}

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

void MainWindow::setActiveWindow(Window *window)
{
	const Window *previousWindow((m_activeWindow && !m_activeWindow->isAboutToClose()) ? m_activeWindow : nullptr);

	m_activeWindow = window;

	if (previousWindow)
	{
		disconnect(previousWindow, &Window::arbitraryActionsStateChanged, this, &MainWindow::arbitraryActionsStateChanged);
		disconnect(previousWindow, &Window::categorizedActionsStateChanged, this, &MainWindow::categorizedActionsStateChanged);
	}

	if (window)
	{
		connect(window, &Window::arbitraryActionsStateChanged, this, &MainWindow::arbitraryActionsStateChanged);
		connect(window, &Window::categorizedActionsStateChanged, this, &MainWindow::categorizedActionsStateChanged);
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

	MainWindow *mainWindow(nullptr);
	const QWidget *widget(qobject_cast<QWidget*>(parent));

	if (widget && widget->window())
	{
		parent = widget->window();
	}

	while (parent)
	{
		if (parent->metaObject()->className() == QLatin1String("Otter::MainWindow"))
		{
			mainWindow = qobject_cast<MainWindow*>(parent);

			break;
		}

		parent = parent->parent();
	}

	if (!mainWindow)
	{
		mainWindow = Application::getActiveWindow();
	}

	return mainWindow;
}

QWidget* MainWindow::findVisibleWidget(const QVector<QPointer<QWidget> > &widgets) const
{
	for (int i = 0; i < widgets.count(); ++i)
	{
		QWidget *widget(widgets.at(i));

		if (widget && widget->isVisible())
		{
			return widget;
		}
	}

	return nullptr;
}

AddressWidget* MainWindow::findAddressField() const
{
	const Window *activeWindow(m_workspace->getActiveWindow());
	const ToolBarWidget *addressToolBar(activeWindow ? activeWindow->getAddressBar() : nullptr);

	if (addressToolBar && !addressToolBar->isCollapsed() && addressToolBar->isVisible())
	{
		AddressWidget *widget(qobject_cast<AddressWidget*>(findVisibleWidget(addressToolBar->getAddressFields())));

		if (widget)
		{
			return widget;
		}
	}

	QMap<int, ToolBarWidget*>::const_iterator iterator;

	for (iterator = m_toolBars.constBegin(); iterator != m_toolBars.constEnd(); ++iterator)
	{
		if (!iterator.value()->isCollapsed() && iterator.value()->isVisible())
		{
			AddressWidget *widget(qobject_cast<AddressWidget*>(findVisibleWidget(iterator.value()->getAddressFields())));

			if (widget)
			{
				return widget;
			}
		}
	}

	return nullptr;
}

SearchWidget* MainWindow::findSearchField() const
{
	const Window *activeWindow(m_workspace->getActiveWindow());
	const ToolBarWidget *addressToolBar(activeWindow ? activeWindow->getAddressBar() : nullptr);

	if (addressToolBar && !addressToolBar->isCollapsed() && addressToolBar->isVisible())
	{
		SearchWidget *widget(qobject_cast<SearchWidget*>(findVisibleWidget(activeWindow->getAddressBar()->getSearchFields())));

		if (widget)
		{
			return widget;
		}
	}

	QMap<int, ToolBarWidget*>::const_iterator iterator;

	for (iterator = m_toolBars.constBegin(); iterator != m_toolBars.constEnd(); ++iterator)
	{
		if (!iterator.value()->isCollapsed() && iterator.value()->isVisible())
		{
			SearchWidget *widget(qobject_cast<SearchWidget*>(findVisibleWidget(iterator.value()->getSearchFields())));

			if (widget)
			{
				return widget;
			}
		}
	}

	return nullptr;
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

Window* MainWindow::openWindow(ContentsWidget *widget, SessionsManager::OpenHints hints, const QVariantMap &parameters)
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
		Session::MainWindow session;
		session.hasToolBarsState = true;

		if (parameters.value(QLatin1String("minimalInterface")).toBool())
		{
			session.toolBars = {getToolBarState(ToolBarsManager::AddressBar), getToolBarState(ToolBarsManager::ProgressBar)};
		}
		else
		{
			session.toolBars = getSession().toolBars;
		}

		window = Application::createWindow({{QLatin1String("hints"), QVariant(hints)}, {QLatin1String("noTabs"), true}}, session)->openWindow(widget);
	}
	else
	{
		window = new Window({{QLatin1String("hints"), QVariant(hints)}, {QLatin1String("size"), ((SettingsManager::getOption(SettingsManager::Interface_NewTabOpeningActionOption).toString() == QLatin1String("maximizeTab")) ? m_workspace->size() : QSize(800, 600))}}, widget, this);

		addWindow(window, hints, parameters.value(QLatin1String("index"), -1).toInt());

		if (!hints.testFlag(SessionsManager::BackgroundOpen))
		{
			m_workspace->setActiveWindow(window, true);
		}
	}

	emit arbitraryActionsStateChanged({ActionsManager::CloseOtherTabsAction});

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
		case ActionsManager::ActionDefinition::EditorScope:
		case ActionsManager::ActionDefinition::WindowScope:
			if (definition.scope == ActionsManager::ActionDefinition::EditorScope && m_editorExecutor.isValid() && m_editorExecutor.getObject() == Application::getFocusObject(true))
			{
				return m_editorExecutor.getActionState(identifier, parameters);
			}

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

			if (parameters.contains(QLatin1String("url")))
			{
				const QUrl url(parameters[QLatin1String("url")].toUrl());

				if (AddonsManager::isSpecialPage(url))
				{
					const AddonsManager::SpecialPageInformation information(AddonsManager::getSpecialPage(url.path()));

					if (!parameters.contains(QLatin1String("icon")) && !information.icon.isNull())
					{
						state.icon = information.icon;
					}

					if (!parameters.contains(QLatin1String("text")))
					{
						state.text = information.title;
					}
				}
			}

			break;
		case ActionsManager::ReopenTabAction:
			if (!m_closedWindows.isEmpty())
			{
				const int index(parameters.value(QLatin1String("index"), 0).toInt());

				state.isEnabled = (index >= 0 && index < m_closedWindows.count());

				if (state.isEnabled)
				{
					state.statusTip = m_closedWindows.at(index).window.getUrl();
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
			{
				const quint64 windowIdentifier(parameters.value(QLatin1String("tab"), 0).toULongLong());

				if (m_windows.contains(windowIdentifier))
				{
					Window *window(m_windows[windowIdentifier]);
					const QString title(window->getTitle());

					state.text = (title.isEmpty() ? tr("actions", "(Untitled)") : title);
					state.icon = window->getIcon();
					state.isEnabled = true;
				}
			}

			break;
		case ActionsManager::OpenBookmarkAction:
			if (parameters.contains(QLatin1String("bookmark")))
			{
				const QVariant bookmarkData(parameters[QLatin1String("bookmark")]);
				const BookmarksModel::Bookmark *bookmark((bookmarkData.type() == QVariant::String) ? BookmarksManager::getBookmark(bookmarkData.toString()) : BookmarksManager::getBookmark(bookmarkData.toULongLong()));

				if (bookmark)
				{
					state.statusTip = bookmark->getUrl().toString();
					state.toolTip = bookmark->getDescription();
					state.text = bookmark->getTitle();
					state.icon = bookmark->getIcon();
					state.isEnabled = true;
				}
			}

			break;
		case ActionsManager::OpenFeedAction:
			{
				FeedsModel::Entry *entry(nullptr);
				Feed *feed(nullptr);

				if (parameters.contains(QLatin1String("entry")))
				{
					entry = FeedsManager::getModel()->getEntry(parameters[QLatin1String("entry")].toULongLong());

					if (entry && entry->getType() == FeedsModel::FeedEntry)
					{
						feed = entry->getFeed();
					}
				}
				else if (parameters.contains(QLatin1String("url")))
				{
					feed = FeedsManager::getFeed(parameters[QLatin1String("url")].toUrl());
				}

				if (feed)
				{
					const int unreadEntriesAmount(feed->getUnreadEntriesAmount());

					state.statusTip = feed->getUrl().toString();
					state.toolTip = feed->getDescription();
					state.text = feed->getTitle();
					state.icon = feed->getIcon();
					state.isEnabled = true;

					if (unreadEntriesAmount > 0)
					{
						state.text.append(QStringLiteral(" (%1)").arg(unreadEntriesAmount));
						state.counter = unreadEntriesAmount;
					}
				}
				else if (entry && entry->getType() == FeedsModel::FolderEntry)
				{
					state.text = entry->getTitle().replace(QLatin1Char('&'), QLatin1String("&&"));
					state.icon = entry->data(Qt::DecorationRole).value<QIcon>();
					state.isEnabled = true;
				}
			}

			break;
		case ActionsManager::FullScreenAction:
			state.icon = ThemesManager::createIcon(isFullScreen() ? QLatin1String("view-restore") : QLatin1String("view-fullscreen"));

			break;
		case ActionsManager::ShowToolBarAction:
			if (parameters.contains(QLatin1String("toolBar")))
			{
				const int toolBarIdentifier((parameters[QLatin1String("toolBar")].type() == QVariant::String) ? ToolBarsManager::getToolBarIdentifier(parameters[QLatin1String("toolBar")].toString()) : parameters[QLatin1String("toolBar")].toInt());
				const ToolBarsManager::ToolBarDefinition toolBarDefinition(ToolBarsManager::getToolBarDefinition(toolBarIdentifier));
				const ToolBarsManager::ToolBarsMode mode(windowState().testFlag(Qt::WindowFullScreen) ? ToolBarsManager::FullScreenMode : ToolBarsManager::NormalMode);

				state.text = toolBarDefinition.getTitle();
				state.isChecked = ToolBarWidget::calculateShouldBeVisible(toolBarDefinition, getToolBarState(toolBarIdentifier), mode);
				state.isEnabled = true;
			}

			break;
		case ActionsManager::ShowMenuBarAction:
			state = getActionState(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::MenuBar}});

			break;
		case ActionsManager::ShowTabBarAction:
			state = getActionState(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::TabBar}});

			break;
		case ActionsManager::ShowSidebarAction:
			state = getActionState(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::SideBar}});

			break;
		case ActionsManager::ShowErrorConsoleAction:
			state = getActionState(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::ErrorConsoleBar}});

			break;
		case ActionsManager::ShowPanelAction:
			{
				const QString panel(parameters.value(QLatin1String("panel")).toString());

				if (panel.isEmpty())
				{
					state.icon = ThemesManager::createIcon(QLatin1String("window-close"));
					state.text = QCoreApplication::translate("actions", "Close Panel");
				}
				else
				{
					state.icon = SidebarWidget::getPanelIcon(panel);
					state.text = SidebarWidget::getPanelTitle(panel);
				}
			}

			break;
		case ActionsManager::OpenPanelAction:
			{
				const ToolBarsManager::ToolBarDefinition toolBarDefinition(ToolBarsManager::getToolBarDefinition(parameters.value(QLatin1String("sidebar"), ToolBarsManager::SideBar).toInt()));

				state.isEnabled = (toolBarDefinition.isValid() && !toolBarDefinition.currentPanel.isEmpty());
			}

			break;
		default:
			break;
	}

	return state;
}

Session::MainWindow MainWindow::getSession() const
{
	const QVector<Qt::ToolBarArea> areas({Qt::LeftToolBarArea, Qt::RightToolBarArea, Qt::TopToolBarArea, Qt::BottomToolBarArea});
	Session::MainWindow session;
	session.geometry = saveGeometry();
	session.index = getCurrentWindowIndex();
	session.splitters = m_splitters;
	session.hasToolBarsState = true;
	session.toolBars.reserve(m_toolBarStates.count() + m_toolBars.count());
	session.toolBars = m_toolBarStates.values().toVector();

	for (int i = 0; i < 4; ++i)
	{
		const QVector<ToolBarWidget*> toolBars(getToolBars(areas.at(i)));

		for (int j = 0; j < toolBars.count(); ++j)
		{
			ToolBarWidget *toolBar(toolBars.at(j));
			Session::MainWindow::ToolBarState state(toolBar->getState());
			state.location = areas.at(i);
			state.identifier = toolBar->getIdentifier();
			state.row = j;

			session.toolBars.append(state);
		}
	}

	session.toolBars.squeeze();

	if (isPrivate())
	{
		session.index = -1;

		return session;
	}

	session.windows.reserve(m_windows.count());

	for (int i = 0; i < m_windows.count(); ++i)
	{
		const Window *window(getWindowByIndex(i));

		if (window && !window->isPrivate())
		{
			session.windows.append(window->getSession());
		}
		else if (i <= session.index)
		{
			--session.index;
		}
	}

	session.windows.squeeze();

	return session;
}

Session::MainWindow::ToolBarState MainWindow::getToolBarState(int identifier) const
{
	if (m_toolBarStates.contains(identifier))
	{
		return m_toolBarStates[identifier];
	}

	if (m_toolBars.contains(identifier))
	{
		return m_toolBars[identifier]->getState();
	}

	Session::MainWindow::ToolBarState state;
	state.identifier = identifier;
	state.normalVisibility = Session::MainWindow::ToolBarState::AlwaysHiddenToolBar;
	state.fullScreenVisibility = Session::MainWindow::ToolBarState::AlwaysHiddenToolBar;

	return state;
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
	});

	return toolBars;
}

QVector<Session::ClosedWindow> MainWindow::getClosedWindows() const
{
	return m_closedWindows;
}

QVector<int> MainWindow::getSplitterSizes(const QString &identifier) const
{
	return m_splitters.value(identifier);
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
	const QVector<quint64> windows(createOrderedWindowList(true));
	QVector<quint64>::const_reverse_iterator iterator;

	for (iterator = windows.rbegin(); iterator != windows.rend(); ++iterator)
	{
		const quint64 identifier(*iterator);
		const Window *window(getWindowByIdentifier(identifier));

		if (window && window->getUrl() == url)
		{
			if (activate)
			{
				setActiveWindowByIdentifier(identifier);
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

bool MainWindow::isSessionRestored() const
{
	return m_isSessionRestored;
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
			SessionsManager::markSessionAsModified();

			break;
		case QEvent::Resize:
			if (m_tabSwitcher && m_tabSwitcher->isVisible())
			{
				m_tabSwitcher->resize(size());
			}

			SessionsManager::markSessionAsModified();

			break;
		case QEvent::StatusTip:
			emit statusMessageChanged(static_cast<QStatusTipEvent*>(event)->tip());

			break;
		case QEvent::WindowStateChange:
			{
				SessionsManager::markSessionAsModified();

				if (windowState().testFlag(Qt::WindowFullScreen) != static_cast<QWindowStateChangeEvent*>(event)->oldState().testFlag(Qt::WindowFullScreen))
				{
					const ToolBarsManager::ToolBarsMode mode(windowState().testFlag(Qt::WindowFullScreen) ? ToolBarsManager::FullScreenMode : ToolBarsManager::NormalMode);

					if (m_menuBar)
					{
						m_menuBar->setVisible(ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar).getVisibility(mode) == ToolBarsManager::AlwaysVisibleToolBar);
					}

					if (m_statusBar)
					{
						m_statusBar->setVisible(ToolBarsManager::getToolBarDefinition(ToolBarsManager::StatusBar).getVisibility(mode) == ToolBarsManager::AlwaysVisibleToolBar);
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

					for (int i = 0; i < toolBars.count(); ++i)
					{
						ToolBarWidget *toolBar(toolBars.at(i));

						if (toolBar->shouldBeVisible(mode))
						{
							toolBar->removeEventFilter(this);
							toolBar->show();
						}
						else
						{
							toolBar->hide();
						}
					}

					emit arbitraryActionsStateChanged({ActionsManager::FullScreenAction, ActionsManager::ShowToolBarAction});
					emit fullScreenStateChanged(mode == ToolBarsManager::FullScreenMode);
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
			SessionsManager::markSessionAsModified();

			emit activated();

			break;
		default:
			break;
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
		Application::getInstance()->triggerAction(m_identifier, parameters, ActionsManager::KeyboardTrigger);
	}
	else
	{
		m_mainWindow->triggerAction(m_identifier, parameters, ActionsManager::KeyboardTrigger);
	}
}

}
