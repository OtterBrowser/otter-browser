/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Menu.h"
#include "../core/Application.h"

namespace Otter
{

TrayIcon::TrayIcon(Application *parent) : QObject(parent),
	m_trayIcon(new QSystemTrayIcon(this)),
	m_autoHideTimer(0)
{
	const QVector<int> actions({-1, ActionsManager::NewTabAction, ActionsManager::NewTabPrivateAction, -1, ActionsManager::BookmarksAction, ActionsManager::TransfersAction, ActionsManager::HistoryAction, ActionsManager::NotesAction, -1, ActionsManager::ExitAction});
	const ActionExecutor::Object executor(Application::getInstance(), Application::getInstance());
	Menu *menu(new Menu());
	menu->addAction(tr("Show Windows"), this, &TrayIcon::toggleWindowsVisibility);

	for (int i = 0; i < actions.count(); ++i)
	{
		const int identifier(actions.at(i));

		if (identifier < 0)
		{
			menu->addSeparator();

			continue;
		}

		Action *action(new Action(identifier, {}, executor, menu));

		switch (identifier)
		{
			case ActionsManager::BookmarksAction:
				action->setTextOverride(QT_TRANSLATE_NOOP("actions", "Bookmarks"));

				break;
			case ActionsManager::HistoryAction:
				action->setTextOverride(QT_TRANSLATE_NOOP("actions", "History"));

				break;
			case ActionsManager::NotesAction:
				action->setTextOverride(QT_TRANSLATE_NOOP("actions", "Notes"));

				break;
			default:
				break;
		}

		menu->addAction(action);
	}

	m_trayIcon->setIcon(Application::windowIcon());
	m_trayIcon->setContextMenu(menu);
	m_trayIcon->setToolTip(tr("Otter Browser"));
	m_trayIcon->show();

	setParent(nullptr);

	connect(Application::getInstance(), &Application::aboutToQuit, this, &TrayIcon::hide);
	connect(this, &TrayIcon::destroyed, menu, &Menu::deleteLater);
	connect(parent, &TrayIcon::destroyed, this, &TrayIcon::deleteLater);
	connect(menu, &Menu::aboutToShow, this, [&]()
	{
		const QList<QAction*> actions(m_trayIcon->contextMenu()->actions());

		if (!actions.isEmpty())
		{
			actions.at(0)->setText(Application::isHidden() ? tr("Show Windows") : tr("Hide Windows"));
		}
	});
	connect(m_trayIcon, &QSystemTrayIcon::activated, this, [&](QSystemTrayIcon::ActivationReason reason)
	{
		if (reason == QSystemTrayIcon::Trigger)
		{
			toggleWindowsVisibility();
		}
	});
}

void TrayIcon::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_autoHideTimer)
	{
		killTimer(m_autoHideTimer);

		m_autoHideTimer = 0;

		disconnect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &TrayIcon::handleMessageClicked);

		m_notification->markAsIgnored();
	}
}

void TrayIcon::hide()
{
	m_trayIcon->hide();

	Application::processEvents();
}

void TrayIcon::toggleWindowsVisibility()
{
	Application::setHidden(!Application::isHidden());
}

void TrayIcon::handleMessageClicked()
{
	disconnect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &TrayIcon::handleMessageClicked);

	if (m_autoHideTimer != 0)
	{
		killTimer(m_autoHideTimer);

		m_autoHideTimer = 0;
	}

	m_notification->markAsClicked();
}

void TrayIcon::showMessage(const Notification::Message &message)
{
	m_trayIcon->showMessage(message.getTitle(), message.message, message.getIcon());

	const int visibilityDuration(SettingsManager::getOption(SettingsManager::Interface_NotificationVisibilityDurationOption).toInt());

	if (visibilityDuration > 0)
	{
		if (m_autoHideTimer > 0)
		{
			killTimer(m_autoHideTimer);
		}

		m_autoHideTimer = startTimer(visibilityDuration * 1000);
	}
}

void TrayIcon::showNotification(Notification *notification)
{
	m_notification = notification;

	notification->markAsShown();

	connect(notification, &Notification::modified, this, [&]()
	{
		if (notification == m_notification)
		{
			showMessage(m_notification->getMessage());
		}
	});
	connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &TrayIcon::handleMessageClicked);

	showMessage(notification->getMessage());
}

bool TrayIcon::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		m_trayIcon->setToolTip(tr("Otter Browser"));
	}

	return QObject::event(event);
}

}
