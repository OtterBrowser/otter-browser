/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "Notification.h"
#include "Application.h"
#include "PlatformIntegration.h"
#include "SessionsManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtMultimedia/QSoundEffect>

namespace Otter
{

Notification::Notification(const QString &message, NotificationLevel level, int event, QObject *parent) : QObject(parent),
	m_message(message),
	m_level(level),
	m_event(event)
{
}

void Notification::markClicked()
{
	emit clicked();

	deleteLater();
}

void Notification::markIgnored()
{
	emit ignored();

	deleteLater();
}

QString Notification::getMessage() const
{
	return m_message;
}

Notification::NotificationLevel Notification::getLevel() const
{
	return m_level;
}

Notification* Notification::createNotification(int event, const QString &message, NotificationLevel level, QObject *parent)
{
	QString eventName;

	if (event == TransferCompletedNotification)
	{
		eventName = QLatin1String("TransferCompleted");
	}

	Notification *notification = new Notification(message, level, event, Application::getInstance());
	QSettings notificationSettings(SessionsManager::getReadableDataPath(QLatin1String("notifications.ini")), QSettings::IniFormat);
	const QString playSound = notificationSettings.value(eventName + QLatin1String("/playSound"), QString()).toString();

	if (!playSound.isEmpty())
	{
		QSoundEffect effect;
		effect.setSource(QUrl::fromLocalFile(playSound));
		effect.setLoopCount(1);
		effect.setVolume(0.5);
		effect.play();
	}

	if (notificationSettings.value(eventName + QLatin1String("/showAlert"), false).toBool())
	{
		Application::getInstance()->alert(MainWindow::findMainWindow(parent));
	}

	if (notificationSettings.value(eventName + QLatin1String("/showNotification"), false).toBool())
	{
		Application::getInstance()->showNotification(notification);
	}

	return notification;
}

}

