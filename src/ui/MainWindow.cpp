/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "MdiWidget.h"
#include "PreferencesDialog.h"
#include "SaveSessionDialog.h"
#include "SessionsManagerDialog.h"
#include "../core/ActionsManager.h"
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/HistoryManager.h"
#include "../core/SettingsManager.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"
#include "../core/WebBackend.h"
#include "../core/WebBackendsManager.h"

#include "ui_MainWindow.h"

#include <QtCore/QTextCodec>
#include <QtCore/QtMath>
#include <QtGui/QClipboard>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

namespace Otter
{

MainWindow::MainWindow(bool privateSession, const SessionMainWindow &windows, QWidget *parent) : QMainWindow(parent),
	m_windowsManager(NULL),
	m_sessionsGroup(NULL),
	m_textEncodingGroup(NULL),
	m_userAgentGroup(NULL),
	m_closedWindowsMenu(new QMenu(this)),
	m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);
	m_ui->menuBookmarks->installEventFilter(this);

#ifdef Q_OS_WIN
	m_taskbarButton = NULL;
#endif

	SessionsManager::setActiveWindow(this);

	QList<QAction*> actions;
	actions << m_ui->menuFile->actions() << m_ui->menuEdit->actions() << m_ui->menuView->actions() << m_ui->menuHistory->actions() << m_ui->menuBookmarks->actions() << m_ui->menuTools->actions() << m_ui->menuHelp->actions();

	ActionsManager::registerWindow(this, actions);

	m_ui->actionNewTab->setIcon(Utils::getIcon(QLatin1String("tab-new")));
	m_ui->actionNewTabPrivate->setIcon(Utils::getIcon(QLatin1String("tab-new-private")));
	m_ui->actionNewWindow->setIcon(Utils::getIcon(QLatin1String("window-new")));
	m_ui->actionNewWindowPrivate->setIcon(Utils::getIcon(QLatin1String("window-new-private")));
	m_ui->actionOpen->setIcon(Utils::getIcon(QLatin1String("document-open")));
	m_ui->actionCloseTab->setIcon(Utils::getIcon(QLatin1String("tab-close")));
	m_ui->actionSave->setIcon(Utils::getIcon(QLatin1String("document-save")));
	m_ui->actionPrint->setIcon(Utils::getIcon(QLatin1String("document-print")));
	m_ui->actionPrintPreview->setIcon(Utils::getIcon(QLatin1String("document-print-preview")));
	m_ui->actionExit->setIcon(Utils::getIcon(QLatin1String("application-exit")));
	m_ui->actionUndo->setIcon(Utils::getIcon(QLatin1String("edit-undo")));
	m_ui->actionUndo->setData(UndoAction);
	m_ui->actionRedo->setIcon(Utils::getIcon(QLatin1String("edit-redo")));
	m_ui->actionRedo->setData(RedoAction);
	m_ui->actionCut->setIcon(Utils::getIcon(QLatin1String("edit-cut")));
	m_ui->actionCut->setData(CutAction);
	m_ui->actionCopy->setIcon(Utils::getIcon(QLatin1String("edit-copy")));
	m_ui->actionCopy->setData(CopyAction);
	m_ui->actionPaste->setIcon(Utils::getIcon(QLatin1String("edit-paste")));
	m_ui->actionPaste->setData(PasteAction);
	m_ui->actionDelete->setIcon(Utils::getIcon(QLatin1String("edit-delete")));
	m_ui->actionDelete->setData(DeleteAction);
	m_ui->actionSelectAll->setIcon(Utils::getIcon(QLatin1String("edit-select-all")));
	m_ui->actionSelectAll->setData(SelectAllAction);
	m_ui->actionFind->setIcon(Utils::getIcon(QLatin1String("edit-find")));
	m_ui->actionFind->setData(FindAction);
	m_ui->actionFindNext->setData(FindNextAction);
	m_ui->actionFindPrevious->setData(FindPreviousAction);
	m_ui->actionReload->setIcon(Utils::getIcon(QLatin1String("view-refresh")));
	m_ui->actionReload->setData(ReloadAction);
	m_ui->actionStop->setIcon(Utils::getIcon(QLatin1String("process-stop")));
	m_ui->actionStop->setData(StopAction);
	m_ui->actionZoomIn->setIcon(Utils::getIcon(QLatin1String("zoom-in")));
	m_ui->actionZoomIn->setData(ZoomInAction);
	m_ui->actionZoomOut->setIcon(Utils::getIcon(QLatin1String("zoom-out")));
	m_ui->actionZoomOut->setData(ZoomOutAction);
	m_ui->actionZoomOriginal->setIcon(Utils::getIcon(QLatin1String("zoom-original")));
	m_ui->actionZoomOriginal->setData(ZoomOriginalAction);
	m_ui->actionFullScreen->setIcon(Utils::getIcon(QLatin1String("view-fullscreen")));
	m_ui->actionViewSource->setData(ViewSourceAction);
	m_ui->actionInspectPage->setData(InspectPageAction);
	m_ui->actionGoBack->setIcon(Utils::getIcon(QLatin1String("go-previous")));
	m_ui->actionGoBack->setData(GoBackAction);
	m_ui->actionGoForward->setIcon(Utils::getIcon(QLatin1String("go-next")));
	m_ui->actionGoForward->setData(GoForwardAction);
	m_ui->actionRewindBack->setIcon(Utils::getIcon(QLatin1String("go-first")));
	m_ui->actionRewindBack->setData(RewindBackAction);
	m_ui->actionRewindForward->setIcon(Utils::getIcon(QLatin1String("go-last")));
	m_ui->actionRewindForward->setData(RewindForwardAction);
	m_ui->menuClosedWindows->setIcon(Utils::getIcon(QLatin1String("user-trash")));
	m_ui->menuClosedWindows->setEnabled(false);
	m_ui->actionViewHistory->setIcon(Utils::getIcon(QLatin1String("view-history")));
	m_ui->actionClearHistory->setIcon(Utils::getIcon(QLatin1String("edit-clear-history")));
	m_ui->actionAddBookmark->setIcon(Utils::getIcon(QLatin1String("bookmark-new")));
	m_ui->actionManageBookmarks->setIcon(Utils::getIcon(QLatin1String("bookmarks-organize")));
	m_ui->actionAboutApplication->setIcon(Utils::getIcon(QLatin1String("otter"), false));
	m_ui->actionAboutQt->setIcon(Utils::getIcon(QLatin1String("qt"), false));
	m_ui->statusBar->setup();

	setStyleSheet(QLatin1String("QMainWindow::separator {width:0;height:0;}"));

	m_ui->tabsDockWidget->setup(m_closedWindowsMenu);
	m_ui->tabsDockWidget->setFloating(true);
	m_ui->tabsDockWidget->adjustSize();
	m_ui->tabsDockWidget->setFloating(false);

	MdiWidget *mdiWidget = new MdiWidget(this);

	setCentralWidget(mdiWidget);

	m_windowsManager = new WindowsManager(mdiWidget, m_ui->tabsDockWidget->getTabBar(), m_ui->statusBar, (privateSession || SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool()));

	SessionsManager::registerWindow(m_windowsManager);

#ifdef Q_OS_WIN
	connect(TransfersManager::getInstance(), SIGNAL(transferUpdated(TransferInformation*)), this , SLOT(updateWindowsTaskbarProgress()));
	connect(TransfersManager::getInstance(), SIGNAL(transferStarted(TransferInformation*)), this , SLOT(updateWindowsTaskbarProgress()));
	connect(TransfersManager::getInstance(), SIGNAL(transferFinished(TransferInformation*)), this , SLOT(updateWindowsTaskbarProgress()));
	connect(TransfersManager::getInstance(), SIGNAL(transferRemoved(TransferInformation*)), this , SLOT(updateWindowsTaskbarProgress()));
	connect(TransfersManager::getInstance(), SIGNAL(transferStopped(TransferInformation*)), this , SLOT(updateWindowsTaskbarProgress()));
#endif

	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(updateBookmarks(int)));
	connect(SessionsManager::getInstance(), SIGNAL(closedWindowsChanged()), this, SLOT(updateClosedWindows()));
	connect(TransfersManager::getInstance(), SIGNAL(transferStarted(TransferInformation*)), this, SLOT(actionTransfers()));
	connect(m_windowsManager, SIGNAL(requestedAddBookmark(QUrl,QString)), this, SLOT(actionAddBookmark(QUrl,QString)));
	connect(m_windowsManager, SIGNAL(requestedNewWindow(bool,bool,QUrl)), this, SIGNAL(requestedNewWindow(bool,bool,QUrl)));
	connect(m_windowsManager, SIGNAL(windowTitleChanged(QString)), this, SLOT(setWindowTitle(QString)));
	connect(m_windowsManager, SIGNAL(closedWindowsAvailableChanged(bool)), m_ui->tabsDockWidget, SLOT(setClosedWindowsMenuEnabled(bool)));
	connect(m_windowsManager, SIGNAL(closedWindowsAvailableChanged(bool)), m_ui->menuClosedWindows, SLOT(setEnabled(bool)));
	connect(m_windowsManager, SIGNAL(actionsChanged()), this, SLOT(updateActions()));
	connect(m_closedWindowsMenu, SIGNAL(aboutToShow()), this, SLOT(menuClosedWindowsAboutToShow()));
	connect(m_ui->consoleDockWidget, SIGNAL(visibilityChanged(bool)), m_ui->actionErrorConsole, SLOT(setChecked(bool)));
	connect(m_ui->actionNewTab, SIGNAL(triggered()), m_windowsManager, SLOT(open()));
	connect(m_ui->actionNewTabPrivate, SIGNAL(triggered()), this, SLOT(actionNewTabPrivate()));
	connect(m_ui->actionNewWindow, SIGNAL(triggered()), this, SIGNAL(requestedNewWindow()));
	connect(m_ui->actionNewWindowPrivate, SIGNAL(triggered()), this, SLOT(actionNewWindowPrivate()));
	connect(m_ui->actionOpen, SIGNAL(triggered()), this, SLOT(actionOpen()));
	connect(m_ui->actionCloseTab, SIGNAL(triggered()), m_windowsManager, SLOT(close()));
	connect(m_ui->actionSaveSession, SIGNAL(triggered()), this, SLOT(actionSaveSession()));
	connect(m_ui->actionManageSessions, SIGNAL(triggered()), this, SLOT(actionManageSessions()));
	connect(m_ui->actionPrint, SIGNAL(triggered()), m_windowsManager, SLOT(print()));
	connect(m_ui->actionPrintPreview, SIGNAL(triggered()), m_windowsManager, SLOT(printPreview()));
	connect(m_ui->actionWorkOffline, SIGNAL(toggled(bool)), this, SLOT(actionWorkOffline(bool)));
	connect(m_ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(m_ui->actionUndo, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionRedo, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionCut, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionCopy, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionPaste, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionDelete, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionSelectAll, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionFind, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionFindNext, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionFindPrevious, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionZoomIn, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionZoomOut, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionZoomOriginal, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionFullScreen, SIGNAL(triggered()), this, SLOT(actionFullScreen()));
	connect(m_ui->actionViewSource, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionInspectPage, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionReload, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionStop, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionGoBack, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionGoForward, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionRewindBack, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionRewindForward, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionViewHistory, SIGNAL(triggered()), this, SLOT(actionViewHistory()));
	connect(m_ui->actionClearHistory, SIGNAL(triggered()), this, SLOT(actionClearHistory()));
	connect(m_ui->actionAddBookmark, SIGNAL(triggered()), this, SLOT(actionAddBookmark()));
	connect(m_ui->actionManageBookmarks, SIGNAL(triggered()), this, SLOT(actionManageBookmarks()));
	connect(m_ui->actionCookies, SIGNAL(triggered()), this, SLOT(actionCookies()));
	connect(m_ui->actionTransfers, SIGNAL(triggered()), this, SLOT(actionTransfers()));
	connect(m_ui->actionErrorConsole, SIGNAL(toggled(bool)), this, SLOT(actionErrorConsole(bool)));
	connect(m_ui->actionPreferences, SIGNAL(triggered()), this, SLOT(actionPreferences()));
	connect(m_ui->actionAboutApplication, SIGNAL(triggered()), this, SLOT(actionAboutApplication()));
	connect(m_ui->actionAboutQt, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));
	connect(m_ui->menuFile, SIGNAL(aboutToShow()), this, SLOT(menuFileAboutToShow()));
	connect(m_ui->menuSessions, SIGNAL(aboutToShow()), this, SLOT(menuSessionsAboutToShow()));
	connect(m_ui->menuSessions, SIGNAL(triggered(QAction*)), this, SLOT(actionSession(QAction*)));
	connect(m_ui->menuUserAgent, SIGNAL(aboutToShow()), this, SLOT(menuUserAgentAboutToShow()));
	connect(m_ui->menuTextEncoding, SIGNAL(aboutToShow()), this, SLOT(menuTextEncodingAboutToShow()));
	connect(m_ui->menuClosedWindows, SIGNAL(aboutToShow()), this, SLOT(menuClosedWindowsAboutToShow()));
	connect(m_ui->menuBookmarks, SIGNAL(aboutToShow()), this, SLOT(menuBookmarksAboutToShow()));

	m_windowsManager->restore(windows);

	m_ui->panelDockWidget->hide();
	m_ui->consoleDockWidget->hide();

	SettingsManager::setDefaultValue(QLatin1String("Window/Size"), size());
	SettingsManager::setDefaultValue(QLatin1String("Window/Position"), pos());
	SettingsManager::setDefaultValue(QLatin1String("Window/State"), QByteArray());

	updateActions();
	resize(SettingsManager::getValue(QLatin1String("Window/Size")).toSize());
	move(SettingsManager::getValue(QLatin1String("Window/Position")).toPoint());
	restoreState(SettingsManager::getValue(QLatin1String("Window/State")).toByteArray());
	setWindowTitle(QStringLiteral("%1 - Otter").arg(m_windowsManager->getTitle()));
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
			messageBox.setInformativeText("Do you want to continue?");
			messageBox.setIcon(QMessageBox::Question);
			messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
			messageBox.setDefaultButton(QMessageBox::Cancel);
			messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

			if (messageBox.exec() == QMessageBox::Yes)
			{
				runningTransfers = 0;
			}

			SettingsManager::setValue(QLatin1String("Choices/WarnQuitTransfers"), !messageBox.checkBox()->isChecked());

			if (runningTransfers > 0)
			{
				event->ignore();

				return;
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
				NetworkManager::clearCookies();
			}

			if (clearSettings.contains(QLatin1String("downloads")))
			{
				TransfersManager::clearTransfers();
			}

			if (clearSettings.contains(QLatin1String("cache")))
			{
				NetworkManager::clearCache();
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
		SessionsManager::storeClosedWindow(m_windowsManager);
	}

	m_windowsManager->closeAll();

	SettingsManager::setValue(QLatin1String("Window/Size"), size());
	SettingsManager::setValue(QLatin1String("Window/Position"), pos());
	SettingsManager::setValue(QLatin1String("Window/State"), saveState());

	application->removeWindow(this);

	event->accept();
}

void MainWindow::gatherBookmarks(int folder)
{
	const QList<BookmarkInformation*> bookmarks = BookmarksManager::getFolder(folder);

	for (int i = 0; i < bookmarks.count(); ++i)
	{
		if (bookmarks.at(i)->type == FolderBookmark)
		{
			gatherBookmarks(bookmarks.at(i)->identifier);
		}
		else if (bookmarks.at(i)->type == UrlBookmark)
		{
			m_bookmarksToOpen.append(bookmarks.at(i)->url);
		}
	}
}

void MainWindow::openUrl(const QUrl &url)
{
	m_windowsManager->open(url);
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

void MainWindow::actionWorkOffline(bool enabled)
{
	SettingsManager::setValue(QLatin1String("Network/WorkOffline"), enabled);
}

void MainWindow::actionFullScreen()
{
	if (isFullScreen())
	{
		showNormal();
	}
	else
	{
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

void MainWindow::actionTextEncoding(QAction *action)
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

	m_windowsManager->setDefaultTextEncoding(encoding.toLower());
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
	ClearHistoryDialog dialog(SettingsManager::getValue(QLatin1String("History/ClearOnClose")).toStringList(), false, this);
	dialog.exec();
}

void MainWindow::actionAddBookmark(const QUrl &url, const QString &title)
{
	const QString bookmarkUrl = (url.isValid() ? url.toString(QUrl::RemovePassword) : m_windowsManager->getUrl().toString(QUrl::RemovePassword));

	if (bookmarkUrl.isEmpty() || (BookmarksManager::hasBookmark(bookmarkUrl) && QMessageBox::warning(this, tr("Warning"), tr("You already have this address in your bookmarks.\nDo you want to continue?"), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Cancel))
	{
		return;
	}

	BookmarkInformation *bookmark = new BookmarkInformation();
	bookmark->url = bookmarkUrl;
	bookmark->title = (url.isValid() ? title : m_windowsManager->getTitle());
	bookmark->type = UrlBookmark;

	BookmarkPropertiesDialog dialog(bookmark, 0, this);

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
		m_windowsManager->open(QUrl(action->data().toString()));
	}
}

void MainWindow::actionOpenBookmarkFolder()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (!action)
	{
		return;
	}

	gatherBookmarks(action->data().toInt());

	if (m_bookmarksToOpen.isEmpty())
	{
		return;
	}

	if (m_bookmarksToOpen.count() > 1 && SettingsManager::getValue(QLatin1String("Choices/WarnOpenBookmarkFolder")).toBool())
	{
		QMessageBox messageBox;
		messageBox.setWindowTitle(tr("Question"));
		messageBox.setText(tr("You are about to open %n bookmarks.", "", m_bookmarksToOpen.count()));
		messageBox.setInformativeText("Do you want to continue?");
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		messageBox.setDefaultButton(QMessageBox::Yes);
		messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

		if (messageBox.exec() == QMessageBox::Cancel)
		{
			m_bookmarksToOpen.clear();
		}

		SettingsManager::setValue(QLatin1String("Choices/WarnOpenBookmarkFolder"), !messageBox.checkBox()->isChecked());
	}

	for (int i = 0; i < m_bookmarksToOpen.count(); ++i)
	{
		m_windowsManager->open(QUrl(m_bookmarksToOpen.at(i)));
	}

	m_bookmarksToOpen.clear();
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

void MainWindow::actionPreferences()
{
	PreferencesDialog dialog(QLatin1String("general"), this);
	dialog.exec();
}

void MainWindow::actionAboutApplication()
{
	QMessageBox::about(this, QLatin1String("Otter"), QString(tr("<b>Otter %1</b><br>Web browser controlled by the user, not vice-versa.").arg(QApplication::applicationVersion())));
}

void MainWindow::menuFileAboutToShow()
{
	m_ui->actionWorkOffline->setChecked(SettingsManager::getValue(QLatin1String("Network/WorkOffline")).toBool());
}

void MainWindow::menuSessionsAboutToShow()
{
	if (m_sessionsGroup)
	{
		m_sessionsGroup->deleteLater();

		QAction *saveSessionAction = m_ui->menuSessions->actions().at(0);
		saveSessionAction->setParent(this);

		QAction *manageSessionsAction = m_ui->menuSessions->actions().at(1);
		manageSessionsAction->setParent(this);

		m_ui->menuSessions->clear();
		m_ui->menuSessions->addAction(saveSessionAction);
		m_ui->menuSessions->addAction(manageSessionsAction);
		m_ui->menuSessions->addSeparator();
	}

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

		QAction *action = m_ui->menuSessions->addAction(tr("%1 (%n tab(s))", "", windows).arg(sorted.at(i).title.isEmpty() ? tr("(Untitled)") : QString(sorted.at(i).title).replace(QLatin1Char('&'), QLatin1String("&&"))));
		action->setData(sorted.at(i).path);
		action->setCheckable(true);
		action->setChecked(sorted.at(i).path == currentSession);

		m_sessionsGroup->addAction(action);
	}
}

void MainWindow::menuUserAgentAboutToShow()
{
	if (m_userAgentGroup)
	{
		m_userAgentGroup->deleteLater();

		m_ui->menuUserAgent->clear();
	}

	const QStringList userAgents = NetworkManager::getUserAgents();
	const QString userAgent = m_windowsManager->getUserAgent().first.toLower();

	m_userAgentGroup = new QActionGroup(this);
	m_userAgentGroup->setExclusive(true);

	QAction *defaultAction = m_ui->menuUserAgent->addAction(tr("Default"));
	defaultAction->setData(QLatin1String("default"));
	defaultAction->setCheckable(true);
	defaultAction->setChecked(userAgent == QLatin1String("default"));

	m_userAgentGroup->addAction(defaultAction);

	m_ui->menuUserAgent->addSeparator();

	for (int i = 0; i < userAgents.count(); ++i)
	{
		const QString title = NetworkManager::getUserAgent(userAgents.at(i)).title;
		QAction *userAgentAction = m_ui->menuUserAgent->addAction((title.isEmpty() ? tr("(Untitled)") : title));
		userAgentAction->setData(userAgents.at(i));
		userAgentAction->setCheckable(true);
		userAgentAction->setChecked(userAgent == userAgents.at(i));

		m_userAgentGroup->addAction(userAgentAction);
	}

	m_ui->menuUserAgent->addSeparator();

	QAction *customAction = m_ui->menuUserAgent->addAction(tr("Custom"));
	customAction->setData(QLatin1String("custom"));
	customAction->setCheckable(true);
	customAction->setChecked(userAgent == QLatin1String("custom"));

	m_userAgentGroup->addAction(customAction);

	connect(m_ui->menuUserAgent, SIGNAL(triggered(QAction*)), this, SLOT(actionUserAgent(QAction*)));

	if (!m_userAgentGroup->checkedAction() && m_ui->menuUserAgent->actions().count() > 2)
	{
		m_ui->menuUserAgent->actions().first()->setChecked(true);
	}
}

void MainWindow::menuTextEncodingAboutToShow()
{
	if (!m_textEncodingGroup)
	{
		QList<int> textCodecs;
		textCodecs << 106 << 1015 << 1017 << 4 << 5 << 6 << 7 << 8 << 82 << 10 << 85 << 12 << 13 << 109 << 110 << 112 << 2250 << 2251 << 2252 << 2253 << 2254 << 2255 << 2256 << 2257 << 2258 << 18 << 39 << 17 << 38 << 2026;

		m_textEncodingGroup = new QActionGroup(this);
		m_textEncodingGroup->setExclusive(true);

		QAction *defaultAction = m_ui->menuTextEncoding->addAction(tr("Auto Detect"));
		defaultAction->setData(-1);
		defaultAction->setCheckable(true);

		m_textEncodingGroup->addAction(defaultAction);

		m_ui->menuTextEncoding->addSeparator();

		for (int i = 0; i < textCodecs.count(); ++i)
		{
			QTextCodec *codec = QTextCodec::codecForMib(textCodecs.at(i));

			if (!codec)
			{
				continue;
			}

			QAction *textCodecAction = m_ui->menuTextEncoding->addAction(codec->name());
			textCodecAction->setData(textCodecs.at(i));
			textCodecAction->setCheckable(true);

			m_textEncodingGroup->addAction(textCodecAction);
		}

		connect(m_ui->menuTextEncoding, SIGNAL(triggered(QAction*)), this, SLOT(actionTextEncoding(QAction*)));
	}

	const QString encoding = m_windowsManager->getDefaultTextEncoding().toLower();

	for (int i = 2; i < m_ui->menuTextEncoding->actions().count(); ++i)
	{
		QAction *action = m_ui->menuTextEncoding->actions().at(i);

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

	if (!m_textEncodingGroup->checkedAction())
	{
		m_ui->menuTextEncoding->actions().first()->setChecked(true);
	}
}

void MainWindow::menuClosedWindowsAboutToShow()
{
	m_closedWindowsMenu->clear();

	m_ui->menuClosedWindows->clear();
	m_ui->menuClosedWindows->addAction(Utils::getIcon(QLatin1String("edit-clear")), tr("Clear"), this, SLOT(actionClearClosedWindows()))->setData(0);
	m_ui->menuClosedWindows->addSeparator();

	const QStringList windows = SessionsManager::getClosedWindows();

	if (!windows.isEmpty())
	{
		for (int i = 0; i < windows.count(); ++i)
		{
			m_ui->menuClosedWindows->addAction(m_ui->menuClosedWindows->fontMetrics().elidedText(tr("Window - %1").arg(windows.at(i)), Qt::ElideRight, 300), this, SLOT(actionRestoreClosedWindow()))->setData(-(i + 1));
		}

		m_ui->menuClosedWindows->addSeparator();
	}

	WebBackend *backend = WebBackendsManager::getBackend();
	const QList<SessionWindow> tabs = m_windowsManager->getClosedWindows();

	for (int i = 0; i < tabs.count(); ++i)
	{
		m_ui->menuClosedWindows->addAction(backend->getIconForUrl(QUrl(tabs.at(i).getUrl())), m_ui->menuClosedWindows->fontMetrics().elidedText(tabs.at(i).getTitle(), Qt::ElideRight, 300), this, SLOT(actionRestoreClosedWindow()))->setData(i + 1);
	}

	m_closedWindowsMenu->addActions(m_ui->menuClosedWindows->actions());
}

void MainWindow::menuBookmarksAboutToShow()
{
	QMenu *menu = qobject_cast<QMenu*>(sender());

	if (!menu || !menu->menuAction())
	{
		return;
	}

	menu->setObjectName(QLatin1String("bookmarks"));
	menu->installEventFilter(this);

	const int folder = menu->menuAction()->data().toInt();

	if ((folder == 0 && menu->actions().count() == 3) || (folder != 0 && menu->actions().isEmpty()))
	{
		WebBackend *backend = WebBackendsManager::getBackend();
		const QList<BookmarkInformation*> bookmarks = BookmarksManager::getFolder(folder);

		if (folder != 0 && bookmarks.count() > 1)
		{
			QAction *openAllAction = menu->addAction(Utils::getIcon(QLatin1String("document-open-folder")), tr("Open All"));
			openAllAction->setData(folder);

			connect(openAllAction, SIGNAL(triggered()), this, SLOT(actionOpenBookmarkFolder()));

			menu->addSeparator();
		}

		for (int i = 0; i < bookmarks.count(); ++i)
		{
			if (bookmarks.at(i)->type == FolderBookmark || bookmarks.at(i)->type == UrlBookmark)
			{
				QAction *action = menu->addAction(((bookmarks.at(i)->type == FolderBookmark) ? Utils::getIcon(QLatin1String("inode-directory")) : backend->getIconForUrl(QUrl(bookmarks.at(i)->url))), (bookmarks.at(i)->title.isEmpty() ? tr("(Untitled)") : QString(bookmarks.at(i)->title).replace(QLatin1Char('&'), QLatin1String("&&"))));
				action->setToolTip(bookmarks.at(i)->description);

				if (bookmarks.at(i)->type == FolderBookmark)
				{
					action->setData(bookmarks.at(i)->identifier);

					if (!bookmarks.at(i)->children.isEmpty())
					{
						action->setMenu(new QMenu());

						connect(action->menu(), SIGNAL(aboutToShow()), this, SLOT(menuBookmarksAboutToShow()));
					}
				}
				else
				{
					action->setData(bookmarks.at(i)->url);

					connect(action, SIGNAL(triggered()), this, SLOT(actionOpenBookmark()));
				}
			}
			else
			{
				menu->addSeparator();
			}
		}
	}
}

void MainWindow::openBookmark()
{
	const QUrl url(m_currentBookmark);

	if (url.isValid())
	{
		QAction *action = qobject_cast<QAction*>(sender());

		m_windowsManager->open(url, (action ? static_cast<OpenHints>(action->data().toInt()) : DefaultOpen));

		m_ui->menuBookmarks->close();
	}

	m_currentBookmark = QString();
}

void MainWindow::triggerWindowAction()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		m_windowsManager->triggerAction(static_cast<WindowAction>(action->data().toInt()), action->isChecked());
	}
}

void MainWindow::updateClosedWindows()
{
	m_ui->tabsDockWidget->setClosedWindowsMenuEnabled(m_windowsManager->getClosedWindows().count() > 0 || SessionsManager::getClosedWindows().count() > 0);
}

void MainWindow::updateBookmarks(int folder)
{
	if (m_ui->menuBookmarks->actions().count() == 3)
	{
		return;
	}

	for (int i = (m_ui->menuBookmarks->actions().count() - 1); i > 2; --i)
	{
		QAction *action = m_ui->menuBookmarks->actions().at(i);

		if (folder == 0)
		{
			action->deleteLater();

			m_ui->menuBookmarks->removeAction(action);
		}
		else if (m_ui->menuBookmarks->actions().at(i)->menu())
		{
			action->menu()->deleteLater();
			action->setMenu(new QMenu());

			connect(action->menu(), SIGNAL(aboutToShow()), this, SLOT(menuBookmarksAboutToShow()));
		}
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
		m_ui->actionUndo->setEnabled(undoAction->isEnabled());
		m_ui->actionUndo->setText(undoAction->text());
	}
	else
	{
		m_ui->actionUndo->setEnabled(false);
		m_ui->actionUndo->setText(tr("Undo"));
	}

	QAction *redoAction = m_windowsManager->getAction(RedoAction);

	if (redoAction)
	{
		m_ui->actionRedo->setEnabled(redoAction->isEnabled());
		m_ui->actionRedo->setText(redoAction->text());
	}
	else
	{
		m_ui->actionRedo->setEnabled(false);
		m_ui->actionRedo->setText(tr("Redo"));
	}

	updateAction(m_windowsManager->getAction(CutAction), m_ui->actionCut);
	updateAction(m_windowsManager->getAction(CopyAction), m_ui->actionCopy);
	updateAction(m_windowsManager->getAction(PasteAction), m_ui->actionPaste);
	updateAction(m_windowsManager->getAction(DeleteAction), m_ui->actionDelete);
	updateAction(m_windowsManager->getAction(SelectAllAction), m_ui->actionSelectAll);
	updateAction(m_windowsManager->getAction(FindAction), m_ui->actionFind);
	updateAction(m_windowsManager->getAction(FindNextAction), m_ui->actionFindNext);
	updateAction(m_windowsManager->getAction(FindPreviousAction), m_ui->actionFindPrevious);
	updateAction(m_windowsManager->getAction(ReloadAction), m_ui->actionReload);
	updateAction(m_windowsManager->getAction(StopAction), m_ui->actionStop);
	updateAction(m_windowsManager->getAction(ViewSourceAction), m_ui->actionViewSource);
	updateAction(m_windowsManager->getAction(InspectPageAction), m_ui->actionInspectPage);
	updateAction(m_windowsManager->getAction(GoBackAction), m_ui->actionGoBack);
	updateAction(m_windowsManager->getAction(RewindBackAction), m_ui->actionRewindBack);
	updateAction(m_windowsManager->getAction(GoForwardAction), m_ui->actionGoForward);
	updateAction(m_windowsManager->getAction(RewindForwardAction), m_ui->actionRewindForward);

	const bool canZoom = m_windowsManager->canZoom();

	m_ui->actionZoomOut->setEnabled(canZoom);
	m_ui->actionZoomIn->setEnabled(canZoom);
	m_ui->actionZoomOriginal->setEnabled(canZoom);
}

#ifdef Q_OS_WIN
void MainWindow::updateWindowsTaskbarProgress()
{
	const QList<TransferInformation*> transfers = TransfersManager::getInstance()->getTransfers();
	qint64 bytesTotal = 0;
	qint64 bytesReceived = 0;
	bool hasActiveTransfers = false;

	for (int i = 0; i < transfers.count(); ++i)
	{
		if (transfers[i]->state == RunningTransfer && transfers[i]->bytesTotal > 0)
		{
			hasActiveTransfers = true;
			bytesTotal += transfers[i]->bytesTotal;
			bytesReceived += transfers[i]->bytesReceived;
		}
	}

	if (hasActiveTransfers)
	{
		if (!m_taskbarButton)
		{
			m_taskbarButton = new QWinTaskbarButton(this);
			m_taskbarButton->setWindow(windowHandle());
			m_taskbarButton->progress()->show();
		}

		m_taskbarButton->progress()->setValue((bytesReceived > 0) ? qFloor(((qreal) bytesReceived / bytesTotal) * 100) : 0);
	}
	else if (m_taskbarButton)
	{
		m_taskbarButton->progress()->reset();
		m_taskbarButton->progress()->hide();
		m_taskbarButton->deleteLater();
		m_taskbarButton = NULL;
	}
}
#endif

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
			m_ui->actionFullScreen->setIcon(Utils::getIcon(QLatin1String("view-restore")));
			m_ui->menuBar->hide();
			m_ui->statusBar->hide();

			centralWidget()->installEventFilter(this);
		}
		else
		{
			m_ui->actionFullScreen->setIcon(Utils::getIcon(QLatin1String("view-fullscreen")));
			m_ui->menuBar->show();
			m_ui->statusBar->show();

			centralWidget()->removeEventFilter(this);
		}

		return result;
	}

	if (event->type() == QEvent::WindowActivate)
	{
		SessionsManager::setActiveWindow(this);
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
