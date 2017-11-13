/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "WindowsContentsWidget.h"
#include "../../../core/Application.h"
#include "../../../core/SessionModel.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Action.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/Window.h"

#include "ui_WindowsContentsWidget.h"

#include <QtWidgets/QMenu>

namespace Otter
{

WindowsContentsWidget::WindowsContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_ui(new Ui::WindowsContentsWidget)
{
	m_ui->setupUi(this);

	m_ui->windowsViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->windowsViewWidget->setModel(SessionsManager::getModel());
	m_ui->windowsViewWidget->expandAll();
	m_ui->windowsViewWidget->viewport()->setMouseTracking(true);

	connect(m_ui->windowsViewWidget, &ItemViewWidget::customContextMenuRequested, this, &WindowsContentsWidget::showContextMenu);
	connect(m_ui->windowsViewWidget, &ItemViewWidget::clicked, this, &WindowsContentsWidget::activateWindow);
}

WindowsContentsWidget::~WindowsContentsWidget()
{
	delete m_ui;
}

void WindowsContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void WindowsContentsWidget::print(QPrinter *printer)
{
	m_ui->windowsViewWidget->render(printer);
}

void WindowsContentsWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	switch (identifier)
	{
		case ActionsManager::SelectAllAction:
			m_ui->windowsViewWidget->selectAll();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->windowsViewWidget->setFocus();

			break;
		default:
			ContentsWidget::triggerAction(identifier, parameters);

			break;
	}
}

void WindowsContentsWidget::activateWindow(const QModelIndex &index)
{
	const SessionModel::EntityType type(static_cast<SessionModel::EntityType>(index.data(SessionModel::TypeRole).toInt()));

	switch (type)
	{
		case SessionModel::MainWindowEntity:
			Application::getInstance()->triggerAction(ActionsManager::ActivateWindowAction, {{QLatin1String("window"), index.data(SessionModel::IdentifierRole).toULongLong()}});

			break;
		case SessionModel::WindowEntity:
			{
				const WindowSessionItem *windowItem(static_cast<WindowSessionItem*>(m_ui->windowsViewWidget->getSourceModel()->itemFromIndex(index)));
				const Window *window(windowItem ? windowItem->getActiveWindow() : nullptr);

				if (window)
				{
					const MainWindow *mainWindow(window->getMainWindow());

					if (mainWindow)
					{
						Application::getInstance()->triggerAction(ActionsManager::ActivateWindowAction, {{QLatin1String("window"), mainWindow->getIdentifier()}});
					}

					Application::triggerAction(ActionsManager::ActivateTabAction, {{QLatin1String("tab"), index.data(SessionModel::IdentifierRole).toULongLong()}}, window->getMainWindow());
				}
			}

			break;
		default:
			break;
	}
}

void WindowsContentsWidget::showContextMenu(const QPoint &position)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));
	const QModelIndex index(m_ui->windowsViewWidget->indexAt(position));
	SessionModel::EntityType type(static_cast<SessionModel::EntityType>(index.data(SessionModel::TypeRole).toInt()));
	ActionExecutor::Object executor(mainWindow, mainWindow);
	QMenu menu(this);
	menu.addAction(new Action(ActionsManager::NewWindowAction, {}, executor, &menu));
	menu.addAction(new Action(ActionsManager::NewWindowPrivateAction, {}, executor, &menu));

	if (!index.data(SessionModel::IsTrashedRole).toBool())
	{
		if (type == SessionModel::MainWindowEntity)
		{
			const MainWindowSessionItem *mainWindowItem(static_cast<MainWindowSessionItem*>(SessionsManager::getModel()->itemFromIndex(index)));

			if (mainWindowItem)
			{
				executor = ActionExecutor::Object(mainWindowItem->getMainWindow(), mainWindowItem->getMainWindow());

				menu.addAction(new Action(ActionsManager::NewTabAction, {}, executor, &menu));
				menu.addAction(new Action(ActionsManager::NewTabPrivateAction, {}, executor, &menu));
				menu.addSeparator();
				menu.addAction(new Action(ActionsManager::CloseWindowAction, {}, executor, &menu));
			}
		}
		else if (type == SessionModel::WindowEntity)
		{
			const WindowSessionItem *windowItem(static_cast<WindowSessionItem*>(SessionsManager::getModel()->itemFromIndex(index)));

			if (windowItem)
			{
				executor = ActionExecutor::Object(windowItem->getActiveWindow()->getMainWindow(), windowItem->getActiveWindow()->getMainWindow());

				menu.addAction(new Action(ActionsManager::NewTabAction, {}, executor, &menu));
				menu.addAction(new Action(ActionsManager::NewTabPrivateAction, {}, executor, &menu));
				menu.addSeparator();
				menu.addAction(new Action(ActionsManager::CloseTabAction, {}, ActionExecutor::Object(windowItem->getActiveWindow(), windowItem->getActiveWindow()), &menu));
			}
		}
	}

	menu.exec(m_ui->windowsViewWidget->mapToGlobal(position));
}

QString WindowsContentsWidget::getTitle() const
{
	return tr("Windows");
}

QLatin1String WindowsContentsWidget::getType() const
{
	return QLatin1String("windows");
}

QUrl WindowsContentsWidget::getUrl() const
{
	return QUrl(QLatin1String("about:windows"));
}

QIcon WindowsContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("tab"), false);
}

}
