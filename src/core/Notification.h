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

#ifndef OTTER_NOTIFICATION_H
#define OTTER_NOTIFICATION_H

#include <QtCore/QObject>

namespace Otter
{

enum NotificationLevel
{
	InformationNotificationLevel = 0,
	WarningNotificationLevel = 1,
	ErrorNotificationLevel = 2
};

class Notification : public QObject
{
	Q_OBJECT

public:
	void markClicked();
	void markIgnored();
	static Notification* createNotification(const QString &event, const QString &message, NotificationLevel level = InformationNotificationLevel, QObject *parent = NULL);
	QString getMessage() const;
	NotificationLevel getLevel() const;

protected:
	explicit Notification(const QString &message, NotificationLevel level, QObject *parent);

private:
	QString m_message;
	NotificationLevel m_level;

signals:
	void clicked();
	void ignored();
};

}

#endif
