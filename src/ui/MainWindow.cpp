#include "MainWindow.h"
#include "TabBarWidget.h"
#include "../core/ActionsManager.h"
#include "../core/SettingsManager.h"
#include "../core/WindowsManager.h"

#include "ui_MainWindow.h"

#include <QtGui/QCloseEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMessageBox>

namespace Otter
{

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
	m_windowsManager(NULL),
	m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);

	TabBarWidget *tabBar = new TabBarWidget(m_ui->tabsWidgetContents);

	m_ui->tabsWidgetContents->layout()->addWidget(tabBar);
	m_ui->tabsWidget->setTitleBarWidget(NULL);

	ActionsManager::registerWindow(this);
	ActionsManager::registerActions(this, m_ui->menuFile->actions());
	ActionsManager::registerActions(this, m_ui->menuEdit->actions());
	ActionsManager::registerActions(this, m_ui->menuView->actions());
	ActionsManager::registerActions(this, m_ui->menuHistory->actions());
	ActionsManager::registerActions(this, m_ui->menuBookmarks->actions());
	ActionsManager::registerActions(this, m_ui->menuTools->actions());
	ActionsManager::registerActions(this, m_ui->menuHelp->actions());

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
	m_ui->actionStop->setIcon(QIcon::fromTheme("process-stop", QIcon(":/icons/process-stop.png")));
	m_ui->actionZoomIn->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/icons/zoom-in.png")));
	m_ui->actionZoomOut->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/icons/zoom-out.png")));
	m_ui->actionZoomOriginal->setIcon(QIcon::fromTheme("zoom-original", QIcon(":/icons/zoom-original.png")));
	m_ui->actionBack->setIcon(QIcon::fromTheme("go-previous", QIcon(":/icons/go-previous.png")));
	m_ui->actionForward->setIcon(QIcon::fromTheme("go-next", QIcon(":/icons/go-next.png")));
	m_ui->actionViewHistory->setIcon(QIcon::fromTheme("view-history", QIcon(":/icons/view-history.png")));
	m_ui->actionClearHistory->setIcon(QIcon::fromTheme("edit-clear-history", QIcon(":/icons/edit-clear-history.png")));
	m_ui->actionAboutApplication->setIcon(QIcon(":/icons/otter.png"));
	m_ui->actionAboutQt->setIcon(QIcon(":/icons/qt.png"));

	resize(SettingsManager::getValue("Window/size", size()).toSize());
	move(SettingsManager::getValue("Window/position", pos()).toPoint());
	restoreState(SettingsManager::getValue("Window/state", QByteArray()).toByteArray());

	m_windowsManager = new WindowsManager(m_ui->mdiArea, tabBar);

	setWindowTitle(m_windowsManager->getTitle());

	m_ui->panelWidget->hide();

	connect(m_windowsManager, SIGNAL(windowTitleChanged(QString)), this, SLOT(setWindowTitle(QString)));
	connect(m_ui->actionNewTab, SIGNAL(triggered()), m_windowsManager, SLOT(open()));
	connect(m_ui->actionNewTabPrivate, SIGNAL(triggered()), this, SLOT(actionNewTabPrivate()));
	connect(m_ui->actionOpen, SIGNAL(triggered()), this, SLOT(actionOpen()));
	connect(m_ui->actionCloseTab, SIGNAL(triggered()), m_windowsManager, SLOT(close()));
	connect(m_ui->actionPrint, SIGNAL(triggered()), m_windowsManager, SLOT(print()));
	connect(m_ui->actionPrintPreview, SIGNAL(triggered()), m_windowsManager, SLOT(printPreview()));
	connect(m_ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(m_ui->actionZoomIn, SIGNAL(triggered()), m_windowsManager, SLOT(zoomIn()));
	connect(m_ui->actionZoomOut, SIGNAL(triggered()), m_windowsManager, SLOT(zoomOut()));
	connect(m_ui->actionZoomOriginal, SIGNAL(triggered()), m_windowsManager, SLOT(zoomOriginal()));
	connect(m_ui->actionReload, SIGNAL(triggered()), m_windowsManager, SLOT(reload()));
	connect(m_ui->actionStop, SIGNAL(triggered()), m_windowsManager, SLOT(stop()));
	connect(m_ui->actionBack, SIGNAL(triggered()), m_windowsManager, SLOT(goBack()));
	connect(m_ui->actionForward, SIGNAL(triggered()), m_windowsManager, SLOT(goForward()));
	connect(m_ui->actionAboutApplication, SIGNAL(triggered()), this, SLOT(actionAboutApplication()));
	connect(m_ui->actionAboutQt, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));
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
	SettingsManager::setValue("Window/size", size());
	SettingsManager::setValue("Window/position", pos());
	SettingsManager::setValue("Window/state", saveState());

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

void MainWindow::actionAboutApplication()
{
	QMessageBox::about(this, "Otter", QString(tr("<b>Otter %1</b><br>Ultra flexible web browser.").arg(QApplication::applicationVersion())));
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

}
