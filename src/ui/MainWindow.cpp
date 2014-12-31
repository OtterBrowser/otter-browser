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
#include "ActionWidget.h"
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
#include "toolbars/ZoomWidget.h"
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

	m_mdiWidget = new MdiWidget(this);

	setCentralWidget(m_mdiWidget);

	m_windowsManager = new WindowsManager((isPrivate || SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool()), this);

	m_tabBarToolBarWidget = new TabBarToolBarWidget(this);
	m_tabBarToolBarWidget->setMovable(!SettingsManager::getValue(QLatin1String("Interface/LockToolBars")).toBool());

	addToolBar(m_tabBarToolBarWidget);

	m_ui->statusBar->addPermanentWidget(new ActionWidget(Action::ZoomOutAction, NULL, this));
	m_ui->statusBar->addPermanentWidget(new ZoomWidget(this));
	m_ui->statusBar->addPermanentWidget(new ActionWidget(Action::ZoomInAction, NULL, this));
	m_ui->statusBar->setFixedHeight(m_ui->statusBar->sizeHint().height() * 0.7);

	SessionsManager::registerWindow(this);

	m_actionsManager->getAction(Action::WorkOfflineAction)->setChecked(SettingsManager::getValue(QLatin1String("Network/WorkOffline")).toBool());
	m_actionsManager->getAction(Action::ShowMenuBarAction)->setChecked(SettingsManager::getValue(QLatin1String("Interface/ShowMenuBar")).toBool());
	m_actionsManager->getAction(Action::LockToolBarsAction)->setChecked(SettingsManager::getValue(QLatin1String("Interface/LockToolBars")).toBool());
	m_actionsManager->getAction(Action::ExitAction)->setMenuRole(QAction::QuitRole);
	m_actionsManager->getAction(Action::PreferencesAction)->setMenuRole(QAction::PreferencesRole);
	m_actionsManager->getAction(Action::AboutQtAction)->setMenuRole(QAction::AboutQtRole);
	m_actionsManager->getAction(Action::AboutApplicationAction)->setMenuRole(QAction::AboutRole);

	if (SettingsManager::getValue(QLatin1String("Interface/ShowMenuBar")).toBool())
	{
		createMenuBar();
	}

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(TransfersManager::getInstance(), SIGNAL(transferStarted(TransferInformation*)), this, SLOT(transferStarted()));
	connect(m_windowsManager, SIGNAL(requestedAddBookmark(QUrl,QString)), this, SLOT(addBookmark(QUrl,QString)));
	connect(m_windowsManager, SIGNAL(requestedNewWindow(bool,bool,QUrl)), this, SIGNAL(requestedNewWindow(bool,bool,QUrl)));
	connect(m_windowsManager, SIGNAL(windowTitleChanged(QString)), this, SLOT(updateWindowTitle(QString)));
	connect(m_ui->consoleDockWidget, SIGNAL(visibilityChanged(bool)), m_actionsManager->getAction(Action::ShowErrorConsoleAction), SLOT(setChecked(bool)));
	connect(m_ui->sidebarDockWidget, SIGNAL(visibilityChanged(bool)), m_actionsManager->getAction(Action::ShowSidebarAction), SLOT(setChecked(bool)));
	connect(m_ui->sidebarDockWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), m_ui->sidebarWidget, SLOT(locationChanged(Qt::DockWidgetArea)));

	m_windowsManager->restore(session);

	m_ui->sidebarDockWidget->hide();
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

	m_tabBarToolBarWidget->updateOrientation();
}

MainWindow::~MainWindow()
{
	delete m_ui;
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	menu.addAction(m_actionsManager->getAction(Action::LockToolBarsAction));
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

void MainWindow::openUrl(const QString &text)
{
	if (text.isEmpty())
	{
		m_windowsManager->triggerAction(Action::NewTabAction);

		return;
	}

	BookmarksItem *bookmark = BookmarksManager::getBookmark(text);

	if (bookmark)
	{
		m_windowsManager->open(bookmark);

		return;
	}

	if (text == QString(QLatin1Char('~')) || text.startsWith(QLatin1Char('~') + QDir::separator()))
	{
		const QStringList locations = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);

		if (!locations.isEmpty())
		{
			m_windowsManager->open(QUrl(locations.first() + text.mid(1)));

			return;
		}
	}

	if (QFileInfo(text).exists())
	{
		m_windowsManager->open(QUrl::fromLocalFile(QFileInfo(text).canonicalFilePath()));

		return;
	}

	const QUrl url = QUrl::fromUserInput(text);

	if (url.isValid() && (url.isLocalFile() || QRegularExpression(QLatin1String("^(\\w+\\:\\S+)|([\\w\\-]+\\.[a-zA-Z]{2,}(/\\S*)?$)")).match(text).hasMatch()))
	{
		m_windowsManager->open(url);

		return;
	}

	const QString keyword = text.section(QLatin1Char(' '), 0, 0);
	SearchInformation *engine = SearchesManager::getSearchEngine(keyword, true);

	if (engine)
	{
		m_windowsManager->search(text.section(QLatin1Char(' '), 1), engine->identifier);

		return;
	}

	if (keyword == QLatin1String("?"))
	{
		m_windowsManager->search(text.section(QLatin1Char(' '), 1), QString());

		return;
	}

	m_windowsManager->search(text, QString());
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
		case Action::NewTabAction:
			m_windowsManager->open(QUrl(), NewTabOpen);

			break;
		case Action::NewTabPrivateAction:
			m_windowsManager->open(QUrl(), NewTabPrivateOpen);

			break;
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
			addBookmark();

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
		case Action::ShowMenuBarAction:
			if (checked && !m_menuBar)
			{
				createMenuBar();
			}
			else if (!checked && m_menuBar)
			{
				m_menuBar->deleteLater();

				setMenuBar(NULL);

				m_menuBar = NULL;
			}

			SettingsManager::setValue(QLatin1String("Interface/ShowMenuBar"), checked);

			break;
		case Action::ShowSidebarAction:
			m_ui->sidebarDockWidget->setVisible(checked);

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

				if (!SessionsManager::hasUrl(url, (sender() != TransfersManager::getInstance())))
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

void MainWindow::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Network/WorkOffline"))
	{
		m_actionsManager->getAction(Action::WorkOfflineAction)->setChecked(value.toBool());
	}
	else if (option == QLatin1String("Interface/LockToolBars"))
	{
		const QList<QToolBar*> toolBars = findChildren<QToolBar*>();
		const bool movable = !value.toBool();

		for (int i = 0; i < toolBars.count(); ++i)
		{
			toolBars.at(i)->setMovable(movable);
		}

		m_actionsManager->getAction(Action::LockToolBarsAction)->setChecked(value.toBool());
	}
	else if (option == QLatin1String("Interface/ShowMenuBar"))
	{
		m_actionsManager->getAction(Action::ShowMenuBarAction)->setChecked(value.toBool());
	}
}

void MainWindow::addBookmark(const QUrl &url, const QString &title)
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
			m_windowsManager->open(url, NewTabBackgroundOpen);
		}
	}
	else if (action == QLatin1String("openPanel"))
	{
		m_ui->sidebarDockWidget->setVisible(true);
		m_ui->sidebarWidget->openPanel(QLatin1String("transfers"));
	}
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
	return (m_tabBarToolBarWidget ? m_tabBarToolBarWidget->getTabBar() : NULL);
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
			SessionsManager::markSessionModified();

			if (isFullScreen())
			{
				m_actionsManager->getAction(Action::FullScreenAction)->setIcon(Utils::getIcon(QLatin1String("view-restore")));
				m_ui->statusBar->hide();

				if (m_menuBar)
				{
					m_menuBar->hide();
				}

				centralWidget()->installEventFilter(this);
			}
			else
			{
				m_actionsManager->getAction(Action::FullScreenAction)->setIcon(Utils::getIcon(QLatin1String("view-fullscreen")));
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
			triggerAction(Action::FullScreenAction);
		}
	}

	return QMainWindow::eventFilter(object, event);
}

}
