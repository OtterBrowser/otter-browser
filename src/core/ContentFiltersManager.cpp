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

#include "ContentFiltersManager.h"
#include "AdblockContentFiltersProfile.h"
#include "Console.h"
#include "JsonSettings.h"
#include "SettingsManager.h"
#include "SessionsManager.h"
#include "Utils.h"
#include "../ui/ItemViewWidget.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtGui/QStandardItemModel>

namespace Otter
{

ContentFiltersManager* ContentFiltersManager::m_instance(nullptr);
QVector<ContentFiltersProfile*> ContentFiltersManager::m_contentBlockingProfiles;
QVector<ContentFiltersProfile*> ContentFiltersManager::m_fraudCheckingProfiles;
ContentFiltersManager::CosmeticFiltersMode ContentFiltersManager::m_cosmeticFiltersMode(AllFilters);
bool ContentFiltersManager::m_areWildcardsEnabled(true);

ContentFiltersManager::ContentFiltersManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
	m_areWildcardsEnabled = SettingsManager::getOption(SettingsManager::ContentBlocking_EnableWildcardsOption).toBool();

	handleOptionChanged(SettingsManager::ContentBlocking_CosmeticFiltersModeOption, SettingsManager::getOption(SettingsManager::ContentBlocking_CosmeticFiltersModeOption).toString());

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &ContentFiltersManager::handleOptionChanged);
}

void ContentFiltersManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new ContentFiltersManager(QCoreApplication::instance());
	}
}

void ContentFiltersManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		const QHash<ContentFiltersProfile::ProfileCategory, QString> categoryTitles({{ContentFiltersProfile::AdvertisementsCategory, QLatin1String("advertisements")}, {ContentFiltersProfile::AnnoyanceCategory, QLatin1String("annoyance")}, {ContentFiltersProfile::PrivacyCategory, QLatin1String("privacy")}, {ContentFiltersProfile::SocialCategory, QLatin1String("social")}, {ContentFiltersProfile::RegionalCategory, QLatin1String("regional")}, {ContentFiltersProfile::OtherCategory, QLatin1String("other")}});
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

			if (!profile || profile->getName() == QLatin1String("custom"))
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

			if (profile->getFlags().testFlag(ContentFiltersProfile::HasCustomUpdateUrlFlag))
			{
				profileObject.insert(QLatin1String("updateUrl"), profile->getUpdateUrl().url());
			}

			profileObject.insert(QLatin1String("category"), categoryTitles.value(profile->getCategory()));

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
}

void ContentFiltersManager::ensureInitialized()
{
	if (!m_contentBlockingProfiles.isEmpty())
	{
		return;
	}

	const QList<QFileInfo> existingProfiles(QDir(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking"))).entryInfoList({QLatin1String("*.txt")}, QDir::Files));
	QJsonObject bundledMainObject(JsonSettings(SessionsManager::getReadableDataPath(QLatin1String("contentBlocking.json"), true)).object());
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

	QJsonObject localMainObject(JsonSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.json"))).object());
	const QHash<QString, ContentFiltersProfile::ProfileCategory> categoryTitles({{QLatin1String("advertisements"), ContentFiltersProfile::AdvertisementsCategory}, {QLatin1String("annoyance"), ContentFiltersProfile::AnnoyanceCategory}, {QLatin1String("privacy"), ContentFiltersProfile::PrivacyCategory}, {QLatin1String("social"), ContentFiltersProfile::SocialCategory}, {QLatin1String("regional"), ContentFiltersProfile::RegionalCategory}, {QLatin1String("other"), ContentFiltersProfile::OtherCategory}});

	for (int i = 0; i < profiles.count(); ++i)
	{
		QJsonObject profileObject(localMainObject.value(profiles.at(i)).toObject());
		const QJsonObject bundledProfileObject(bundledMainObject.value(profiles.at(i)).toObject());
		QString title;
		QUrl updateUrl;
		ContentFiltersProfile::ProfileFlags flags(ContentFiltersProfile::NoFlags);

		if (profiles.at(i) == QLatin1String("custom"))
		{
			title = tr("Custom Rules");
		}
		else if (profileObject.isEmpty())
		{
			profileObject = bundledProfileObject;
			updateUrl = QUrl(profileObject.value(QLatin1String("updateUrl")).toString());
			title = profileObject.value(QLatin1String("title")).toString();
		}
		else
		{
			if (profileObject.value(QLatin1String("isHidden")).toBool())
			{
				continue;
			}

			updateUrl = QUrl(profileObject.value(QLatin1String("updateUrl")).toString());
			title = profileObject.value(QLatin1String("title")).toString();

			if (updateUrl.isEmpty())
			{
				updateUrl = QUrl(bundledProfileObject.value(QLatin1String("updateUrl")).toString());
			}
			else
			{
				flags |= ContentFiltersProfile::HasCustomUpdateUrlFlag;
			}

			if (title.isEmpty())
			{
				title = bundledProfileObject.value(QLatin1String("title")).toString();
			}
			else
			{
				flags |= ContentFiltersProfile::HasCustomTitleFlag;
			}
		}

		QDateTime lastUpdate(QDateTime::fromString(profileObject.value(QLatin1String("lastUpdate")).toString(), Qt::ISODate));
		lastUpdate.setTimeSpec(Qt::UTC);

		const QJsonArray languagesArray(profileObject.value(QLatin1String("languages")).toArray());
		QStringList languages;
		languages.reserve(languagesArray.count());

		for (int j = 0; j < languagesArray.count(); ++j)
		{
			languages.append(languagesArray.at(j).toString());
		}

		ContentFiltersProfile *profile(new AdblockContentFiltersProfile(profiles.at(i), title, updateUrl, lastUpdate, languages, profileObject.value(QLatin1String("updateInterval")).toInt(), categoryTitles.value(profileObject.value(QLatin1String("category")).toString()), flags, m_instance));

		m_contentBlockingProfiles.append(profile);

		connect(profile, &ContentFiltersProfile::profileModified, m_instance, &ContentFiltersManager::profileModified);
		connect(profile, &ContentFiltersProfile::profileModified, m_instance, &ContentFiltersManager::scheduleSave);
	}

	m_contentBlockingProfiles.squeeze();
}

void ContentFiltersManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

void ContentFiltersManager::addProfile(ContentFiltersProfile *profile)
{
	if (profile)
	{
		m_contentBlockingProfiles.append(profile);

		getInstance()->scheduleSave();

		connect(profile, &ContentFiltersProfile::profileModified, m_instance, &ContentFiltersManager::scheduleSave);
	}
}

void ContentFiltersManager::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::ContentBlocking_EnableWildcardsOption:
			m_areWildcardsEnabled = value.toBool();

			break;
		case SettingsManager::ContentBlocking_CosmeticFiltersModeOption:
			{
				const QString cosmeticFiltersMode(value.toString());

				if (cosmeticFiltersMode == QLatin1String("none"))
				{
					m_cosmeticFiltersMode = NoFilters;
				}
				else if (cosmeticFiltersMode == QLatin1String("domainOnly"))
				{
					m_cosmeticFiltersMode = DomainOnlyFilters;
				}
				else
				{
					m_cosmeticFiltersMode = AllFilters;
				}
			}

			break;
		default:
			return;
	}

	for (int i = 0; i < m_contentBlockingProfiles.count(); ++i)
	{
		m_contentBlockingProfiles.at(i)->clear();
	}
}

void ContentFiltersManager::removeProfile(ContentFiltersProfile *profile)
{
	if (!profile || !profile->remove())
	{
		Console::addMessage(tr("Failed to remove content blocking profile file: %1").arg(profile ? profile->getName() : tr("Unknown")), Console::OtherCategory, Console::ErrorLevel);

		return;
	}

	JsonSettings localSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.json")));
	QJsonObject localMainObject(localSettings.object());
	const QJsonObject bundledMainObject(JsonSettings(SessionsManager::getReadableDataPath(QLatin1String("contentBlocking.json"), true)).object());

	if (bundledMainObject.keys().contains(profile->getName()))
	{
		QJsonObject profileObject(localMainObject.value(profile->getName()).toObject());
		profileObject.insert(QLatin1String("isHidden"), true);

		localMainObject.insert(profile->getName(), profileObject);
	}
	else
	{
		localMainObject.remove(profile->getName());
	}

	localSettings.setObject(localMainObject);
	localSettings.save();

	m_contentBlockingProfiles.removeAll(profile);

	profile->deleteLater();
}

QStandardItemModel* ContentFiltersManager::createModel(QObject *parent, const QStringList &profiles)
{
	ensureInitialized();

	QHash<ContentFiltersProfile::ProfileCategory, QMultiMap<QString, QList<QStandardItem*> > > categoryEntries;
	QStandardItemModel *model(new QStandardItemModel(parent));
	model->setHorizontalHeaderLabels({tr("Title"), tr("Update Interval"), tr("Last Update")});
	model->setHeaderData(0, Qt::Horizontal, 250, HeaderViewWidget::WidthRole);

	for (int i = 0; i < m_contentBlockingProfiles.count(); ++i)
	{
		const QString name(m_contentBlockingProfiles.at(i)->getName());

		if (name == QLatin1String("custom"))
		{
			continue;
		}

		const ContentFiltersProfile::ProfileCategory category(m_contentBlockingProfiles.at(i)->getCategory());
		QString title(m_contentBlockingProfiles.at(i)->getTitle());

		if (category == ContentFiltersProfile::RegionalCategory)
		{
			const QVector<QLocale::Language> languages(m_contentBlockingProfiles.at(i)->getLanguages());
			QStringList languageNames;
			languageNames.reserve(languages.count());

			for (int j = 0; j < languages.count(); ++j)
			{
				languageNames.append(QLocale::languageToString(languages.at(j)));
			}

			title = QStringLiteral("%1 [%2]").arg(title).arg(languageNames.join(QLatin1String(", ")));
		}

		QList<QStandardItem*> profileItems({new QStandardItem(title), new QStandardItem(QString::number(m_contentBlockingProfiles.at(i)->getUpdateInterval())), new QStandardItem(Utils::formatDateTime(m_contentBlockingProfiles.at(i)->getLastUpdate()))});
		profileItems[0]->setData(name, NameRole);
		profileItems[0]->setData(m_contentBlockingProfiles.at(i)->getUpdateUrl(), UpdateUrlRole);
		profileItems[0]->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		profileItems[0]->setCheckable(true);
		profileItems[0]->setCheckState(profiles.contains(name) ? Qt::Checked : Qt::Unchecked);
		profileItems[1]->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		profileItems[2]->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		if (!categoryEntries.contains(category))
		{
			categoryEntries[category] = QMultiMap<QString, QList<QStandardItem*> >();
		}

		categoryEntries[category].insert(title, profileItems);
	}

	const QVector<QPair<ContentFiltersProfile::ProfileCategory, QString> > categoryTitles({{ContentFiltersProfile::AdvertisementsCategory, tr("Advertisements")}, {ContentFiltersProfile::AnnoyanceCategory, tr("Annoyance")}, {ContentFiltersProfile::PrivacyCategory, tr("Privacy")}, {ContentFiltersProfile::SocialCategory, tr("Social")}, {ContentFiltersProfile::RegionalCategory, tr("Regional")}, {ContentFiltersProfile::OtherCategory, tr("Other")}});

	for (int i = 0; i < categoryTitles.count(); ++i)
	{
		if (!categoryEntries.contains(categoryTitles.at(i).first))
		{
			continue;
		}

		const QList<QList<QStandardItem*> > profileItems(categoryEntries[categoryTitles.at(i).first].values());
		QList<QStandardItem*> categoryItems({new QStandardItem(categoryTitles.at(i).second), new QStandardItem(), new QStandardItem()});
		categoryItems[0]->setData(categoryTitles.at(i).first, Qt::UserRole);
		categoryItems[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		categoryItems[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		categoryItems[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		for (int j = 0; j < profileItems.count(); ++j)
		{
			categoryItems[0]->appendRow(profileItems.at(j));
		}

		model->appendRow(categoryItems);
	}

	return model;
}

ContentFiltersManager* ContentFiltersManager::getInstance()
{
	return m_instance;
}

ContentFiltersProfile* ContentFiltersManager::getProfile(const QString &profile)
{
	for (int i = 0; i < m_contentBlockingProfiles.count(); ++i)
	{
		if (m_contentBlockingProfiles.at(i)->getName() == profile)
		{
			return m_contentBlockingProfiles.at(i);
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
		if (m_contentBlockingProfiles.at(i)->getUpdateUrl() == url)
		{
			return m_contentBlockingProfiles.at(i);
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
		if (profiles.at(i) >= 0 && profiles.at(i) < m_contentBlockingProfiles.count())
		{
			CheckResult currentResult(m_contentBlockingProfiles.at(profiles.at(i))->checkUrl(baseUrl, requestUrl, resourceType));
			currentResult.profile = profiles.at(i);
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
	}

	return result;
}

ContentFiltersManager::CosmeticFiltersResult ContentFiltersManager::getCosmeticFilters(const QVector<int> &profiles, const QUrl &requestUrl)
{
	if (profiles.isEmpty() || m_cosmeticFiltersMode == NoFilters)
	{
		return {};
	}

	const CosmeticFiltersMode mode(checkUrl(profiles, requestUrl, requestUrl, NetworkManager::OtherType).comesticFiltersMode);

	if (mode == ContentFiltersManager::NoFilters)
	{
		return {};
	}

	CosmeticFiltersResult result;
	const QStringList domains(createSubdomainList(requestUrl.host()));

	for (int i = 0; i < profiles.count(); ++i)
	{
		const int index(profiles.at(i));

		if (index >= 0 && index < m_contentBlockingProfiles.count())
		{
			const CosmeticFiltersResult profileResult(m_contentBlockingProfiles.at(index)->getCosmeticFilters(domains, (mode == DomainOnlyFilters)));

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
	ensureInitialized();

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
	ensureInitialized();

	return m_contentBlockingProfiles;
}

QVector<ContentFiltersProfile*> ContentFiltersManager::getFraudCheckingProfiles()
{
	ensureInitialized();

	return m_fraudCheckingProfiles;
}

QVector<int> ContentFiltersManager::getProfileIdentifiers(const QStringList &names)
{
	ensureInitialized();

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

ContentFiltersManager::CosmeticFiltersMode ContentFiltersManager::getCosmeticFiltersMode()
{
	return m_cosmeticFiltersMode;
}

bool ContentFiltersManager::areWildcardsEnabled()
{
	return m_areWildcardsEnabled;
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

ContentFiltersManager::CheckResult ContentFiltersProfile::checkUrl(const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType)
{
	Q_UNUSED(baseUrl)
	Q_UNUSED(requestUrl)
	Q_UNUSED(resourceType)

	return {};
}

ContentFiltersManager::CosmeticFiltersResult ContentFiltersProfile::getCosmeticFilters(const QStringList &domains, bool isDomainOnly)
{
	Q_UNUSED(domains)
	Q_UNUSED(isDomainOnly)

	return {};
}

ContentFiltersProfile::ProfileCategory ContentFiltersProfile::getCategory() const
{
	return OtherCategory;
}

QVector<QLocale::Language> ContentFiltersProfile::getLanguages() const
{
	return {};
}

bool ContentFiltersProfile::isFraud(const QUrl &url)
{
	Q_UNUSED(url)

	return false;
}

}
