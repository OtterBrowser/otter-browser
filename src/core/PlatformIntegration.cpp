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

#include "PlatformIntegration.h"
#include "Application.h"
#include "Utils.h"

namespace Otter
{

PlatformIntegration::PlatformIntegration(Application *parent) : QObject(parent)
{
}

void PlatformIntegration::runApplication(const QString &command, const QString &fileName) const
{
	Q_UNUSED(command)
	Q_UNUSED(fileName)
}

QList<ApplicationInformation> PlatformIntegration::getApplicationsForMimeType(const QMimeType &mimeType)
{
	Q_UNUSED(mimeType)

	return QList<ApplicationInformation>();
}

bool PlatformIntegration::canShowNotifications() const
{
	return false;
}

bool PlatformIntegration::setAsDefaultBrowser()
{
	return false;
}

bool PlatformIntegration::canSetAsDefaultBrowser() const
{
	return false;
}

bool PlatformIntegration::isDefaultBrowser() const
{
	return false;
}

void PlatformIntegration::showNotification(Notification *notification)
{
	Q_UNUSED(notification)
}

}
