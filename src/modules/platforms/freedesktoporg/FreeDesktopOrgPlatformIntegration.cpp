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

#include "FreeDesktopOrgPlatformIntegration.h"
#include "FreeDesktopOrgPlatformStyle.h"
#include "../../../core/NotificationsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/TransfersManager.h"
#include "../../../core/Utils.h"
#include "../../../../3rdparty/libmimeapps/DesktopEntry.h"
#include "../../../../3rdparty/libmimeapps/Index.h"

#include <QtCore/QThreadPool>
#ifdef OTTER_ENABLE_DBUS
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusReply>
#endif
#include <QtGui/QDesktopServices>
#include <QtGui/QIcon>
#ifdef OTTER_ENABLE_DBUS
#include <QtGui/QRgb>
#endif
#include <QtWidgets/QApplication>

#define DESKTOP_ENTRY_NAME "otter-browser"

#ifdef OTTER_ENABLE_DBUS
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
	argument << QByteArray(reinterpret_cast<const char*>(target.bits()), target.sizeInBytes());
	argument.endStructure();

	return argument;
}

const QDBusArgument& operator>>(const QDBusArgument &argument, QImage &image)
{
	Q_UNUSED(image)
	Q_ASSERT(0);

	return argument;
}
#endif

namespace Otter
{

FreeDesktopOrgPlatformIntegration::FreeDesktopOrgPlatformIntegration(QObject *parent) : PlatformIntegration(parent)
#ifdef OTTER_ENABLE_DBUS
	, m_notificationsInterface(nullptr)
#endif
{
	QGuiApplication::setDesktopFileName(QLatin1String(DESKTOP_ENTRY_NAME) + QLatin1String(".desktop"));

	QTimer::singleShot(250, this, [&]()
	{
		QThreadPool::globalInstance()->start([&]()
		{
			getApplicationsForMimeType(QMimeDatabase().mimeTypeForName(QLatin1String("text/html")));
		});
	});

#ifdef OTTER_ENABLE_DBUS
	connect(TransfersManager::getInstance(), &TransfersManager::transfersChanged, this, [&]()
	{
		const TransfersManager::ActiveTransfersInformation information(TransfersManager::getActiveTransfersInformation());

		setTransfersProgress(information.bytesTotal, information.bytesReceived, information.activeTransfersAmount);
	});
#endif
}

#ifdef OTTER_ENABLE_DBUS
FreeDesktopOrgPlatformIntegration::~FreeDesktopOrgPlatformIntegration()
{
	setTransfersProgress(0, 0, 0);
}
#endif

void FreeDesktopOrgPlatformIntegration::runApplication(const QString &command, const QUrl &url) const
{
	if (command.isEmpty())
	{
		QDesktopServices::openUrl(url);
	}

	std::vector<std::string> fileNames;
	fileNames.push_back((url.isLocalFile() ? QDir::toNativeSeparators(url.toLocalFile()) : url.url()).toStdString());

	const std::vector<std::string> rawArguments(LibMimeApps::DesktopEntry::parseExec(command.toStdString(), fileNames, LibMimeApps::DesktopEntry::ParseOptions::NecessarilyUseUrl));

	if (rawArguments.empty())
	{
		return;
	}

	QStringList arguments;
	arguments.reserve(static_cast<int>(rawArguments.size()) - 1);

	for (std::vector<std::string>::size_type i = 1; i < rawArguments.size(); ++i)
	{
		arguments.append(QString::fromStdString(rawArguments.at(i)));
	}

	QProcess::startDetached(QString::fromStdString(rawArguments.at(0)), arguments);
}

#ifdef OTTER_ENABLE_DBUS
void FreeDesktopOrgPlatformIntegration::handleNotificationCallFinished(QDBusPendingCallWatcher *watcher)
{
	Notification *notification(m_notificationWatchers.value(watcher, nullptr));

	if (notification)
	{
		m_notifications[QDBusReply<quint32>(*watcher).value()] = notification;
	}

	m_notificationWatchers.remove(watcher);

	watcher->deleteLater();
}

void FreeDesktopOrgPlatformIntegration::handleNotificationIgnored(quint32 identifier, quint32 reason)
{
	Q_UNUSED(reason)

	Notification *notification(m_notifications.value(identifier, nullptr));

	if (notification)
	{
		notification->markAsIgnored();

		m_notifications.remove(identifier);
	}
}

void FreeDesktopOrgPlatformIntegration::handleNotificationClicked(quint32 identifier, const QString &action)
{
	Q_UNUSED(action)

	Notification *notification(m_notifications.value(identifier, nullptr));

	if (notification)
	{
		notification->markAsClicked();

		m_notifications.remove(identifier);
	}
}

void FreeDesktopOrgPlatformIntegration::showNotification(Notification *notification)
{
	if (!m_notificationsInterface)
	{
		m_notificationsInterface = new QDBusInterface(QLatin1String("org.freedesktop.Notifications"), QLatin1String("/org/freedesktop/Notifications"), QLatin1String("org.freedesktop.Notifications"), QDBusConnection::sessionBus(), this);
		m_notificationsInterface->connection().connect(m_notificationsInterface->service(), m_notificationsInterface->path(), m_notificationsInterface->interface(), QLatin1String("NotificationClosed"), this, SLOT(handleNotificationIgnored(quint32,quint32)));
		m_notificationsInterface->connection().connect(m_notificationsInterface->service(), m_notificationsInterface->path(), m_notificationsInterface->interface(), QLatin1String("ActionInvoked"), this, SLOT(handleNotificationClicked(quint32,QString)));

		qDBusRegisterMetaType<QImage>();
	}

	const uint replacedIdentifier(m_notifications.key(notification, 0));
	const Notification::Message message(notification->getMessage());
	const int visibilityDuration(SettingsManager::getOption(SettingsManager::Interface_NotificationVisibilityDurationOption).toInt());
	QVariantList arguments;
	arguments << QApplication::applicationName();
	arguments << replacedIdentifier;
	arguments << QString();
	arguments << message.getTitle();
	arguments << message.message;
	arguments << QStringList({QLatin1String("default"), QString()});
	arguments << QVariantMap({{QLatin1String("desktop-entry"), QLatin1String(DESKTOP_ENTRY_NAME)}, {QLatin1String("image-data"), (message.image.isNull() ? message.getIcon().pixmap(128, 128).toImage() : message.image)}});
	arguments << ((visibilityDuration < 0) ? -1 : (visibilityDuration * 1000));

	QDBusPendingCallWatcher *watcher(new QDBusPendingCallWatcher(m_notificationsInterface->asyncCallWithArgumentList(QLatin1String("Notify"), arguments), this));

	m_notificationWatchers[watcher] = notification;

	if (replacedIdentifier == 0)
	{
		notification->markAsShown();

		connect(notification, &Notification::modified, this, [=]()
		{
			showNotification(notification);
		});
		connect(notification, &Notification::requestedClose, this, [=]()
		{
			const uint identifier(m_notifications.key(notification, 0));

			if (identifier > 0)
			{
				m_notificationsInterface->asyncCallWithArgumentList(QLatin1String("CloseNotification"), {identifier});
			}
		});
	}

	connect(watcher, &QDBusPendingCallWatcher::finished, this, &FreeDesktopOrgPlatformIntegration::handleNotificationCallFinished);
}

void FreeDesktopOrgPlatformIntegration::setTransfersProgress(qint64 bytesTotal, qint64 bytesReceived, qint64 transfersAmount)
{
	const bool hasActiveTransfers(transfersAmount > 0);
	QVariantMap properties;
	properties[QLatin1String("count")] = transfersAmount;
	properties[QLatin1String("count-visible")] = hasActiveTransfers;
	properties[QLatin1String("progress")] = ((bytesReceived > 0) ? Utils::calculatePercent(bytesReceived, bytesTotal, 1) : 0.0);
	properties[QLatin1String("progress-visible")] = hasActiveTransfers;

	QDBusMessage message(QDBusMessage::createSignal(QLatin1String("/com/canonical/unity/launcherentry/9ddcf02c30a33cd63e9762f06957263f"), QLatin1String("com.canonical.Unity.LauncherEntry"), QLatin1String("Update")));
	message.setArguments({QLatin1String("application://") + QLatin1String(DESKTOP_ENTRY_NAME) + QLatin1String(".desktop"), properties});

	QDBusConnection::sessionBus().send(message);
}
#endif

Style* FreeDesktopOrgPlatformIntegration::createStyle(const QString &name) const
{
	if (name.isEmpty() || name.startsWith(QLatin1String("gtk"), Qt::CaseInsensitive))
	{
		return new FreeDesktopOrgPlatformStyle(name);
	}

	return nullptr;
}

QVector<ApplicationInformation> FreeDesktopOrgPlatformIntegration::getApplicationsForMimeType(const QMimeType &mimeType)
{
	if (m_applicationsCache.contains(mimeType.name()))
	{
		return m_applicationsCache[mimeType.name()];
	}

	const LibMimeApps::Index index(QLocale().bcp47Name().toStdString());
	const std::vector<LibMimeApps::DesktopEntry> entries(index.appsForMime(mimeType.name().toStdString()));
	QVector<ApplicationInformation> applications;
	applications.reserve(static_cast<int>(entries.size()));

	for (std::vector<LibMimeApps::DesktopEntry>::size_type i = 0; i < entries.size(); ++i)
	{
		const LibMimeApps::DesktopEntry entry(entries.at(i));

		applications.append({QString::fromStdString(entry.executable()), QString::fromStdString(entry.name()), QIcon::fromTheme(QString::fromStdString(entry.icon()))});
	}

	m_applicationsCache[mimeType.name()] = applications;

	return applications;
}

#ifdef OTTER_ENABLE_DBUS
bool FreeDesktopOrgPlatformIntegration::canShowNotifications() const
{
	return (m_notificationsInterface ? m_notificationsInterface->isValid() : true);
}
#endif

}
