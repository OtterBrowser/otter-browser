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

#ifndef OTTER_NOTIFICATIONSMANAGER_H
#define OTTER_NOTIFICATIONSMANAGER_H

#include "../core/ThemesManager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QVariant>
#include <QtGui/QIcon>

namespace Otter
{

class NotificationsManager;

class Notification final : public QObject
{
	Q_OBJECT

public:
	struct Message final
	{
		enum Level
		{
			InformationLevel = 0,
			WarningLevel,
			ErrorLevel
		};

		QString title;
		QString message;
		QIcon icon;
		QImage image;
		QDateTime creationTime = QDateTime::currentDateTimeUtc();
		Level level = InformationLevel;
		int event = -1;

		QString getTitle() const
		{
			if (!title.isEmpty())
			{
				return title;
			}

			switch (level)
			{
				case ErrorLevel:
					return QCoreApplication::translate("Otter::NotificationsManager", "Error");
				case Notification::Message::WarningLevel:
					return QCoreApplication::translate("Otter::NotificationsManager", "Warning");
				default:
					break;
			}

			return QCoreApplication::translate("Otter::NotificationsManager", "Information");
		}

		QIcon getIcon() const
		{
			if (!icon.isNull())
			{
				return icon;
			}

			if (!image.isNull())
			{
				return QIcon(QPixmap::fromImage(image));
			}

			switch (level)
			{
				case ErrorLevel:
					return ThemesManager::createIcon(QLatin1String("dialog-error"));
				case WarningLevel:
					return ThemesManager::createIcon(QLatin1String("dialog-warning"));
				default:
					break;
			}

			return ThemesManager::createIcon(QLatin1String("dialog-information"));
		}
	};

	void setData(const QVariant &data);
	void setMessage(const Message &message);
	Message getMessage() const;
	QVariant getData() const;

public slots:
	void markAsShown();
	void markAsClicked();
	void markAsIgnored();
	void requestClose();

protected:
	explicit Notification(const Message &message, QObject *parent);

private:
	Message m_message;
	QVariant m_data;

signals:
	void shown();
	void clicked();
	void ignored();
	void modified();
	void requestedClose();

friend class NotificationsManager;
};

class NotificationsManager final : public QObject
{
	Q_OBJECT

public:
	enum EventIdentifier
	{
		FeedUpdatedEvent = 0,
		TransferCompletedEvent,
		UpdateAvailableEvent,
		WebPageNotificationEvent
	};

	Q_ENUM(EventIdentifier)

	struct EventDefinition final
	{
		QString title;
		QString description;
		QString playSound;
		int identifier = -1;
		bool showAlert = false;
		bool showNotification = false;

		QString getTitle() const
		{
			return QCoreApplication::translate("notifications", title.toUtf8());
		}

		QString getDescription() const
		{
			return QCoreApplication::translate("notifications", description.toUtf8());
		}

		bool isValid() const
		{
			return (identifier >= 0);
		}
	};

	static void createInstance();
	static NotificationsManager* getInstance();
	static Notification* createNotification(const Notification::Message &message, QObject *parent = nullptr);
	static QString getEventName(int identifier);
	static EventDefinition getEventDefinition(int identifier);
	static QVector<EventDefinition> getEventDefinitions();
	static int registerEvent(const QString &title, const QString &description = {});

protected:
	explicit NotificationsManager(QObject *parent);

private:
	static NotificationsManager *m_instance;
	static QMap<int, QString> m_identifiers;
	static QVector<EventDefinition> m_definitions;
	static int m_eventIdentifierEnumerator;
};

}

#endif
