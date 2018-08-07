/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include <QtGui/QStandardItemModel>

namespace Otter
{

class ContentFiltersProfile;

class ContentFiltersManager final : public QObject
{
	Q_OBJECT

public:
	enum DataRole
	{
		NameRole = Qt::UserRole,
		UpdateUrlRole
	};

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
	static void addProfile(ContentFiltersProfile *profile);
	static void removeProfile(ContentFiltersProfile *profile);
	static QStandardItemModel* createModel(QObject *parent, const QStringList &profiles);
	static ContentFiltersManager* getInstance();
	static ContentFiltersProfile* getProfile(const QString &profile);
	static ContentFiltersProfile* getProfile(const QUrl &url);
	static ContentFiltersProfile* getProfile(int identifier);
	static CheckResult checkUrl(const QVector<int> &profiles, const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType);
	static CosmeticFiltersResult getCosmeticFilters(const QVector<int> &profiles, const QUrl &requestUrl);
	static QStringList createSubdomainList(const QString &domain);
	static QStringList getProfileNames();
	static QVector<ContentFiltersProfile*> getContentBlockingProfiles();
	static QVector<ContentFiltersProfile*> getFraudCheckingProfiles();
	static QVector<int> getProfileIdentifiers(const QStringList &names);
	static CosmeticFiltersMode getCosmeticFiltersMode();
	static bool areWildcardsEnabled();
	static bool isFraud(const QUrl &url);

protected:
	explicit ContentFiltersManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	static void ensureInitialized();

protected slots:
	void scheduleSave();
	void handleOptionChanged(int identifier, const QVariant &value);

private:
	int m_saveTimer;

	static ContentFiltersManager *m_instance;
	static QVector<ContentFiltersProfile*> m_contentBlockingProfiles;
	static QVector<ContentFiltersProfile*> m_fraudCheckingProfiles;
	static CosmeticFiltersMode m_cosmeticFiltersMode;
	static bool m_areWildcardsEnabled;

signals:
	void profileModified(const QString &profile);
};

class ContentFiltersProfile : public QObject
{
	Q_OBJECT

public:
	enum ProfileError
	{
		NoError = 0,
		ReadError,
		DownloadError,
		ChecksumError
	};

	enum ProfileFlag
	{
		NoFlags = 0,
		HasCustomTitleFlag = 1,
		HasCustomUpdateUrlFlag = 2
	};

	Q_DECLARE_FLAGS(ProfileFlags, ProfileFlag)

	enum ProfileCategory
	{
		OtherCategory = 0,
		AdvertisementsCategory = 1,
		AnnoyanceCategory = 2,
		PrivacyCategory = 4,
		SocialCategory = 8,
		RegionalCategory = 16
	};

	explicit ContentFiltersProfile(QObject *parent = nullptr);

	virtual void clear() = 0;
	virtual void setCategory(ProfileCategory category) = 0;
	virtual void setTitle(const QString &title) = 0;
	virtual void setUpdateInterval(int interval) = 0;
	virtual void setUpdateUrl(const QUrl &url) = 0;
	virtual QString getName() const = 0;
	virtual QString getTitle() const = 0;
	virtual QUrl getUpdateUrl() const = 0;
	virtual QDateTime getLastUpdate() const = 0;
	virtual ContentFiltersManager::CheckResult checkUrl(const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType) = 0;
	virtual ContentFiltersManager::CosmeticFiltersResult getCosmeticFilters(const QStringList &domains, bool isDomainOnly);
	virtual QVector<QLocale::Language> getLanguages() const;
	virtual ProfileCategory getCategory() const;
	virtual ProfileError getError() const = 0;
	virtual ProfileFlags getFlags() const = 0;
	virtual int getUpdateInterval() const = 0;
	virtual bool update() = 0;
	virtual bool remove() = 0;
	virtual bool isUpdating() const = 0;
	virtual bool isFraud(const QUrl &url);

signals:
	void profileModified(const QString &profile);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::ContentFiltersProfile::ProfileFlags)

#endif
