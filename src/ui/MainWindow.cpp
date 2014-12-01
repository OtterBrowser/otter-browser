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
#include "ImportDialog.h"
#include "MdiWidget.h"
#include "Menu.h"
#include "OpenAddressDialog.h"
#include "OpenBookmarkDialog.h"
#include "PreferencesDialog.h"
#include "SaveSessionDialog.h"
#include "SessionsManagerDialog.h"
#include "preferences/ContentBlockingDialog.h"
#include "../core/ActionsManager.h"
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/BookmarksModel.h"
#include "../core/HistoryManager.h"
#include "../core/NetworkManagerFactory.h"
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
#include <QtCore/QTextCodec>
#include <QtGui/QClipboard>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

namespace Otter
{

MainWindow::MainWindow(bool isPrivate, const SessionMainWindow &windows, QWidget *parent) : QMainWindow(parent),
	m_actionsManager(NULL),
	m_windowsManager(NULL),
	m_menuBar(NULL),
	m_sessionsGroup(NULL),
	m_characterEncodingGroup(NULL),
	m_userAgentGroup(NULL),
	m_closedWindowsMenu(new QMenu(this)),
	m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);

	m_actionsManager = new ActionsManager(this);

	SessionsManager::setActiveWindow(this);

	m_ui->statusBar->setup();

	m_ui->tabsDockWidget->setup(m_closedWindowsMenu);
	m_ui->tabsDockWidget->setFloating(true);
	m_ui->tabsDockWidget->adjustSize();
	m_ui->tabsDockWidget->setFloating(false);

	MdiWidget *mdiWidget = new MdiWidget(this);

	setCentralWidget(mdiWidget);

	m_windowsManager = new WindowsManager(mdiWidget, m_ui->tabsDockWidget->getTabBar(), m_ui->statusBar, (isPrivate || SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool()));

	SessionsManager::registerWindow(this);

	m_actionsManager->getAction(QLatin1String("WorkOffline"))->setChecked(SettingsManager::getValue(QLatin1String("Network/WorkOffline")).toBool());
	m_actionsManager->getAction(QLatin1String("ShowMenuBar"))->setChecked(SettingsManager::getValue(QLatin1String("Interface/ShowMenuBar")).toBool());
	m_actionsManager->getAction(QLatin1String("Exit"))->setMenuRole(QAction::QuitRole);
	m_actionsManager->getAction(QLatin1String("Preferences"))->setMenuRole(QAction::PreferencesRole);
	m_actionsManager->getAction(QLatin1String("AboutQt"))->setMenuRole(QAction::AboutQtRole);
	m_actionsManager->getAction(QLatin1String("AboutApplication"))->setMenuRole(QAction::AboutRole);

	if (SettingsManager::getValue(QLatin1String("Interface/ShowMenuBar")).toBool())
	{
		createMenuBar();
	}

	connect(BookmarksManager::getInstance(), SIGNAL(modelModified()), this, SLOT(updateBookmarks()));
	connect(SessionsManager::getInstance(), SIGNAL(closedWindowsChanged()), this, SLOT(updateClosedWindows()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(TransfersManager::getInstance(), SIGNAL(transferStarted(TransferInformation*)), this, SLOT(actionTransfers()));
	connect(m_windowsManager, SIGNAL(requestedAddBookmark(QUrl,QString)), this, SLOT(actionAddBookmark(QUrl,QString)));
	connect(m_windowsManager, SIGNAL(requestedNewWindow(bool,bool,QUrl)), this, SIGNAL(requestedNewWindow(bool,bool,QUrl)));
	connect(m_windowsManager, SIGNAL(windowTitleChanged(QString)), this, SLOT(updateWindowTitle(QString)));
	connect(m_windowsManager, SIGNAL(closedWindowsAvailableChanged(bool)), m_ui->tabsDockWidget, SLOT(setClosedWindowsMenuEnabled(bool)));
	connect(m_windowsManager, SIGNAL(actionsChanged()), this, SLOT(updateActions()));
	connect(m_closedWindowsMenu, SIGNAL(aboutToShow()), this, SLOT(menuClosedWindowsAboutToShow()));
	connect(m_ui->consoleDockWidget, SIGNAL(visibilityChanged(bool)), m_actionsManager->getAction(QLatin1String("ErrorConsole")), SLOT(setChecked(bool)));
	connect(m_ui->sidebarDockWidget, SIGNAL(visibilityChanged(bool)), m_actionsManager->getAction(QLatin1String("Sidebar")), SLOT(setChecked(bool)));
	connect(m_ui->sidebarDockWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), m_ui->sidebarWidget, SLOT(locationChanged(Qt::DockWidgetArea)));
	connect(m_actionsManager->getAction(QLatin1String("NewTab")), SIGNAL(triggered()), m_windowsManager, SLOT(open()));
	connect(m_actionsManager->getAction(QLatin1String("NewTabPrivate")), SIGNAL(triggered()), this, SLOT(actionNewTabPrivate()));
	connect(m_actionsManager->getAction(QLatin1String("NewWindow")), SIGNAL(triggered()), this, SIGNAL(requestedNewWindow()));
	connect(m_actionsManager->getAction(QLatin1String("NewWindowPrivate")), SIGNAL(triggered()), this, SLOT(actionNewWindowPrivate()));
	connect(m_actionsManager->getAction(QLatin1String("Open")), SIGNAL(triggered()), this, SLOT(actionOpen()));
	connect(m_actionsManager->getAction(QLatin1String("CloneTab")), SIGNAL(triggered()), m_windowsManager, SLOT(clone()));
	connect(m_actionsManager->getAction(QLatin1String("CloseTab")), SIGNAL(triggered()), m_windowsManager, SLOT(close()));
	connect(m_actionsManager->getAction(QLatin1String("SaveSession")), SIGNAL(triggered()), this, SLOT(actionSaveSession()));
	connect(m_actionsManager->getAction(QLatin1String("ManageSessions")), SIGNAL(triggered()), this, SLOT(actionManageSessions()));
	connect(m_actionsManager->getAction(QLatin1String("Print")), SIGNAL(triggered()), m_windowsManager, SLOT(print()));
	connect(m_actionsManager->getAction(QLatin1String("PrintPreview")), SIGNAL(triggered()), m_windowsManager, SLOT(printPreview()));
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
	connect(m_actionsManager->getAction(QLatin1String("GoToPage")), SIGNAL(triggered()), this, SLOT(actionGoToPage()));
	connect(m_actionsManager->getAction(QLatin1String("QuickBookmarkAccess")), SIGNAL(triggered()), this, SLOT(actionQuickBookmarkAccess()));

	m_windowsManager->restore(windows);

	m_ui->sidebarDockWidget->hide();
	m_ui->consoleDockWidget->hide();

	SettingsManager::setDefaultValue(QLatin1String("Window/Geometry"), QByteArray());
	SettingsManager::setDefaultValue(QLatin1String("Window/State"), QByteArray());

	updateActions();
	updateWindowTitle(m_windowsManager->getTitle());
	restoreGeometry(SettingsManager::getValue(QLatin1String("Window/Geometry")).toByteArray());
	restoreState(SettingsManager::getValue(QLatin1String("Window/State")).toByteArray());
}

MainWindow::~MainWindow()
{
	delete m_ui;
}

void MainWindow::changeEvent(QEvent *event)
{
	QMainWindow::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			if (m_actionsManager)
			{
				m_actionsManager->updateActions();
			}

			break;
		default:
			break;
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (SessionsManager::isLastWindow())
	{
		const QList<TransferInformation*> transfers = TransfersManager::getTransfers();
		int runningTransfers = 0;
		bool transfersDialog = false;

		for (int i = 0; i < transfers.count(); ++i)
		{
			if (transfers.at(i)->state == RunningTransfer)
			{
				++runningTransfers;
			}
		}

		if (runningTransfers > 0 && SettingsManager::getValue(QLatin1String("Choices/WarnQuitTransfers")).toBool())
		{
			QMessageBox messageBox;
			messageBox.setWindowTitle(tr("Question"));
			messageBox.setText(tr("You are about to quit while %n files are still being downloaded.", "", runningTransfers));
			messageBox.setInformativeText(tr("Do you want to continue?"));
			messageBox.setIcon(QMessageBox::Question);
			messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
			messageBox.setDefaultButton(QMessageBox::Cancel);
			messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

			QPushButton *hideButton = messageBox.addButton(tr("Hide"), QMessageBox::ActionRole);
			const int result = messageBox.exec();

			if (messageBox.clickedButton() == hideButton)
			{
				Application::getInstance()->setHidden(true);

				event->ignore();

				return;
			}
			else if (result == QMessageBox::Yes)
			{
				runningTransfers = 0;
				transfersDialog = true;
			}

			SettingsManager::setValue(QLatin1String("Choices/WarnQuitTransfers"), !messageBox.checkBox()->isChecked());

			if (runningTransfers > 0)
			{
				event->ignore();

				return;
			}
		}

		const QString warnQuitMode = SettingsManager::getValue(QLatin1String("Choices/WarnQuit")).toString();

		if (!transfersDialog && warnQuitMode != QLatin1String("noWarn"))
		{
			if (warnQuitMode == QLatin1String("alwaysWarn") || (m_windowsManager->getWindowCount() > 1 && warnQuitMode == QLatin1String("warnOpenTabs")))
			{
				QMessageBox messageBox;
				messageBox.setWindowTitle(tr("Question"));
				messageBox.setText(tr("You are about to quit the current Otter Browser session."));
				messageBox.setInformativeText(tr("Do you want to continue?"));
				messageBox.setIcon(QMessageBox::Question);
				messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
				messageBox.setDefaultButton(QMessageBox::Yes);
				messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

				QPushButton *hideButton = messageBox.addButton(tr("Hide"), QMessageBox::ActionRole);
				const int result = messageBox.exec();

				if (result == QMessageBox::Cancel)
				{
					event->ignore();

					return;
				}
				else if (messageBox.clickedButton() == hideButton)
				{
					Application::getInstance()->setHidden(true);

					event->ignore();

					return;
				}

				if (messageBox.checkBox()->isChecked())
				{
					SettingsManager::setValue(QLatin1String("Choices/WarnQuit"), QLatin1String("noWarn"));
				}
			}
		}

		QStringList clearSettings = SettingsManager::getValue(QLatin1String("History/ClearOnClose")).toStringList();
		clearSettings.removeAll(QString());

		if (!clearSettings.isEmpty())
		{
			if (clearSettings.contains(QLatin1String("browsing")))
			{
				HistoryManager::clearHistory();
			}

			if (clearSettings.contains(QLatin1String("cookies")))
			{
				NetworkManagerFactory::clearCookies();
			}

			if (clearSettings.contains(QLatin1String("downloads")))
			{
				TransfersManager::clearTransfers();
			}

			if (clearSettings.contains(QLatin1String("cache")))
			{
				NetworkManagerFactory::clearCache();
			}
		}
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

	SettingsManager::setValue(QLatin1String("Window/Geometry"), saveGeometry());
	SettingsManager::setValue(QLatin1String("Window/State"), saveState());

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
		m_menuBar->addMenu(new Menu(menuBar.at(i).toObject(), m_menuBar));
	}

	Menu *bookmarksMenu = getMenu(QLatin1String("MenuBookmarks"));

	if (bookmarksMenu)
	{
		bookmarksMenu->installEventFilter(this);

		connect(bookmarksMenu, SIGNAL(aboutToShow()), this, SLOT(menuBookmarksAboutToShow()));
	}

	Menu *closedWindowsMenu = getMenu(QLatin1String("MenuClosedWindows"));

	if (closedWindowsMenu)
	{
		closedWindowsMenu->setIcon(Utils::getIcon(QLatin1String("user-trash")));
		closedWindowsMenu->setEnabled(false);

		connect(m_windowsManager, SIGNAL(closedWindowsAvailableChanged(bool)), closedWindowsMenu, SLOT(setEnabled(bool)));
		connect(closedWindowsMenu, SIGNAL(aboutToShow()), this, SLOT(menuClosedWindowsAboutToShow()));
	}

	Menu *sessionsMenu = getMenu(QLatin1String("MenuSessions"));

	if (sessionsMenu)
	{
		connect(sessionsMenu, SIGNAL(aboutToShow()), this, SLOT(menuSessionsAboutToShow()));
		connect(sessionsMenu, SIGNAL(triggered(QAction*)), this, SLOT(actionSession(QAction*)));
	}

	Menu *importExportMenu = getMenu(QLatin1String("MenuImportExport"));

	if (importExportMenu)
	{
		importExportMenu->addAction(tr("Import Opera Bookmarks"))->setData(QLatin1String("OperaBookmarks"));
		importExportMenu->addAction(tr("Import HTML Bookmarks"))->setData(QLatin1String("HtmlBookmarks"));

		connect(importExportMenu, SIGNAL(triggered(QAction*)), this, SLOT(actionImport(QAction*)));
	}

	Menu *userAgentMenu = getMenu(QLatin1String("MenuUserAgent"));

	if (userAgentMenu)
	{
		connect(userAgentMenu, SIGNAL(aboutToShow()), this, SLOT(menuUserAgentAboutToShow()));
		connect(userAgentMenu, SIGNAL(triggered(QAction*)), this, SLOT(actionUserAgent(QAction*)));
	}

	Menu *characterEncodingMenu = getMenu(QLatin1String("MenuCharacterEncoding"));

	if (characterEncodingMenu)
	{
		connect(characterEncodingMenu, SIGNAL(aboutToShow()), this, SLOT(menuCharacterEncodingAboutToShow()));
		connect(characterEncodingMenu, SIGNAL(triggered(QAction*)), this, SLOT(actionCharacterEncoding(QAction*)));
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
	else if (option == QLatin1String("Interface/ShowMenuBar"))
	{
		m_actionsManager->getAction(QLatin1String("ShowMenuBar"))->setChecked(value.toBool());
	}
}

void MainWindow::actionNewTabPrivate()
{
	m_windowsManager->open(QUrl(), PrivateOpen);
}

void MainWindow::actionNewWindowPrivate()
{
	emit requestedNewWindow(true);
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

void MainWindow::actionSession(QAction *action)
{
	if (!action->data().isNull())
	{
		SessionsManager::restoreSession(SessionsManager::getSession(action->data().toString()));
	}
}

void MainWindow::actionImport(QAction *action)
{
	if (action)
	{
		ImportDialog::createDialog(action->data().toString(), this);
	}
}

void MainWindow::actionWorkOffline(bool enabled)
{
	SettingsManager::setValue(QLatin1String("Network/WorkOffline"), enabled);
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

void MainWindow::actionUserAgent(QAction *action)
{
	if (action)
	{
		m_windowsManager->setUserAgent(action->data().toString());
	}
}

void MainWindow::actionCharacterEncoding(QAction *action)
{
	QString encoding;

	if (action && action->data().toInt() > 0)
	{
		QTextCodec *codec = QTextCodec::codecForMib(action->data().toInt());

		if (codec)
		{
			encoding = codec->name();
		}
	}

	m_windowsManager->setOption(QLatin1String("Content/DefaultCharacterEncoding"), encoding.toLower());
}

void MainWindow::actionClearClosedWindows()
{
	m_windowsManager->clearClosedWindows();

	SessionsManager::clearClosedWindows();
}

void MainWindow::actionRestoreClosedWindow()
{
	QAction *action = qobject_cast<QAction*>(sender());
	const int index = ((action && action->data().type() == QVariant::Int) ? action->data().toInt() : 0);

	if (index > 0)
	{
		m_windowsManager->restore(index - 1);
	}
	else if (index < 0)
	{
		SessionsManager::restoreClosedWindow(-index - 1);
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

void MainWindow::actionOpenBookmark()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action && !action->data().toString().isEmpty())
	{
		m_windowsManager->open(QUrl(action->data().toString()), (SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool() ? CurrentTabOpen : DefaultOpen));
	}
}

void MainWindow::actionOpenBookmarkFolder()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		m_windowsManager->open(dynamic_cast<BookmarksItem*>(BookmarksManager::getModel()->itemFromIndex(action->data().toModelIndex())));
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

void MainWindow::actionGoToPage()
{
	OpenAddressDialog dialog(this);

	connect(&dialog, SIGNAL(requestedLoadUrl(QUrl,OpenHints)), m_windowsManager, SLOT(open(QUrl,OpenHints)));
	connect(&dialog, SIGNAL(requestedSearch(QString,QString,OpenHints)), m_windowsManager, SLOT(search(QString,QString,OpenHints)));

	dialog.exec();
}

void MainWindow::actionQuickBookmarkAccess()
{
	OpenBookmarkDialog dialog(this);

	connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*)), m_windowsManager, SLOT(open(BookmarksItem*)));

	dialog.exec();
}

void MainWindow::menuSessionsAboutToShow()
{
	Menu *sessionsMenu = getMenu(QLatin1String("MenuSessions"));

	if (!sessionsMenu)
	{
		return;
	}

	if (m_sessionsGroup)
	{
		m_sessionsGroup->deleteLater();
	}

	sessionsMenu->clear();
	sessionsMenu->addAction(m_actionsManager->getAction(QLatin1String("SaveSession")));
	sessionsMenu->addAction(m_actionsManager->getAction(QLatin1String("ManageSessions")));
	sessionsMenu->addSeparator();

	m_sessionsGroup = new QActionGroup(this);
	m_sessionsGroup->setExclusive(true);

	const QStringList sessions = SessionsManager::getSessions();
	QMultiHash<QString, SessionInformation> information;

	for (int i = 0; i < sessions.count(); ++i)
	{
		const SessionInformation session = SessionsManager::getSession(sessions.at(i));

		information.insert((session.title.isEmpty() ? tr("(Untitled)") : session.title), session);
	}

	const QList<SessionInformation> sorted = information.values();
	const QString currentSession = SessionsManager::getCurrentSession();

	for (int i = 0; i < sorted.count(); ++i)
	{
		int windows = 0;

		for (int j = 0; j < sorted.at(i).windows.count(); ++j)
		{
			windows += sorted.at(i).windows.at(j).windows.count();
		}

		QAction *action = sessionsMenu->addAction(tr("%1 (%n tab(s))", "", windows).arg(sorted.at(i).title.isEmpty() ? tr("(Untitled)") : QString(sorted.at(i).title).replace(QLatin1Char('&'), QLatin1String("&&"))));
		action->setData(sorted.at(i).path);
		action->setCheckable(true);
		action->setChecked(sorted.at(i).path == currentSession);

		m_sessionsGroup->addAction(action);
	}
}

void MainWindow::menuUserAgentAboutToShow()
{
	Menu *userAgentMenu = getMenu(QLatin1String("MenuUserAgent"));

	if (!userAgentMenu)
	{
		return;
	}

	if (m_userAgentGroup)
	{
		m_userAgentGroup->deleteLater();

		userAgentMenu->clear();
	}

	const QStringList userAgents = NetworkManagerFactory::getUserAgents();
	const QString userAgent = m_windowsManager->getUserAgent().first.toLower();

	m_userAgentGroup = new QActionGroup(this);
	m_userAgentGroup->setExclusive(true);

	QAction *defaultAction = userAgentMenu->addAction(tr("Default"));
	defaultAction->setData(QLatin1String("default"));
	defaultAction->setCheckable(true);
	defaultAction->setChecked(userAgent == QLatin1String("default"));

	m_userAgentGroup->addAction(defaultAction);

	userAgentMenu->addSeparator();

	for (int i = 0; i < userAgents.count(); ++i)
	{
		const QString title = NetworkManagerFactory::getUserAgent(userAgents.at(i)).title;
		QAction *userAgentAction = userAgentMenu->addAction((title.isEmpty() ? tr("(Untitled)") : Utils::elideText(title, userAgentMenu)));
		userAgentAction->setData(userAgents.at(i));
		userAgentAction->setCheckable(true);
		userAgentAction->setChecked(userAgent == userAgents.at(i));

		m_userAgentGroup->addAction(userAgentAction);
	}

	userAgentMenu->addSeparator();

	QAction *customAction = userAgentMenu->addAction(tr("Custom"));
	customAction->setData(QLatin1String("custom"));
	customAction->setCheckable(true);
	customAction->setChecked(userAgent == QLatin1String("custom"));

	m_userAgentGroup->addAction(customAction);

	if (!m_userAgentGroup->checkedAction() && userAgentMenu->actions().count() > 2)
	{
		userAgentMenu->actions().first()->setChecked(true);
	}
}

void MainWindow::menuCharacterEncodingAboutToShow()
{
	Menu *characterEncodignMenu = getMenu(QLatin1String("MenuCharacterEncoding"));

	if (!characterEncodignMenu)
	{
		return;
	}

	if (!m_characterEncodingGroup)
	{
		QList<int> textCodecs;
		textCodecs << 106 << 1015 << 1017 << 4 << 5 << 6 << 7 << 8 << 82 << 10 << 85 << 12 << 13 << 109 << 110 << 112 << 2250 << 2251 << 2252 << 2253 << 2254 << 2255 << 2256 << 2257 << 2258 << 18 << 39 << 17 << 38 << 2026;

		m_characterEncodingGroup = new QActionGroup(this);
		m_characterEncodingGroup->setExclusive(true);

		QAction *defaultAction = characterEncodignMenu->addAction(tr("Auto Detect"));
		defaultAction->setData(-1);
		defaultAction->setCheckable(true);

		m_characterEncodingGroup->addAction(defaultAction);

		characterEncodignMenu->addSeparator();

		for (int i = 0; i < textCodecs.count(); ++i)
		{
			QTextCodec *codec = QTextCodec::codecForMib(textCodecs.at(i));

			if (!codec)
			{
				continue;
			}

			QAction *textCodecAction = characterEncodignMenu->addAction(Utils::elideText(codec->name(), characterEncodignMenu));
			textCodecAction->setData(textCodecs.at(i));
			textCodecAction->setCheckable(true);

			m_characterEncodingGroup->addAction(textCodecAction);
		}
	}

	const QString encoding = m_windowsManager->getOption(QLatin1String("Content/DefaultCharacterEncoding")).toString().toLower();

	for (int i = 2; i < characterEncodignMenu->actions().count(); ++i)
	{
		QAction *action = characterEncodignMenu->actions().at(i);

		if (!action)
		{
			continue;
		}

		action->setChecked(encoding == action->text().toLower());

		if (action->isChecked())
		{
			break;
		}
	}

	if (!m_characterEncodingGroup->checkedAction())
	{
		characterEncodignMenu->actions().first()->setChecked(true);
	}
}

void MainWindow::menuClosedWindowsAboutToShow()
{
	Menu *closedWindowsMenu = getMenu(QLatin1String("MenuClosedWindows"));

	if (!closedWindowsMenu)
	{
		return;
	}

	m_closedWindowsMenu->clear();

	closedWindowsMenu->clear();
	closedWindowsMenu->addAction(Utils::getIcon(QLatin1String("edit-clear")), tr("Clear"), this, SLOT(actionClearClosedWindows()))->setData(0);
	closedWindowsMenu->addSeparator();

	const QStringList windows = SessionsManager::getClosedWindows();

	if (!windows.isEmpty())
	{
		for (int i = 0; i < windows.count(); ++i)
		{
			closedWindowsMenu->addAction(Utils::elideText(tr("Window - %1").arg(windows.at(i)), closedWindowsMenu), this, SLOT(actionRestoreClosedWindow()))->setData(-(i + 1));
		}

		closedWindowsMenu->addSeparator();
	}

	WebBackend *backend = WebBackendsManager::getBackend();
	const QList<SessionWindow> tabs = m_windowsManager->getClosedWindows();

	for (int i = 0; i < tabs.count(); ++i)
	{
		closedWindowsMenu->addAction(backend->getIconForUrl(QUrl(tabs.at(i).getUrl())), Utils::elideText(tabs.at(i).getTitle(), closedWindowsMenu), this, SLOT(actionRestoreClosedWindow()))->setData(i + 1);
	}

	m_closedWindowsMenu->addActions(closedWindowsMenu->actions());
}

void MainWindow::menuBookmarksAboutToShow()
{
	QMenu *menu = qobject_cast<QMenu*>(sender());

	if (!menu || !menu->menuAction())
	{
		return;
	}

	if (menu->objectName().isEmpty())
	{
		menu->setObjectName(QLatin1String("bookmarks"));
		menu->installEventFilter(this);
	}

	const QModelIndex index = menu->menuAction()->data().toModelIndex();

	if ((index.isValid() && !menu->actions().isEmpty()) || (!index.isValid() && menu->actions().count() != 3))
	{
		return;
	}

	QStandardItem *branch = (index.isValid() ? BookmarksManager::getModel()->itemFromIndex(index) : BookmarksManager::getModel()->getRootItem());

	if (!branch)
	{
		return;
	}

	if (index.isValid() && branch->rowCount() > 1)
	{
		QAction *openAllAction = menu->addAction(Utils::getIcon(QLatin1String("document-open-folder")), tr("Open All"));
		openAllAction->setData(index);

		connect(openAllAction, SIGNAL(triggered()), this, SLOT(actionOpenBookmarkFolder()));

		menu->addSeparator();
	}

	for (int i = 0; i < branch->rowCount(); ++i)
	{
		QStandardItem *item = branch->child(i);

		if (!item)
		{
			continue;
		}

		const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(item->data(BookmarksModel::TypeRole).toInt());

		if (type == BookmarksItem::RootBookmark || type == BookmarksItem::FolderBookmark || type == BookmarksItem::UrlBookmark)
		{
			QAction *action = menu->addAction(item->data(Qt::DecorationRole).value<QIcon>(), (item->data(BookmarksModel::TitleRole).toString().isEmpty() ? tr("(Untitled)") : Utils::elideText(QString(item->data(BookmarksModel::TitleRole).toString()).replace(QLatin1Char('&'), QLatin1String("&&")), menu)));
			action->setToolTip(item->data(BookmarksModel::DescriptionRole).toString());

			if (type == BookmarksItem::UrlBookmark)
			{
				action->setData(item->data(BookmarksModel::UrlRole).toString());

				connect(action, SIGNAL(triggered()), this, SLOT(actionOpenBookmark()));
			}
			else
			{
				action->setData(item->index());

				if (item->rowCount() > 0)
				{
					action->setMenu(new QMenu());

					connect(action->menu(), SIGNAL(aboutToShow()), this, SLOT(menuBookmarksAboutToShow()));
				}
			}
		}
		else
		{
			menu->addSeparator();
		}
	}
}

void MainWindow::openBookmark()
{
	const QUrl url(m_currentBookmark);

	if (url.isValid())
	{
		QAction *action = qobject_cast<QAction*>(sender());

		m_windowsManager->open(url, (action ? static_cast<OpenHints>(action->data().toInt()) : DefaultOpen) | (SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool() ? CurrentTabOpen : DefaultOpen));

		Menu *bookmarksMenu = getMenu(QLatin1String("MenuBookmarks"));

		if (bookmarksMenu)
		{
			bookmarksMenu->close();
		}
	}

	m_currentBookmark = QString();
}

void MainWindow::updateClosedWindows()
{
	m_ui->tabsDockWidget->setClosedWindowsMenuEnabled(m_windowsManager->getClosedWindows().count() > 0 || SessionsManager::getClosedWindows().count() > 0);
}

void MainWindow::updateBookmarks()
{
	Menu *bookmarksMenu = getMenu(QLatin1String("MenuBookmarks"));

	if (!bookmarksMenu || bookmarksMenu->actions().count() == 3)
	{
		return;
	}

	for (int i = (bookmarksMenu->actions().count() - 1); i > 2; --i)
	{
		bookmarksMenu->actions().at(i)->deleteLater();
		bookmarksMenu->removeAction(bookmarksMenu->actions().at(i));
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

Menu* MainWindow::getMenu(const QString &identifier)
{
	if (!m_menuBar)
	{
		return NULL;
	}

	return m_menuBar->findChild<Menu*>(identifier);
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
	if (event->type() == QEvent::WindowStateChange)
	{
		const bool result = QMainWindow::event(event);

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

		return result;
	}
	else if (event->type() == QEvent::WindowActivate)
	{
		SessionsManager::setActiveWindow(this);
	}
	else if (event->type() == QEvent::LanguageChange)
	{
		updateWindowTitle(m_windowsManager->getTitle());
	}

	return QMainWindow::event(event);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::ContextMenu && object->objectName().contains(QLatin1String("bookmarks"), Qt::CaseInsensitive))
	{
		QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent*>(event);
		QMenu *menu = qobject_cast<QMenu*>(object);

		if (contextMenuEvent && menu)
		{
			QAction *action = menu->actionAt(contextMenuEvent->pos());

			if (action && action->data().type() == QVariant::String)
			{
				m_currentBookmark = action->data().toString();

				QMenu contextMenu(this);
				contextMenu.addAction(Utils::getIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openBookmark()));
				contextMenu.addAction(tr("Open in New Tab"), this, SLOT(openBookmark()))->setData(NewTabOpen);
				contextMenu.addAction(tr("Open in New Background Tab"), this, SLOT(openBookmark()))->setData(NewTabBackgroundOpen);
				contextMenu.addSeparator();
				contextMenu.addAction(tr("Open in New Window"), this, SLOT(openBookmark()))->setData(NewWindowOpen);
				contextMenu.addAction(tr("Open in New Background Window"), this, SLOT(openBookmark()))->setData(NewWindowBackgroundOpen);
				contextMenu.exec(contextMenuEvent->globalPos());

				return true;
			}
		}
	}

	if (event->type() == QEvent::MouseButtonRelease && object->objectName().contains(QLatin1String("bookmarks"), Qt::CaseInsensitive))
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
		QMenu *menu = qobject_cast<QMenu*>(object);

		if (mouseEvent && (mouseEvent->button() == Qt::LeftButton || mouseEvent->button() == Qt::MiddleButton) && menu)
		{
			QAction *action = menu->actionAt(mouseEvent->pos());

			if (action && action->data().type() == QVariant::String)
			{
				OpenHints hints = DefaultOpen;

				if (mouseEvent->button() == Qt::MiddleButton || mouseEvent->modifiers() & Qt::ControlModifier)
				{
					hints = NewTabBackgroundOpen;
				}
				else if (mouseEvent->modifiers() & Qt::ShiftModifier)
				{
					hints = NewTabOpen;
				}
				else
				{
					hints = (SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool() ? CurrentTabOpen : DefaultOpen);
				}

				m_windowsManager->open(QUrl(action->data().toString()), hints);

				Menu *bookmarksMenu = getMenu(QLatin1String("MenuBookmarks"));

				if (bookmarksMenu)
				{
					bookmarksMenu->close();
				}

				return true;
			}
		}
	}

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
