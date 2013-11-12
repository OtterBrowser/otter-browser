#include "MainWindow.h"
#include "TabBarWidget.h"
#include "../core/ActionsManager.h"
#include "../core/SettingsManager.h"
#include "../core/WindowsManager.h"

#include "ui_MainWindow.h"

#include <QtGui/QCloseEvent>
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
	ActionsManager::registerAction(this, m_ui->actionNewTab);
	ActionsManager::registerAction(this, m_ui->actionNewPrivateTab);
	ActionsManager::registerAction(this, m_ui->actionNewWindow);
	ActionsManager::registerAction(this, m_ui->actionNewPrivateWindow);
	ActionsManager::registerAction(this, m_ui->actionOpen);
	ActionsManager::registerAction(this, m_ui->actionCloseTab);
	ActionsManager::registerAction(this, m_ui->actionSave);
	ActionsManager::registerAction(this, m_ui->actionPrint);
	ActionsManager::registerAction(this, m_ui->actionPrintPreview);
	ActionsManager::registerAction(this, m_ui->actionExit);
	ActionsManager::registerAction(this, m_ui->actionUndo);
	ActionsManager::registerAction(this, m_ui->actionRedo);
	ActionsManager::registerAction(this, m_ui->actionCut);
	ActionsManager::registerAction(this, m_ui->actionCopy);
	ActionsManager::registerAction(this, m_ui->actionPaste);
	ActionsManager::registerAction(this, m_ui->actionSelectAll);
	ActionsManager::registerAction(this, m_ui->actionFind);
	ActionsManager::registerAction(this, m_ui->actionZoomIn);
	ActionsManager::registerAction(this, m_ui->actionZoomOut);
	ActionsManager::registerAction(this, m_ui->actionZoomOriginal);
	ActionsManager::registerAction(this, m_ui->actionAboutApplication);
	ActionsManager::registerAction(this, m_ui->actionAboutQt);

	m_ui->actionNewTab->setIcon(QIcon::fromTheme("tab-new", QIcon(":/icons/tab-new.png")));
	m_ui->actionNewPrivateTab->setIcon(QIcon::fromTheme("tab-new-private", QIcon(":/icons/tab-new-private.png")));
	m_ui->actionNewWindow->setIcon(QIcon::fromTheme("window-new", QIcon(":/icons/window-new.png")));
	m_ui->actionNewPrivateWindow->setIcon(QIcon::fromTheme("window-new-private", QIcon(":/icons/window-new-private.png")));
	m_ui->actionOpen->setIcon(QIcon::fromTheme("document-open", QIcon(":/icons/document-open.png")));
	m_ui->actionCloseTab->setIcon(QIcon::fromTheme("tab-close", QIcon(":/icons/tab-close.png")));
	m_ui->actionSave->setIcon(QIcon::fromTheme("document-save", QIcon(":/icons/document-save.png")));
	m_ui->actionPrint->setIcon(QIcon::fromTheme("document-print", QIcon(":/icons/document-print.png")));
	m_ui->actionPrintPreview->setIcon(QIcon::fromTheme("document-print-preview", QIcon(":/icons/document-print-preview.png")));
	m_ui->actionExit->setIcon(QIcon::fromTheme("application-exit", QIcon(":/icons/application-exit.png")));
	m_ui->actionUndo->setIcon(QIcon::fromTheme("edit-undo", QIcon(":/icons/edit-undo.png")));
	m_ui->actionRedo->setIcon(QIcon::fromTheme("edit-redo", QIcon(":/icons/edit-redo.png")));
	m_ui->actionCut->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/icons/edit-cut.png")));
	m_ui->actionCopy->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/icons/edit-copy.png")));
	m_ui->actionPaste->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/icons/edit-paste.png")));
	m_ui->actionSelectAll->setIcon(QIcon::fromTheme("edit-select-all", QIcon(":/icons/edit-select-all.png")));
	m_ui->actionFind->setIcon(QIcon::fromTheme("edit-find", QIcon(":/icons/edit-find.png")));
	m_ui->actionZoomIn->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/icons/zoom-in.png")));
	m_ui->actionZoomOut->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/icons/zoom-out.png")));
	m_ui->actionZoomOriginal->setIcon(QIcon::fromTheme("zoom-original", QIcon(":/icons/zoom-original.png")));
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
	connect(m_ui->actionCloseTab, SIGNAL(triggered()), m_windowsManager, SLOT(close()));
	connect(m_ui->actionPrint, SIGNAL(triggered()), m_windowsManager, SLOT(print()));
	connect(m_ui->actionPrintPreview, SIGNAL(triggered()), m_windowsManager, SLOT(printPreview()));
	connect(m_ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(m_ui->actionZoomIn, SIGNAL(triggered()), m_windowsManager, SLOT(zoomIn()));
	connect(m_ui->actionZoomOut, SIGNAL(triggered()), m_windowsManager, SLOT(zoomOut()));
	connect(m_ui->actionZoomOriginal, SIGNAL(triggered()), m_windowsManager, SLOT(zoomOriginal()));
	connect(m_ui->actionAboutApplication, SIGNAL(triggered()), this, SLOT(actionAboutApplication()));
	connect(m_ui->actionAboutQt, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));
}

MainWindow::~MainWindow()
{
	delete m_ui;
}

void MainWindow::openUrl(const QUrl &url)
{
	m_windowsManager->open(url);
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

}
