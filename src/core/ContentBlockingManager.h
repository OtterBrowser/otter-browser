/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_CONTENTBLOCKINGMANAGER_H
#define OTTER_CONTENTBLOCKINGMANAGER_H

#include "NetworkManager.h"

#include <QtCore/QUrl>
#include <QtGui/QStandardItemModel>

namespace Otter
{

class ContentBlockingProfile;

class ContentBlockingManager : public QObject
{
	Q_OBJECT

public:
	enum CosmeticFiltersMode
	{
		NoFiltersMode = 0,
		DomainOnlyFiltersMode = 1,
		AllFiltersMode = 2
	};

	struct CheckResult
	{
		QString profile;
		QString rule;
		CosmeticFiltersMode comesticFiltersMode = AllFiltersMode;
		bool isBlocked = false;
		bool isException = false;
	};

	static void createInstance(QObject *parent = nullptr);
	static void addProfile(ContentBlockingProfile *profile);
	static QStandardItemModel* createModel(QObject *parent, const QStringList &profiles);
	static ContentBlockingManager* getInstance();
	static ContentBlockingProfile* getProfile(const QString &profile);
	static CheckResult checkUrl(const QVector<int> &profiles, const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType);
	static QStringList createSubdomainList(const QString &domain);
	static QStringList getStyleSheet(const QVector<int> &profiles);
	static QStringList getStyleSheetBlackList(const QString &domain, const QVector<int> &profiles);
	static QStringList getStyleSheetWhiteList(const QString &domain, const QVector<int> &profiles);
	static QVector<ContentBlockingProfile*> getProfiles();
	static QVector<int> getProfileList(const QStringList &names);
	static CosmeticFiltersMode getCosmeticFiltersMode();
	static bool areWildcardsEnabled();
	static bool updateProfile(const QString &profile);

public slots:
	void scheduleSave();

protected:
	explicit ContentBlockingManager(QObject *parent = nullptr);

	void timerEvent(QTimerEvent *event);

protected slots:
	void optionChanged(int identifier, const QVariant &value);

private:
	int m_saveTimer;

	static ContentBlockingManager *m_instance;
	static QVector<ContentBlockingProfile*> m_profiles;
	static CosmeticFiltersMode m_cosmeticFiltersMode;
	static bool m_areWildcardsEnabled;

signals:
	void profileModified(const QString &profile);
};

}

#endif
