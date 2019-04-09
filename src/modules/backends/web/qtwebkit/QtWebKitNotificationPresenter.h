/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2019 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef QTWEBKITNOTIFICATIONPRESENTER_H
#define QTWEBKITNOTIFICATIONPRESENTER_H

#include "qwebkitplatformplugin.h"
#include "../../../../core/NotificationsManager.h"

namespace Otter
{

class QtWebKitNotificationPresenter final : public QWebNotificationPresenter
{
public:
	explicit QtWebKitNotificationPresenter();

	void showNotification(const QWebNotificationData *data) override;

protected:
	void showMessage(const Notification::Message &message);

private:
	QUrl m_openerUrl;
};

}

#endif
