/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "ContentBlockingManager.h"
#include "Console.h"
#include "ContentBlockingProfile.h"
#include "SettingsManager.h"
#include "SessionsManager.h"

#include <QtCore/QDir>

namespace Otter
{

ContentBlockingManager* ContentBlockingManager::m_instance = NULL;
QVector<ContentBlockingProfile*> ContentBlockingManager::m_profiles;

ContentBlockingManager::ContentBlockingManager(QObject *parent) : QObject(parent)
{
}

void ContentBlockingManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new ContentBlockingManager(parent);

		loadProfiles();
	}
}

void ContentBlockingManager::loadProfiles()
{
	const QString contentBlockingPath = SessionsManager::getWritableDataPath(QLatin1String("blocking"));
	const QDir directory(contentBlockingPath);

	if (!directory.exists())
	{
		QDir().mkpath(contentBlockingPath);
	}

	const QList<QFileInfo> availableProfiles = QDir(QLatin1String(":/blocking/")).entryInfoList(QStringList(QLatin1String("*.txt")), QDir::Files);

	for (int i = 0; i < availableProfiles.count(); ++i)
	{
		const QString path = directory.filePath(availableProfiles.at(i).fileName());

		if (!QFile::exists(path))
		{
			QFile::copy(availableProfiles.at(i).filePath(), path);
			QFile::setPermissions(path, (QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther));
		}
	}

	const QList<QFileInfo> existingProfiles = directory.entryInfoList(QStringList(QLatin1String("*.txt")), QDir::Files);

	for (int i = 0; i < existingProfiles.count(); ++i)
	{
		ContentBlockingProfile *profile = new ContentBlockingProfile(existingProfiles.at(i).absoluteFilePath(), m_instance);

		m_profiles.append(profile);

		connect(profile, SIGNAL(profileModified(QString)), m_instance, SIGNAL(profileModified(QString)));
	}
}

ContentBlockingManager* ContentBlockingManager::getInstance()
{
	return m_instance;
}

QStringList ContentBlockingManager::getStyleSheet(const QVector<int> &profiles)
{
	QStringList styleSheet;

	for (int i = 0; i < profiles.count(); ++i)
	{
		if (profiles[i] >= 0 && profiles[i] < m_profiles.count())
		{
			styleSheet += m_profiles.at(profiles[i])->getStyleSheet();
		}
	}

	return styleSheet;
}

ContentBlockingInformation ContentBlockingManager::getProfile(const QString &profile)
{
	for (int i = 0; i < m_profiles.count(); ++i)
	{
		if (m_profiles[i]->getInformation().name == profile)
		{
			return m_profiles[i]->getInformation();
		}
	}

	return ContentBlockingInformation();
}

QStringList ContentBlockingManager::createSubdomainList(const QString &domain)
{
	QStringList subdomainList;
	int dotPosition = domain.lastIndexOf(QLatin1Char('.'));
	dotPosition = domain.lastIndexOf(QLatin1Char('.'), dotPosition - 1);

	while (dotPosition != -1)
	{
		subdomainList.append(domain.mid(dotPosition + 1));

		dotPosition = domain.lastIndexOf(QLatin1Char('.'), dotPosition - 1);
	}

	subdomainList.append(domain);

	return subdomainList;
}

QVector<ContentBlockingInformation> ContentBlockingManager::getProfiles()
{
	QVector<ContentBlockingInformation> profiles;
	profiles.reserve(m_profiles.count());

	for (int i = 0; i < m_profiles.count(); ++i)
	{
		profiles.append(m_profiles.at(i)->getInformation());
	}

	return profiles;
}

QMultiHash<QString, QString> ContentBlockingManager::getStyleSheetBlackList(const QVector<int> &profiles)
{
	QMultiHash<QString, QString> blackList;

	for (int i = 0; i < profiles.count(); ++i)
	{
		if (profiles[i] >= 0 && profiles[i] < m_profiles.count())
		{
			blackList += m_profiles.at(profiles[i])->getStyleSheetBlackList();
		}
	}

	return blackList;
}

QMultiHash<QString, QString> ContentBlockingManager::getStyleSheetWhiteList(const QVector<int> &profiles)
{
	QMultiHash<QString, QString> whiteList;

	for (int i = 0; i < profiles.count(); ++i)
	{
		if (profiles[i] >= 0 && profiles[i] < m_profiles.count())
		{
			whiteList += m_profiles.at(profiles[i])->getStyleSheetWhiteList();
		}
	}

	return whiteList;
}

QVector<int> ContentBlockingManager::getProfileList(const QStringList &names)
{
	QVector<int> profiles;
	profiles.reserve(names.count());

	for (int i = 0; i < m_profiles.count(); ++i)
	{
		if (names.contains(m_profiles.at(i)->getInformation().name))
		{
			profiles.append(i);
		}
	}

	return profiles;
}

bool ContentBlockingManager::updateProfile(const QString &profile)
{
	for (int i = 0; i < m_profiles.count(); ++i)
	{
		if (m_profiles.at(i)->getInformation().name == profile)
		{
			return m_profiles[i]->downloadRules();
		}
	}

	return false;
}

bool ContentBlockingManager::isUrlBlocked(const QVector<int> &profiles, const QUrl &baseUrl, const QUrl &requestUrl, ResourceType resourceType)
{
	if (profiles.isEmpty())
	{
		return false;
	}

	const QString scheme = requestUrl.scheme();

	if (scheme != QLatin1String("http") && scheme != QLatin1String("https"))
	{
		return false;
	}

	for (int i = 0; i < profiles.count(); ++i)
	{
		if (profiles[i] >= 0 && profiles[i] < m_profiles.count() && m_profiles.at(profiles[i])->isUrlBlocked(baseUrl, requestUrl, resourceType))
		{
			return true;
		}
	}

	return false;
}

}
