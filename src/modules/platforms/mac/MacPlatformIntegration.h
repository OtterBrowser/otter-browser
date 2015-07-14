/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_MACPLATFORMINTEGRATION_H
#define OTTER_MACPLATFORMINTEGRATION_H

#include "../../../core/Utils.h"
#include "../../../core/PlatformIntegration.h"

namespace Otter
{

class MacPlatformIntegration : public PlatformIntegration
{
	Q_OBJECT

public:
	explicit MacPlatformIntegration(Application *parent);

	void markNotificationClicked(quint64 identifier);
	void runApplication(const QString &command, const QUrl &url = QUrl()) const;
	void showNotification(Notification *notification);
	QList<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType);
	bool canShowNotifications() const;

protected:
	void timerEvent(QTimerEvent *event);

protected slots:
	void updateDockIcon();

private:
	QHash<quint64, Notification*> m_notifications;
	quint64 m_notificationIdentifier;
	int m_notificationsWatcherTimer;
	bool m_isDockIconViewVisible;
};

}

#endif
