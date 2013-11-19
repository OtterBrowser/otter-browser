#include "MainWindow.h"
#include "TabBarWidget.h"
#include "SessionsManagerDialog.h"
#include "../core/ActionsManager.h"
#include "../core/Application.h"
#include "../core/SettingsManager.h"
#include "../backends/web/WebBackendsManager.h"

#include "ui_MainWindow.h"

#include <QtCore/QTextCodec>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolButton>

namespace Otter
{

MainWindow::MainWindow(bool privateSession, const SessionEntry &windows, QWidget *parent) : QMainWindow(parent),
	m_windowsManager(NULL),
	m_closedWindowsAction(new QAction(QIcon(QIcon::fromTheme("user-trash", QIcon(":/icons/user-trash.png"))), tr("Closed Tabs"), this)),
	m_sessionsGroup(NULL),
	m_textEncodingGroup(NULL),
	m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);

	setStyleSheet("QMainWindow::separator {width:0;height:0;}");

	m_closedWindowsAction->setMenu(new QMenu(this));
	m_closedWindowsAction->setEnabled(false);

	TabBarWidget *tabBar = new TabBarWidget(m_ui->tabsWidgetContents);
	QToolButton *trashButton = new QToolButton(m_ui->tabsWidgetContents);
	trashButton->setAutoRaise(true);
	trashButton->setDefaultAction(m_closedWindowsAction);

	QBoxLayout *tabsLayout = new QBoxLayout(QBoxLayout::LeftToRight, m_ui->tabsWidgetContents);
	tabsLayout->addWidget(tabBar);
	tabsLayout->addWidget(trashButton);
	tabsLayout->setContentsMargins(0, 0, 0, 0);
	tabsLayout->setSpacing(0);

	m_ui->tabsWidgetContents->setLayout(tabsLayout);
	m_ui->tabsWidget->setTitleBarWidget(NULL);

	ActionsManager::setActiveWindow(this);
	ActionsManager::registerWindow(this);
	ActionsManager::registerActions(this, m_ui->menuFile->actions());
	ActionsManager::registerActions(this, m_ui->menuEdit->actions());
	ActionsManager::registerActions(this, m_ui->menuView->actions());
	ActionsManager::registerActions(this, m_ui->menuHistory->actions());
	ActionsManager::registerActions(this, m_ui->menuBookmarks->actions());
	ActionsManager::registerActions(this, m_ui->menuTools->actions());
	ActionsManager::registerActions(this, m_ui->menuHelp->actions());
	ActionsManager::registerAction(this, "OpenLinkInThisTab", tr("Open"));
	ActionsManager::registerAction(this, "OpenLinkInNewTab", tr("Open in New Tab"));
	ActionsManager::registerAction(this, "OpenLinkInNewTabBackground", tr("Open in New Background Tab"));
	ActionsManager::registerAction(this, "OpenLinkInNewWindow", tr("Open in New Window"));
	ActionsManager::registerAction(this, "OpenLinkInNewWindowBackground", tr("Open in New Background Window"));
	ActionsManager::registerAction(this, "CopyLinkToClipboard", tr("Copy Link to Clipboard"));
	ActionsManager::registerAction(this, "OpenFrameInThisTab", tr("Open"));
	ActionsManager::registerAction(this, "OpenFrameInNewTab", tr("Open in New Tab"));
	ActionsManager::registerAction(this, "OpenFrameInNewTabBackground", tr("Open in New Background Tab"));
	ActionsManager::registerAction(this, "CopyFrameLinkToClipboard", tr("Copy Frame Link to Clipboard"));
	ActionsManager::registerAction(this, "ReloadFrame", tr("Reload"));
	ActionsManager::registerAction(this, "ViewSourceFrame", tr("View Source"));
	ActionsManager::registerAction(this, "SaveLinkToDisk", tr("Save Link Target As..."));
	ActionsManager::registerAction(this, "SaveLinkToDownloads", tr("Save to Downloads"));
	ActionsManager::registerAction(this, "BookmarkLink", tr("Bookmark Link..."));
	ActionsManager::registerAction(this, "ReloadTime", tr("Reload Each"));
	ActionsManager::registerAction(this, "CopyAddress", tr("Copy Address"));
	ActionsManager::registerAction(this, "Validate", tr("Validate"));
	ActionsManager::registerAction(this, "ContentBlocking", tr("Content Blocking..."));
	ActionsManager::registerAction(this, "WebsitePreferences", tr("Website Preferences..."));
	ActionsManager::registerAction(this, "ImageProperties", tr("Image Properties..."));
	ActionsManager::registerAction(this, "OpenImageInNewTab", tr("Open Image"));
	ActionsManager::registerAction(this, "SaveImageToDisk", tr("Save Image..."));
	ActionsManager::registerAction(this, "CopyImageToClipboard", tr("Copy Image to Clipboard."));
	ActionsManager::registerAction(this, "CopyImageUrlToClipboard", tr("Copy Image Link to Clipboard"));
	ActionsManager::registerAction(this, "Search", tr("Search"));
	ActionsManager::registerAction(this, "SearchMenu", tr("Search Using"));
	ActionsManager::registerAction(this, "OpenSelectionAsLink", tr("Go to This Address"));
	ActionsManager::registerAction(this, "ClearAll", tr("Clear All"));
	ActionsManager::registerAction(this, "SpellCheck", tr("Check Spelling"));
	ActionsManager::registerAction(this, "CreateSearch", tr("Create Search..."));

	m_ui->actionNewTab->setIcon(QIcon::fromTheme("tab-new", QIcon(":/icons/tab-new.png")));
	m_ui->actionNewTabPrivate->setIcon(QIcon::fromTheme("tab-new-private", QIcon(":/icons/tab-new-private.png")));
	m_ui->actionNewWindow->setIcon(QIcon::fromTheme("window-new", QIcon(":/icons/window-new.png")));
	m_ui->actionNewWindowPrivate->setIcon(QIcon::fromTheme("window-new-private", QIcon(":/icons/window-new-private.png")));
	m_ui->actionOpen->setIcon(QIcon::fromTheme("document-open", QIcon(":/icons/document-open.png")));
	m_ui->actionCloseTab->setIcon(QIcon::fromTheme("tab-close", QIcon(":/icons/tab-close.png")));
	m_ui->actionSave->setIcon(QIcon::fromTheme("document-save", QIcon(":/icons/document-save.png")));
	m_ui->actionPrint->setIcon(QIcon::fromTheme("document-print", QIcon(":/icons/document-print.png")));
	m_ui->actionPrintPreview->setIcon(QIcon::fromTheme("document-print-preview", QIcon(":/icons/document-print-preview.png")));
	m_ui->actionExit->setIcon(QIcon::fromTheme("application-exit", QIcon(":/icons/application-exit.png")));
	m_ui->actionUndo->setIcon(QIcon::fromTheme("edit-undo", QIcon(":/icons/edit-undo.png")));
	m_ui->actionRedo->setIcon(QIcon::fromTheme("edit-redo", QIcon(":/icons/edit-redo.png")));
	m_ui->actionCut->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/icons/edit-cut.png")));
	m_ui->actionCopy->setIcon(QIcon::fromTheme("edit-copy", QIcon(":/icons/edit-copy.png")));
	m_ui->actionPaste->setIcon(QIcon::fromTheme("edit-paste", QIcon(":/icons/edit-paste.png")));
	m_ui->actionDelete->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/icons/edit-delete.png")));
	m_ui->actionSelectAll->setIcon(QIcon::fromTheme("edit-select-all", QIcon(":/icons/edit-select-all.png")));
	m_ui->actionFind->setIcon(QIcon::fromTheme("edit-find", QIcon(":/icons/edit-find.png")));
	m_ui->actionReload->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/icons/view-refresh.png")));
	m_ui->actionReload->setData(ReloadAction);
	m_ui->actionStop->setIcon(QIcon::fromTheme("process-stop", QIcon(":/icons/process-stop.png")));
	m_ui->actionStop->setData(StopAction);
	m_ui->actionZoomIn->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/icons/zoom-in.png")));
	m_ui->actionZoomIn->setData(ZoomInAction);
	m_ui->actionZoomOut->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/icons/zoom-out.png")));
	m_ui->actionZoomOut->setData(ZoomOutAction);
	m_ui->actionZoomOriginal->setIcon(QIcon::fromTheme("zoom-original", QIcon(":/icons/zoom-original.png")));
	m_ui->actionZoomOriginal->setData(ZoomOriginalAction);
	m_ui->actionGoBack->setIcon(QIcon::fromTheme("go-previous", QIcon(":/icons/go-previous.png")));
	m_ui->actionGoBack->setData(GoBackAction);
	m_ui->actionGoForward->setIcon(QIcon::fromTheme("go-next", QIcon(":/icons/go-next.png")));
	m_ui->actionGoForward->setData(GoForwardAction);
	m_ui->actionRewindBack->setIcon(QIcon::fromTheme("go-first", QIcon(":/icons/go-first.png")));
	m_ui->actionRewindBack->setData(RewindBackAction);
	m_ui->actionRewindForward->setIcon(QIcon::fromTheme("go-last", QIcon(":/icons/go-last.png")));
	m_ui->actionRewindForward->setData(RewindForwardAction);
	m_ui->menuClosedWindows->setIcon(QIcon::fromTheme("user-trash", QIcon(":/icons/user-trash.png")));
	m_ui->menuClosedWindows->setEnabled(false);
	m_ui->actionViewHistory->setIcon(QIcon::fromTheme("view-history", QIcon(":/icons/view-history.png")));
	m_ui->actionClearHistory->setIcon(QIcon::fromTheme("edit-clear-history", QIcon(":/icons/edit-clear-history.png")));
	m_ui->actionAboutApplication->setIcon(QIcon(":/icons/otter.png"));
	m_ui->actionAboutQt->setIcon(QIcon(":/icons/qt.png"));
	m_ui->statusBar->setup();

	m_windowsManager = new WindowsManager(m_ui->mdiArea, tabBar, m_ui->statusBar, privateSession);
	m_windowsManager->restore(windows.windows);
	m_windowsManager->setCurrentWindow(windows.index);

	setWindowTitle(m_windowsManager->getTitle());

	connect(SessionsManager::getInstance(), SIGNAL(closedWindowsChanged()), this, SLOT(updateClosedWindows()));
	connect(m_windowsManager, SIGNAL(windowTitleChanged(QString)), this, SLOT(setWindowTitle(QString)));
	connect(m_windowsManager, SIGNAL(closedWindowsAvailableChanged(bool)), m_closedWindowsAction, SLOT(setEnabled(bool)));
	connect(m_windowsManager, SIGNAL(closedWindowsAvailableChanged(bool)), m_ui->menuClosedWindows, SLOT(setEnabled(bool)));
	connect(m_closedWindowsAction->menu(), SIGNAL(aboutToShow()), this, SLOT(menuClosedWindowsAboutToShow()));
	connect(m_closedWindowsAction->menu(), SIGNAL(triggered(QAction*)), this, SLOT(actionClosedWindows(QAction*)));
	connect(m_ui->tabsWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), tabBar, SLOT(setOrientation(Qt::DockWidgetArea)));
	connect(m_ui->actionNewTab, SIGNAL(triggered()), m_windowsManager, SLOT(open()));
	connect(m_ui->actionNewTabPrivate, SIGNAL(triggered()), this, SLOT(actionNewTabPrivate()));
	connect(m_ui->actionNewWindow, SIGNAL(triggered()), this, SIGNAL(requestedNewWindow()));
	connect(m_ui->actionNewWindowPrivate, SIGNAL(triggered()), this, SIGNAL(requestedNewWindowPrivate()));
	connect(m_ui->actionOpen, SIGNAL(triggered()), this, SLOT(actionOpen()));
	connect(m_ui->actionCloseTab, SIGNAL(triggered()), m_windowsManager, SLOT(close()));
	connect(m_ui->actionManageSessions, SIGNAL(triggered()), this, SLOT(actionManageSessions()));
	connect(m_ui->actionPrint, SIGNAL(triggered()), m_windowsManager, SLOT(print()));
	connect(m_ui->actionPrintPreview, SIGNAL(triggered()), m_windowsManager, SLOT(printPreview()));
	connect(m_ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(m_ui->actionZoomIn, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionZoomOut, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionZoomOriginal, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionReload, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionStop, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionGoBack, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionGoForward, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionRewindBack, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionRewindForward, SIGNAL(triggered()), this, SLOT(triggerWindowAction()));
	connect(m_ui->actionAboutApplication, SIGNAL(triggered()), this, SLOT(actionAboutApplication()));
	connect(m_ui->actionAboutQt, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));
	connect(m_ui->menuSessions, SIGNAL(aboutToShow()), this, SLOT(menuSessionsAboutToShow()));
	connect(m_ui->menuSessions, SIGNAL(triggered(QAction*)), this, SLOT(actionSession(QAction*)));
	connect(m_ui->menuTextEncoding, SIGNAL(aboutToShow()), this, SLOT(menuTextEncodingAboutToShow()));
	connect(m_ui->menuClosedWindows, SIGNAL(aboutToShow()), this, SLOT(menuClosedWindowsAboutToShow()));

	resize(SettingsManager::getValue("Window/size", size()).toSize());
	move(SettingsManager::getValue("Window/position", pos()).toPoint());
	restoreState(SettingsManager::getValue("Window/state", QByteArray()).toByteArray());

	m_ui->panelWidget->hide();
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
	Application *application = qobject_cast<Application*>(QCoreApplication::instance());

	if (application && application->getWindows().count() == 1)
	{
		if (SessionsManager::getCurrentSession() == "default")
		{
			SessionsManager::saveSession();
		}
	}
	else
	{
		SessionsManager::storeClosedWindow(m_windowsManager);
	}

	SettingsManager::setValue("Window/size", size());
	SettingsManager::setValue("Window/position", pos());
	SettingsManager::setValue("Window/state", saveState());

	if (application)
	{
		application->removeWindow(this);
	}

	event->accept();
}

void MainWindow::openUrl(const QUrl &url)
{
	m_windowsManager->open(url);
}

void MainWindow::actionOpen()
{
	const QUrl url = QFileDialog::getOpenFileUrl(this, tr("Open File"));

	if (!url.isEmpty())
	{
		m_windowsManager->open(url);
	}
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
		SessionsManager::restoreSession(action->data().toString());
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

void MainWindow::actionClosedWindows(QAction *action)
{
	const int index = action->data().toInt();

	if (index == 0)
	{
		m_windowsManager->clearClosedWindows();
	}
	else if (index > 0)
	{
		m_windowsManager->restore(index - 1);
	}
	else
	{
		SessionsManager::restoreClosedWindow(-index - 1);
	}
}

void MainWindow::actionAboutApplication()
{
	QMessageBox::about(this, "Otter", QString(tr("<b>Otter %1</b><br>Ultra flexible web browser.").arg(QApplication::applicationVersion())));
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

		QAction *action = m_ui->menuSessions->addAction(sorted.at(i).title.isEmpty() ? tr("(Untitled)") : sorted.at(i).title);
		action->setData(sorted.at(i).path);
		action->setCheckable(true);

		if (sorted.at(i).path == currentSession)
		{
			action->setChecked(true);
		}

		m_sessionsGroup->addAction(action);
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

		if (action && encoding == action->text().toLower())
		{
			action->setChecked(true);

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
	m_ui->menuClosedWindows->clear();

	m_closedWindowsAction->menu()->clear();

	QAction *clearAction = m_ui->menuClosedWindows->addAction(QIcon::fromTheme("edit-clear", QIcon(":/icons/edit-clear.png")), tr("Clear"));
	clearAction->setData(0);

	m_ui->menuClosedWindows->addSeparator();

	const QStringList windows = SessionsManager::getClosedWindows();

	if (!windows.isEmpty())
	{
		for (int i = 0; i < windows.count(); ++i)
		{
			QAction *action = m_ui->menuClosedWindows->addAction(QString("Window - %1").arg(windows.at(i)));
			action->setData(-(i + 1));
		}

		m_ui->menuClosedWindows->addSeparator();
	}

	WebBackend *backend = WebBackendsManager::getBackend();
	const QList<SessionWindow> tabs = m_windowsManager->getClosedWindows();

	for (int i = 0; i < tabs.count(); ++i)
	{
		QAction *action = m_ui->menuClosedWindows->addAction(backend->getIconForUrl(QUrl(tabs.at(i).url())), tabs.at(i).title());
		action->setData(i + 1);
	}

	m_closedWindowsAction->menu()->addActions(m_ui->menuClosedWindows->actions());
}

void MainWindow::triggerWindowAction()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		m_windowsManager->triggerAction(static_cast<WebAction>(action->data().toInt()));
	}
}

void MainWindow::updateClosedWindows()
{
	m_closedWindowsAction->setEnabled(m_windowsManager->getClosedWindows().count() || (SessionsManager::getClosedWindows().count() > 0));
}

bool MainWindow::event(QEvent *event)
{
	if (event->type() == QEvent::WindowActivate)
	{
		ActionsManager::setActiveWindow(this);
	}

	return QMainWindow::event(event);
}

void MainWindow::actionNewTabPrivate()
{
	m_windowsManager->open(QUrl(), true);
}

WindowsManager *MainWindow::getWindowsManager()
{
	return m_windowsManager;
}

}
