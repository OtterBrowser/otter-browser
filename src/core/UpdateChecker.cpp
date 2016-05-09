/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "UpdateChecker.h"
#include "Application.h"
#include "Console.h"
#include "NetworkManager.h"
#include "NetworkManagerFactory.h"
#include "PlatformIntegration.h"
#include "SettingsManager.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Otter
{

UpdateChecker::UpdateChecker(QObject *parent, bool inBackground) : QObject(parent),
	m_networkReply(NULL),
	m_isInBackground(inBackground)
{
	const QUrl url(SettingsManager::getValue(QLatin1String("Updates/ServerUrl")).toString());

	if (!url.isValid())
	{
		Console::addMessage(QCoreApplication::translate("main", "Unable to check for updates. Invalid URL: %1").arg(url.url()), OtherMessageCategory, ErrorMessageLevel);

		deleteLater();

		return;
	}

	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());

	m_networkReply = NetworkManagerFactory::getNetworkManager()->get(request);

	connect(m_networkReply, SIGNAL(finished()), this, SLOT(runUpdateCheck()));
}

void UpdateChecker::runUpdateCheck()
{
	m_networkReply->deleteLater();

	if (m_networkReply->error() != QNetworkReply::NoError)
	{
		Console::addMessage(QCoreApplication::translate("main", "Unable to check for updates: %1").arg(m_networkReply->errorString()), OtherMessageCategory, ErrorMessageLevel);

		deleteLater();

		return;
	}

	QStringList activeChannels(SettingsManager::getValue(QLatin1String("Updates/ActiveChannels")).toStringList());
	activeChannels.removeAll(QString());

	const QJsonObject updateData(QJsonDocument::fromJson(m_networkReply->readAll()).object());
	const QJsonArray channels(updateData.value(QLatin1String("channels")).toArray());
	const QString platform(Application::getInstance()->getPlatformIntegration()->getPlatform());
	const int mainVersion(QCoreApplication::applicationVersion().remove(QChar('.')).toInt());
	const int subVersion(QString(OTTER_VERSION_WEEKLY).toInt());
	QList<UpdateInformation> availableUpdates;

	for (int i = 0; i < channels.count(); ++i)
	{
		if (channels.at(i).isObject())
		{
			const QJsonObject object(channels.at(i).toObject());
			const QString identifier(object[QLatin1String("identifier")].toString());
			const QString channelVersion(object[QLatin1String("version")].toString());

			if (activeChannels.contains(identifier, Qt::CaseInsensitive) || (!m_isInBackground && activeChannels.isEmpty()))
			{
				const int channelMainVersion(channelVersion.trimmed().remove(QChar('.')).toInt());

				if (channelMainVersion == 0)
				{
					Console::addMessage(QCoreApplication::translate("main", "Unable to parse version number: %1").arg(channelVersion), OtherMessageCategory, ErrorMessageLevel);

					continue;
				}

				const int channelSubVersion(object[QLatin1String("subVersion")].toString().toInt());

				if (mainVersion < channelMainVersion || (subVersion > 0 && subVersion < channelSubVersion))
				{
					UpdateInformation information;
					information.channel = identifier;
					information.version = channelVersion;
					information.isAvailable = object[QLatin1String("availablePlatforms")].toArray().contains(QJsonValue(platform));
					information.detailsUrl = QUrl(object[QLatin1String("detailsUrl")].toString());
					information.scriptUrl = QUrl(object[QLatin1String("scriptUrl")].toString().replace(QLatin1String("%VERSION%"), channelVersion).replace(QLatin1String("%PLATFORM%"), platform));
					information.fileUrl = QUrl(object[QLatin1String("fileUrl")].toString().replace(QLatin1String("%VERSION%"), channelVersion).replace(QLatin1String("%PLATFORM%"), platform).replace(QLatin1String("%TIMESTAMP%"), QString::number(QDateTime::currentDateTime().toUTC().toTime_t())));

					if (!object[QLatin1String("subVersion")].toString().isEmpty())
					{
						information.version.append(QLatin1Char('#') + object[QLatin1String("subVersion")].toString());
					}

					availableUpdates.append(information);
				}
			}
		}
	}

	SettingsManager::setValue(QLatin1String("Updates/LastCheck"), QDate::currentDate().toString(Qt::ISODate));

	emit finished(availableUpdates);

	deleteLater();
}

}
