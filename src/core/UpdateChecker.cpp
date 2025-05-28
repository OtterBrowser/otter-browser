/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "UpdateChecker.h"
#include "Application.h"
#include "Console.h"
#include "Job.h"
#include "PlatformIntegration.h"
#include "SettingsManager.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Otter
{

UpdateChecker::UpdateChecker(QObject *parent, bool isInBackground) : QObject(parent)
{
	const QUrl url(SettingsManager::getOption(SettingsManager::Updates_ServerUrlOption).toString());

	if (!url.isValid())
	{
		Console::addMessage(QCoreApplication::translate("main", "Unable to check for updates. Invalid URL: %1").arg(url.url()), Console::OtherCategory, Console::ErrorLevel);

		deleteLater();

		return;
	}

	DataFetchJob *job(new DataFetchJob(url, this));

	connect(job, &Job::jobFinished, this, [=](bool isSuccess)
	{
		QIODevice *device(job->getData());

		if (!isSuccess)
		{
			Console::addMessage(QCoreApplication::translate("main", "Unable to check for updates: %1").arg(device ? device->errorString() : tr("Download failure")), Console::OtherCategory, Console::ErrorLevel);

			deleteLater();

			return;
		}

		QStringList activeChannels(SettingsManager::getOption(SettingsManager::Updates_ActiveChannelsOption).toStringList());
		activeChannels.removeAll({});

		const PlatformIntegration *integration(Application::getPlatformIntegration());
		const QJsonArray channelsArray(QJsonDocument::fromJson(device->readAll()).object().value(QLatin1String("channels")).toArray());
		const QString platform(integration ? integration->getPlatformName() : QString());
		const int mainVersion(QCoreApplication::applicationVersion().remove(QLatin1Char('.')).toInt());
		const int subVersion(QStringLiteral(OTTER_VERSION_WEEKLY).toInt());
		int latestVersion(0);
		int latestVersionIndex(0);
		QVector<UpdateInformation> availableUpdates;

		for (int i = 0; i < channelsArray.count(); ++i)
		{
			if (channelsArray.at(i).isObject())
			{
				const QJsonObject channelObject(channelsArray.at(i).toObject());
				const QString identifier(channelObject.value(QLatin1String("identifier")).toString());
				const QString channelVersion(channelObject.value(QLatin1String("version")).toString());

				if (activeChannels.contains(identifier, Qt::CaseInsensitive) || (!isInBackground && activeChannels.isEmpty()))
				{
					const int channelMainVersion(channelVersion.trimmed().remove(QLatin1Char('.')).toInt());

					if (channelMainVersion == 0)
					{
						Console::addMessage(QCoreApplication::translate("main", "Unable to parse version number: %1").arg(channelVersion), Console::OtherCategory, Console::ErrorLevel);

						continue;
					}

					const int channelSubVersion(channelObject.value(QLatin1String("subVersion")).toString().toInt());

					if (mainVersion < channelMainVersion || (subVersion > 0 && subVersion < channelSubVersion))
					{
						UpdateInformation information;
						information.channel = identifier;
						information.version = channelVersion;
						information.isAvailable = channelObject.value(QLatin1String("availablePlatforms")).toVariant().toStringList().contains(platform);
						information.detailsUrl = QUrl(channelObject.value(QLatin1String("detailsUrl")).toString());
						information.scriptUrl = QUrl(channelObject.value(QLatin1String("scriptUrl")).toString().replace(QLatin1String("%VERSION%"), channelVersion).replace(QLatin1String("%PLATFORM%"), platform));
						information.fileUrl = QUrl(channelObject.value(QLatin1String("fileUrl")).toString().replace(QLatin1String("%VERSION%"), channelVersion).replace(QLatin1String("%PLATFORM%"), platform).replace(QLatin1String("%TIMESTAMP%"), QString::number(QDateTime::currentMSecsSinceEpoch() / 1000)));

						if (!channelObject.value(QLatin1String("subVersion")).toString().isEmpty())
						{
							information.version.append(QLatin1Char('#') + channelObject.value(QLatin1String("subVersion")).toString());
						}

						if (mainVersion < channelMainVersion && latestVersion < channelMainVersion)
						{
							latestVersion = channelMainVersion;
							latestVersionIndex = availableUpdates.count();
						}

						availableUpdates.append(information);
					}
				}
			}
		}

		SettingsManager::setOption(SettingsManager::Updates_LastCheckOption, QDate::currentDate().toString(Qt::ISODate));

		emit finished(availableUpdates, latestVersionIndex);

		deleteLater();
	});

	job->start();
}

}
