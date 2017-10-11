/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Action.h"
#include "MainWindow.h"
#include "Menu.h"
#include "../core/Application.h"
#include "../core/NotificationsManager.h"

#include <QtCore/QTimer>

namespace Otter
{

TrayIcon::TrayIcon(Application *parent) : QObject(parent),
	m_trayIcon(new QSystemTrayIcon(this)),
	m_autoHideTimer(0)
{
	const QVector<int> actions({-1, ActionsManager::NewTabAction, ActionsManager::NewTabPrivateAction, -1, ActionsManager::BookmarksAction, ActionsManager::TransfersAction, ActionsManager::HistoryAction, ActionsManager::NotesAction, -1, ActionsManager::ExitAction});
	ActionExecutor::Object executor(Application::getInstance(), Application::getInstance());
	Menu *menu(new Menu());
	menu->addAction(tr("Show Windows"), this, SLOT(handleActivated()));

	for (int i = 0; i < actions.count(); ++i)
	{
		if (actions.at(i) < 0)
		{
			menu->addSeparator();
		}
		else
		{
			Action *action(new Action(actions.at(i), {}, executor, menu));

			switch (actions.at(i))
			{
				case ActionsManager::BookmarksAction:
					action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Bookmarks"));

					break;
				case ActionsManager::TransfersAction:
					action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Transfers"));

					break;
				case ActionsManager::HistoryAction:
					action->setOverrideText(QT_TRANSLATE_NOOP("actions", "History"));

					break;
				case ActionsManager::NotesAction:
					action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Notes"));

					break;
				default:
					break;
			}

			menu->addAction(action);
		}
	}

	m_trayIcon->setIcon(parent->windowIcon());
	m_trayIcon->setContextMenu(menu);
	m_trayIcon->setToolTip(tr("Otter Browser"));
	m_trayIcon->show();

	setParent(nullptr);

	connect(Application::getInstance(), SIGNAL(aboutToQuit()), this, SLOT(hide()));
	connect(this, SIGNAL(destroyed()), menu, SLOT(deleteLater()));
	connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
	connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
	connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(handleActivated(QSystemTrayIcon::ActivationReason)));
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

void TrayIcon::handleActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger)
	{
		Application *application(Application::getInstance());
		application->setHidden(!application->isHidden());
	}
}

void TrayIcon::hide()
{
	m_trayIcon->hide();

	Application::getInstance()->processEvents();
}

void TrayIcon::updateMenu()
{
	if (!m_trayIcon->contextMenu()->actions().isEmpty())
	{
		m_trayIcon->contextMenu()->actions().at(0)->setText(Application::isHidden() ? tr("Show Windows") : tr("Hide Windows"));
	}
}

void TrayIcon::showMessage(Notification *notification)
{
	m_notification = notification;

	connect(m_trayIcon, SIGNAL(messageClicked()), this, SLOT(messageClicked()));

	m_trayIcon->showMessage(tr("Otter Browser"), notification->getMessage(), QSystemTrayIcon::MessageIcon(m_notification->getLevel() + 1));

	const int visibilityDuration(SettingsManager::getOption(SettingsManager::Interface_NotificationVisibilityDurationOption).toInt());

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

	m_notification->markAsClicked();
}

void TrayIcon::messageIgnored()
{
	disconnect(m_trayIcon, SIGNAL(messageClicked()), this, SLOT(messageClicked()));

	m_notification->markAsIgnored();
}

}
