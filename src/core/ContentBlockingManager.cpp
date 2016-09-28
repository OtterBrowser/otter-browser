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
#include "Utils.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtGui/QStandardItemModel>

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
	}
}

QStandardItemModel* ContentBlockingManager::createModel(QObject *parent, const QStringList &profiles)
{
	const QSettings profilesSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.ini")), QSettings::IniFormat);
	QHash<ContentBlockingProfile::ProfileCategory, QMultiMap<QString, QList<QStandardItem*> > > categoryEntries;
	QStandardItemModel *model(new QStandardItemModel(parent));
	model->setHorizontalHeaderLabels(QStringList({tr("Title"), tr("Update Interval"), tr("Last Update")}));

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
			const QList<QLocale::Language> languages(m_profiles.at(i)->getLanguages());
			QStringList languageNames;

			for (int j = 0; j < languages.count(); ++j)
			{
				languageNames.append(QLocale::languageToString(languages.at(j)));
			}

			title = QStringLiteral("%1 [%2]").arg(title).arg(languageNames.join(QLatin1String(", ")));
		}

		QList<QStandardItem*> profileItems({new QStandardItem(title), new QStandardItem(profilesSettings.value(name + QLatin1String("/updateInterval")).toString()), new QStandardItem(Utils::formatDateTime(profilesSettings.value(name + QLatin1String("/lastUpdate")).toDateTime()))});
		profileItems[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		profileItems[0]->setData(name, Qt::UserRole);
		profileItems[0]->setData(m_profiles.at(i)->getUpdateUrl(), (Qt::UserRole + 1));
		profileItems[0]->setCheckable(true);
		profileItems[0]->setCheckState(profiles.contains(name) ? Qt::Checked : Qt::Unchecked);
		profileItems[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		profileItems[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		if (!categoryEntries.contains(category))
		{
			categoryEntries[category] = QMultiMap<QString, QList<QStandardItem*> >();
		}

		categoryEntries[category].insert(title, profileItems);
	}

	QList<QPair<ContentBlockingProfile::ProfileCategory, QString> > categoryTitles({qMakePair(ContentBlockingProfile::AdvertisementsCategory, tr("Advertisements")), qMakePair(ContentBlockingProfile::AnnoyanceCategory, tr("Annoyance")), qMakePair(ContentBlockingProfile::PrivacyCategory, tr("Privacy")), qMakePair(ContentBlockingProfile::SocialCategory, tr("Social")), qMakePair(ContentBlockingProfile::RegionalCategory, tr("Regional")), qMakePair(ContentBlockingProfile::OtherCategory, tr("Other"))});

	for (int i = 0; i < categoryTitles.count(); ++i)
	{
		if (!categoryEntries.contains(categoryTitles.at(i).first))
		{
			continue;
		}

		const QList<QList<QStandardItem*> > profileItems(categoryEntries[categoryTitles.at(i).first].values());
		QStandardItem* categoryItem(new QStandardItem(categoryTitles.at(i).second));
		categoryItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		for (int j = 0; j < profileItems.count(); ++j)
		{
			categoryItem->appendRow(profileItems.at(j));
		}

		model->appendRow(categoryItem);
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

	return NULL;
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

	for (int i = 0; i < profiles.count(); ++i)
	{
		if (profiles[i] >= 0 && profiles[i] < m_profiles.count())
		{
			const CheckResult currentResult(m_profiles.at(profiles[i])->checkUrl(baseUrl, requestUrl, resourceType));

			if (currentResult.isBlocked)
			{
				return currentResult;
			}
		}
	}

	return CheckResult();
}

QStringList ContentBlockingManager::createSubdomainList(const QString &domain)
{
	QStringList subdomainList;
	int dotPosition(domain.lastIndexOf(QLatin1Char('.')));
	dotPosition = domain.lastIndexOf(QLatin1Char('.'), dotPosition - 1);

	while (dotPosition != -1)
	{
		subdomainList.append(domain.mid(dotPosition + 1));

		dotPosition = domain.lastIndexOf(QLatin1Char('.'), dotPosition - 1);
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
		QList<QFileInfo> allProfiles(QDir(QLatin1String(":/contentBlocking/")).entryInfoList(QStringList(QLatin1String("*.txt")), QDir::Files));
		allProfiles.append(QDir(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking"))).entryInfoList(QStringList(QLatin1String("*.txt")), QDir::Files));

		for (int i = 0; i < allProfiles.count(); ++i)
		{
			const QString name(allProfiles.at(i).completeBaseName());

			if (!profiles.contains(name))
			{
				profiles.append(name);
			}
		}

		profiles.sort();

		m_profiles.reserve(profiles.count());

		for (int i = 0; i < profiles.count(); ++i)
		{
			ContentBlockingProfile *profile(new ContentBlockingProfile(profiles.at(i), m_instance));

			m_profiles.append(profile);

			connect(profile, SIGNAL(profileModified(QString)), m_instance, SIGNAL(profileModified(QString)));
		}
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
