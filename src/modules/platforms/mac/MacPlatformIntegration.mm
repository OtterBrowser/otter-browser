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

#include "MacPlatformIntegration.h"
#include "../../../core/Application.h"
#include "../../../core/NotificationsManager.h"
#include "../../../core/SettingsManager.h"

#include <QtCore/QDateTime>
#include <QtCore/QSysInfo>
#include <QtGui/QDesktopServices>

#import <AppKit/AppKit.h>

@interface MacPlatformIntegrationUserNotificationCenterDelegate : NSObject<NSUserNotificationCenterDelegate>

{
	@public
		Otter::MacPlatformIntegration *m_platformIntegration;
}

- (id)initWithPlatformIntegration:(Otter::MacPlatformIntegration*)platformIntegration;
- (void)userNotificationCenter:(NSUserNotificationCenter*)center didActivateNotification:(NSUserNotification*)notification;
- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center shouldPresentNotification:(NSUserNotification*)notification;

@end

@implementation MacPlatformIntegrationUserNotificationCenterDelegate

-(id)initWithPlatformIntegration:(Otter::MacPlatformIntegration*)platformIntegration
{
	self = [super init];

	if (self)
	{
		m_platformIntegration = platformIntegration;

		[[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:self];
	}

	return self;
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center didActivateNotification:(NSUserNotification*)notification
{
	Q_UNUSED(center);

	m_platformIntegration->markNotificationClicked(QString::fromNSString(notification.identifier).toULongLong());
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center shouldPresentNotification:(NSUserNotification*)notification
{
	Q_UNUSED(center);
	Q_UNUSED(notification);

	return YES;
}

@end

namespace Otter
{

MacPlatformIntegration::MacPlatformIntegration(Application *parent) : PlatformIntegration(parent),
	m_notificationIdentifier(0),
	m_notificationsWatcherTimer(0)
{
	[[MacPlatformIntegrationUserNotificationCenterDelegate alloc] initWithPlatformIntegration:this];
}

void MacPlatformIntegration::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_notificationsWatcherTimer)
	{
		const int visibilityDuration = SettingsManager::getValue(QLatin1String("Interface/NotificationVisibilityDuration")).toInt();
		QHash<quint64, NSUserNotification*> existingNotifications;

		for (NSUserNotification *notification in [[NSUserNotificationCenter defaultUserNotificationCenter] deliveredNotifications])
		{
			existingNotifications[QString::fromNSString(notification.identifier).toULongLong()] = notification;
		}

		const QList<quint64> allNotifications = m_notifications.keys();

		for (int i = 0; i < allNotifications.count(); ++i)
		{
			if (!existingNotifications.contains(allNotifications.at(i)))
			{
				m_notifications[allNotifications.at(i)]->markIgnored();

				m_notifications.remove(allNotifications.at(i));
			}
			else if (m_notifications[allNotifications.at(i)]->getCreationTime().secsTo(QDateTime::currentDateTime()) >= visibilityDuration)
			{
				[[NSUserNotificationCenter defaultUserNotificationCenter] removeDeliveredNotification:existingNotifications[allNotifications.at(i)]];

				m_notifications[allNotifications.at(i)]->markIgnored();

				m_notifications.remove(allNotifications.at(i));
			}
		}

		if (m_notifications.isEmpty())
		{
			killTimer(m_notificationsWatcherTimer);

			m_notificationsWatcherTimer = 0;
		}
	}
}

void MacPlatformIntegration::markNotificationClicked(quint64 identifier)
{
	if (m_notifications.contains(identifier))
	{
		m_notifications[identifier]->markClicked();

		m_notifications.remove(identifier);
	}
}

void MacPlatformIntegration::runApplication(const QString &command, const QString &fileName) const
{
	if (command.isEmpty())
	{
		QDesktopServices::openUrl(QUrl(fileName));
	}

///TODO
}

void MacPlatformIntegration::showNotification(Notification *notification)
{
	if (QSysInfo::MacintoshVersion < QSysInfo::MV_10_8)
	{
		return;
	}

	QString title;

	switch (notification->getLevel())
	{
		case Notification::ErrorLevel:
			title = tr("Error");

			break;
		case Notification::WarningLevel:
			title = tr("Warning");

			break;
		default:
			title = tr("Information");

			break;
	}

	const quint64 identifier = ++m_notificationIdentifier;

	m_notifications[identifier] = notification;

	NSUserNotification *nativeNotification = [[NSUserNotification alloc] init];
	nativeNotification.title = [NSString stringWithUTF8String:title.toUtf8().data()];
	nativeNotification.informativeText = [NSString stringWithUTF8String:notification->getMessage().toUtf8().data()];
	nativeNotification.identifier = [NSString stringWithUTF8String:QString::number(identifier).toUtf8().data()];;

	[[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:nativeNotification];

	if (m_notificationsWatcherTimer == 0)
	{
		m_notificationsWatcherTimer = startTimer(1000);
	}
}

QList<ApplicationInformation> MacPlatformIntegration::getApplicationsForMimeType(const QMimeType &mimeType)
{
///TODO

	return QList<ApplicationInformation>();
}

bool MacPlatformIntegration::canShowNotifications() const
{
	return (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_8);
}

}
