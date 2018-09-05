/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
	menu->addAction(tr("Show Windows"), this, &TrayIcon::toggleWindowsVisibility);

	for (int i = 0; i < actions.count(); ++i)
	{
		if (actions.at(i) < 0)
		{
			menu->addSeparator();
		}
		else
		{
			switch (actions.at(i))
			{
				case ActionsManager::BookmarksAction:
					menu->addAction(new Action(actions.at(i), {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Bookmarks")}}, executor, menu));

					break;
				case ActionsManager::TransfersAction:
					menu->addAction(new Action(actions.at(i), {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Transfers")}}, executor, menu));

					break;
				case ActionsManager::HistoryAction:
					menu->addAction(new Action(actions.at(i), {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "History")}}, executor, menu));

					break;
				case ActionsManager::NotesAction:
					menu->addAction(new Action(actions.at(i), {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Notes")}}, executor, menu));

					break;
				default:
					menu->addAction(new Action(actions.at(i), {}, executor, menu));

					break;
			}
		}
	}

	m_trayIcon->setIcon(parent->windowIcon());
	m_trayIcon->setContextMenu(menu);
	m_trayIcon->setToolTip(tr("Otter Browser"));
	m_trayIcon->show();

	setParent(nullptr);

	connect(Application::getInstance(), &Application::aboutToQuit, this, &TrayIcon::hide);
	connect(this, &TrayIcon::destroyed, menu, &Menu::deleteLater);
	connect(parent, &TrayIcon::destroyed, this, &TrayIcon::deleteLater);
	connect(menu, &Menu::aboutToShow, this, &TrayIcon::updateMenu);
	connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayIcon::handleTrayIconActivated);
}

void TrayIcon::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_autoHideTimer)
	{
		killTimer(m_autoHideTimer);

		m_autoHideTimer = 0;

		handleMessageIgnored();
	}
}

void TrayIcon::hide()
{
	m_trayIcon->hide();

	Application::getInstance()->processEvents();
}

void TrayIcon::toggleWindowsVisibility()
{
	Application::setHidden(!Application::isHidden());
}

void TrayIcon::handleTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger)
	{
		toggleWindowsVisibility();
	}
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

void TrayIcon::handleMessageIgnored()
{
	disconnect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &TrayIcon::handleMessageClicked);

	m_notification->markAsIgnored();
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

	connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &TrayIcon::handleMessageClicked);

	QSystemTrayIcon::MessageIcon icon(QSystemTrayIcon::NoIcon);

	switch (notification->getLevel())
	{
		case Notification::ErrorLevel:
			icon = QSystemTrayIcon::Critical;

			break;
		case Notification::WarningLevel:
			icon = QSystemTrayIcon::Warning;

			break;
		default:
			icon = QSystemTrayIcon::Information;

			break;
	}

	m_trayIcon->showMessage(tr("Otter Browser"), notification->getMessage(), icon);

	const int visibilityDuration(SettingsManager::getOption(SettingsManager::Interface_NotificationVisibilityDurationOption).toInt());

	if (visibilityDuration > 0)
	{
		m_autoHideTimer = startTimer(visibilityDuration * 1000);
	}
}

}
