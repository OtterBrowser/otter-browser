/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "FreeDesktopOrgPlatformIntegration.h"
#include "../../../core/Application.h"
#include "../../../core/NotificationsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../../3rdparty/libmimeapps/DesktopEntry.h"
#include "../../../../3rdparty/libmimeapps/Index.h"

#include <QtConcurrent/QtConcurrent>
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusReply>
#include <QtGui/QDesktopServices>
#include <QtGui/QIcon>
#include <QtGui/QRgb>

QDBusArgument& operator<<(QDBusArgument &argument, const QImage &image)
{
	if (image.isNull())
	{
		argument.beginStructure();
		argument << 0 << 0 << 0 << false << 0 << 0 << QByteArray();
		argument.endStructure();

		return argument;
	}

	const QImage scaled(image.scaledToHeight(100, Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32));

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	// ABGR -> ARGB
	QImage target(scaled.rgbSwapped());
#else
	// ABGR -> GBAR
	QImage target(scaled.size(), scaled.format());

	for (int y = 0; y < target.height(); ++y)
	{
		QRgb *p((QRgb*) scaled.scanLine(y));
		QRgb *q((QRgb*) target.scanLine(y));
		QRgb *end(p + scaled.width());

		while (p < end)
		{
			*q = qRgba(qGreen(*p), qBlue(*p), qAlpha(*p), qRed(*p));
			++p;
			++q;
		}
	}
#endif

	argument.beginStructure();
	argument << target.width();
	argument << target.height();
	argument << target.bytesPerLine();
	argument << target.hasAlphaChannel();

	const int channels(target.isGrayscale() ? 1 : (target.hasAlphaChannel() ? 4 : 3));

	argument << (target.depth() / channels);
	argument << channels;
	argument << QByteArray(reinterpret_cast<const char*>(target.bits()), target.byteCount());
	argument.endStructure();

	return argument;
}

const QDBusArgument& operator>>(const QDBusArgument &argument, QImage &image)
{
	Q_UNUSED(image)
	Q_ASSERT(0);

	return argument;
}

namespace Otter
{

FreeDesktopOrgPlatformIntegration::FreeDesktopOrgPlatformIntegration(Application *parent) : PlatformIntegration(parent),
	m_notificationsInterface(new QDBusInterface(QLatin1String("org.freedesktop.Notifications"), QLatin1String("/org/freedesktop/Notifications"), QLatin1String("org.freedesktop.Notifications"), QDBusConnection::sessionBus(), this))
{
	qDBusRegisterMetaType<QImage>();

	m_notificationsInterface->connection().connect(m_notificationsInterface->service(), m_notificationsInterface->path(), m_notificationsInterface->interface(), QLatin1String("NotificationClosed"), this, SLOT(notificationIgnored(quint32,quint32)));
	m_notificationsInterface->connection().connect(m_notificationsInterface->service(), m_notificationsInterface->path(), m_notificationsInterface->interface(), QLatin1String("ActionInvoked"), this, SLOT(notificationClicked(quint32,QString)));

	QTimer::singleShot(250, this, SLOT(createApplicationsCacheThread()));
}

void FreeDesktopOrgPlatformIntegration::runApplication(const QString &command, const QUrl &url) const
{
	if (command.isEmpty())
	{
		QDesktopServices::openUrl(url);
	}

	std::vector<std::string> fileNames;
	fileNames.push_back((url.isLocalFile() ? QDir::toNativeSeparators(url.toLocalFile()) : url.url()).toStdString());

	std::vector<std::string> parsed(LibMimeApps::DesktopEntry::parseExec(command.toStdString(), fileNames));

	if (parsed.size() < 1)
	{
		return;
	}

	QStringList arguments;

	for (std::vector<std::string>::size_type i = 1; i < parsed.size(); ++i)
	{
		arguments.append(QString::fromStdString(parsed.at(i)));
	}

	QProcess::startDetached(QString::fromStdString(parsed.at(0)), arguments);
}

void FreeDesktopOrgPlatformIntegration::createApplicationsCache()
{
	m_applicationsCache[QLatin1String("text/html")] = getApplicationsForMimeType(QMimeDatabase().mimeTypeForName(QLatin1String("text/html")));
}

void FreeDesktopOrgPlatformIntegration::createApplicationsCacheThread()
{
	QtConcurrent::run(this, &FreeDesktopOrgPlatformIntegration::createApplicationsCache);
}

void FreeDesktopOrgPlatformIntegration::notificationCallFinished(QDBusPendingCallWatcher *watcher)
{
	Notification *notification(m_notificationWatchers.value(watcher, nullptr));

	if (notification)
	{
		m_notifications[QDBusReply<quint32>(*watcher).value()] = notification;
	}

	m_notificationWatchers.remove(watcher);

	watcher->deleteLater();
}

void FreeDesktopOrgPlatformIntegration::notificationIgnored(quint32 identifier, quint32 reason)
{
	Q_UNUSED(reason)

	Notification *notification(m_notifications.value(identifier, nullptr));

	if (notification)
	{
		notification->markIgnored();

		m_notifications.remove(identifier);
	}
}

void FreeDesktopOrgPlatformIntegration::notificationClicked(quint32 identifier, const QString &action)
{
	Q_UNUSED(action)

	Notification *notification(m_notifications.value(identifier, nullptr));

	if (notification)
	{
		notification->markClicked();

		m_notifications.remove(identifier);
	}
}

void FreeDesktopOrgPlatformIntegration::showNotification(Notification *notification)
{
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

	const int visibilityDuration(SettingsManager::getValue(SettingsManager::Interface_NotificationVisibilityDurationOption).toInt());
	QVariantMap hints;
	hints[QLatin1String("image_data")] = Application::windowIcon().pixmap(128, 128).toImage();

	QVariantList arguments;
	arguments << Application::applicationName();
	arguments << uint(0);
	arguments << QString();
	arguments << tr("Notification");
	arguments << notification->getMessage();
	arguments << QStringList();
	arguments << hints;
	arguments << ((visibilityDuration < 0) ? -1 : (visibilityDuration * 1000));

	QDBusPendingCallWatcher *watcher(new QDBusPendingCallWatcher(m_notificationsInterface->asyncCallWithArgumentList(QLatin1String("Notify"), arguments), this));

	m_notificationWatchers[watcher] = notification;

	connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(notificationCallFinished(QDBusPendingCallWatcher*)));
}

QList<ApplicationInformation> FreeDesktopOrgPlatformIntegration::getApplicationsForMimeType(const QMimeType &mimeType)
{
	if (m_applicationsCache.contains(mimeType.name()))
	{
		return m_applicationsCache[mimeType.name()];
	}

	LibMimeApps::Index index(QLocale().bcp47Name().toStdString());
	std::vector<LibMimeApps::DesktopEntry> applications(index.appsForMime(mimeType.name().toStdString()));
	QList<ApplicationInformation> result;

	for (std::vector<LibMimeApps::DesktopEntry>::size_type i = 0; i < applications.size(); ++i)
	{
		ApplicationInformation application;
		application.command = QString::fromStdString(applications.at(i).executable());
		application.name = QString::fromStdString(applications.at(i).name());
		application.icon = QIcon::fromTheme(QString::fromStdString(applications.at(i).icon()));

		result.append(application);
	}

	m_applicationsCache[mimeType.name()] = result;

	return result;
}

bool FreeDesktopOrgPlatformIntegration::canShowNotifications() const
{
	return m_notificationsInterface->isValid();
}

}
