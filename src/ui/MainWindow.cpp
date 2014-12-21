/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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
#include "Menu.h"
#include "OpenAddressDialog.h"
#include "OpenBookmarkDialog.h"
#include "PreferencesDialog.h"
#include "SaveSessionDialog.h"
#include "SessionsManagerDialog.h"
#include "TabBarToolBarWidget.h"
#include "preferences/ContentBlockingDialog.h"
#include "../core/ActionsManager.h"
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/BookmarksModel.h"
#include "../core/HistoryManager.h"
#include "../core/SearchesManager.h"
#include "../core/SettingsManager.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"
#include "../core/WebBackend.h"
#include "../core/WebBackendsManager.h"

#include "ui_MainWindow.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtGui/QClipboard>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

namespace Otter
{

MainWindow::MainWindow(bool isPrivate, const SessionMainWindow &session, QWidget *parent) : QMainWindow(parent),
	m_actionsManager(NULL),
	m_windowsManager(NULL),
	m_tabBarToolBarWidget(NULL),
	m_menuBar(NULL),
	m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);

	m_actionsManager = new ActionsManager(this);

	SessionsManager::setActiveWindow(this);

	m_ui->statusBar->setup();

	MdiWidget *mdiWidget = new MdiWidget(this);

	setCentralWidget(mdiWidget);

	m_windowsManager = new WindowsManager(mdiWidget, m_ui->statusBar, (isPrivate || SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool()));

	m_tabBarToolBarWidget = new TabBarToolBarWidget(this);
	m_tabBarToolBarWidget->setMovable(!SettingsManager::getValue(QLatin1String("Interface/LockToolBars")).toBool());

	addToolBar(m_tabBarToolBarWidget);

	m_windowsManager->setTabBar(m_tabBarToolBarWidget->getTabBar());

	SessionsManager::registerWindow(this);

	m_actionsManager->getAction(QLatin1String("WorkOffline"))->setChecked(SettingsManager::getValue(QLatin1String("Network/WorkOffline")).toBool());
	m_actionsManager->getAction(QLatin1String("ShowMenuBar"))->setChecked(SettingsManager::getValue(QLatin1String("Interface/ShowMenuBar")).toBool());
	m_actionsManager->getAction(QLatin1String("LockToolBars"))->setChecked(SettingsManager::getValue(QLatin1String("Interface/LockToolBars")).toBool());
	m_actionsManager->getAction(QLatin1String("Exit"))->setMenuRole(QAction::QuitRole);
	m_actionsManager->getAction(QLatin1String("Preferences"))->setMenuRole(QAction::PreferencesRole);
	m_actionsManager->getAction(QLatin1String("AboutQt"))->setMenuRole(QAction::AboutQtRole);
	m_actionsManager->getAction(QLatin1String("AboutApplication"))->setMenuRole(QAction::AboutRole);

	if (SettingsManager::getValue(QLatin1String("Interface/ShowMenuBar")).toBool())
	{
		createMenuBar();
	}

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(TransfersManager::getInstance(), SIGNAL(transferStarted(TransferInformation*)), this, SLOT(transferStarted()));
	connect(m_windowsManager, SIGNAL(requestedAddBookmark(QUrl,QString)), this, SLOT(actionAddBookmark(QUrl,QString)));
	connect(m_windowsManager, SIGNAL(requestedNewWindow(bool,bool,QUrl)), this, SIGNAL(requestedNewWindow(bool,bool,QUrl)));
	connect(m_windowsManager, SIGNAL(windowTitleChanged(QString)), this, SLOT(updateWindowTitle(QString)));
	connect(m_windowsManager, SIGNAL(actionsChanged()), this, SLOT(updateActions()));
	connect(m_ui->consoleDockWidget, SIGNAL(visibilityChanged(bool)), m_actionsManager->getAction(QLatin1String("ErrorConsole")), SLOT(setChecked(bool)));
	connect(m_ui->sidebarDockWidget, SIGNAL(visibilityChanged(bool)), m_actionsManager->getAction(QLatin1String("Sidebar")), SLOT(setChecked(bool)));
	connect(m_ui->sidebarDockWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), m_ui->sidebarWidget, SLOT(locationChanged(Qt::DockWidgetArea)));
	connect(m_actionsManager->getAction(QLatin1String("Open")), SIGNAL(triggered()), this, SLOT(actionOpen()));
	connect(m_actionsManager->getAction(QLatin1String("SaveSession")), SIGNAL(triggered()), this, SLOT(actionSaveSession()));
	connect(m_actionsManager->getAction(QLatin1String("ManageSessions")), SIGNAL(triggered()), this, SLOT(actionManageSessions()));
	connect(m_actionsManager->getAction(QLatin1String("WorkOffline")), SIGNAL(toggled(bool)), this, SLOT(actionWorkOffline(bool)));
	connect(m_actionsManager->getAction(QLatin1String("ShowMenuBar")), SIGNAL(toggled(bool)), this, SLOT(actionShowMenuBar(bool)));
	connect(m_actionsManager->getAction(QLatin1String("Exit")), SIGNAL(triggered()), Application::getInstance(), SLOT(close()));
	connect(m_actionsManager->getAction(QLatin1String("FullScreen")), SIGNAL(triggered()), this, SLOT(actionFullScreen()));
	connect(m_actionsManager->getAction(QLatin1String("ViewHistory")), SIGNAL(triggered()), this, SLOT(actionViewHistory()));
	connect(m_actionsManager->getAction(QLatin1String("ClearHistory")), SIGNAL(triggered()), this, SLOT(actionClearHistory()));
	connect(m_actionsManager->getAction(QLatin1String("AddBookmark")), SIGNAL(triggered()), this, SLOT(actionAddBookmark()));
	connect(m_actionsManager->getAction(QLatin1String("ManageBookmarks")), SIGNAL(triggered()), this, SLOT(actionManageBookmarks()));
	connect(m_actionsManager->getAction(QLatin1String("Cookies")), SIGNAL(triggered()), this, SLOT(actionCookies()));
	connect(m_actionsManager->getAction(QLatin1String("Transfers")), SIGNAL(triggered()), this, SLOT(actionTransfers()));
	connect(m_actionsManager->getAction(QLatin1String("ErrorConsole")), SIGNAL(toggled(bool)), this, SLOT(actionErrorConsole(bool)));
	connect(m_actionsManager->getAction(QLatin1String("Sidebar")), SIGNAL(toggled(bool)), this, SLOT(actionSidebar(bool)));
	connect(m_actionsManager->getAction(QLatin1String("ContentBlocking")), SIGNAL(triggered()), this, SLOT(actionContentBlocking()));
	connect(m_actionsManager->getAction(QLatin1String("Preferences")), SIGNAL(triggered()), this, SLOT(actionPreferences()));
	connect(m_actionsManager->getAction(QLatin1String("SwitchApplicationLanguage")), SIGNAL(triggered()), this, SLOT(actionSwitchApplicationLanguage()));
	connect(m_actionsManager->getAction(QLatin1String("AboutApplication")), SIGNAL(triggered()), this, SLOT(actionAboutApplication()));
	connect(m_actionsManager->getAction(QLatin1String("AboutQt")), SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));
	connect(m_actionsManager->getAction(QLatin1String("CloseWindow")), SIGNAL(triggered()), this, SLOT(close()));
	connect(m_actionsManager->getAction(QLatin1String("LockToolBars")), SIGNAL(toggled(bool)), this, SLOT(actionLockToolBars(bool)));
	connect(m_actionsManager->getAction(QLatin1String("GoToPage")), SIGNAL(triggered()), this, SLOT(actionGoToPage()));
	connect(m_actionsManager->getAction(QLatin1String("QuickBookmarkAccess")), SIGNAL(triggered()), this, SLOT(actionQuickBookmarkAccess()));

	m_windowsManager->restore(session);

	m_ui->sidebarDockWidget->hide();
	m_ui->consoleDockWidget->hide();

	updateActions();
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

	m_tabBarToolBarWidget->updateOrientation();
}

MainWindow::~MainWindow()
{
	delete m_ui;
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	menu.addAction(m_actionsManager->getAction(QLatin1String("LockToolBars")));
	menu.exec(event->globalPos());
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

void MainWindow::createMenuBar()
{
	m_menuBar = new QMenuBar(this);

	setMenuBar(m_menuBar);

	const QString menuBarPath = (SessionsManager::getProfilePath() + QLatin1String("/menuBar.json"));
	QFile menuBarFile(QFile::exists(menuBarPath) ? menuBarPath : QLatin1String(":/other/menuBar.json"));
	menuBarFile.open(QFile::ReadOnly);

	const QJsonArray menuBar = QJsonDocument::fromJson(menuBarFile.readAll()).array();

	for (int i = 0; i < menuBar.count(); ++i)
	{
		Menu *menu = new Menu(m_menuBar);
		menu->load(menuBar.at(i).toObject());

		m_menuBar->addMenu(menu);
	}
}

void MainWindow::openUrl(const QString &input)
{
	BookmarksItem *bookmark = BookmarksManager::getBookmark(input);

	if (bookmark)
	{
		m_windowsManager->open(bookmark);

		return;
	}

	if (input == QString(QLatin1Char('~')) || input.startsWith(QLatin1Char('~') + QDir::separator()))
	{
		const QStringList locations = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);

		if (!locations.isEmpty())
		{
			m_windowsManager->open(QUrl(locations.first() + input.mid(1)));

			return;
		}
	}

	if (QFileInfo(input).exists())
	{
		m_windowsManager->open(QUrl::fromLocalFile(QFileInfo(input).canonicalFilePath()));

		return;
	}

	const QUrl url = QUrl::fromUserInput(input);

	if (url.isValid() && (url.isLocalFile() || QRegularExpression(QLatin1String("^(\\w+\\:\\S+)|([\\w\\-]+\\.[a-zA-Z]{2,}(/\\S*)?$)")).match(input).hasMatch()))
	{
		m_windowsManager->open(url);

		return;
	}

	const QString keyword = input.section(QLatin1Char(' '), 0, 0);
	const QStringList engines = SearchesManager::getSearchEngines();
	SearchInformation *engine = NULL;

	for (int i = 0; i < engines.count(); ++i)
	{
		engine = SearchesManager::getSearchEngine(engines.at(i));

		if (engine && keyword == engine->keyword)
		{
			m_windowsManager->search(input.section(QLatin1Char(' '), 1), engine->identifier);

			return;
		}
	}

	m_windowsManager->open();
}

void MainWindow::storeWindowState()
{
	m_previousState = windowState();
}

void MainWindow::restoreWindowState()
{
	setWindowState(m_previousState);
}

void MainWindow::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Network/WorkOffline"))
	{
		m_actionsManager->getAction(QLatin1String("WorkOffline"))->setChecked(value.toBool());
	}
	else if (option == QLatin1String("Interface/LockToolBars"))
	{
		const QList<QToolBar*> toolBars = findChildren<QToolBar*>();
		const bool movable = !value.toBool();

		for (int i = 0; i < toolBars.count(); ++i)
		{
			toolBars.at(i)->setMovable(movable);
		}

		m_actionsManager->getAction(QLatin1String("LockToolBars"))->setChecked(value.toBool());
	}
	else if (option == QLatin1String("Interface/ShowMenuBar"))
	{
		m_actionsManager->getAction(QLatin1String("ShowMenuBar"))->setChecked(value.toBool());
	}
}

void MainWindow::actionOpen()
{
	const QString path = QFileDialog::getOpenFileName(this, tr("Open File"));

	if (!path.isEmpty())
	{
		m_windowsManager->open(QUrl(path));
	}
}

void MainWindow::actionSaveSession()
{
	SaveSessionDialog dialog(this);
	dialog.exec();
}

void MainWindow::actionManageSessions()
{
	SessionsManagerDialog dialog(this);
	dialog.exec();
}

void MainWindow::actionWorkOffline(bool enable)
{
	SettingsManager::setValue(QLatin1String("Network/WorkOffline"), enable);
}

void MainWindow::actionShowMenuBar(bool enable)
{
	if (enable && !m_menuBar)
	{
		createMenuBar();
	}
	else if (!enable && m_menuBar)
	{
		m_menuBar->deleteLater();

		setMenuBar(NULL);

		m_menuBar = NULL;
	}

	SettingsManager::setValue(QLatin1String("Interface/ShowMenuBar"), enable);
}

void MainWindow::actionFullScreen()
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
}

void MainWindow::actionViewHistory()
{
	const QUrl url(QLatin1String("about:history"));

	if (!SessionsManager::hasUrl(url, true))
	{
		m_windowsManager->open(url);
	}
}

void MainWindow::actionClearHistory()
{
	ClearHistoryDialog dialog(SettingsManager::getValue(QLatin1String("History/ManualClearOptions")).toStringList(), false, this);
	dialog.exec();
}

void MainWindow::actionAddBookmark(const QUrl &url, const QString &title)
{
	const QString bookmarkUrl = (url.isValid() ? url.toString(QUrl::RemovePassword) : m_windowsManager->getUrl().toString(QUrl::RemovePassword));

	if (bookmarkUrl.isEmpty() || (BookmarksManager::hasBookmark(bookmarkUrl) && QMessageBox::warning(this, tr("Warning"), tr("You already have this address in your bookmarks.\nDo you want to continue?"), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Cancel))
	{
		return;
	}

	BookmarksItem *bookmark = new BookmarksItem(BookmarksItem::UrlBookmark, bookmarkUrl, (url.isValid() ? title : m_windowsManager->getTitle()));
	BookmarkPropertiesDialog dialog(bookmark, NULL, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		delete bookmark;
	}
}

void MainWindow::actionManageBookmarks()
{
	const QUrl url(QLatin1String("about:bookmarks"));

	if (!SessionsManager::hasUrl(url, true))
	{
		m_windowsManager->open(url);
	}
}

void MainWindow::actionCookies()
{
	const QUrl url(QLatin1String("about:cookies"));

	if (!SessionsManager::hasUrl(url, true))
	{
		m_windowsManager->open(url);
	}
}

void MainWindow::actionTransfers()
{
	const QUrl url(QLatin1String("about:transfers"));

	if (!SessionsManager::hasUrl(url, (sender() != TransfersManager::getInstance())))
	{
		m_windowsManager->open(url);
	}
}

void MainWindow::actionErrorConsole(bool enabled)
{
	m_ui->consoleDockWidget->setVisible(enabled);
}

void MainWindow::actionSidebar(bool enabled)
{
	m_ui->sidebarDockWidget->setVisible(enabled);
}

void MainWindow::actionContentBlocking()
{
	ContentBlockingDialog dialog(this);
	dialog.exec();
}

void MainWindow::actionPreferences()
{
	PreferencesDialog dialog(QLatin1String("general"), this);
	dialog.exec();
}

void MainWindow::actionSwitchApplicationLanguage()
{
	LocaleDialog dialog(this);
	dialog.exec();
}

void MainWindow::actionAboutApplication()
{
	WebBackend *backend = WebBackendsManager::getBackend();
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

void MainWindow::actionLockToolBars(bool lock)
{
	SettingsManager::setValue(QLatin1String("Interface/LockToolBars"), lock);
}

void MainWindow::actionGoToPage()
{
	OpenAddressDialog dialog(this);

	connect(&dialog, SIGNAL(requestedLoadUrl(QUrl,OpenHints)), m_windowsManager, SLOT(open(QUrl,OpenHints)));
	connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)), m_windowsManager, SLOT(open(BookmarksItem*,OpenHints)));
	connect(&dialog, SIGNAL(requestedSearch(QString,QString,OpenHints)), m_windowsManager, SLOT(search(QString,QString,OpenHints)));

	dialog.exec();
}

void MainWindow::actionQuickBookmarkAccess()
{
	OpenBookmarkDialog dialog(this);

	connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*)), m_windowsManager, SLOT(open(BookmarksItem*)));

	dialog.exec();
}

void MainWindow::transferStarted()
{
	const QString action = SettingsManager::getValue(QLatin1String("Browser/TransferStartingAction")).toString();

	if (action == QLatin1String("openTab"))
	{
		actionTransfers();
	}
	else if (action == QLatin1String("openBackgroundTab"))
	{
		const QUrl url(QLatin1String("about:transfers"));

		if (!SessionsManager::hasUrl(url, true))
		{
			m_windowsManager->open(url, NewTabBackgroundOpen);
		}
	}
	else if (action == QLatin1String("openPanel"))
	{
		m_ui->sidebarDockWidget->setVisible(true);
		m_ui->sidebarWidget->openPanel(QLatin1String("transfers"));
	}
}

void MainWindow::updateAction(QAction *source, QAction *target)
{
	if (source)
	{
		target->setEnabled(source->isEnabled());

		if (target->isCheckable())
		{
			target->setChecked(source->isChecked());
		}
	}
	else
	{
		target->setEnabled(false);
	}
}

void MainWindow::updateActions()
{
	QAction *undoAction = m_windowsManager->getAction(UndoAction);

	if (undoAction)
	{
		m_actionsManager->getAction(QLatin1String("Undo"))->setEnabled(undoAction->isEnabled());
		m_actionsManager->getAction(QLatin1String("Undo"))->setText(undoAction->text());
	}
	else
	{
		m_actionsManager->getAction(QLatin1String("Undo"))->setEnabled(false);
		m_actionsManager->getAction(QLatin1String("Undo"))->setText(tr("Undo"));
	}

	QAction *redoAction = m_windowsManager->getAction(RedoAction);

	if (redoAction)
	{
		m_actionsManager->getAction(QLatin1String("Redo"))->setEnabled(redoAction->isEnabled());
		m_actionsManager->getAction(QLatin1String("Redo"))->setText(redoAction->text());
	}
	else
	{
		m_actionsManager->getAction(QLatin1String("Redo"))->setEnabled(false);
		m_actionsManager->getAction(QLatin1String("Redo"))->setText(tr("Redo"));
	}

	updateAction(m_windowsManager->getAction(CutAction), m_actionsManager->getAction(QLatin1String("Cut")));
	updateAction(m_windowsManager->getAction(CopyAction), m_actionsManager->getAction(QLatin1String("Copy")));
	updateAction(m_windowsManager->getAction(PasteAction), m_actionsManager->getAction(QLatin1String("Paste")));
	updateAction(m_windowsManager->getAction(DeleteAction), m_actionsManager->getAction(QLatin1String("Delete")));
	updateAction(m_windowsManager->getAction(SelectAllAction), m_actionsManager->getAction(QLatin1String("SelectAll")));
	updateAction(m_windowsManager->getAction(FindAction), m_actionsManager->getAction(QLatin1String("Find")));
	updateAction(m_windowsManager->getAction(FindNextAction), m_actionsManager->getAction(QLatin1String("FindNext")));
	updateAction(m_windowsManager->getAction(FindPreviousAction), m_actionsManager->getAction(QLatin1String("FindPrevious")));
	updateAction(m_windowsManager->getAction(ReloadAction), m_actionsManager->getAction(QLatin1String("Reload")));
	updateAction(m_windowsManager->getAction(StopAction), m_actionsManager->getAction(QLatin1String("Stop")));
	updateAction(m_windowsManager->getAction(ViewSourceAction), m_actionsManager->getAction(QLatin1String("ViewSource")));
	updateAction(m_windowsManager->getAction(InspectPageAction), m_actionsManager->getAction(QLatin1String("InspectPage")));
	updateAction(m_windowsManager->getAction(GoBackAction), m_actionsManager->getAction(QLatin1String("GoBack")));
	updateAction(m_windowsManager->getAction(RewindAction), m_actionsManager->getAction(QLatin1String("Rewind")));
	updateAction(m_windowsManager->getAction(GoForwardAction), m_actionsManager->getAction(QLatin1String("GoForward")));
	updateAction(m_windowsManager->getAction(FastForwardAction), m_actionsManager->getAction(QLatin1String("FastForward")));

	const bool canZoom = m_windowsManager->canZoom();

	m_actionsManager->getAction(QLatin1String("ZoomOut"))->setEnabled(canZoom);
	m_actionsManager->getAction(QLatin1String("ZoomIn"))->setEnabled(canZoom);
	m_actionsManager->getAction(QLatin1String("ZoomOriginal"))->setEnabled(canZoom);
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

			if (m_actionsManager)
			{
				m_actionsManager->updateActions();
			}

			break;
		case QEvent::WindowStateChange:
			SessionsManager::markSessionModified();

			if (isFullScreen())
			{
				m_actionsManager->getAction(QLatin1String("FullScreen"))->setIcon(Utils::getIcon(QLatin1String("view-restore")));
				m_ui->statusBar->hide();

				if (m_menuBar)
				{
					m_menuBar->hide();
				}

				centralWidget()->installEventFilter(this);
			}
			else
			{
				m_actionsManager->getAction(QLatin1String("FullScreen"))->setIcon(Utils::getIcon(QLatin1String("view-fullscreen")));
				m_ui->statusBar->show();

				if (m_menuBar)
				{
					m_menuBar->show();
				}

				centralWidget()->removeEventFilter(this);
			}

			break;
		case QEvent::WindowActivate:
			SessionsManager::setActiveWindow(this);
		case QEvent::Resize:
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
	if (event->type() == QEvent::KeyPress && isFullScreen())
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent->key() == Qt::Key_Escape)
		{
			actionFullScreen();
		}
	}

	return QMainWindow::eventFilter(object, event);
}

}
