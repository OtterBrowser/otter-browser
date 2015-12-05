/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "BookmarkPropertiesDialog.h"
#include "ClearHistoryDialog.h"
#include "ContentsWidget.h"
#include "LocaleDialog.h"
#include "WorkspaceWidget.h"
#include "Menu.h"
#include "MenuBarWidget.h"
#include "OpenAddressDialog.h"
#include "OpenBookmarkDialog.h"
#include "PreferencesDialog.h"
#include "ReportDialog.h"
#include "SaveSessionDialog.h"
#include "SessionsManagerDialog.h"
#include "StatusBarWidget.h"
#include "TabBarWidget.h"
#include "TabSwitcherWidget.h"
#include "ToolBarAreaWidget.h"
#include "ToolBarWidget.h"
#include "UpdateCheckerDialog.h"
#include "Window.h"
#include "preferences/ContentBlockingDialog.h"
#include "toolbars/ActionWidget.h"
#include "../core/ActionsManager.h"
#include "../core/AddonsManager.h"
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/HistoryManager.h"
#include "../core/InputInterpreter.h"
#include "../core/SettingsManager.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"
#include "../core/WebBackend.h"

#include "ui_MainWindow.h"

#include <QtGui/QCloseEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

MainWindow::MainWindow(bool isPrivate, const SessionMainWindow &session, QWidget *parent) : QMainWindow(parent),
	m_windowsManager(NULL),
	m_tabSwitcher(NULL),
	m_workspace(new WorkspaceWidget(this)),
	m_topToolBarArea(NULL),
	m_bottomToolBarArea(NULL),
	m_leftToolBarArea(NULL),
	m_rightToolBarArea(NULL),
	m_tabBar(NULL),
	m_menuBar(NULL),
	m_statusBar(NULL),
	m_sidebarToggle(NULL),
	m_sidebar(NULL),
	m_splitter(new QSplitter(this)),
	m_currentWindow(NULL),
	m_tabSwitcherKey(0),
	m_tabSwictherTimer(0),
	m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);

	m_standardActions.fill(NULL, ActionsManager::getActionDefinitions().count());

	updateShortcuts();

	SessionsManager::setActiveWindow(this);

	m_windowsManager = new WindowsManager((isPrivate || SessionsManager::isPrivate() || SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool()), this);
	m_topToolBarArea = new ToolBarAreaWidget(Qt::TopToolBarArea, this);
	m_bottomToolBarArea = new ToolBarAreaWidget(Qt::BottomToolBarArea, this);
	m_leftToolBarArea = new ToolBarAreaWidget(Qt::LeftToolBarArea, this);
	m_rightToolBarArea = new ToolBarAreaWidget(Qt::RightToolBarArea, this);

	m_workspace->updateActions();

	m_splitter->setChildrenCollapsible(false);
	m_splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_splitter->addWidget(m_workspace);

	QWidget *verticalWidget = new QWidget(this);
	QWidget *horizontalWidget = new QWidget(verticalWidget);
	QHBoxLayout *horizontalLayout = new QHBoxLayout(horizontalWidget);
	horizontalLayout->setContentsMargins(0, 0, 0, 0);
	horizontalLayout->setSpacing(0);
	horizontalLayout->addWidget(m_leftToolBarArea);
	horizontalLayout->addWidget(m_splitter);
	horizontalLayout->addWidget(m_rightToolBarArea);

	QVBoxLayout *verticalLayout = new QVBoxLayout(verticalWidget);
	verticalLayout->setContentsMargins(0, 0, 0, 0);
	verticalLayout->setSpacing(0);
	verticalLayout->addWidget(m_topToolBarArea);
	verticalLayout->addWidget(horizontalWidget);
	verticalLayout->addWidget(m_bottomToolBarArea);

	setCentralWidget(verticalWidget);

	SessionsManager::registerWindow(this);

	getAction(ActionsManager::WorkOfflineAction)->setChecked(SettingsManager::getValue(QLatin1String("Network/WorkOffline")).toBool());
	getAction(ActionsManager::ShowMenuBarAction)->setChecked(ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar).visibility != ToolBarsManager::AlwaysHiddenToolBar);
	getAction(ActionsManager::ShowSidebarAction)->setChecked(SettingsManager::getValue(QLatin1String("Sidebar/Visible")).toBool());
	getAction(ActionsManager::LockToolBarsAction)->setChecked(ToolBarsManager::areToolBarsLocked());
	getAction(ActionsManager::ExitAction)->setMenuRole(QAction::QuitRole);
	getAction(ActionsManager::PreferencesAction)->setMenuRole(QAction::PreferencesRole);
	getAction(ActionsManager::AboutQtAction)->setMenuRole(QAction::AboutQtRole);
	getAction(ActionsManager::AboutApplicationAction)->setMenuRole(QAction::AboutRole);

	if (ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar).visibility != ToolBarsManager::AlwaysHiddenToolBar)
	{
		m_menuBar = new MenuBarWidget(this);

		setMenuBar(m_menuBar);
	}

	if (ToolBarsManager::getToolBarDefinition(ToolBarsManager::StatusBar).visibility != ToolBarsManager::AlwaysHiddenToolBar)
	{
		m_statusBar = new StatusBarWidget(this);

		setStatusBar(m_statusBar);
	}

	optionChanged(QLatin1String("Sidebar/ShowToggleEdge"), SettingsManager::getValue(QLatin1String("Sidebar/ShowToggleEdge")));
	optionChanged(QLatin1String("Sidebar/Visible"), SettingsManager::getValue(QLatin1String("Sidebar/Visible")));

	connect(ActionsManager::getInstance(), SIGNAL(shortcutsChanged()), this, SLOT(updateShortcuts()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(ToolBarsManager::getInstance(), SIGNAL(toolBarModified(int)), this, SLOT(toolBarModified(int)));
	connect(TransfersManager::getInstance(), SIGNAL(transferStarted(Transfer*)), this, SLOT(transferStarted()));
	connect(m_windowsManager, SIGNAL(requestedAddBookmark(QUrl,QString,QString)), this, SLOT(addBookmark(QUrl,QString,QString)));
	connect(m_windowsManager, SIGNAL(requestedEditBookmark(QUrl)), this, SLOT(editBookmark(QUrl)));
	connect(m_windowsManager, SIGNAL(requestedNewWindow(bool,bool,QUrl)), this, SIGNAL(requestedNewWindow(bool,bool,QUrl)));
	connect(m_windowsManager, SIGNAL(windowTitleChanged(QString)), this, SLOT(updateWindowTitle(QString)));
	connect(m_ui->consoleDockWidget, SIGNAL(visibilityChanged(bool)), getAction(ActionsManager::ShowErrorConsoleAction), SLOT(setChecked(bool)));

	m_windowsManager->restore(session);

	m_ui->consoleDockWidget->hide();

	updateWindowTitle(m_windowsManager->getTitle());

	if (!session.geometry.isEmpty())
	{
		restoreGeometry(session.geometry);
	}
	else if (SettingsManager::getValue(QLatin1String("Interface/MaximizeNewWindows")).toBool())
	{
		showMaximized();
	}
}

MainWindow::~MainWindow()
{
	delete m_ui;
}

void MainWindow::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_tabSwictherTimer)
	{
		killTimer(m_tabSwictherTimer);

		m_tabSwictherTimer = 0;

		if (m_windowsManager->getWindowCount() > 1)
		{
			if (!m_tabSwitcher)
			{
				m_tabSwitcher = new TabSwitcherWidget(m_windowsManager, this);
			}

			m_tabSwitcher->raise();
			m_tabSwitcher->resize(size());
			m_tabSwitcher->show(TabSwitcherWidget::KeyboardReason);
			m_tabSwitcher->selectTab(m_tabSwitcherKey == Qt::Key_Tab);
		}
		else
		{
			m_windowsManager->triggerAction((m_tabSwitcherKey == Qt::Key_Tab) ? ActionsManager::ActivateTabOnRightAction : ActionsManager::ActivateTabOnLeftAction);
		}
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (SessionsManager::isLastWindow() && !Application::getInstance()->canClose())
	{
		event->ignore();

		return;
	}

	Application *application = Application::getInstance();

	if (application->getWindows().count() == 1)
	{
		if (SessionsManager::getCurrentSession() == QLatin1String("default"))
		{
			SessionsManager::saveSession();
		}
	}
	else
	{
		SessionsManager::storeClosedWindow(this);
	}

	m_windowsManager->closeAll();

	application->removeWindow(this);

	event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab)
	{
		if (m_tabSwictherTimer == 0 && m_windowsManager->getWindowCount() > 1)
		{
			m_tabSwitcherKey = event->key();
			m_tabSwictherTimer = startTimer(200);
		}
		else
		{
			if (m_tabSwictherTimer > 0)
			{
				killTimer(m_tabSwictherTimer);

				m_tabSwictherTimer = 0;
			}

			if (m_windowsManager->getWindowCount() > 1)
			{
				if (!m_tabSwitcher)
				{
					m_tabSwitcher = new TabSwitcherWidget(m_windowsManager, this);
				}

				m_tabSwitcher->raise();
				m_tabSwitcher->resize(size());
				m_tabSwitcher->show(TabSwitcherWidget::KeyboardReason);
				m_tabSwitcher->selectTab(event->key() == Qt::Key_Tab);
			}
			else
			{
				m_windowsManager->triggerAction((event->key() == Qt::Key_Tab) ? ActionsManager::ActivateTabOnRightAction : ActionsManager::ActivateTabOnLeftAction);
			}
		}

		event->accept();
	}
	else
	{
		QMainWindow::keyPressEvent(event);
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Control && m_tabSwictherTimer > 0)
	{
		killTimer(m_tabSwictherTimer);

		m_tabSwictherTimer = 0;

		QVariantMap parameters;
		parameters[QLatin1String("includeMinimized")] = true;

		m_workspace->triggerAction(((m_tabSwitcherKey == Qt::Key_Tab) ? ActionsManager::ActivatePreviouslyUsedTabAction : ActionsManager::ActivateLeastRecentlyUsedTabAction), parameters);
	}

	QMainWindow::keyReleaseEvent(event);
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
	if (m_tabSwitcher && m_tabSwitcher->isVisible())
	{
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

void MainWindow::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Network/WorkOffline"))
	{
		getAction(ActionsManager::WorkOfflineAction)->setChecked(value.toBool());
	}
	else if (option == QLatin1String("Interface/LockToolBars"))
	{
		getAction(ActionsManager::LockToolBarsAction)->setChecked(value.toBool());
	}
	else if (option == QLatin1String("Sidebar/CurrentPanel") || option == QLatin1String("Sidebar/Reverse"))
	{
		placeSidebars();
	}
	else if (option == QLatin1String("Sidebar/ShowToggleEdge"))
	{
		if (m_sidebarToggle && !value.toBool())
		{
			m_sidebarToggle->deleteLater();
			m_sidebarToggle = NULL;

			placeSidebars();
		}
		else if (!m_sidebarToggle && value.toBool())
		{
			m_sidebarToggle = new ActionWidget(ActionsManager::ShowSidebarAction, NULL, this);
			m_sidebarToggle->setFixedWidth(6);
			m_sidebarToggle->setText(QString());

			placeSidebars();
		}
	}
	else if (option == QLatin1String("Sidebar/Visible"))
	{
		if (m_sidebar && !value.toBool())
		{
			m_sidebar->hide();

			placeSidebars();
		}
		else if (value.toBool())
		{
			if (!m_sidebar)
			{
				m_sidebar = new SidebarWidget(this);
				m_sidebar->show();

				connect(m_splitter, SIGNAL(splitterMoved(int,int)), m_sidebar, SLOT(scheduleSizeSave()));
			}

			m_sidebar->show();

			placeSidebars();
		}
	}
}

void MainWindow::startToolBarDragging()
{
	m_topToolBarArea->setContentsMargins(0, 4, 0, 4);
	m_bottomToolBarArea->setContentsMargins(0, 4, 0, 4);
	m_leftToolBarArea->setContentsMargins(4, 0, 4, 0);
	m_rightToolBarArea->setContentsMargins(4, 0, 4, 0);
}

void MainWindow::endToolBarDragging()
{
	m_topToolBarArea->setContentsMargins(0, 0, 0, 0);
	m_bottomToolBarArea->setContentsMargins(0, 0, 0, 0);
	m_leftToolBarArea->setContentsMargins(0, 0, 0, 0);
	m_rightToolBarArea->setContentsMargins(0, 0, 0, 0);
}

void MainWindow::moveToolBar(ToolBarWidget *toolBar, Qt::ToolBarArea area)
{
	switch (area)
	{
		case Qt::BottomToolBarArea:
			m_bottomToolBarArea->insertToolBar(toolBar);

			break;
		case Qt::LeftToolBarArea:
			m_leftToolBarArea->insertToolBar(toolBar);

			break;
		case Qt::RightToolBarArea:
			m_rightToolBarArea->insertToolBar(toolBar);

			break;
		default:
			m_topToolBarArea->insertToolBar(toolBar);

			break;
	}
}

void MainWindow::openUrl(const QString &text)
{
	if (text.isEmpty())
	{
		m_windowsManager->triggerAction(ActionsManager::NewTabAction);

		return;
	}
	else
	{
		InputInterpreter *interpreter = new InputInterpreter(this);

		connect(interpreter, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), m_windowsManager, SLOT(open(BookmarksItem*,WindowsManager::OpenHints)));
		connect(interpreter, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), m_windowsManager, SLOT(open(QUrl,WindowsManager::OpenHints)));
		connect(interpreter, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), m_windowsManager, SLOT(search(QString,QString,WindowsManager::OpenHints)));

		interpreter->interpret(text, ((!m_workspace->getActiveWindow() || Utils::isUrlEmpty(m_workspace->getActiveWindow()->getUrl())) ? WindowsManager::CurrentTabOpen : WindowsManager::NewTabOpen));
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

void MainWindow::triggerAction(int identifier, const QVariantMap &parameters)
{
	switch (identifier)
	{
		case ActionsManager::NewWindowAction:
			emit requestedNewWindow(false, false, QUrl());

			break;
		case ActionsManager::NewWindowPrivateAction:
			emit requestedNewWindow(true, false, QUrl());

			break;
		case ActionsManager::OpenAction:
			{
				const QUrl url = QFileDialog::getOpenFileUrl(this, tr("Open File"));

				if (url.isValid())
				{
					m_windowsManager->open(url);
				}
			}

			break;
		case ActionsManager::ClosePrivateTabsPanicAction:
			if (SessionsManager::isPrivate())
			{
				Application::getInstance()->close();
			}
			else
			{
				const QList<MainWindow*> windows = SessionsManager::getWindows();

				for (int i = 0; i < windows.count(); ++i)
				{
					if (windows[i]->getWindowsManager()->isPrivate())
					{
						windows[i]->close();
					}
					else
					{
						windows[i]->getWindowsManager()->triggerAction(ActionsManager::ClosePrivateTabsAction);
					}
				}
			}

			break;
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

			break;
		case ActionsManager::CloseWindowAction:
			close();

			break;
		case ActionsManager::SessionsAction:
			{
				SessionsManagerDialog dialog(this);
				dialog.exec();
			}

			break;
		case ActionsManager::SaveSessionAction:
			{
				SaveSessionDialog dialog(this);
				dialog.exec();
			}

			break;
		case ActionsManager::GoToPageAction:
			{
				OpenAddressDialog dialog(this);

				connect(&dialog, SIGNAL(requestedLoadUrl(QUrl,WindowsManager::OpenHints)), m_windowsManager, SLOT(open(QUrl,WindowsManager::OpenHints)));
				connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), m_windowsManager, SLOT(open(BookmarksItem*,WindowsManager::OpenHints)));
				connect(&dialog, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), m_windowsManager, SLOT(search(QString,QString,WindowsManager::OpenHints)));

				dialog.exec();
			}

			break;
		case ActionsManager::GoToHomePageAction:
			{
				const QString homePage = SettingsManager::getValue(QLatin1String("Browser/HomePage")).toString();

				if (!homePage.isEmpty())
				{
					m_windowsManager->open(QUrl(homePage), WindowsManager::CurrentTabOpen);
				}
			}

			break;
		case ActionsManager::BookmarksAction:
			{
				const QUrl url(QLatin1String("about:bookmarks"));

				if (!SessionsManager::hasUrl(url, true))
				{
					m_windowsManager->open(url);
				}
			}

			break;
		case ActionsManager::BookmarkPageAction:
			addBookmark(QUrl(), QString(), QString(), true);

			break;
		case ActionsManager::QuickBookmarkAccessAction:
			{
				OpenBookmarkDialog dialog(this);

				connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*)), m_windowsManager, SLOT(open(BookmarksItem*)));

				dialog.exec();
			}

			break;
		case ActionsManager::WorkOfflineAction:
			SettingsManager::setValue(QLatin1String("Network/WorkOffline"), parameters.value(QLatin1String("isChecked")).toBool());

			break;
		case ActionsManager::FullScreenAction:
			if (isFullScreen())
			{
				restoreWindowState();
			}
			else
			{
				storeWindowState();
				showFullScreen();
			}

			break;
		case ActionsManager::ShowTabSwitcherAction:
			if (!m_tabSwitcher)
			{
				m_tabSwitcher = new TabSwitcherWidget(m_windowsManager, this);
			}

			m_tabSwitcher->raise();
			m_tabSwitcher->resize(size());
			m_tabSwitcher->show(TabSwitcherWidget::ActionReason);

			break;
		case ActionsManager::ShowMenuBarAction:
			{
				ToolBarsManager::ToolBarDefinition definition = ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar);
				definition.visibility = (parameters.value(QLatin1String("isChecked")).toBool() ? ToolBarsManager::AlwaysVisibleToolBar : ToolBarsManager::AlwaysHiddenToolBar);

				ToolBarsManager::setToolBar(definition);
			}

			break;
		case ActionsManager::ShowTabBarAction:
			{
				ToolBarsManager::ToolBarDefinition definition = ToolBarsManager::getToolBarDefinition(ToolBarsManager::TabBar);
				definition.visibility = (parameters.value(QLatin1String("isChecked")).toBool() ? ToolBarsManager::AlwaysVisibleToolBar : ToolBarsManager::AlwaysHiddenToolBar);

				ToolBarsManager::setToolBar(definition);
			}

			break;
		case ActionsManager::ShowSidebarAction:
			SettingsManager::setValue(QLatin1String("Sidebar/Visible"), parameters.value(QLatin1String("isChecked")).toBool());

			break;
		case ActionsManager::OpenPanelAction:
			if (m_sidebar && m_sidebar->getCurrentPanel())
			{
				m_windowsManager->open(m_sidebar->getCurrentPanel()->getUrl(), WindowsManager::NewTabOpen);
			}

			break;
		case ActionsManager::ClosePanelAction:
			SettingsManager::setValue(QLatin1String("Sidebar/CurrentPanel"), QString());

			break;
		case ActionsManager::ShowErrorConsoleAction:
			m_ui->consoleDockWidget->setVisible(parameters.value(QLatin1String("isChecked")).toBool());

			break;
		case ActionsManager::LockToolBarsAction:
			ToolBarsManager::setToolBarsLocked(parameters.value(QLatin1String("isChecked")).toBool());

			break;
		case ActionsManager::ContentBlockingAction:
			{
				ContentBlockingDialog dialog(this);
				dialog.exec();
			}

			break;
		case ActionsManager::HistoryAction:
			{
				const QUrl url(QLatin1String("about:history"));

				if (!SessionsManager::hasUrl(url, true))
				{
					m_windowsManager->open(url);
				}
			}

			break;
		case ActionsManager::ClearHistoryAction:
			{
				ClearHistoryDialog dialog(SettingsManager::getValue(QLatin1String("History/ManualClearOptions")).toStringList(), false, this);
				dialog.exec();
			}

			break;
		case ActionsManager::NotesAction:
			{
				const QUrl url(QLatin1String("about:notes"));

				if (!SessionsManager::hasUrl(url, true))
				{
					m_windowsManager->open(url);
				}
			}

			break;
		case ActionsManager::TransfersAction:
			{
				const QUrl url(QLatin1String("about:transfers"));

				if (!SessionsManager::hasUrl(url, true))
				{
					m_windowsManager->open(url);
				}
			}

			break;
		case ActionsManager::CookiesAction:
			{
				const QUrl url(QLatin1String("about:cookies"));

				if (!SessionsManager::hasUrl(url, true))
				{
					m_windowsManager->open(url);
				}
			}

			break;
		case ActionsManager::PreferencesAction:
			{
				PreferencesDialog dialog(QLatin1String("general"), this);
				dialog.exec();
			}

			break;
		case ActionsManager::SwitchApplicationLanguageAction:
			{
				LocaleDialog dialog(this);
				dialog.exec();
			}

			break;
		case ActionsManager::CheckForUpdatesAction:
			{
				UpdateCheckerDialog *dialog = new UpdateCheckerDialog(this);
				dialog->show();
			}

			break;
		case ActionsManager::DiagnosticReportAction:
			{
				ReportDialog *dialog = new ReportDialog(this);
				dialog->show();
			}

			break;
		case ActionsManager::AboutApplicationAction:
			{
				WebBackend *backend = AddonsManager::getWebBackend();
				QString about = tr("<b>Otter %1</b><br>Web browser controlled by the user, not vice-versa.").arg(Application::getInstance()->getFullVersion());
				about.append(QLatin1String("<br><br>") + tr("Web backend: %1 %2.").arg(backend->getTitle()).arg(backend->getEngineVersion()) + QLatin1String("<br><br>"));

				if (QSslSocket::supportsSsl())
				{
					 about.append(tr("SSL library version: %1.").arg(QSslSocket::sslLibraryVersionString()));
				}
				else
				{
					about.append(tr("SSL library not available."));
				}

				QMessageBox::about(this, QLatin1String("Otter"), about);
			}

			break;
		case ActionsManager::AboutQtAction:
			Application::getInstance()->aboutQt();

			break;
		case ActionsManager::ExitAction:
			Application::getInstance()->close();

			break;
		default:
			m_windowsManager->triggerAction(identifier, parameters);

			break;
	}
}

void MainWindow::triggerAction()
{
	QShortcut *shortcut = qobject_cast<QShortcut*>(sender());

	if (shortcut)
	{
		for (int i = 0; i < m_actionShortcuts.count(); ++i)
		{
			if (m_actionShortcuts[i].second.contains(shortcut))
			{
				const ActionDefinition definition = ActionsManager::getActionDefinition(m_actionShortcuts[i].first);

				if (definition.identifier >= 0)
				{
					if (definition.isCheckable)
					{
						Action *action = getAction(m_actionShortcuts[i].first);

						if (action)
						{
							action->toggle();
						}
					}
					else
					{
						triggerAction(m_actionShortcuts[i].first);
					}
				}

				return;
			}
		}

		return;
	}

	Action *action = qobject_cast<Action*>(sender());

	if (action)
	{
		triggerAction(action->getIdentifier(), action->data().toMap());
	}
}

void MainWindow::triggerAction(bool checked)
{
	Q_UNUSED(checked)

	Action *action = qobject_cast<Action*>(sender());

	if (action)
	{
		QVariantMap parameters = action->data().toMap();

		if (action->isCheckable())
		{
			parameters[QLatin1String("isChecked")] = action->isChecked();
		}

		triggerAction(action->getIdentifier(), parameters);
	}
}

void MainWindow::addBookmark(const QUrl &url, const QString &title, const QString &description, bool warn)
{
	const QUrl bookmarkUrl = (url.isValid() ? url : m_windowsManager->getUrl()).adjusted(QUrl::RemovePassword);

	if (bookmarkUrl.isEmpty() || (warn && BookmarksManager::hasBookmark(bookmarkUrl) && QMessageBox::warning(this, tr("Warning"), tr("You already have this address in your bookmarks.\nDo you want to continue?"), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Cancel))
	{
		return;
	}

	BookmarkPropertiesDialog dialog(bookmarkUrl, (url.isValid() ? title : m_windowsManager->getTitle()), description, NULL, -1, true, this);
	dialog.exec();
}

void MainWindow::editBookmark(const QUrl &url)
{
	const QList<BookmarksItem*> bookmarks = BookmarksManager::getModel()->getBookmarks(url);

	if (!bookmarks.isEmpty())
	{
		BookmarkPropertiesDialog dialog(bookmarks.at(0), this);
		dialog.exec();
	}
}

void MainWindow::toolBarModified(int identifier)
{
	if (identifier == ToolBarsManager::MenuBar)
	{
		const bool showMenuBar = (ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar).visibility != ToolBarsManager::AlwaysHiddenToolBar);

		if (m_menuBar && !showMenuBar)
		{
			m_menuBar->deleteLater();
			m_menuBar = NULL;

			setMenuBar(NULL);
		}
		else if (!m_menuBar && showMenuBar)
		{
			m_menuBar = new MenuBarWidget(this);

			setMenuBar(m_menuBar);
		}

		getAction(ActionsManager::ShowMenuBarAction)->setChecked(showMenuBar);
	}
	else if (identifier == ToolBarsManager::StatusBar)
	{
		const bool showStatusBar = (ToolBarsManager::getToolBarDefinition(ToolBarsManager::StatusBar).visibility != ToolBarsManager::AlwaysHiddenToolBar);

		if (m_statusBar && !showStatusBar)
		{
			m_statusBar->deleteLater();
			m_statusBar = NULL;

			setStatusBar(NULL);
		}
		else if (!m_statusBar && showStatusBar)
		{
			m_statusBar = new StatusBarWidget(this);

			setStatusBar(m_statusBar);
		}
	}
}

void MainWindow::transferStarted()
{
	const QString action = SettingsManager::getValue(QLatin1String("Browser/TransferStartingAction")).toString();

	if (action == QLatin1String("openTab"))
	{
		triggerAction(ActionsManager::TransfersAction);
	}
	else if (action == QLatin1String("openBackgroundTab"))
	{
		const QUrl url(QLatin1String("about:transfers"));

		if (!SessionsManager::hasUrl(url, false))
		{
			m_windowsManager->open(url, (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen));
		}
	}
	else if (action == QLatin1String("openPanel"))
	{
		QVariantMap parameters;
		parameters[QLatin1String("isChecked")] = true;

		triggerAction(ActionsManager::ShowSidebarAction, parameters);

		m_sidebar->selectPanel(QLatin1String("transfers"));
	}
}

void MainWindow::placeSidebars()
{
	if (m_sidebarToggle)
	{
		m_splitter->addWidget(m_sidebarToggle);
	}

	if (m_sidebar)
	{
		m_splitter->addWidget(m_sidebar);
	}

	m_splitter->addWidget(m_workspace);

	if (SettingsManager::getValue(QString("Sidebar/Reverse")).toBool())
	{
		for (int i = m_splitter->count() - 1; i >= 0; --i)
		{
			m_splitter->addWidget(m_splitter->widget(i));
		}
	}

	updateSidebars();
}

void MainWindow::updateSidebars()
{
	QList<int> sizes;
	const int sidebarSize = ((m_sidebar && m_sidebar->isVisible()) ? m_sidebar->sizeHint().width() : 0);
	const int sidebarToggleSize = (m_sidebarToggle ? m_sidebarToggle->width() : 0);
	const bool isReversed = SettingsManager::getValue(QLatin1String("Sidebar/Reverse")).toBool();

	if (m_sidebarToggle)
	{
		sizes.append(sidebarToggleSize);
	}

	if (m_sidebar && m_sidebar->isVisible())
	{
		sizes.append(sidebarSize);
	}

	sizes.append(m_splitter->width() - sidebarSize - sidebarToggleSize - ((sizes.count() - 1) * m_splitter->handleWidth()));

	if (isReversed)
	{
		QList<int> sizesCopy(sizes);

		sizes.clear();

		for (int i = sizesCopy.count() - 1; i >= 0; --i)
		{
			sizes.append(sizesCopy[i]);
		}
	}

	const int sidebarToggleIndex = (m_sidebarToggle ? (m_splitter->indexOf(m_sidebarToggle) + (isReversed ? 0 : 1)) : -1);

	for (int i = 0; i < m_splitter->count(); ++i)
	{
		if (m_splitter->handle(i))
		{
			m_splitter->handle(i)->setMaximumWidth((i == sidebarToggleIndex || !m_sidebar || !m_sidebar->isVisible()) ? 0 : QWIDGETSIZE_MAX);
		}
	}

	m_splitter->setSizes(sizes);
}

void MainWindow::updateWindowTitle(const QString &title)
{
	m_windowTitle = (title.isEmpty() ? QStringLiteral("Otter") : QStringLiteral("%1 - Otter").arg(title));

	setWindowTitle(m_windowTitle);
}

void MainWindow::updateShortcuts()
{
	for (int i = 0; i < m_actionShortcuts.count(); ++i)
	{
		qDeleteAll(m_actionShortcuts[i].second);
	}

	m_actionShortcuts.clear();

	const QVector<ActionDefinition> definitions = ActionsManager::getActionDefinitions();
	QList<QKeySequence> standardShortcuts;
	standardShortcuts << QKeySequence(QKeySequence::Copy) << QKeySequence(QKeySequence::Cut) << QKeySequence(QKeySequence::Delete) << QKeySequence(QKeySequence::Paste) << QKeySequence(QKeySequence::Redo) << QKeySequence(QKeySequence::SelectAll) << QKeySequence(QKeySequence::Undo);

	for (int i = 0; i < definitions.count(); ++i)
	{
		QVector<QShortcut*> shortcuts;
		shortcuts.reserve(definitions[i].shortcuts.count());

		for (int j = 0; j < definitions[i].shortcuts.count(); ++j)
		{
			if (!standardShortcuts.contains(definitions[i].shortcuts[j]))
			{
				QShortcut *shortcut = new QShortcut(definitions[i].shortcuts[j], this);

				shortcuts.append(shortcut);

				connect(shortcut, SIGNAL(activated()), this, SLOT(triggerAction()));
			}
		}

		m_actionShortcuts.append(qMakePair(i, shortcuts));
	}
}

void MainWindow::setCurrentWindow(Window *window)
{
	Window *previousWindow = m_currentWindow;

	m_currentWindow = window;

	for (int i = 0; i < m_standardActions.count(); ++i)
	{
		if (m_standardActions[i] && Action::isLocal(m_standardActions[i]->getIdentifier()))
		{
			const int identifier = m_standardActions[i]->getIdentifier();
			Action *previousAction = ((previousWindow && previousWindow->getContentsWidget()) ? previousWindow->getContentsWidget()->getAction(identifier) : NULL);
			Action *currentAction = (window ? window->getContentsWidget()->getAction(identifier) : NULL);

			if (previousAction)
			{
				disconnect(previousAction, SIGNAL(changed()), m_standardActions[i], SLOT(setup()));
			}

			m_standardActions[i]->setup(currentAction);

			if (currentAction)
			{
				connect(currentAction, SIGNAL(changed()), m_standardActions[i], SLOT(setup()));
			}
		}
	}
}

void MainWindow::setTabBar(TabBarWidget *tabBar)
{
	m_tabBar = tabBar;
}

MainWindow* MainWindow::findMainWindow(QObject *parent)
{
	if (!parent)
	{
		return NULL;
	}

	if (parent->metaObject()->className() == QLatin1String("Otter::MainWindow"))
	{
		return qobject_cast<MainWindow*>(parent);
	}

	MainWindow *window = NULL;
	QWidget *widget = qobject_cast<QWidget*>(parent);

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
		window = qobject_cast<MainWindow*>(SessionsManager::getActiveWindow());
	}

	return window;
}

Action* MainWindow::getAction(int identifier)
{
	if (identifier < 0 || identifier >= m_standardActions.count())
	{
		return NULL;
	}

	if (!m_standardActions[identifier])
	{
		const ActionDefinition definition = ActionsManager::getActionDefinition(identifier);
		Action *action = new Action(identifier, this);

		m_standardActions[identifier] = action;

		addAction(action);

		if (definition.isCheckable)
		{
			connect(action, SIGNAL(toggled(bool)), this, SLOT(triggerAction(bool)));
		}
		else
		{
			connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));
		}
	}

	return m_standardActions.value(identifier, NULL);
}

WorkspaceWidget* MainWindow::getWorkspace()
{
	return m_workspace;
}

TabBarWidget* MainWindow::getTabBar()
{
	return m_tabBar;
}

WindowsManager* MainWindow::getWindowsManager()
{
	return m_windowsManager;
}

bool MainWindow::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			updateWindowTitle(m_windowsManager->getTitle());

			break;
		case QEvent::WindowStateChange:
			{
				QWindowStateChangeEvent *stateChangeEvent = dynamic_cast<QWindowStateChangeEvent*>(event);

				SessionsManager::markSessionModified();

				if (stateChangeEvent && windowState().testFlag(Qt::WindowFullScreen) != stateChangeEvent->oldState().testFlag(Qt::WindowFullScreen))
				{
					if (isFullScreen())
					{
						getAction(ActionsManager::FullScreenAction)->setIcon(Utils::getIcon(QLatin1String("view-restore")));

						if (m_statusBar)
						{
							m_statusBar->hide();
						}

						if (m_menuBar)
						{
							m_menuBar->hide();
						}

						m_workspace->installEventFilter(this);
					}
					else
					{
						getAction(ActionsManager::FullScreenAction)->setIcon(Utils::getIcon(QLatin1String("view-fullscreen")));

						if (m_statusBar)
						{
							m_statusBar->show();
						}

						if (m_menuBar)
						{
							m_menuBar->show();
						}

						m_workspace->removeEventFilter(this);
					}

					emit controlsHiddenChanged(windowState().testFlag(Qt::WindowFullScreen));
				}
			}

			break;
		case QEvent::WindowTitleChange:
			if (m_windowTitle != windowTitle() && m_windowsManager)
			{
				updateWindowTitle(m_windowsManager->getTitle());
			}

			break;
		case QEvent::WindowActivate:
			SessionsManager::setActiveWindow(this);
		case QEvent::Resize:
			updateSidebars();

			if (m_tabSwitcher && m_tabSwitcher->isVisible())
			{
				m_tabSwitcher->resize(size());
			}

			SessionsManager::markSessionModified();

			break;
		case QEvent::StatusTip:
			{
				QStatusTipEvent *statusTipEvent = static_cast<QStatusTipEvent*>(event);

				if (statusTipEvent)
				{
					emit statusMessageChanged(statusTipEvent->tip());
				}
			}

			break;
		case QEvent::Move:
			SessionsManager::markSessionModified();

			break;
		default:
			break;
	}

	return QMainWindow::event(event);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_workspace && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent && keyEvent->key() == Qt::Key_Escape)
		{
			triggerAction(ActionsManager::FullScreenAction);
		}
	}

	return QMainWindow::eventFilter(object, event);
}

}
