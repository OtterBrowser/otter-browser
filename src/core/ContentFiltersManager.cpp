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

#include "ContentFiltersManager.h"
#include "AdblockContentFiltersProfile.h"
#include "Application.h"
#include "Console.h"
#include "JsonSettings.h"
#include "SessionsManager.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtCore/QTimeZone>

namespace Otter
{

ContentFiltersManager* ContentFiltersManager::m_instance(nullptr);
QVector<ContentFiltersProfile*> ContentFiltersManager::m_contentBlockingProfiles;
QVector<ContentFiltersProfile*> ContentFiltersManager::m_fraudCheckingProfiles;

ContentFiltersManager::ContentFiltersManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
	QTimer::singleShot(1000, this, [&]()
	{
		initialize();
	});
}

void ContentFiltersManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new ContentFiltersManager(QCoreApplication::instance());
	}
}

void ContentFiltersManager::initialize()
{
	if (!m_contentBlockingProfiles.isEmpty())
	{
		return;
	}

	const QList<QFileInfo> existingProfiles(QDir(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking"))).entryInfoList({QLatin1String("*.txt")}, QDir::Files));
	const QJsonObject bundledMainObject(JsonSettings(SessionsManager::getReadableDataPath(QLatin1String("contentBlocking.json"), true)).object());
	QStringList profiles;
	profiles.reserve(existingProfiles.count());

	for (int i = 0; i < existingProfiles.count(); ++i)
	{
		const QString name(existingProfiles.at(i).completeBaseName());

		if (!profiles.contains(name))
		{
			profiles.append(name);
		}
	}

	QJsonObject::const_iterator iterator;

	for (iterator = bundledMainObject.constBegin(); iterator != bundledMainObject.constEnd(); ++iterator)
	{
		const QString name(iterator.key());

		if (!profiles.contains(name))
		{
			profiles.append(name);
		}
	}

	profiles.sort();

	m_contentBlockingProfiles.reserve(profiles.count());

	const QJsonObject localMainObject(JsonSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.json"))).object());
	const QHash<QString, ContentFiltersProfile::ProfileCategory> categoryTitles({{QLatin1String("advertisements"), ContentFiltersProfile::AdvertisementsCategory}, {QLatin1String("annoyance"), ContentFiltersProfile::AnnoyanceCategory}, {QLatin1String("privacy"), ContentFiltersProfile::PrivacyCategory}, {QLatin1String("social"), ContentFiltersProfile::SocialCategory}, {QLatin1String("regional"), ContentFiltersProfile::RegionalCategory}, {QLatin1String("other"), ContentFiltersProfile::OtherCategory}});

	for (int i = 0; i < profiles.count(); ++i)
	{
		const QString name(profiles.at(i));
		QJsonObject profileObject(localMainObject.value(name).toObject());
		const QJsonObject bundledProfileObject(bundledMainObject.value(name).toObject());
		ContentFiltersProfile::ProfileSummary profileSummary;
		profileSummary.name = name;

		ContentFiltersProfile::ProfileFlags flags(ContentFiltersProfile::NoFlags);

		if (profileObject.isEmpty())
		{
			profileObject = bundledProfileObject;
			profileSummary.title = profileObject.value(QLatin1String("title")).toString();
			profileSummary.updateUrl = QUrl(profileObject.value(QLatin1String("updateUrl")).toString());
		}
		else
		{
			if (profileObject.value(QLatin1String("isHidden")).toBool())
			{
				continue;
			}

			profileSummary.title = profileObject.value(QLatin1String("title")).toString();
			profileSummary.updateUrl = QUrl(profileObject.value(QLatin1String("updateUrl")).toString());

			if (profileSummary.updateUrl.isEmpty())
			{
				profileSummary.updateUrl = QUrl(bundledProfileObject.value(QLatin1String("updateUrl")).toString());
			}

			if (profileSummary.title.isEmpty())
			{
				profileSummary.title = bundledProfileObject.value(QLatin1String("title")).toString();
			}
			else
			{
				flags |= ContentFiltersProfile::HasCustomTitleFlag;
			}
		}

		const QString cosmeticFiltersMode(profileObject.value(QLatin1String("cosmeticFiltersMode")).toString());

		if (cosmeticFiltersMode == QLatin1String("none"))
		{
			profileSummary.cosmeticFiltersMode = NoFilters;
		}
		else if (cosmeticFiltersMode == QLatin1String("domainOnly"))
		{
			profileSummary.cosmeticFiltersMode = DomainOnlyFilters;
		}
		else
		{
			profileSummary.cosmeticFiltersMode = AllFilters;
		}

		profileSummary.lastUpdate = QDateTime::fromString(profileObject.value(QLatin1String("lastUpdate")).toString(), Qt::ISODate);
		profileSummary.lastUpdate.setTimeZone(QTimeZone::utc());
		profileSummary.category = categoryTitles.value(profileObject.value(QLatin1String("category")).toString());
		profileSummary.updateInterval = profileObject.value(QLatin1String("updateInterval")).toInt();
		profileSummary.areWildcardsEnabled = profileObject.value(QLatin1String("areWildcardsEnabled")).toBool();

		const QJsonArray languagesArray(profileObject.value(QLatin1String("languages")).toArray());
		QStringList languages;
		languages.reserve(languagesArray.count());

		for (int j = 0; j < languagesArray.count(); ++j)
		{
			languages.append(languagesArray.at(j).toString());
		}

		ContentFiltersProfile *profile(new AdblockContentFiltersProfile(profileSummary, languages, flags, m_instance));

		m_contentBlockingProfiles.append(profile);

		connect(profile, &ContentFiltersProfile::profileModified, profile, [=]()
		{
			m_instance->scheduleSave();

			emit m_instance->profileModified(profile->getName());
		});
	}

	m_contentBlockingProfiles.squeeze();
}

void ContentFiltersManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		save();
	}
}

void ContentFiltersManager::scheduleSave()
{
	if (Application::isAboutToQuit())
	{
		if (m_saveTimer != 0)
		{
			killTimer(m_saveTimer);

			m_saveTimer = 0;
		}

		save();
	}
	else if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

void ContentFiltersManager::save()
{
	const QHash<ContentFiltersProfile::ProfileCategory, QString> categories({{ContentFiltersProfile::AdvertisementsCategory, QLatin1String("advertisements")}, {ContentFiltersProfile::AnnoyanceCategory, QLatin1String("annoyance")}, {ContentFiltersProfile::PrivacyCategory, QLatin1String("privacy")}, {ContentFiltersProfile::SocialCategory, QLatin1String("social")}, {ContentFiltersProfile::RegionalCategory, QLatin1String("regional")}, {ContentFiltersProfile::OtherCategory, QLatin1String("other")}});
	const QHash<CosmeticFiltersMode, QString> cosmeticFiltersModes({{AllFilters, QLatin1String("all")}, {DomainOnlyFilters, QLatin1String("domainOnly")}, {NoFilters, QLatin1String("none")}});
	JsonSettings settings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.json")));
	QJsonObject mainObject(settings.object());
	QJsonObject::iterator iterator(mainObject.begin());

	while (iterator != mainObject.end())
	{
		const QJsonObject profileObject(mainObject.value(iterator.key()).toObject());

		if (profileObject.value(QLatin1String("isHidden")).toBool())
		{
			++iterator;
		}
		else
		{
			iterator = mainObject.erase(iterator);
		}
	}

	for (int i = 0; i < m_contentBlockingProfiles.count(); ++i)
	{
		const ContentFiltersProfile *profile(m_contentBlockingProfiles.at(i));

		if (!profile)
		{
			continue;
		}

		QJsonObject profileObject;
		const int updateInterval(profile->getUpdateInterval());

		if (updateInterval > 0)
		{
			profileObject.insert(QLatin1String("updateInterval"), updateInterval);
		}

		const QDateTime lastUpdate(profile->getLastUpdate());

		if (lastUpdate.isValid())
		{
			profileObject.insert(QLatin1String("lastUpdate"), lastUpdate.toString(Qt::ISODate));
		}

		if (profile->getFlags().testFlag(ContentFiltersProfile::HasCustomTitleFlag))
		{
			profileObject.insert(QLatin1String("title"), profile->getTitle());
		}

		profileObject.insert(QLatin1String("updateUrl"), profile->getUpdateUrl().url());
		profileObject.insert(QLatin1String("category"), categories.value(profile->getCategory()));
		profileObject.insert(QLatin1String("cosmeticFiltersMode"), cosmeticFiltersModes.value(profile->getCosmeticFiltersMode()));
		profileObject.insert(QLatin1String("areWildcardsEnabled"), profile->areWildcardsEnabled());

		const QVector<QLocale::Language> languages(m_contentBlockingProfiles.at(i)->getLanguages());

		if (!languages.contains(QLocale::AnyLanguage))
		{
			QJsonArray languagesArray;

			for (int j = 0; j < languages.count(); ++j)
			{
				languagesArray.append(QLocale(languages.at(j)).name());
			}

			profileObject.insert(QLatin1String("languages"), languagesArray);
		}

		mainObject.insert(profile->getName(), profileObject);
	}

	settings.setObject(mainObject);
	settings.save();
}

void ContentFiltersManager::addProfile(ContentFiltersProfile *profile)
{
	if (!profile)
	{
		return;
	}

	bool isReplacing(false);

	for (int i = 0; i < m_contentBlockingProfiles.count(); ++i)
	{
		ContentFiltersProfile *currentProfile(m_contentBlockingProfiles.at(i));

		if (currentProfile->getName() == profile->getName())
		{
			isReplacing = true;

			currentProfile->deleteLater();

			m_contentBlockingProfiles.replace(i, profile);

			break;
		}
	}

	if (!isReplacing)
	{
		m_contentBlockingProfiles.append(profile);
	}

	m_instance->scheduleSave();

	emit m_instance->profileAdded(profile->getName());

	connect(profile, &ContentFiltersProfile::profileModified, m_instance, &ContentFiltersManager::scheduleSave);
}

void ContentFiltersManager::removeProfile(ContentFiltersProfile *profile, bool removeFile)
{
	if (!profile || (removeFile && !profile->remove()))
	{
		Console::addMessage(tr("Failed to remove content blocking profile file: %1").arg(profile ? profile->getName() : tr("Unknown")), Console::OtherCategory, Console::ErrorLevel);

		return;
	}

	const QString name(profile->getName());
	JsonSettings localSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.json")));
	QJsonObject localMainObject(localSettings.object());
	const QJsonObject bundledMainObject(JsonSettings(SessionsManager::getReadableDataPath(QLatin1String("contentBlocking.json"), true)).object());

	if (bundledMainObject.keys().contains(name))
	{
		QJsonObject profileObject(localMainObject.value(name).toObject());
		profileObject.insert(QLatin1String("isHidden"), true);

		localMainObject.insert(name, profileObject);
	}
	else
	{
		localMainObject.remove(name);
	}

	localSettings.setObject(localMainObject);
	localSettings.save();

	m_contentBlockingProfiles.removeAll(profile);

	profile->deleteLater();

	emit m_instance->profileRemoved(name);
}

ContentFiltersManager* ContentFiltersManager::getInstance()
{
	return m_instance;
}

ContentFiltersProfile* ContentFiltersManager::getProfile(const QString &name)
{
	for (int i = 0; i < m_contentBlockingProfiles.count(); ++i)
	{
		ContentFiltersProfile *profile(m_contentBlockingProfiles.at(i));

		if (profile->getName() == name)
		{
			return profile;
		}
	}

	return nullptr;
}

ContentFiltersProfile* ContentFiltersManager::getProfile(const QUrl &url)
{
	if (!url.isValid())
	{
		return nullptr;
	}

	for (int i = 0; i < m_contentBlockingProfiles.count(); ++i)
	{
		ContentFiltersProfile *profile(m_contentBlockingProfiles.at(i));

		if (profile->getUpdateUrl() == url)
		{
			return profile;
		}
	}

	return nullptr;
}

ContentFiltersProfile* ContentFiltersManager::getProfile(int identifier)
{
	return m_contentBlockingProfiles.value(identifier, nullptr);
}

ContentFiltersManager::CheckResult ContentFiltersManager::checkUrl(const QVector<int> &profiles, const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType)
{
	if (profiles.isEmpty())
	{
		return {};
	}

	const QString scheme(requestUrl.scheme());

	if (scheme != QLatin1String("http") && scheme != QLatin1String("https"))
	{
		return {};
	}

	CheckResult result;
	result.isFraud = ((resourceType == NetworkManager::MainFrameType || resourceType == NetworkManager::SubFrameType) ? isFraud(requestUrl) : false);

	for (int i = 0; i < profiles.count(); ++i)
	{
		const int profile(profiles.at(i));

		if (profile < 0 || profile >= m_contentBlockingProfiles.count())
		{
			continue;
		}

		CheckResult currentResult(m_contentBlockingProfiles.at(profile)->checkUrl(baseUrl, requestUrl, resourceType));
		currentResult.profile = profile;
		currentResult.isFraud = result.isFraud;

		if (currentResult.isBlocked)
		{
			result = currentResult;
		}
		else if (currentResult.isException)
		{
			return currentResult;
		}
	}

	return result;
}

ContentFiltersManager::CosmeticFiltersResult ContentFiltersManager::getCosmeticFilters(const QVector<int> &profiles, const QUrl &requestUrl)
{
	if (profiles.isEmpty())
	{
		return {};
	}

	const CosmeticFiltersMode mode(checkUrl(profiles, requestUrl, requestUrl, NetworkManager::OtherType).comesticFiltersMode);

	if (mode == NoFilters)
	{
		return {};
	}

	CosmeticFiltersResult result;
	const QStringList domains(createSubdomainList(requestUrl.host()));
	const bool isDomainOnly(mode == DomainOnlyFilters);

	for (int i = 0; i < profiles.count(); ++i)
	{
		const int index(profiles.at(i));

		if (index >= 0 && index < m_contentBlockingProfiles.count())
		{
			const CosmeticFiltersResult profileResult(m_contentBlockingProfiles.at(index)->getCosmeticFilters(domains, isDomainOnly));

			result.rules.append(profileResult.rules);
			result.exceptions.append(profileResult.exceptions);
		}
	}

	return result;
}

QStringList ContentFiltersManager::createSubdomainList(const QString &domain)
{
	QStringList subdomainList;
	int dotPosition(domain.lastIndexOf(QLatin1Char('.')));
	dotPosition = domain.lastIndexOf(QLatin1Char('.'), (dotPosition - 1));

	while (dotPosition != -1)
	{
		subdomainList.append(domain.mid(dotPosition + 1));

		dotPosition = domain.lastIndexOf(QLatin1Char('.'), (dotPosition - 1));
	}

	subdomainList.append(domain);

	return subdomainList;
}

QStringList ContentFiltersManager::getProfileNames()
{
	initialize();

	QStringList names;
	names.reserve(m_contentBlockingProfiles.count());

	for (int i = 0; i < m_contentBlockingProfiles.count(); ++i)
	{
		names.append(m_contentBlockingProfiles.at(i)->getName());
	}

	return names;
}

QVector<ContentFiltersProfile*> ContentFiltersManager::getContentBlockingProfiles()
{
	initialize();

	return m_contentBlockingProfiles;
}

QVector<ContentFiltersProfile*> ContentFiltersManager::getFraudCheckingProfiles()
{
	initialize();

	return m_fraudCheckingProfiles;
}

QVector<int> ContentFiltersManager::getProfileIdentifiers(const QStringList &names)
{
	if (names.isEmpty())
	{
		return {};
	}

	initialize();

	QVector<int> identifiers;
	identifiers.reserve(names.count());

	for (int i = 0; i < m_contentBlockingProfiles.count(); ++i)
	{
		if (names.contains(m_contentBlockingProfiles.at(i)->getName()))
		{
			identifiers.append(i);
		}
	}

	return identifiers;
}

bool ContentFiltersManager::isFraud(const QUrl &url)
{
	for (int i = 0; i < m_fraudCheckingProfiles.count(); ++i)
	{
		if (m_fraudCheckingProfiles.at(i)->isFraud(url))
		{
			return true;
		}
	}

	return false;
}

ContentFiltersProfile::ContentFiltersProfile(QObject *parent) : QObject(parent)
{
}

bool ContentFiltersProfile::isFraud(const QUrl &url)
{
	Q_UNUSED(url)

	return false;
}

}
