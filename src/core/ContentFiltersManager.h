/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_CONTENTFILTERSMANAGER_H
#define OTTER_CONTENTFILTERSMANAGER_H

#include "NetworkManager.h"

#include <QtCore/QUrl>

namespace Otter
{

class ContentFiltersProfile;

class ContentFiltersManager final : public QObject
{
	Q_OBJECT

public:
	enum CosmeticFiltersMode
	{
		NoFilters = 0,
		DomainOnlyFilters,
		AllFilters
	};

	struct CheckResult final
	{
		QString rule;
		CosmeticFiltersMode comesticFiltersMode = AllFilters;
		int profile = -1;
		bool isBlocked = false;
		bool isException = false;
		bool isFraud = false;
	};

	struct CosmeticFiltersResult final
	{
		QStringList rules;
		QStringList exceptions;
	};

	static void createInstance();
	static void initialize();
	static void addProfile(ContentFiltersProfile *profile);
	static void removeProfile(ContentFiltersProfile *profile, bool removeFile = false);
	static ContentFiltersManager* getInstance();
	static ContentFiltersProfile* getProfile(const QString &name);
	static ContentFiltersProfile* getProfile(const QUrl &url);
	static ContentFiltersProfile* getProfile(int identifier);
	static CheckResult checkUrl(const QVector<int> &profiles, const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType);
	static CosmeticFiltersResult getCosmeticFilters(const QVector<int> &profiles, const QUrl &requestUrl);
	static QStringList createSubdomainList(const QString &domain);
	static QStringList getProfileNames();
	static QVector<ContentFiltersProfile*> getContentBlockingProfiles();
	static QVector<ContentFiltersProfile*> getFraudCheckingProfiles();
	static QVector<int> getProfileIdentifiers(const QStringList &names);
	static bool isFraud(const QUrl &url);

protected:
	explicit ContentFiltersManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	void save();

protected slots:
	void scheduleSave();

private:
	int m_saveTimer;

	static ContentFiltersManager *m_instance;
	static QVector<ContentFiltersProfile*> m_contentBlockingProfiles;
	static QVector<ContentFiltersProfile*> m_fraudCheckingProfiles;

signals:
	void profileAdded(const QString &profile);
	void profileModified(const QString &profile);
	void profileRemoved(const QString &profile);
};

class ContentFiltersProfile : public QObject
{
	Q_OBJECT

public:
	enum ProfileError
	{
		NoError = 0,
		DownloadError,
		ReadError,
		ParseError
	};

	enum ProfileFlag
	{
		NoFlags = 0,
		HasCustomTitleFlag = 1
	};

	Q_DECLARE_FLAGS(ProfileFlags, ProfileFlag)

	enum ProfileCategory
	{
		OtherCategory = 0,
		AdvertisementsCategory,
		AnnoyanceCategory,
		PrivacyCategory,
		SocialCategory,
		RegionalCategory
	};

	struct ProfileSummary final
	{
		QString name;
		QString title;
		QDateTime lastUpdate;
		QUrl updateUrl;
		ProfileCategory category = OtherCategory;
		ContentFiltersManager::CosmeticFiltersMode cosmeticFiltersMode = ContentFiltersManager::AllFilters;
		int updateInterval = 0;
		bool areWildcardsEnabled = false;
	};

	explicit ContentFiltersProfile(QObject *parent = nullptr);

	virtual void clear() = 0;
	virtual void setProfileSummary(const ProfileSummary &profileSummary) = 0;
	virtual QString getName() const = 0;
	virtual QString getTitle() const = 0;
	virtual QString getPath() const = 0;
	virtual QUrl getUpdateUrl() const = 0;
	virtual QDateTime getLastUpdate() const = 0;
	virtual ProfileSummary getProfileSummary() const = 0;
	virtual ContentFiltersManager::CheckResult checkUrl(const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType) = 0;
	virtual ContentFiltersManager::CosmeticFiltersResult getCosmeticFilters(const QStringList &domains, bool isDomainOnly) = 0;
	virtual QVector<QLocale::Language> getLanguages() const = 0;
	virtual ProfileCategory getCategory() const = 0;
	virtual ContentFiltersManager::CosmeticFiltersMode getCosmeticFiltersMode() const = 0;
	virtual ProfileError getError() const = 0;
	virtual ProfileFlags getFlags() const = 0;
	virtual int getUpdateInterval() const = 0;
	virtual int getUpdateProgress() const = 0;
	virtual bool update(const QUrl &url = {}) = 0;
	virtual bool remove() = 0;
	virtual bool areWildcardsEnabled() const = 0;
	virtual bool isUpdating() const = 0;
	virtual bool isFraud(const QUrl &url);

signals:
	void profileModified();
	void updateProgressChanged(int progress);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::ContentFiltersProfile::ProfileFlags)

#endif
