/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_PLATFORMINTEGRATION_H
#define OTTER_PLATFORMINTEGRATION_H

#include <QtCore/QObject>
#include <QtCore/QMimeType>
#include <QtCore/QUrl>

namespace Otter
{

struct ApplicationInformation;
class Notification;
class Style;

class PlatformIntegration : public QObject
{
	Q_OBJECT

public:
	explicit PlatformIntegration(QObject *parent);

	virtual void runApplication(const QString &command, const QUrl &url = {}) const = 0;
	virtual void startLinkDrag(const QUrl &url, const QString &title, const QPixmap &pixmap, QObject *parent = nullptr) const;
	virtual Style* createStyle(const QString &name) const = 0;
	virtual QVector<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType) = 0;
	virtual QString getPreferredPasswordsBackend() const;
	virtual QString getPlatformName() const;
	virtual bool canShowNotifications() const;
	virtual bool canSetAsDefaultBrowser() const;
	virtual bool isDefaultBrowser() const;
	virtual bool installUpdate() const;

public slots:
	virtual void showNotification(Notification *notification);
	virtual bool setAsDefaultBrowser();

protected:
	virtual QString getUpdaterBinary() const;
};

}

#endif
