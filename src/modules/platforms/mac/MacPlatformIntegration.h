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

#ifndef OTTER_MACPLATFORMINTEGRATION_H
#define OTTER_MACPLATFORMINTEGRATION_H

#include "../../../core/PlatformIntegration.h"

#include <QtCore/QTemporaryDir>
#if QT_VERSION >= 0x060000
#include <QtGui/QAction>
#endif
#include <QtGui/QDrag>
#if QT_VERSION < 0x060000
#include <QtWidgets/QAction>
#endif

namespace Otter
{

class MacPlatformLinkDrag final : public QDrag
{
public:
	explicit MacPlatformLinkDrag(QObject *parent);

	QString getPath() const;

private:
	QTemporaryDir m_directory;
};

class MacPlatformIntegration final : public PlatformIntegration
{
	Q_OBJECT

public:
	explicit MacPlatformIntegration(QObject *parent);

	void markNotificationClicked(quint64 identifier);
	void runApplication(const QString &command, const QUrl &url = {}) const override;
	void startLinkDrag(const QUrl &url, const QString &title, const QPixmap &pixmap, QObject *parent = nullptr) const override;
	Style* createStyle(const QString &name) const override;
	QVector<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType) override;
	bool canShowNotifications() const override;

public slots:
	void showNotification(Notification *notification) override;

protected:
	void timerEvent(QTimerEvent *event) override;

private:
	QHash<quint64, Notification*> m_notifications;
	quint64 m_notificationIdentifier;
	int m_notificationsWatcherTimer;
};

}

#endif
