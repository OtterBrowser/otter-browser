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

#ifndef OTTER_NOTIFICATIONSMANAGER_H
#define OTTER_NOTIFICATIONSMANAGER_H

#include <QtCore/QObject>

namespace Otter
{

struct EventDefinition
{
	QString title;
	QString description;
	QString playSound;
	int identifier;
	bool showAlert;
	bool showNotification;

	EventDefinition() : identifier(-1), showAlert(false), showNotification(false) {}
};

class NotificationsManager;

class Notification : public QObject
{
	Q_OBJECT

public:
	enum NotificationLevel
	{
		InformationLevel = 0,
		WarningLevel = 1,
		ErrorLevel = 2
	};

	void markClicked();
	void markIgnored();
	QString getMessage() const;
	NotificationLevel getLevel() const;

protected:
	explicit Notification(const QString &message, NotificationLevel level, int event, QObject *parent);

private:
	QString m_message;
	NotificationLevel m_level;
	int m_event;

signals:
	void clicked();
	void ignored();

friend class NotificationsManager;
};

class NotificationsManager : public QObject
{
	Q_OBJECT
	Q_ENUMS(EventIdentifier)

public:
	enum EventIdentifier
	{
		TransferCompletedEvent = 0,
		UpdateAvailableEvent = 1
	};

	static void createInstance(QObject *parent = NULL);
	static NotificationsManager* getInstance();
	static Notification* createNotification(int event, const QString &message, Notification::NotificationLevel level = Notification::InformationLevel, QObject *parent = NULL);
	static QString getEventName(int identifier);
	static EventDefinition getEventDefinition(int identifier);
	static QVector<EventDefinition> getEventDefinitions();
	static int registerEvent(const QString &text, const QString &description = QString());

protected:
	explicit NotificationsManager(QObject *parent = NULL);

private:
	static NotificationsManager *m_instance;
	static QMap<int, QString> m_identifiers;
	static QVector<EventDefinition> m_definitions;
};

}

#endif
