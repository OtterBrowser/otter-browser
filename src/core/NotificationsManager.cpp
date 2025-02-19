/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "IniSettings.h"
#include "SessionsManager.h"
#include "../ui/MainWindow.h"

#include <QtMultimedia/QSoundEffect>

namespace Otter
{

NotificationsManager* NotificationsManager::m_instance(nullptr);
QMap<int, QString> NotificationsManager::m_identifiers;
QVector<NotificationsManager::EventDefinition> NotificationsManager::m_definitions;
int NotificationsManager::m_eventIdentifierEnumerator(0);

Notification::Notification(const Message &message, QObject *parent) : QObject(parent)
{
	setMessage(message);
}

void Notification::markAsShown()
{
	emit shown();
}

void Notification::markAsClicked()
{
	emit clicked();

	deleteLater();
}

void Notification::markAsIgnored()
{
	emit ignored();

	deleteLater();
}

void Notification::requestClose()
{
	emit requestedClose();
}

void Notification::setData(const QVariant &data)
{
	m_data = data;
}

void Notification::setMessage(const Notification::Message &message)
{
	m_message = message;

	if (m_message.title.isEmpty() && m_message.event >= 0)
	{
		const NotificationsManager::EventDefinition definition(NotificationsManager::getEventDefinition(m_message.event));

		if (definition.isValid())
		{
			m_message.title = definition.getTitle();
		}
	}

	emit modified();
}

Notification::Message Notification::getMessage() const
{
	return m_message;
}

QVariant Notification::getData() const
{
	return m_data;
}

NotificationsManager::NotificationsManager(QObject *parent) : QObject(parent)
{
	registerEvent(QT_TRANSLATE_NOOP("notifications", "Feed Updated"), QT_TRANSLATE_NOOP("notifications", "Feed update was completed"));
	registerEvent(QT_TRANSLATE_NOOP("notifications", "Download Completed"), QT_TRANSLATE_NOOP("notifications", "File download was completed"));
	registerEvent(QT_TRANSLATE_NOOP("notifications", "Update Available"), QT_TRANSLATE_NOOP("notifications", "Update is available to be downloaded"));
	registerEvent(QT_TRANSLATE_NOOP("notifications", "Webpage Notification"), QT_TRANSLATE_NOOP("notifications", "Webpage wants to show a notification"));
}

void NotificationsManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new NotificationsManager(QCoreApplication::instance());
		m_eventIdentifierEnumerator = staticMetaObject.indexOfEnumerator(QLatin1String("EventIdentifier").data());
	}
}

Notification* NotificationsManager::createNotification(const Notification::Message &message, QObject *parent)
{
	Notification *notification(new Notification(message, QCoreApplication::instance()));
	const EventDefinition definition(getEventDefinition(message.event));

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
			connect(effect, &QSoundEffect::playingChanged, m_instance, [=]()
			{
				if (!effect->isPlaying())
				{
					effect->deleteLater();
				}
			});

			effect->play();
		}
	}

	if (definition.showAlert)
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(parent));

		if (mainWindow)
		{
			Application::alert(mainWindow);
		}
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

	return EnumeratorMapper(staticMetaObject.enumerator(m_eventIdentifierEnumerator), QLatin1String("Event")).mapToName(identifier);
}

NotificationsManager::EventDefinition NotificationsManager::getEventDefinition(int identifier)
{
	if (identifier < 0 || identifier >= m_definitions.count())
	{
		return {};
	}

	const IniSettings bundledSettings(SessionsManager::getReadableDataPath(QLatin1String("notifications.ini"), true), m_instance);
	const IniSettings localSettings(SessionsManager::getReadableDataPath(QLatin1String("notifications.ini")), m_instance);
	const IniSettings *settings(&localSettings);
	const QString eventName(getEventName(identifier));

	if (!localSettings.getGroups().contains(eventName))
	{
		settings = &bundledSettings;
	}

	EventDefinition definition(m_definitions.at(identifier));
	definition.playSound = settings->getValue(eventName + QLatin1String("/playSound"), {}).toString();
	definition.showAlert = settings->getValue(eventName + QLatin1String("/showAlert"), false).toBool();
	definition.showNotification = settings->getValue(eventName + QLatin1String("/showNotification"), false).toBool();

	return definition;
}

QVector<NotificationsManager::EventDefinition> NotificationsManager::getEventDefinitions()
{
	QVector<EventDefinition> definitions;
	definitions.reserve(m_definitions.count());

	for (int i = 0; i < m_definitions.count(); ++i)
	{
		definitions.append(getEventDefinition(i));
	}

	return definitions;
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
