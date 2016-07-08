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

#ifndef OTTER_CONTENTBLOCKINGMANAGER_H
#define OTTER_CONTENTBLOCKINGMANAGER_H

#include "NetworkManager.h"

#include <QtCore/QUrl>

namespace Otter
{

class ContentBlockingProfile;
struct ContentBlockingInformation;

class ContentBlockingManager : public QObject
{
	Q_OBJECT

public:
	struct CheckResult
	{
		QUrl url;
		QString profile;
		NetworkManager::ResourceType resourceType;
		bool isBlocked;

		CheckResult() : resourceType(NetworkManager::OtherType), isBlocked(false) {}
	};

	static void createInstance(QObject *parent = NULL);
	static ContentBlockingManager* getInstance();
	static CheckResult checkUrl(const QVector<int> &profiles, const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType);
	static ContentBlockingInformation getProfile(const QString &profile);
	static QStringList createSubdomainList(const QString &domain);
	static QStringList getStyleSheet(const QVector<int> &profiles);
	static QStringList getStyleSheetBlackList(const QString &domain, const QVector<int> &profiles);
	static QStringList getStyleSheetWhiteList(const QString &domain, const QVector<int> &profiles);
	static QVector<ContentBlockingInformation> getProfiles();
	static QVector<int> getProfileList(const QStringList &names);
	static bool updateProfile(const QString &profile);

protected:
	explicit ContentBlockingManager(QObject *parent = NULL);

	static void loadProfiles();

private:
	static ContentBlockingManager *m_instance;
	static QVector<ContentBlockingProfile*> m_profiles;

signals:
	void profileModified(const QString &profile);
};

}

#endif
