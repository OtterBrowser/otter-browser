/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "MacPlatformIntegration.h"
#include "MacPlatformStyle.h"
#include "../../../core/Application.h"
#include "../../../core/NotificationsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/TransfersManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/Action.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/Menu.h"

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtCore/QTemporaryFile>
#include <QtCore/QXmlStreamWriter>
#include <QtGui/QDesktopServices>
#include <QtMacExtras/QtMac>

#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>

@interface MacPlatformIntegrationDockIconView : NSView
{
	double m_value;
}

- (void)setProgress:(double)value;
+ (MacPlatformIntegrationDockIconView*)getSharedDockIconView;

@end

static MacPlatformIntegrationDockIconView *getSharedDockIconView = nil;

@implementation MacPlatformIntegrationDockIconView

+ (MacPlatformIntegrationDockIconView*)getSharedDockIconView
{
	if (getSharedDockIconView == nil)
	{
		getSharedDockIconView = [[MacPlatformIntegrationDockIconView alloc] init];

		[[MacPlatformIntegrationDockIconView getSharedDockIconView] setProgress:-1];
	}

	return getSharedDockIconView;
}

- (void)setProgress:(double)value
{
	m_value = value;

	[[NSApp dockTile] display];
}

- (void)drawRect:(NSRect)dirtyRectangle
{
	Q_UNUSED(dirtyRectangle)

	NSRect boundary = [self bounds];

	[[NSApp applicationIconImage] drawInRect:boundary fromRect:NSZeroRect operation:NSCompositingOperationCopy fraction:1.0];

	if (m_value < 0)
	{
		return;
	}

	NSRect rectangle = boundary;
	rectangle.size.height *= 0.13;
	rectangle.size.width *= 0.8;
	rectangle.origin.x = ((NSWidth(boundary) - NSWidth(rectangle)) / 2.0);
	rectangle.origin.y = (NSHeight(boundary) * 0.13);

	CGFloat radius = (rectangle.size.height / 2);
	NSBezierPath *bezierPath = [NSBezierPath bezierPathWithRoundedRect:rectangle xRadius:radius yRadius:radius];
	[bezierPath setLineWidth:2.0];

	[[NSColor grayColor] set];

	[bezierPath stroke];

	rectangle = NSInsetRect(rectangle, 2.0, 2.0);

	radius = (rectangle.size.height / 2.0);

	bezierPath = [NSBezierPath bezierPathWithRoundedRect:rectangle xRadius:radius yRadius:radius];
	[bezierPath setLineWidth:1.0];
	[bezierPath addClip];

	rectangle.size.width = floor(rectangle.size.width * m_value);

	[[NSColor colorWithSRGBRed:0.2 green:1 blue:0.2 alpha:1] set];

	NSRectFill(rectangle);
}

@end

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

MacPlatformLinkDrag::MacPlatformLinkDrag(QObject *parent) : QDrag(parent)
{
}

QString MacPlatformLinkDrag::getPath() const
{
	return m_directory.path();
}

MacPlatformIntegration::MacPlatformIntegration(QObject *parent) : PlatformIntegration(parent),
	m_notificationIdentifier(0),
	m_notificationsWatcherTimer(0)
{
	[[MacPlatformIntegrationUserNotificationCenterDelegate alloc] initWithPlatformIntegration:this];
	[[NSApp dockTile] setContentView:[MacPlatformIntegrationDockIconView getSharedDockIconView]];

	const QVector<int> actions({ActionsManager::NewTabAction, ActionsManager::NewTabPrivateAction, -1, ActionsManager::BookmarksAction, ActionsManager::TransfersAction, ActionsManager::HistoryAction, ActionsManager::NotesAction});
	ActionExecutor::Object executor(Application::getInstance(), Application::getInstance());
	Menu *menu(new Menu());

	for (int i = 0; i < actions.count(); ++i)
	{
		if (actions.at(i) < 0)
		{
			menu->addSeparator();
		}
		else
		{
			Action *action(new Action(actions.at(i), {}, executor, menu));

			switch (actions.at(i))
			{
				case ActionsManager::BookmarksAction:
					action->setTextOverride(QT_TRANSLATE_NOOP("actions", "Bookmarks"));

					break;
				case ActionsManager::HistoryAction:
					action->setTextOverride(QT_TRANSLATE_NOOP("actions", "History"));

					break;
				case ActionsManager::NotesAction:
					action->setTextOverride(QT_TRANSLATE_NOOP("actions", "Notes"));

					break;
				default:
					break;
			}

			menu->addAction(action);
		}
	}

	menu->setAsDockMenu();

	connect(TransfersManager::getInstance(), &TransfersManager::transfersChanged, this, [&]()
	{
		const TransfersManager::ActiveTransfersInformation information(TransfersManager::getActiveTransfersInformation());

		[[MacPlatformIntegrationDockIconView getSharedDockIconView] setProgress:((information.activeTransfersAmount > 0 && information.bytesTotal > 0) ? Utils::calculatePercent(information.bytesReceived, information.bytesTotal) : -1)];
		[[[NSApplication sharedApplication] dockTile] setBadgeLabel:((information.activeTransfersAmount > 0) ? [NSString stringWithFormat:@"%d", information.activeTransfersAmount] : @"")];
	});
}

void MacPlatformIntegration::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_notificationsWatcherTimer)
	{
		const int visibilityDuration(SettingsManager::getOption(SettingsManager::Interface_NotificationVisibilityDurationOption).toInt());
		QHash<quint64, NSUserNotification*> existingNotifications;

		for (NSUserNotification *notification in [[NSUserNotificationCenter defaultUserNotificationCenter] deliveredNotifications])
		{
			existingNotifications[QString::fromNSString(notification.identifier).toULongLong()] = notification;
		}

		const QList<quint64> allNotifications(m_notifications.keys());

		for (int i = 0; i < allNotifications.count(); ++i)
		{
			if (!existingNotifications.contains(allNotifications.at(i)))
			{
				m_notifications[allNotifications.at(i)]->markAsIgnored();

				m_notifications.remove(allNotifications.at(i));
			}
			else if (m_notifications[allNotifications.at(i)]->getMessage().creationTime.secsTo(QDateTime::currentDateTimeUtc()) >= visibilityDuration)
			{
				[[NSUserNotificationCenter defaultUserNotificationCenter] removeDeliveredNotification:existingNotifications[allNotifications.at(i)]];

				m_notifications[allNotifications.at(i)]->markAsIgnored();

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
		m_notifications[identifier]->markAsClicked();

		m_notifications.remove(identifier);
	}
}

void MacPlatformIntegration::runApplication(const QString &command, const QUrl &url) const
{
	if (command.isEmpty())
	{
		QDesktopServices::openUrl(url);
	}

	[[NSWorkspace sharedWorkspace] openFile:url.toLocalFile().toNSString() withApplication:command.toNSString()];
}

void MacPlatformIntegration::startLinkDrag(const QUrl &url, const QString &title, const QPixmap &pixmap, QObject *parent) const
{
	MacPlatformLinkDrag *drag(new MacPlatformLinkDrag(parent));
	QMimeData *mimeData(new QMimeData());
	mimeData->setText(url.toString());

	QFile file(drag->getPath() + QDir::separator() + Utils::extractHost(url) + QLatin1String(".webloc"));

	if (file.open(QIODevice::WriteOnly))
	{
		QXmlStreamWriter stream(&file);
		stream.setAutoFormatting(true);
		stream.writeStartDocument();
		stream.writeDTD(QLatin1String("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"));
		stream.writeStartElement(QLatin1String("plist"));
		stream.writeAttribute(QLatin1String("version"), QLatin1String("1.0"));
		stream.writeStartElement(QLatin1String("dict"));
		stream.writeTextElement(QLatin1String("key"), QLatin1String("URL"));
		stream.writeTextElement(QLatin1String("string"), url.toString());
		stream.writeEndElement();
		stream.writeEndElement();
		stream.writeEndDocument();

		file.close();

		mimeData->setUrls({QUrl::fromLocalFile(file.fileName())});
	}
	else
	{
		mimeData->setUrls({url});
	}

	if (!title.isEmpty())
	{
		mimeData->setProperty("x-url-title", title);
	}

	mimeData->setProperty("x-url-string", url.toString());

	drag->setMimeData(mimeData);
	drag->setPixmap(pixmap);
	drag->exec(Qt::MoveAction);
}

void MacPlatformIntegration::showNotification(Notification *notification)
{
	const quint64 identifier(++m_notificationIdentifier);

	m_notifications[identifier] = notification;

	notification->markAsShown();

	NSUserNotification *nativeNotification = [[NSUserNotification alloc] init];
	nativeNotification.title = [NSString stringWithUTF8String:notification->getMessage().getTitle().toUtf8().data()];
	nativeNotification.informativeText = [NSString stringWithUTF8String:notification->getMessage().message.toUtf8().data()];
	nativeNotification.identifier = [NSString stringWithUTF8String:QString::number(identifier).toUtf8().data()];

	[[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:nativeNotification];

	if (m_notificationsWatcherTimer == 0 && SettingsManager::getOption(SettingsManager::Interface_NotificationVisibilityDurationOption).toInt() > 0)
	{
		m_notificationsWatcherTimer = startTimer(1000);
	}
}

Style* MacPlatformIntegration::createStyle(const QString &name) const
{
	if (name.isEmpty() || name.toLower() == QLatin1String("macintosh"))
	{
		return new MacPlatformStyle(name);
	}

	return nullptr;
}

QVector<ApplicationInformation> MacPlatformIntegration::getApplicationsForMimeType(const QMimeType &mimeType)
{
	if (mimeType.preferredSuffix().isEmpty())
	{
		return {};
	}

	NSRect iconRectangle(NSMakeRect(0, 0, 64, 64));
	QTemporaryFile file(QLatin1String("XXXXXX.") + mimeType.preferredSuffix());
	file.open();

	QUrl url(file.fileName());
	url.setScheme(QLatin1String("file"));

	const CFArrayRef urls(LSCopyApplicationURLsForURL(url.toCFURL(), kLSRolesAll));

	if (!urls)
	{
		return {};
	}

	const int count(CFArrayGetCount(urls));
	QVector<ApplicationInformation> applications;
	applications.reserve(count);

	for (int i = 0; i < count; ++i)
	{
		const QString path(QUrl::fromCFURL(static_cast<CFURLRef>(CFArrayGetValueAtIndex(urls, i))).toLocalFile());
		ApplicationInformation application;
		application.command = path;
		application.name = QFileInfo(path).bundleName();
		application.icon = QIcon(QtMac::fromCGImageRef([[[NSWorkspace sharedWorkspace] iconForFile:path.toNSString()] CGImageForProposedRect:&iconRectangle context:nil hints:nil]));

		applications.append(application);
	}

	return applications;
}

bool MacPlatformIntegration::canShowNotifications() const
{
	return true;
}

}
