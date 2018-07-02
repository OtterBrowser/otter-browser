/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "HistoryManager.h"
#include "AddonsManager.h"
#include "Application.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "ThemesManager.h"

#include <QtCore/QTimerEvent>

namespace Otter
{

HistoryManager* HistoryManager::m_instance(nullptr);
HistoryModel* HistoryManager::m_browsingHistoryModel(nullptr);
HistoryModel* HistoryManager::m_typedHistoryModel(nullptr);
bool HistoryManager::m_isEnabled(false);
bool HistoryManager::m_isStoringFavicons(true);

HistoryManager::HistoryManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
	m_dayTimer = startTimer(QTime::currentTime().msecsTo(QTime(23, 59, 59, 999)));

	handleOptionChanged(SettingsManager::History_RememberBrowsingOption);
	handleOptionChanged(SettingsManager::History_StoreFaviconsOption);

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &HistoryManager::handleOptionChanged);
}

void HistoryManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new HistoryManager(QCoreApplication::instance());
	}
}

void HistoryManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		save();
	}
	else if (event->timerId() == m_dayTimer)
	{
		killTimer(m_dayTimer);

		if (!m_browsingHistoryModel)
		{
			getBrowsingHistoryModel();
		}

		if (!m_typedHistoryModel)
		{
			getTypedHistoryModel();
		}

		const int period(SettingsManager::getOption(SettingsManager::History_BrowsingLimitPeriodOption).toInt());

		m_browsingHistoryModel->clearOldestEntries(period);
		m_typedHistoryModel->clearOldestEntries(period);

		scheduleSave();

		emit dayChanged();

		m_dayTimer = startTimer(QTime::currentTime().msecsTo(QTime(23, 59, 59, 999)));
	}
}

void HistoryManager::scheduleSave()
{
	if (Application::isAboutToQuit())
	{
		save();
	}
	else if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

void HistoryManager::save()
{
	if (m_browsingHistoryModel)
	{
		m_browsingHistoryModel->save(SessionsManager::getWritableDataPath(QLatin1String("browsingHistory.json")));
	}

	if (m_typedHistoryModel)
	{
		m_typedHistoryModel->save(SessionsManager::getWritableDataPath(QLatin1String("typedHistory.json")));
	}
}

void HistoryManager::clearHistory(uint period)
{
	if (!m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	if (!m_typedHistoryModel)
	{
		getTypedHistoryModel();
	}

	m_browsingHistoryModel->clearRecentEntries(period);
	m_typedHistoryModel->clearRecentEntries(period);

	m_instance->scheduleSave();
}

void HistoryManager::removeEntry(quint64 identifier)
{
	if (!m_isEnabled)
	{
		return;
	}

	if (!m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	m_browsingHistoryModel->removeEntry(identifier);

	m_instance->scheduleSave();
}

void HistoryManager::removeEntries(const QVector<quint64> &identifiers)
{
	if (!m_isEnabled)
	{
		return;
	}

	if (!m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	for (int i = 0; i < identifiers.count(); ++i)
	{
		m_browsingHistoryModel->removeEntry(identifiers.at(i));
	}

	m_instance->scheduleSave();
}

void HistoryManager::updateEntry(quint64 identifier, const QUrl &url, const QString &title, const QIcon &icon)
{
	if (!m_isEnabled || !url.isValid())
	{
		return;
	}

	if (!SettingsManager::getOption(SettingsManager::History_RememberBrowsingOption, Utils::extractHost(url)).toBool())
	{
		removeEntry(identifier);

		return;
	}

	if (!m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	HistoryModel::Entry *item(m_browsingHistoryModel->getEntry(identifier));

	if (item)
	{
		item->setData(url, HistoryModel::UrlRole);
		item->setData(title, HistoryModel::TitleRole);
		item->setIcon(icon);
	}

	m_instance->scheduleSave();
}

void HistoryManager::handleOptionChanged(int identifier)
{
	switch (identifier)
	{
		case SettingsManager::Browser_PrivateModeOption:
		case SettingsManager::History_RememberBrowsingOption:
			m_isEnabled = (SettingsManager::getOption(SettingsManager::History_RememberBrowsingOption).toBool() && !SettingsManager::getOption(SettingsManager::Browser_PrivateModeOption).toBool());

			break;
		case SettingsManager::History_BrowsingLimitAmountGlobalOption:
			{
				if (!m_browsingHistoryModel)
				{
					getBrowsingHistoryModel();
				}

				if (!m_typedHistoryModel)
				{
					getTypedHistoryModel();
				}

				const int limit(SettingsManager::getOption(SettingsManager::History_BrowsingLimitAmountGlobalOption).toInt());

				m_browsingHistoryModel->clearExcessEntries(limit);
				m_typedHistoryModel->clearExcessEntries(limit);

				scheduleSave();
			}

			break;
		case SettingsManager::History_BrowsingLimitPeriodOption:
			{
				if (!m_browsingHistoryModel)
				{
					getBrowsingHistoryModel();
				}

				if (!m_typedHistoryModel)
				{
					getTypedHistoryModel();
				}

				const int period(SettingsManager::getOption(SettingsManager::History_BrowsingLimitPeriodOption).toInt());

				m_browsingHistoryModel->clearOldestEntries(period);
				m_typedHistoryModel->clearOldestEntries(period);

				scheduleSave();
			}

			break;
		case SettingsManager::History_StoreFaviconsOption:
			m_isStoringFavicons = SettingsManager::getOption(identifier).toBool();

			break;
		default:
			break;
	}
}

HistoryManager* HistoryManager::getInstance()
{
	return m_instance;
}

HistoryModel* HistoryManager::getBrowsingHistoryModel()
{
	if (!m_browsingHistoryModel)
	{
		m_browsingHistoryModel = new HistoryModel(SessionsManager::getWritableDataPath(QLatin1String("browsingHistory.json")), HistoryModel::BrowsingHistory, m_instance);
	}

	return m_browsingHistoryModel;
}

HistoryModel* HistoryManager::getTypedHistoryModel()
{
	if (!m_typedHistoryModel && m_instance)
	{
		m_typedHistoryModel = new HistoryModel(SessionsManager::getWritableDataPath(QLatin1String("typedHistory.json")), HistoryModel::TypedHistory, m_instance);
	}

	return m_typedHistoryModel;
}

QIcon HistoryManager::getIcon(const QUrl &url)
{
	if (Utils::isUrlEmpty(url))
	{
		return ThemesManager::createIcon(QLatin1String("tab"));
	}

	if (url.scheme() == QLatin1String("about"))
	{
		const QStringList specialPages(AddonsManager::getSpecialPages());

		for (int i = 0; i < specialPages.count(); ++i)
		{
			const AddonsManager::SpecialPageInformation information(AddonsManager::getSpecialPage(specialPages.at(i)));

			if (url == information.url)
			{
				return information.icon;
			}
		}
	}

///TODO

	return ThemesManager::createIcon(QLatin1String("text-html"));
}

HistoryModel::Entry* HistoryManager::getEntry(quint64 identifier)
{
	if (!m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	return m_browsingHistoryModel->getEntry(identifier);
}

QVector<HistoryModel::HistoryEntryMatch> HistoryManager::findEntries(const QString &prefix, bool isTypedInOnly)
{
	if (!m_typedHistoryModel)
	{
		getTypedHistoryModel();
	}

	if (!isTypedInOnly && !m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	QVector<HistoryModel::HistoryEntryMatch> entries(m_typedHistoryModel->findEntries(prefix, true));

	if (!isTypedInOnly)
	{
		entries.append(m_browsingHistoryModel->findEntries(prefix));
	}

	return entries;
}

quint64 HistoryManager::addEntry(const QUrl &url, const QString &title, const QIcon &icon, bool isTypedIn)
{
	if (!m_isEnabled || !url.isValid() || !SettingsManager::getOption(SettingsManager::History_RememberBrowsingOption, Utils::extractHost(url)).toBool())
	{
		return 0;
	}

	if (!m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	const quint64 identifier(m_browsingHistoryModel->addEntry(url, title, icon, QDateTime::currentDateTimeUtc())->getIdentifier());

	if (isTypedIn)
	{
		if (!m_typedHistoryModel)
		{
			getTypedHistoryModel();
		}

		m_typedHistoryModel->addEntry(url, title, icon, QDateTime::currentDateTimeUtc());
	}

	const int limit(SettingsManager::getOption(SettingsManager::History_BrowsingLimitAmountGlobalOption).toInt());

	if (limit > 0 && m_browsingHistoryModel->rowCount() > limit)
	{
		for (int i = (m_browsingHistoryModel->rowCount() - 1); i >= limit; --i)
		{
			m_browsingHistoryModel->removeEntry(m_browsingHistoryModel->index(i, 0).data(HistoryModel::IdentifierRole).toULongLong());
		}
	}

	m_instance->scheduleSave();

	return identifier;
}

bool HistoryManager::hasEntry(const QUrl &url)
{
	if (!m_isEnabled || !url.isValid())
	{
		return false;
	}

	if (!m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	return m_browsingHistoryModel->hasEntry(url);
}

}
