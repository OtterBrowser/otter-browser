/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2010 David Sansome <me@davidsansome.com>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_FREEDESKTOPORGPLATFORMINTEGRATION_H
#define OTTER_FREEDESKTOPORGPLATFORMINTEGRATION_H

#include "../../../core/PlatformIntegration.h"

#ifdef OTTER_ENABLE_DBUS
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusPendingCallWatcher>

QDBusArgument& operator<<(QDBusArgument &argument, const QImage &image);
const QDBusArgument& operator>>(const QDBusArgument &argument, QImage &image);
#endif

namespace Otter
{

class FreeDesktopOrgPlatformIntegration final : public PlatformIntegration
{
	Q_OBJECT

public:
	explicit FreeDesktopOrgPlatformIntegration(QObject *parent);
#ifdef OTTER_ENABLE_DBUS
	~FreeDesktopOrgPlatformIntegration();
#endif

	void runApplication(const QString &command, const QUrl &url = {}) const override;
	Style* createStyle(const QString &name) const override;
	QVector<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType) override;
#ifdef OTTER_ENABLE_DBUS
	bool canShowNotifications() const override;

public slots:
	void showNotification(Notification *notification) override;

protected:
	void setTransfersProgress(qint64 bytesTotal, qint64 bytesReceived, qint64 transfersAmount);

protected slots:
	void handleNotificationCallFinished(QDBusPendingCallWatcher *watcher);
	void handleNotificationIgnored(quint32 identifier, quint32 reason);
	void handleNotificationClicked(quint32 identifier, const QString &action);
#endif

private:
#ifdef OTTER_ENABLE_DBUS
	QDBusInterface *m_notificationsInterface;
	QHash<QDBusPendingCallWatcher*, Notification*> m_notificationWatchers;
	QHash<quint32, Notification*> m_notifications;
#endif
	QHash<QString, QVector<ApplicationInformation> > m_applicationsCache;
};

}

#endif
