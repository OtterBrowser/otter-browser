#include "MainWindow.h"
#include "TabBarWidget.h"
#include "../core/ActionsManager.h"
#include "../core/SettingsManager.h"
#include "../core/WindowsManager.h"

#include "ui_MainWindow.h"

#include <QtGui/QCloseEvent>
#include <QtWidgets/QMdiSubWindow>

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

	m_windowsManager = new WindowsManager(m_ui->mdiArea, tabBar);

	resize(SettingsManager::getValue("Window/size", size()).toSize());
	move(SettingsManager::getValue("Window/position", pos()).toPoint());
	restoreState(SettingsManager::getValue("Window/state", QByteArray()).toByteArray());
	setWindowTitle(m_windowsManager->getTitle());

	m_ui->panelWidget->hide();

	connect(m_windowsManager, SIGNAL(windowTitleChanged(QString)), this, SLOT(setWindowTitle(QString)));
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

bool MainWindow::event(QEvent *event)
{
	if (event->type() == QEvent::WindowActivate)
	{
		ActionsManager::setActiveWindow(this);
	}

	return QMainWindow::event(event);
}

}
