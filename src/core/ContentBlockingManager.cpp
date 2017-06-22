/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "ContentBlockingManager.h"
#include "Console.h"
#include "ContentBlockingProfile.h"
#include "JsonSettings.h"
#include "SettingsManager.h"
#include "SessionsManager.h"
#include "Utils.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtGui/QStandardItemModel>

namespace Otter
{

ContentBlockingManager* ContentBlockingManager::m_instance(nullptr);
QVector<ContentBlockingProfile*> ContentBlockingManager::m_profiles;
ContentBlockingManager::CosmeticFiltersMode ContentBlockingManager::m_cosmeticFiltersMode(AllFiltersMode);
bool ContentBlockingManager::m_areWildcardsEnabled(true);

ContentBlockingManager::ContentBlockingManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
	m_areWildcardsEnabled = SettingsManager::getOption(SettingsManager::ContentBlocking_EnableWildcardsOption).toBool();

	handleOptionChanged(SettingsManager::ContentBlocking_CosmeticFiltersModeOption, SettingsManager::getOption(SettingsManager::ContentBlocking_CosmeticFiltersModeOption).toString());

	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));
}

void ContentBlockingManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new ContentBlockingManager(QCoreApplication::instance());
	}
}

void ContentBlockingManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		const QHash<ContentBlockingProfile::ProfileCategory, QString> categoryTitles({{ContentBlockingProfile::AdvertisementsCategory, QLatin1String("advertisements")}, {ContentBlockingProfile::AnnoyanceCategory, QLatin1String("annoyance")}, {ContentBlockingProfile::PrivacyCategory, QLatin1String("privacy")}, {ContentBlockingProfile::SocialCategory, QLatin1String("social")}, {ContentBlockingProfile::RegionalCategory, QLatin1String("regional")}, {ContentBlockingProfile::OtherCategory, QLatin1String("other")}});
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

		for (int i = 0; i < m_profiles.count(); ++i)
		{
			ContentBlockingProfile *profile(m_profiles.at(i));

			if (profile == nullptr || profile->getName() == QLatin1String("custom"))
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

			if (profile->getFlags().testFlag(ContentBlockingProfile::HasCustomTitleFlag))
			{
				profileObject.insert(QLatin1String("title"), profile->getTitle());
			}

			if (profile->getFlags().testFlag(ContentBlockingProfile::HasCustomUpdateUrlFlag))
			{
				profileObject.insert(QLatin1String("updateUrl"), profile->getUpdateUrl().url());
			}

			profileObject.insert(QLatin1String("category"), categoryTitles.value(profile->getCategory()));

			const QVector<QLocale::Language> languages(m_profiles.at(i)->getLanguages());

			if (!languages.contains(QLocale::AnyLanguage))
			{
				QJsonArray languageNames;

				for (int j = 0; j < languages.count(); ++j)
				{
					languageNames.append(QLocale(languages.at(j)).name());
				}

				profileObject.insert(QLatin1String("languages"), languageNames);
			}

			mainObject.insert(profile->getName(), profileObject);
		}

		settings.setObject(mainObject);
		settings.save();
	}
}

void ContentBlockingManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

void ContentBlockingManager::addProfile(ContentBlockingProfile *profile)
{
	if (profile)
	{
		m_profiles.append(profile);

		getInstance()->scheduleSave();

		connect(profile, SIGNAL(profileModified(QString)), m_instance, SLOT(scheduleSave()));
	}
}

void ContentBlockingManager::handleOptionChanged(int identifier, const QVariant &value)
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
					m_cosmeticFiltersMode = NoFiltersMode;
				}
				else if (cosmeticFiltersMode == QLatin1String("domainOnly"))
				{
					m_cosmeticFiltersMode = DomainOnlyFiltersMode;
				}
				else
				{
					m_cosmeticFiltersMode = AllFiltersMode;
				}
			}

			break;
		default:
			return;
	}

	for (int i = 0; i < m_profiles.count(); ++i)
	{
		m_profiles[i]->clear();
	}
}

void ContentBlockingManager::removeProfile(ContentBlockingProfile *profile)
{
	if (!profile || !profile->remove())
	{
		Console::addMessage(tr("Failed to remove content blocking profile file: %1").arg(profile ? profile->getName() : tr("Unknown")), Console::OtherCategory, Console::ErrorLevel);

		return;
	}

	JsonSettings localSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.json")));
	QJsonObject localMainObject(localSettings.object());
	QJsonObject bundledMainObject(JsonSettings(SessionsManager::getReadableDataPath(QLatin1String("contentBlocking.json"), true)).object());

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

	m_profiles.removeAll(profile);

	profile->deleteLater();
}

QStandardItemModel* ContentBlockingManager::createModel(QObject *parent, const QStringList &profiles)
{
	QHash<ContentBlockingProfile::ProfileCategory, QMultiMap<QString, QList<QStandardItem*> > > categoryEntries;
	QStandardItemModel *model(new QStandardItemModel(parent));
	model->setHorizontalHeaderLabels(QStringList({tr("Title"), tr("Update Interval"), tr("Last Update")}));

	if (m_profiles.isEmpty())
	{
		getProfiles();
	}

	for (int i = 0; i < m_profiles.count(); ++i)
	{
		const QString name(m_profiles.at(i)->getName());

		if (name == QLatin1String("custom"))
		{
			continue;
		}

		ContentBlockingProfile::ProfileCategory category(m_profiles.at(i)->getCategory());
		QString title(m_profiles.at(i)->getTitle());

		if (category == ContentBlockingProfile::RegionalCategory)
		{
			const QVector<QLocale::Language> languages(m_profiles.at(i)->getLanguages());
			QStringList languageNames;

			for (int j = 0; j < languages.count(); ++j)
			{
				languageNames.append(QLocale::languageToString(languages.at(j)));
			}

			title = QStringLiteral("%1 [%2]").arg(title).arg(languageNames.join(QLatin1String(", ")));
		}

		QList<QStandardItem*> profileItems({new QStandardItem(title), new QStandardItem(QString::number(m_profiles.at(i)->getUpdateInterval())), new QStandardItem(Utils::formatDateTime(m_profiles.at(i)->getLastUpdate()))});
		profileItems[0]->setData(name, Qt::UserRole);
		profileItems[0]->setData(m_profiles.at(i)->getUpdateUrl(), (Qt::UserRole + 1));
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

	const QVector<QPair<ContentBlockingProfile::ProfileCategory, QString> > categoryTitles({{ContentBlockingProfile::AdvertisementsCategory, tr("Advertisements")}, {ContentBlockingProfile::AnnoyanceCategory, tr("Annoyance")}, {ContentBlockingProfile::PrivacyCategory, tr("Privacy")}, {ContentBlockingProfile::SocialCategory, tr("Social")}, {ContentBlockingProfile::RegionalCategory, tr("Regional")}, {ContentBlockingProfile::OtherCategory, tr("Other")}});

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

ContentBlockingManager* ContentBlockingManager::getInstance()
{
	return m_instance;
}

ContentBlockingProfile* ContentBlockingManager::getProfile(const QString &profile)
{
	for (int i = 0; i < m_profiles.count(); ++i)
	{
		if (m_profiles[i]->getName() == profile)
		{
			return m_profiles[i];
		}
	}

	return nullptr;
}

ContentBlockingManager::CheckResult ContentBlockingManager::checkUrl(const QVector<int> &profiles, const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType)
{
	if (profiles.isEmpty())
	{
		return CheckResult();
	}

	const QString scheme(requestUrl.scheme());

	if (scheme != QLatin1String("http") && scheme != QLatin1String("https"))
	{
		return CheckResult();
	}

	CheckResult result;

	for (int i = 0; i < profiles.count(); ++i)
	{
		if (profiles[i] >= 0 && profiles[i] < m_profiles.count())
		{
			const CheckResult currentResult(m_profiles.at(profiles[i])->checkUrl(baseUrl, requestUrl, resourceType));

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

QStringList ContentBlockingManager::createSubdomainList(const QString &domain)
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

QStringList ContentBlockingManager::getStyleSheetBlackList(const QString &domain, const QVector<int> &profiles)
{
	QStringList data;

	for (int i = 0; i < profiles.count(); ++i)
	{
		if (profiles[i] >= 0 && profiles[i] < m_profiles.count())
		{
			data.append(m_profiles.at(profiles[i])->getStyleSheetBlackList(domain));
		}
	}

	return data;
}

QStringList ContentBlockingManager::getStyleSheetWhiteList(const QString &domain, const QVector<int> &profiles)
{
	QStringList data;

	for (int i = 0; i < profiles.count(); ++i)
	{
		if (profiles[i] >= 0 && profiles[i] < m_profiles.count())
		{
			data.append(m_profiles.at(profiles[i])->getStyleSheetWhiteList(domain));
		}
	}

	return data;
}

QVector<ContentBlockingProfile*> ContentBlockingManager::getProfiles()
{
	if (m_profiles.isEmpty())
	{
		QStringList profiles;
		const QList<QFileInfo> existingProfiles(QDir(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking"))).entryInfoList(QStringList(QLatin1String("*.txt")), QDir::Files));
		QJsonObject bundledMainObject(JsonSettings(SessionsManager::getReadableDataPath(QLatin1String("contentBlocking.json"), true)).object());

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

		m_profiles.reserve(profiles.count());

		QJsonObject localMainObject(JsonSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.json"))).object());
		const QHash<QString, ContentBlockingProfile::ProfileCategory> categoryTitles({{QLatin1String("advertisements"), ContentBlockingProfile::AdvertisementsCategory}, {QLatin1String("annoyance"), ContentBlockingProfile::AnnoyanceCategory}, {QLatin1String("privacy"), ContentBlockingProfile::PrivacyCategory}, {QLatin1String("social"), ContentBlockingProfile::SocialCategory}, {QLatin1String("regional"), ContentBlockingProfile::RegionalCategory}, {QLatin1String("other"), ContentBlockingProfile::OtherCategory}});

		for (int i = 0; i < profiles.count(); ++i)
		{
			QJsonObject profileObject(localMainObject.value(profiles.at(i)).toObject());
			const QJsonObject bundledProfileObject(bundledMainObject.value(profiles.at(i)).toObject());
			QString title;
			QUrl updateUrl;
			ContentBlockingProfile::ProfileFlags flags(ContentBlockingProfile::NoFlags);

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
					flags |= ContentBlockingProfile::HasCustomUpdateUrlFlag;
				}

				if (title.isEmpty())
				{
					title = bundledProfileObject.value(QLatin1String("title")).toString();
				}
				else
				{
					flags |= ContentBlockingProfile::HasCustomTitleFlag;
				}
			}

			const QJsonArray languages(profileObject.value(QLatin1String("languages")).toArray());
			QStringList parsedLanguages;

			for (int j = 0; j < languages.count(); ++j)
			{
				parsedLanguages.append(languages.at(j).toString());
			}

			ContentBlockingProfile *profile(new ContentBlockingProfile(profiles.at(i), title, updateUrl, QDateTime::fromString(profileObject.value(QLatin1String("lastUpdate")).toString(), Qt::ISODate), parsedLanguages, profileObject.value(QLatin1String("updateInterval")).toInt(), categoryTitles.value(profileObject.value(QLatin1String("category")).toString()), flags, m_instance));

			m_profiles.append(profile);

			connect(profile, SIGNAL(profileModified(QString)), m_instance, SIGNAL(profileModified(QString)));
			connect(profile, SIGNAL(profileModified(QString)), m_instance, SLOT(scheduleSave()));
		}

		m_profiles.squeeze();
	}

	return m_profiles;
}

QVector<int> ContentBlockingManager::getProfileList(const QStringList &names)
{
	QVector<int> profiles;
	profiles.reserve(names.count());

	if (m_profiles.isEmpty())
	{
		getProfiles();
	}

	for (int i = 0; i < m_profiles.count(); ++i)
	{
		if (names.contains(m_profiles.at(i)->getName()))
		{
			profiles.append(i);
		}
	}

	return profiles;
}

ContentBlockingManager::CosmeticFiltersMode ContentBlockingManager::getCosmeticFiltersMode()
{
	return m_cosmeticFiltersMode;
}

bool ContentBlockingManager::areWildcardsEnabled()
{
	return m_areWildcardsEnabled;
}

bool ContentBlockingManager::updateProfile(const QString &profile)
{
	for (int i = 0; i < m_profiles.count(); ++i)
	{
		if (m_profiles.at(i)->getName() == profile)
		{
			return m_profiles[i]->downloadRules();
		}
	}

	return false;
}

}
