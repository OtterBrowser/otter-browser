/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include "Console.h"
#include "NetworkManager.h"
#include "NetworkManagerFactory.h"
#include "NotificationsManager.h"
#include "SettingsManager.h"
#include "WindowsManager.h"
#include "./config.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

namespace Otter
{

UpdateChecker::UpdateChecker(bool showDialog, QObject *parent) : QObject(parent),
	m_networkReply(NULL),
	m_showDialog(showDialog)
{
	checkForUpdates();
}

void UpdateChecker::checkForUpdates()
{
	const QUrl url = SettingsManager::getValue(QLatin1String("Updates/ServerUrl")).toString();

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

	readUpdateData(m_networkReply->readAll());
}

void UpdateChecker::runUpdate()
{
	if (!SessionsManager::hasUrl(m_detailsUrl, true))
	{
		WindowsManager *manager = SessionsManager::getWindowsManager();

		if (manager)
		{
			manager->open(m_detailsUrl);
		}
	}

	deleteLater();
}

void UpdateChecker::readUpdateData(const QByteArray &data)
{
	const QStringList activeChannels = SettingsManager::getValue(QLatin1String("Updates/ActiveChannels")).toStringList();
	const QJsonObject updateData = QJsonDocument::fromJson(data).object();
	const QJsonArray channels = updateData.value(QLatin1String("channels")).toArray();
	int mainVersion = QCoreApplication::applicationVersion().remove(QChar('.')).toInt();
	int subVersion = QString(OTTER_VERSION_WEEKLY).toInt();
	QString availableVersion;
	QString availableChannel;

	for (int i = 0; i < channels.count(); ++i)
	{
		if (channels.at(i).isObject())
		{
			const QJsonObject object = channels.at(i).toObject();
			const QString identifier = object["identifier"].toString();
			const QString channelVersion = object["version"].toString();

			if (activeChannels.contains(identifier, Qt::CaseInsensitive))
			{
				const int channelMainVersion = channelVersion.trimmed().remove(QChar('.')).toInt();

				if (channelMainVersion == 0)
				{
					Console::addMessage(QCoreApplication::translate("main", "Unable to parse version number: %1").arg(channelVersion), OtherMessageCategory, ErrorMessageLevel);

					continue;
				}

				const int channelSubVersion = object["subVersion"].toString().toInt();

				if ((mainVersion < channelMainVersion) || (channelSubVersion > 0 && subVersion < channelSubVersion))
				{
					mainVersion = channelMainVersion;
					subVersion = channelSubVersion;
					availableVersion = channelVersion;
					availableChannel = identifier;
					m_detailsUrl = object["detailsUrl"].toString();
				}
			}
		}
	}

	SettingsManager::setValue(QLatin1String("Updates/LastCheck"), QDate::currentDate().toString(Qt::ISODate));

	if (!availableVersion.isEmpty())
	{
		const QString text = tr("New update %1 from %2 channel is available!").arg(availableVersion).arg(availableChannel);

		if (m_showDialog)
		{
			QMessageBox messageBox;
			messageBox.setWindowTitle(tr("New version of Otter Browser is available"));
			messageBox.setText(text);
			messageBox.setInformativeText(tr("Do you want to open a new tab with download page?"));
			messageBox.setIcon(QMessageBox::Question);
			messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			messageBox.setDefaultButton(QMessageBox::No);

			if (messageBox.exec() == QMessageBox::Yes)
			{
				runUpdate();
			}
			else
			{
				deleteLater();
			}
		}
		else
		{
			Notification *updateNotification = NotificationsManager::createNotification(NotificationsManager::UpdateAvailableEvent, text);

			connect(updateNotification, SIGNAL(clicked()), this, SLOT(runUpdate()));
			connect(updateNotification, SIGNAL(ignored()), this, SLOT(deleteLater()));
		}
	}
	else
	{ 
		deleteLater();
	}
}

}
