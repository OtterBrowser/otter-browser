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

#include "TrayIcon.h"
#include "MainWindow.h"
#include "Menu.h"
#include "../core/ActionsManager.h"
#include "../core/Application.h"

namespace Otter
{

TrayIcon::TrayIcon(Application *parent) : QObject(parent),
	m_icon(new QSystemTrayIcon(this))
{
	Menu *menu = new Menu();
	menu->addAction(-1)->setOverrideText(QT_TRANSLATE_NOOP("actions", "Show Windows"));
	menu->addSeparator();
	menu->addAction(Action::NewTabAction);
	menu->addAction(Action::NewTabPrivateAction);
	menu->addSeparator();
	menu->addAction(Action::BookmarksAction)->setOverrideText(QT_TRANSLATE_NOOP("actions", "Bookmarks"));
	menu->addAction(Action::TransfersAction)->setOverrideText(QT_TRANSLATE_NOOP("actions", "Transfers"));
	menu->addAction(Action::HistoryAction)->setOverrideText(QT_TRANSLATE_NOOP("actions", "History"));
	menu->addSeparator();
	menu->addAction(Action::ExitAction);

	m_icon->setIcon(parent->windowIcon());
	m_icon->setContextMenu(menu);
	m_icon->setToolTip(tr("Otter Browser"));
	m_icon->show();

	setParent(NULL);

	connect(Application::getInstance(), SIGNAL(aboutToQuit()), this, SLOT(hide()));
	connect(this, SIGNAL(destroyed()), menu, SLOT(deleteLater()));
	connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
	connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(triggerAction(QAction*)));
	connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
	connect(m_icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activated(QSystemTrayIcon::ActivationReason)));
}

void TrayIcon::activated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger)
	{
		Application *application = Application::getInstance();
		application->setHidden(!application->isHidden());
	}
}

void TrayIcon::triggerAction(QAction *action)
{
	Action *actionObject = qobject_cast<Action*>(action);

	if (!actionObject)
	{
		return;
	}

	if (actionObject->getIdentifier() < 0)
	{
		activated(QSystemTrayIcon::Trigger);
	}
	else
	{
		ActionsManager::triggerAction(actionObject->getIdentifier(), NULL);
	}
}

void TrayIcon::hide()
{
	m_icon->hide();

	Application::getInstance()->processEvents();
}

void TrayIcon::updateMenu()
{
	m_icon->contextMenu()->actions().at(0)->setText(Application::getInstance()->isHidden() ? tr("Show Windows") : tr("Hide Windows"));
}

}
