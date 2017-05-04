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

WindowsContentsWidget::WindowsContentsWidget(const QVariantMap &parameters, Window *window) : ContentsWidget(parameters, window),
	m_isLoading(true),
	m_ui(new Ui::WindowsContentsWidget)
{
	m_ui->setupUi(this);

	m_ui->windowsViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->windowsViewWidget->setModel(SessionsManager::getModel());
	m_ui->windowsViewWidget->expandAll();
	m_ui->windowsViewWidget->viewport()->setMouseTracking(true);

	connect(m_ui->windowsViewWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
}

WindowsContentsWidget::~WindowsContentsWidget()
{
	delete m_ui;
}

void WindowsContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

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
	Q_UNUSED(parameters)

	switch (identifier)
	{
		case ActionsManager::SelectAllAction:
			m_ui->windowsViewWidget->selectAll();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->windowsViewWidget->setFocus();

			break;
		default:
			break;
	}
}

void WindowsContentsWidget::showContextMenu(const QPoint &position)
{
	const QModelIndex index(m_ui->windowsViewWidget->indexAt(position));
	SessionModel::EntityType type(static_cast<SessionModel::EntityType>(index.data(SessionModel::TypeRole).toInt()));
	QMenu menu(this);
	menu.addAction(Application::createAction(ActionsManager::NewWindowAction, QVariantMap(), true, this));
	menu.addAction(Application::createAction(ActionsManager::NewWindowPrivateAction, QVariantMap(), true, this));

	if (!index.data(SessionModel::IsTrashedRole).toBool())
	{
		if (type == SessionModel::MainWindowEntity)
		{
			MainWindowSessionItem *mainWindowItem(dynamic_cast<MainWindowSessionItem*>(SessionsManager::getModel()->itemFromIndex(index)));

			if (mainWindowItem)
			{
				menu.addAction(mainWindowItem->getMainWindow()->createAction(ActionsManager::NewTabAction));
				menu.addAction(mainWindowItem->getMainWindow()->createAction(ActionsManager::NewTabPrivateAction));
				menu.addSeparator();
				menu.addAction(mainWindowItem->getMainWindow()->createAction(ActionsManager::CloseWindowAction));
			}
		}
		else if (type == SessionModel::WindowEntity)
		{
			WindowSessionItem *windowItem(dynamic_cast<WindowSessionItem*>(SessionsManager::getModel()->itemFromIndex(index)));

			if (windowItem)
			{
				Action *closeTabAction(new Action(ActionsManager::CloseTabAction, &menu));
				closeTabAction->setEnabled(!index.data(SessionModel::IsPinnedRole).toBool());
				closeTabAction->setParameters({{QLatin1String("window"), index.data(SessionModel::IdentifierRole).toULongLong()}});

				menu.addAction(Application::createAction(ActionsManager::NewTabAction, QVariantMap(), true, windowItem->getActiveWindow()->getMainWindow()));
				menu.addAction(Application::createAction(ActionsManager::NewTabPrivateAction, QVariantMap(), true, windowItem->getActiveWindow()->getMainWindow()));
				menu.addSeparator();
				menu.addAction(closeTabAction);

				connect(closeTabAction, SIGNAL(triggered()), windowItem->getActiveWindow()->getMainWindow(), SLOT(triggerAction()));
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
	return ThemesManager::createIcon(QLatin1String("window"), false);
}

WebWidget::LoadingState WindowsContentsWidget::getLoadingState() const
{
	return WebWidget::FinishedLoadingState;
}

}
