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

#include "QtWebKitNotificationPresenter.h"
#include "../../../../core/Job.h"
#include "../../../../core/SessionsManager.h"

namespace Otter
{

QtWebKitNotificationPresenter::QtWebKitNotificationPresenter() = default;

void QtWebKitNotificationPresenter::showMessage(const Notification::Message &message)
{
	const Notification *notification(NotificationsManager::createNotification(message));

	connect(notification, &Notification::clicked, this, [&]()
	{
		if (m_openerUrl.isValid())
		{
			SessionsManager::hasUrl(m_openerUrl, true);
		}

		emit notificationClicked();
	});
	connect(notification, &Notification::ignored, this, &QtWebKitNotificationPresenter::notificationClosed);
}

void QtWebKitNotificationPresenter::showNotification(const QWebNotificationData *data)
{
	m_openerUrl = data->openerPageUrl();

	Notification::Message message;
	message.title = data->title();
	message.message = data->message();
	message.event = NotificationsManager::WebPageNotificationEvent;

	if (data->iconUrl().isEmpty())
	{
		showMessage(message);
	}
	else
	{
		DataFetchJob *job(new DataFetchJob(data->iconUrl(), this));

		connect(job, &DataFetchJob::jobFinished, this, [=]()
		{
			Notification::Message mutableMessage(message);
			mutableMessage.image = QImage::fromData(job->getData()->readAll());

			showMessage(mutableMessage);
		});

		job->setTimeout(3);
		job->start();
	}
}

}
