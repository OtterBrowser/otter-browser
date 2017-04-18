/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "NotificationsManager.h"
#include "Application.h"
#include "PlatformIntegration.h"
#include "SessionsManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QFile>
#include <QtCore/QMetaEnum>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtMultimedia/QSoundEffect>

namespace Otter
{

NotificationsManager* NotificationsManager::m_instance(nullptr);
QMap<int, QString> NotificationsManager::m_identifiers;
QVector<NotificationsManager::EventDefinition> NotificationsManager::m_definitions;
int NotificationsManager::m_eventIdentifierEnumerator(0);

Notification::Notification(const QString &message, NotificationLevel level, int event, QObject *parent) : QObject(parent),
	m_message(message),
	m_creationTime(QDateTime::currentDateTime()),
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

void Notification::setData(const QVariant &data)
{
	m_data = data;
}

QString Notification::getMessage() const
{
	return m_message;
}

QDateTime Notification::getCreationTime() const
{
	return m_creationTime;
}

QVariant Notification::getData() const
{
	return m_data;
}

Notification::NotificationLevel Notification::getLevel() const
{
	return m_level;
}

NotificationsManager::NotificationsManager(QObject *parent) : QObject(parent)
{
	registerEvent(QT_TRANSLATE_NOOP("notifications", "Transfer Completed"), QT_TRANSLATE_NOOP("notifications", "File transfer was completed"));
	registerEvent(QT_TRANSLATE_NOOP("notifications", "Update Available"), QT_TRANSLATE_NOOP("notifications", "Update is available to be downloaded"));
}

void NotificationsManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new NotificationsManager(parent);
		m_eventIdentifierEnumerator = m_instance->metaObject()->indexOfEnumerator(QLatin1String("EventIdentifier").data());
	}
}

Notification* NotificationsManager::createNotification(int event, const QString &message, Notification::NotificationLevel level, QObject *parent)
{
	Notification *notification(new Notification(message, level, event, Application::getInstance()));
	const EventDefinition definition(getEventDefinition(event));

	if (!definition.playSound.isEmpty())
	{
		QSoundEffect *effect(new QSoundEffect(m_instance));
		effect->setSource(QUrl::fromLocalFile(definition.playSound));
		effect->setLoopCount(1);
		effect->setVolume(0.5);

		if (effect->status() == QSoundEffect::Error)
		{
			effect->deleteLater();
		}
		else
		{
			QTimer::singleShot(10000, effect, SLOT(deleteLater()));

			effect->play();
		}
	}

	if (definition.showAlert)
	{
		Application::getInstance()->alert(MainWindow::findMainWindow(parent));
	}

	if (definition.showNotification)
	{
		Application::showNotification(notification);
	}

	return notification;
}

QString NotificationsManager::getEventName(int identifier)
{
	if (m_identifiers.contains(identifier))
	{
		return m_identifiers[identifier];
	}

	QString name(m_instance->metaObject()->enumerator(m_eventIdentifierEnumerator).valueToKey(identifier));

	if (!name.isEmpty())
	{
		name.chop(5);

		return name;
	}

	return QString();
}

NotificationsManager::EventDefinition NotificationsManager::getEventDefinition(int identifier)
{
	if (identifier < 0 || identifier >= m_definitions.count())
	{
		return EventDefinition();
	}

	const QSettings notificationsSettings(SessionsManager::getReadableDataPath(QLatin1String("notifications.ini")), QSettings::IniFormat);
	const QString eventName(getEventName(identifier));

	m_definitions[identifier].playSound = notificationsSettings.value(eventName + QLatin1String("/playSound"), QString()).toString();
	m_definitions[identifier].showAlert = notificationsSettings.value(eventName + QLatin1String("/showAlert"), false).toBool();
	m_definitions[identifier].showNotification = notificationsSettings.value(eventName + QLatin1String("/showNotification"), false).toBool();

	return m_definitions[identifier];
}

QVector<NotificationsManager::EventDefinition> NotificationsManager::getEventDefinitions()
{
	QSettings notificationsSettings(SessionsManager::getReadableDataPath(QLatin1String("notifications.ini")), QSettings::IniFormat);

	for (int i = 0; i < m_definitions.count(); ++i)
	{
		notificationsSettings.beginGroup(getEventName(i));

		m_definitions[i].playSound = notificationsSettings.value(QLatin1String("playSound")).toString();
		m_definitions[i].showAlert = notificationsSettings.value(QLatin1String("showAlert")).toBool();
		m_definitions[i].showNotification = notificationsSettings.value(QLatin1String("showNotification")).toBool();

		notificationsSettings.endGroup();
	}

	return m_definitions;
}

int NotificationsManager::registerEvent(const QString &title, const QString &description)
{
	const int identifier(m_definitions.count());

	EventDefinition definition;
	definition.title = title;
	definition.description = description;
	definition.identifier = identifier;

	m_definitions.append(definition);

	return identifier;
}

}
