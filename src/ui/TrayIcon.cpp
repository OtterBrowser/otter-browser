/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "TrayIcon.h"
#include "MainWindow.h"
#include "Menu.h"
#include "../core/ActionsManager.h"
#include "../core/Application.h"
#include "../core/NotificationsManager.h"

#include <QtCore/QTimer>

namespace Otter
{

TrayIcon::TrayIcon(Application *parent) : QObject(parent),
	m_trayIcon(new QSystemTrayIcon(this)),
	m_autoHideTimer(0)
{
	Menu *menu(new Menu());
	menu->addAction()->setOverrideText(QT_TRANSLATE_NOOP("actions", "Show Windows"));
	menu->addSeparator();
	menu->addAction(ActionsManager::NewTabAction);
	menu->addAction(ActionsManager::NewTabPrivateAction);
	menu->addSeparator();
	menu->addAction(ActionsManager::BookmarksAction)->setOverrideText(QT_TRANSLATE_NOOP("actions", "Bookmarks"));
	menu->addAction(ActionsManager::TransfersAction)->setOverrideText(QT_TRANSLATE_NOOP("actions", "Transfers"));
	menu->addAction(ActionsManager::HistoryAction)->setOverrideText(QT_TRANSLATE_NOOP("actions", "History"));
	menu->addAction(ActionsManager::NotesAction)->setOverrideText(QT_TRANSLATE_NOOP("actions", "Notes"));
	menu->addSeparator();
	menu->addAction(ActionsManager::ExitAction);

	m_trayIcon->setIcon(parent->windowIcon());
	m_trayIcon->setContextMenu(menu);
	m_trayIcon->setToolTip(tr("Otter Browser"));
	m_trayIcon->show();

	setParent(nullptr);

	connect(Application::getInstance(), SIGNAL(aboutToQuit()), this, SLOT(hide()));
	connect(this, SIGNAL(destroyed()), menu, SLOT(deleteLater()));
	connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
	connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(triggerAction(QAction*)));
	connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
	connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activated(QSystemTrayIcon::ActivationReason)));
}

void TrayIcon::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_autoHideTimer)
	{
		killTimer(m_autoHideTimer);

		m_autoHideTimer = 0;

		messageIgnored();
	}
}

void TrayIcon::activated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger)
	{
		Application *application(Application::getInstance());
		application->setHidden(!application->isHidden());
	}
}

void TrayIcon::triggerAction(QAction *action)
{
	Action *actionObject(qobject_cast<Action*>(action));

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
		MainWindow *window(Application::getActiveWindow());

		if (window)
		{
			window->triggerAction(actionObject->getIdentifier());
		}
	}
}

void TrayIcon::hide()
{
	m_trayIcon->hide();

	Application::getInstance()->processEvents();
}

void TrayIcon::updateMenu()
{
	m_trayIcon->contextMenu()->actions().at(0)->setText(Application::getInstance()->isHidden() ? tr("Show Windows") : tr("Hide Windows"));
}

void TrayIcon::showMessage(Notification *notification)
{
	m_notification = notification;

	connect(m_trayIcon, SIGNAL(messageClicked()), this, SLOT(messageClicked()));

	m_trayIcon->showMessage(tr("Otter Browser"), notification->getMessage(), QSystemTrayIcon::MessageIcon(m_notification->getLevel() + 1));

	const int visibilityDuration(SettingsManager::getValue(SettingsManager::Interface_NotificationVisibilityDurationOption).toInt());

	if (visibilityDuration > 0)
	{
		m_autoHideTimer = startTimer(visibilityDuration * 1000);
	}
}

void TrayIcon::messageClicked()
{
	disconnect(m_trayIcon, SIGNAL(messageClicked()), this, SLOT(messageClicked()));

	if (m_autoHideTimer != 0)
	{
		killTimer(m_autoHideTimer);

		m_autoHideTimer = 0;
	}

	m_notification->markClicked();
}

void TrayIcon::messageIgnored()
{
	disconnect(m_trayIcon, SIGNAL(messageClicked()), this, SLOT(messageClicked()));

	m_notification->markIgnored();
}

}
