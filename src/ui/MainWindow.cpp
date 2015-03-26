/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#include "MainWindow.h"
#include "BookmarkPropertiesDialog.h"
#include "ClearHistoryDialog.h"
#include "LocaleDialog.h"
#include "MdiWidget.h"
#include "MenuBarWidget.h"
#include "OpenAddressDialog.h"
#include "OpenBookmarkDialog.h"
#include "PreferencesDialog.h"
#include "SaveSessionDialog.h"
#include "SessionsManagerDialog.h"
#include "TabBarWidget.h"
#include "TabSwitcherWidget.h"
#include "ToolBarWidget.h"
#include "Window.h"
#include "preferences/ContentBlockingDialog.h"
#include "toolbars/ActionWidget.h"
#include "../core/ActionsManager.h"
#include "../core/AddonsManager.h"
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/BookmarksModel.h"
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
	m_actionsManager(NULL),
	m_windowsManager(NULL),
	m_tabSwitcher(NULL),
	m_mdiWidget(new MdiWidget(this)),
	m_tabBar(NULL),
	m_tabBarToolBar(NULL),
	m_statusBar(NULL),
	m_menuBar(NULL),
	m_toggleEdge(NULL),
	m_sidebarWidget(NULL),
	m_splitter(new QSplitter(this)),
	m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);

	m_actionsManager = new ActionsManager(this);

	SessionsManager::setActiveWindow(this);

	m_windowsManager = new WindowsManager((isPrivate || SessionsManager::isPrivate() || SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool()), this);

	m_splitter->setChildrenCollapsible(false);
	m_splitter->addWidget(m_mdiWidget);

	QWidget *centralWidget = new QWidget(this);
	QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
	centralLayout->setContentsMargins(0, 0, 0, 0);
	centralLayout->setSpacing(0);
	centralLayout->addWidget(m_splitter);

	m_statusBar = new ToolBarWidget(QLatin1String("StatusBar"), NULL, centralWidget);
	m_statusBar->setFixedHeight(20);

	centralLayout->addWidget(m_statusBar);

	setCentralWidget(centralWidget);

	const QList<ToolBarDefinition> toolBarDefinitions = ActionsManager::getToolBarDefinitions();

	for (int i = 0; i < toolBarDefinitions.count(); ++i)
	{
		if (toolBarDefinitions.at(i).location != Qt::NoToolBarArea)
		{
			addToolBar(toolBarDefinitions.at(i).identifier);
		}
	}

	SessionsManager::registerWindow(this);

	m_actionsManager->getAction(Action::WorkOfflineAction)->setChecked(SettingsManager::getValue(QLatin1String("Network/WorkOffline")).toBool());
	m_actionsManager->getAction(Action::ShowMenuBarAction)->setChecked(SettingsManager::getValue(QLatin1String("Interface/ShowMenuBar")).toBool());
	m_actionsManager->getAction(Action::ShowSidebarAction)->setChecked(SettingsManager::getValue(QLatin1String("Sidebar/Visible")).toBool());
	m_actionsManager->getAction(Action::LockToolBarsAction)->setChecked(SettingsManager::getValue(QLatin1String("Interface/LockToolBars")).toBool());
	m_actionsManager->getAction(Action::ExitAction)->setMenuRole(QAction::QuitRole);
	m_actionsManager->getAction(Action::PreferencesAction)->setMenuRole(QAction::PreferencesRole);
	m_actionsManager->getAction(Action::AboutQtAction)->setMenuRole(QAction::AboutQtRole);
	m_actionsManager->getAction(Action::AboutApplicationAction)->setMenuRole(QAction::AboutRole);

	if (SettingsManager::getValue(QLatin1String("Interface/ShowMenuBar")).toBool())
	{
		m_menuBar = new MenuBarWidget(this);

		setMenuBar(m_menuBar);
	}

	placeSidebars();

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(TransfersManager::getInstance(), SIGNAL(transferStarted(Transfer*)), this, SLOT(transferStarted()));
	connect(m_actionsManager, SIGNAL(toolBarAdded(QString)), this, SLOT(addToolBar(QString)));
	connect(m_windowsManager, SIGNAL(requestedAddBookmark(QUrl,QString,QString)), this, SLOT(addBookmark(QUrl,QString,QString)));
	connect(m_windowsManager, SIGNAL(requestedNewWindow(bool,bool,QUrl)), this, SIGNAL(requestedNewWindow(bool,bool,QUrl)));
	connect(m_windowsManager, SIGNAL(windowTitleChanged(QString)), this, SLOT(updateWindowTitle(QString)));
	connect(m_ui->consoleDockWidget, SIGNAL(visibilityChanged(bool)), m_actionsManager->getAction(Action::ShowErrorConsoleAction), SLOT(setChecked(bool)));

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

	if (!session.state.isEmpty())
	{
		restoreState(session.state);
	}

	const QList<ToolBarWidget*> toolBars = findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly);

	for (int i = 0; i < toolBars.count(); ++i)
	{
		toolBars.at(i)->notifyAreaChanged();
	}
}

MainWindow::~MainWindow()
{
	delete m_ui;
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
		if (m_windowsManager->getWindowCount() > 1)
		{
			if (!m_tabSwitcher)
			{
				m_tabSwitcher = new TabSwitcherWidget(m_windowsManager, this);
			}

			m_tabSwitcher->raise();
			m_tabSwitcher->resize(size());
			m_tabSwitcher->show(false);
			m_tabSwitcher->selectTab(event->key() == Qt::Key_Tab);
		}
		else
		{
			m_windowsManager->triggerAction(((event->key() == Qt::Key_Tab) ? Action::ActivateTabOnRightAction : Action::ActivateTabOnLeftAction), parentWidget());
		}

		event->accept();
	}
	else
	{
		QMainWindow::keyPressEvent(event);
	}
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
	if (m_tabSwitcher && m_tabSwitcher->isVisible())
	{
		return;
	}

	QMenu menu(this);
	menu.addAction(m_actionsManager->getAction(Action::ShowMenuBarAction));
	menu.addAction(m_actionsManager->getAction(Action::ShowTabBarAction));
	menu.addAction(m_actionsManager->getAction(Action::ShowSidebarAction));
	menu.addAction(m_actionsManager->getAction(Action::ShowErrorConsoleAction));
	menu.addSeparator();
	menu.addAction(m_actionsManager->getAction(Action::LockToolBarsAction));
	menu.exec(event->globalPos());
}

void MainWindow::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Network/WorkOffline"))
	{
		m_actionsManager->getAction(Action::WorkOfflineAction)->setChecked(value.toBool());
	}
	else if (option == QLatin1String("Interface/LockToolBars"))
	{
		const QList<ToolBarWidget*> toolBars = findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly);
		const bool areToolBarsMovable = !value.toBool();

		for (int i = 0; i < toolBars.count(); ++i)
		{
			toolBars.at(i)->setMovable(areToolBarsMovable);
		}

		m_actionsManager->getAction(Action::LockToolBarsAction)->setChecked(value.toBool());
	}
	else if (option == QLatin1String("Interface/ShowMenuBar"))
	{
		m_actionsManager->getAction(Action::ShowMenuBarAction)->setChecked(value.toBool());
	}
	else if (option == QLatin1String("Sidebar/CurrentPanel"))
	{
		updateSidebars();
	}
	else if (option == QLatin1String("Sidebar/Reverse"))
	{
		placeSidebars();
	}
	else if (option == QLatin1String("Sidebar/ShowToggleEdge"))
	{
		if (!m_toggleEdge)
		{
			createToggleEdge();
		}

		placeSidebars();

		m_toggleEdge->setVisible(value.toBool());
	}
	else if (option == QLatin1String("Sidebar/Visible"))
	{
		m_actionsManager->getAction(Action::ShowSidebarAction)->setChecked(value.toBool());

		placeSidebars();
	}
}

void MainWindow::createSidebar()
{
	if (m_sidebarWidget)
	{
		return;
	}

	m_sidebarWidget = new SidebarWidget(this);
}

void MainWindow::createToggleEdge()
{
	if (m_toggleEdge)
	{
		return;
	}

	m_toggleEdge = new ActionWidget(Action::ShowSidebarAction, NULL, this);
	m_toggleEdge->setFixedWidth(6);
	m_toggleEdge->setText(QString());
}

void MainWindow::openUrl(const QString &text)
{
	if (text.isEmpty())
	{
		m_windowsManager->triggerAction(Action::NewTabAction);

		return;
	}
	else
	{
		InputInterpreter *interpreter = new InputInterpreter(this);

		connect(interpreter, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)), m_windowsManager, SLOT(open(BookmarksItem*,OpenHints)));
		connect(interpreter, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), m_windowsManager, SLOT(open(QUrl,OpenHints)));
		connect(interpreter, SIGNAL(requestedSearch(QString,QString,OpenHints)), m_windowsManager, SLOT(search(QString,QString,OpenHints)));

		interpreter->interpret(text, ((!m_mdiWidget->getActiveWindow() || Utils::isUrlEmpty(m_mdiWidget->getActiveWindow()->getUrl())) ? CurrentTabOpen : NewTabOpen));
	}
}

void MainWindow::splitterMoved()
{
	if (!SettingsManager::getValue(QString("Sidebar/CurrentPanel")).toString().isEmpty() && m_sidebarWidget)
	{
		SettingsManager::setValue(QString("Sidebar/Width"), m_splitter->sizes().at(m_splitter->indexOf(m_sidebarWidget)));
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

void MainWindow::triggerAction(int identifier, bool checked)
{
	switch (identifier)
	{
		case Action::NewWindowAction:
			emit requestedNewWindow(false, false, QUrl());

			break;
		case Action::NewWindowPrivateAction:
			emit requestedNewWindow(true, false, QUrl());

			break;
		case Action::OpenAction:
			{
				const QUrl url = QFileDialog::getOpenFileUrl(this, tr("Open File"));

				if (url.isValid())
				{
					m_windowsManager->open(url);
				}
			}

			break;
		case Action::ClosePrivateTabsPanicAction:
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
						windows[i]->getWindowsManager()->triggerAction(Action::ClosePrivateTabsAction);
					}
				}
			}

			break;
		case Action::CloseWindowAction:
			close();

			break;
		case Action::SessionsAction:
			{
				SessionsManagerDialog dialog(this);
				dialog.exec();
			}

			break;
		case Action::SaveSessionAction:
			{
				SaveSessionDialog dialog(this);
				dialog.exec();
			}

			break;
		case Action::GoToPageAction:
			{
				OpenAddressDialog dialog(this);

				connect(&dialog, SIGNAL(requestedLoadUrl(QUrl,OpenHints)), m_windowsManager, SLOT(open(QUrl,OpenHints)));
				connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)), m_windowsManager, SLOT(open(BookmarksItem*,OpenHints)));
				connect(&dialog, SIGNAL(requestedSearch(QString,QString,OpenHints)), m_windowsManager, SLOT(search(QString,QString,OpenHints)));

				dialog.exec();
			}

			break;
		case Action::GoToHomePageAction:
			{
				const QString homePage = SettingsManager::getValue(QLatin1String("Browser/HomePage")).toString();

				if (!homePage.isEmpty())
				{
					m_windowsManager->open(QUrl(homePage), CurrentTabOpen);
				}
			}

			break;
		case Action::BookmarksAction:
			{
				const QUrl url(QLatin1String("about:bookmarks"));

				if (!SessionsManager::hasUrl(url, true))
				{
					m_windowsManager->open(url);
				}
			}

			break;
		case Action::AddBookmarkAction:
			addBookmark(QUrl(), QString(), QString(), true);

			break;
		case Action::QuickBookmarkAccessAction:
			{
				OpenBookmarkDialog dialog(this);

				connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*)), m_windowsManager, SLOT(open(BookmarksItem*)));

				dialog.exec();
			}

			break;
		case Action::WorkOfflineAction:
			SettingsManager::setValue(QLatin1String("Network/WorkOffline"), checked);

			break;
		case Action::FullScreenAction:
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
		case Action::ShowTabSwitcherAction:
			if (!m_tabSwitcher)
			{
				m_tabSwitcher = new TabSwitcherWidget(m_windowsManager, this);
			}

			m_tabSwitcher->raise();
			m_tabSwitcher->resize(size());
			m_tabSwitcher->show(true);

			break;
		case Action::ShowMenuBarAction:
			if (checked && !m_menuBar)
			{
				m_menuBar = new MenuBarWidget(this);

				setMenuBar(m_menuBar);
			}
			else if (!checked && m_menuBar)
			{
				m_menuBar->deleteLater();
				m_menuBar = NULL;

				setMenuBar(NULL);
			}

			SettingsManager::setValue(QLatin1String("Interface/ShowMenuBar"), checked);

			break;
		case Action::ShowTabBarAction:
			m_tabBarToolBar->setVisible(checked);

			break;
		case Action::ShowSidebarAction:
			createSidebar();

			m_sidebarWidget->setVisible(checked);

			SettingsManager::setValue(QLatin1String("Sidebar/Visible"), checked);

			break;
		case Action::OpenPanelAction:
			createSidebar();

			m_sidebarWidget->openPanel();

			break;
		case Action::ClosePanelAction:
			SettingsManager::setValue(QLatin1String("Sidebar/CurrentPanel"), QString());

			break;
		case Action::ShowErrorConsoleAction:
			m_ui->consoleDockWidget->setVisible(checked);

			break;
		case Action::LockToolBarsAction:
			SettingsManager::setValue(QLatin1String("Interface/LockToolBars"), checked);

			break;
		case Action::ContentBlockingAction:
			{
				ContentBlockingDialog dialog(this);
				dialog.exec();
			}

			break;
		case Action::HistoryAction:
			{
				const QUrl url(QLatin1String("about:history"));

				if (!SessionsManager::hasUrl(url, true))
				{
					m_windowsManager->open(url);
				}
			}

			break;
		case Action::ClearHistoryAction:
			{
				ClearHistoryDialog dialog(SettingsManager::getValue(QLatin1String("History/ManualClearOptions")).toStringList(), false, this);
				dialog.exec();
			}

			break;
		case Action::TransfersAction:
			{
				const QUrl url(QLatin1String("about:transfers"));

				if (!SessionsManager::hasUrl(url, true))
				{
					m_windowsManager->open(url);
				}
			}

			break;
		case Action::CookiesAction:
			{
				const QUrl url(QLatin1String("about:cookies"));

				if (!SessionsManager::hasUrl(url, true))
				{
					m_windowsManager->open(url);
				}
			}

			break;
		case Action::PreferencesAction:
			{
				PreferencesDialog dialog(QLatin1String("general"), this);
				dialog.exec();
			}

			break;
		case Action::SwitchApplicationLanguageAction:
			{
				LocaleDialog dialog(this);
				dialog.exec();
			}

			break;
		case Action::AboutApplicationAction:
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
		case Action::AboutQtAction:
			Application::getInstance()->aboutQt();

			break;
		case Action::ExitAction:
			Application::getInstance()->close();

			break;
		default:
			m_windowsManager->triggerAction(identifier, checked);

			break;
	}
}

void MainWindow::addBookmark(const QUrl &url, const QString &title, const QString &description, bool warn)
{
	const QUrl bookmarkUrl = (url.isValid() ? url.adjusted(QUrl::RemovePassword) : m_windowsManager->getUrl().adjusted(QUrl::RemovePassword));

	if (bookmarkUrl.isEmpty() || (warn && BookmarksManager::hasBookmark(bookmarkUrl) && QMessageBox::warning(this, tr("Warning"), tr("You already have this address in your bookmarks.\nDo you want to continue?"), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Cancel))
	{
		return;
	}

	BookmarksItem *bookmark = new BookmarksItem(BookmarksItem::UrlBookmark, 0, bookmarkUrl, (url.isValid() ? title : m_windowsManager->getTitle()));
	bookmark->setData(description, BookmarksModel::DescriptionRole);

	BookmarkPropertiesDialog dialog(bookmark, NULL, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		delete bookmark;
	}
}

void MainWindow::addToolBar(const QString &identifier)
{
	const ToolBarDefinition definition = ActionsManager::getToolBarDefinition(identifier);
	ToolBarWidget *toolBar = new ToolBarWidget(identifier, NULL, this);
	toolBar->setMovable(!SettingsManager::getValue(QLatin1String("Interface/LockToolBars")).toBool());

	QMainWindow::addToolBar(definition.location, toolBar);

	if (identifier == QLatin1String("TabBar"))
	{
		m_tabBarToolBar = toolBar;

		m_tabBar = toolBar->findChild<TabBarWidget*>();
	}
}

void MainWindow::transferStarted()
{
	const QString action = SettingsManager::getValue(QLatin1String("Browser/TransferStartingAction")).toString();

	if (action == QLatin1String("openTab"))
	{
		triggerAction(Action::TransfersAction);
	}
	else if (action == QLatin1String("openBackgroundTab"))
	{
		const QUrl url(QLatin1String("about:transfers"));

		if (!SessionsManager::hasUrl(url, false))
		{
			m_windowsManager->open(url, NewBackgroundTabOpen);
		}
	}
	else if (action == QLatin1String("openPanel"))
	{
		createSidebar();

		m_sidebarWidget->setVisible(true);
		m_sidebarWidget->selectPanel(QLatin1String("transfers"));
	}
}

void MainWindow::placeSidebars()
{
	if (SettingsManager::getValue("Sidebar/ShowToggleEdge").toBool())
	{
		createToggleEdge();

		m_splitter->addWidget(m_toggleEdge);
	}

	if (SettingsManager::getValue("Sidebar/Visible").toBool())
	{
		createSidebar();

		m_splitter->addWidget(m_sidebarWidget);
	}

	m_splitter->addWidget(m_mdiWidget);

	if (SettingsManager::getValue(QString("Sidebar/Reverse")).toBool())
	{
		for (int i = m_splitter->count() - 1; i >= 0; --i)
		{
			m_splitter->addWidget(m_splitter->widget(i));
		}
	}

	if (m_sidebarWidget)
	{
		m_sidebarWidget->setButtonsEdge(SettingsManager::getValue(QString("Sidebar/Reverse")).toBool() ? Qt::RightEdge : Qt::LeftEdge);
		m_sidebarWidget->setVisible(SettingsManager::getValue("Sidebar/Visible").toBool());
	}

	updateSidebars();

	connect(m_splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMoved()));
}

void MainWindow::updateSidebars()
{
	int sidebarSize = m_sidebarWidget ? m_sidebarWidget->sizeHint().width() : 0;
	int toggleEdgeSize = m_toggleEdge ? m_toggleEdge->width() : 0;
	int columns = 3;

	if (!SettingsManager::getValue(QLatin1String("Sidebar/ShowToggleEdge")).toBool())
	{
		toggleEdgeSize = 0;
		--columns;
	}

	if (!SettingsManager::getValue(QLatin1String("Sidebar/Visible")).toBool())
	{
		sidebarSize = 0;
		--columns;
	}

	int mdiSize = (m_splitter->width() - sidebarSize - toggleEdgeSize - ((columns - 1) * m_splitter->handleWidth()));

	QList<int> sizes;

	if (m_toggleEdge)
	{
		sizes.append(toggleEdgeSize);
	}

	if (m_sidebarWidget)
	{
		sizes.append(sidebarSize);
	}

	sizes.append(mdiSize);

	if (SettingsManager::getValue(QLatin1String("Sidebar/Reverse")).toBool())
	{
		QList<int> sizesCopy(sizes);

		sizes.clear();

		for (int i = sizesCopy.count() - 1; i >= 0 ; --i)
		{
			sizes.append(sizesCopy[i]);
		}
	}

	m_splitter->setSizes(sizes);
}

void MainWindow::updateWindowTitle(const QString &title)
{
	setWindowTitle(title.isEmpty() ? QStringLiteral("Otter") : QStringLiteral("%1 - Otter").arg(title));
}

MainWindow* MainWindow::findMainWindow(QObject *parent)
{
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

MdiWidget* MainWindow::getMdi()
{
	return m_mdiWidget;
}

TabBarWidget* MainWindow::getTabBar()
{
	return m_tabBar;
}

ActionsManager* MainWindow::getActionsManager()
{
	return m_actionsManager;
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
					const Qt::ToolBarArea area = toolBarArea(m_tabBarToolBar);
					const QList<ToolBarWidget*> toolBars = findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly);

					if (isFullScreen())
					{
						m_actionsManager->getAction(Action::FullScreenAction)->setIcon(Utils::getIcon(QLatin1String("view-restore")));

						if (area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea)
						{
							m_tabBarToolBar->setMaximumWidth(1);
						}
						else
						{
							m_tabBarToolBar->setMaximumHeight(1);
						}

						if (m_statusBar)
						{
							m_statusBar->hide();
						}

						if (m_menuBar)
						{
							m_menuBar->hide();
						}

						m_mdiWidget->installEventFilter(this);
						m_tabBarToolBar->installEventFilter(this);

						for (int i = 0; i < toolBars.count(); ++i)
						{
							if (toolBars.at(i) != m_tabBarToolBar)
							{
								toolBars.at(i)->hide();
							}
						}
					}
					else
					{
						m_actionsManager->getAction(Action::FullScreenAction)->setIcon(Utils::getIcon(QLatin1String("view-fullscreen")));

						if (area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea)
						{
							m_tabBarToolBar->setMaximumWidth(QWIDGETSIZE_MAX);
						}
						else
						{
							m_tabBarToolBar->setMaximumHeight(QWIDGETSIZE_MAX);
						}

						if (m_statusBar)
						{
							m_statusBar->show();
						}

						if (m_menuBar)
						{
							m_menuBar->show();
						}

						m_mdiWidget->removeEventFilter(this);
						m_tabBarToolBar->removeEventFilter(this);

						for (int i = 0; i < toolBars.count(); ++i)
						{
							if (toolBars.at(i) != m_tabBarToolBar)
							{
								toolBars.at(i)->show();
							}
						}
					}

					emit controlsHiddenChanged(windowState().testFlag(Qt::WindowFullScreen));
				}
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
	if (object == m_mdiWidget && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent && keyEvent->key() == Qt::Key_Escape)
		{
			triggerAction(Action::FullScreenAction);
		}
	}

	if (object == m_tabBarToolBar && event->type() == QEvent::Enter)
	{
		const Qt::ToolBarArea area = toolBarArea(m_tabBarToolBar);

		if (area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea)
		{
			m_tabBarToolBar->setMaximumWidth(QWIDGETSIZE_MAX);
		}
		else
		{
			m_tabBarToolBar->setMaximumHeight(QWIDGETSIZE_MAX);
		}
	}

	if (object == m_tabBarToolBar && event->type() == QEvent::Leave)
	{
		const Qt::ToolBarArea area = toolBarArea(m_tabBarToolBar);

		if (area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea)
		{
			m_tabBarToolBar->setMaximumWidth(1);
		}
		else
		{
			m_tabBarToolBar->setMaximumHeight(1);
		}
	}

	return QMainWindow::eventFilter(object, event);
}

}
