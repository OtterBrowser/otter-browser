/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "HistoryManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "Utils.h"

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QTimerEvent>

namespace Otter
{

HistoryManager* HistoryManager::m_instance = NULL;
HistoryModel* HistoryManager::m_browsingHistoryModel = NULL;
HistoryModel* HistoryManager::m_typedHistoryModel = NULL;
bool HistoryManager::m_isEnabled = false;
bool HistoryManager::m_isStoringFavicons = true;

HistoryManager::HistoryManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
	m_dayTimer = startTimer(QTime::currentTime().msecsTo(QTime(23, 59, 59, 999)));

	optionChanged(QLatin1String("History/RememberBrowsing"));
	optionChanged(QLatin1String("History/StoreFavicons"));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

void HistoryManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new HistoryManager(parent);
	}
}

void HistoryManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		if (m_browsingHistoryModel)
		{
			m_browsingHistoryModel->save(SessionsManager::getWritableDataPath(QLatin1String("browsingHistory.json")));
		}

		if (m_typedHistoryModel)
		{
			m_typedHistoryModel->save(SessionsManager::getWritableDataPath(QLatin1String("typedHistory.json")));
		}
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

		const int period = SettingsManager::getValue(QLatin1String("History/BrowsingLimitPeriod")).toInt();

		m_browsingHistoryModel->clearOldestEntries(period);
		m_typedHistoryModel->clearOldestEntries(period);

		scheduleSave();

		emit dayChanged();

		m_dayTimer = startTimer(QTime::currentTime().msecsTo(QTime(23, 59, 59, 999)));
	}
}

void HistoryManager::optionChanged(const QString &option)
{
	if (option == QLatin1String("History/RememberBrowsing") || option == QLatin1String("Browser/PrivateMode"))
	{
		m_isEnabled =  (SettingsManager::getValue(QLatin1String("History/RememberBrowsing")).toBool() && !SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool());
	}
	else if (option == QLatin1String("History/StoreFavicons"))
	{
		m_isStoringFavicons = SettingsManager::getValue(option).toBool();
	}
	else if (option == QLatin1String("History/BrowsingLimitAmountGlobal"))
	{
		if (!m_browsingHistoryModel)
		{
			getBrowsingHistoryModel();
		}

		if (!m_typedHistoryModel)
		{
			getTypedHistoryModel();
		}

		const int limit = SettingsManager::getValue(QLatin1String("History/BrowsingLimitAmountGlobal")).toInt();

		m_browsingHistoryModel->clearExcessEntries(limit);
		m_typedHistoryModel->clearExcessEntries(limit);

		scheduleSave();
	}
	else if (option == QLatin1String("History/BrowsingLimitPeriod"))
	{
		if (!m_browsingHistoryModel)
		{
			getBrowsingHistoryModel();
		}

		if (!m_typedHistoryModel)
		{
			getTypedHistoryModel();
		}

		const int period = SettingsManager::getValue(QLatin1String("History/BrowsingLimitPeriod")).toInt();

		m_browsingHistoryModel->clearOldestEntries(period);
		m_typedHistoryModel->clearOldestEntries(period);

		scheduleSave();
	}
}

void HistoryManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
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

void HistoryManager::removeEntries(const QList<quint64> &identifiers)
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

	if (!SettingsManager::getValue(QLatin1String("History/RememberBrowsing"), url).toBool())
	{
		removeEntry(identifier);

		return;
	}

	if (!m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	HistoryEntryItem *item = m_browsingHistoryModel->getEntry(identifier);

	if (item)
	{
		item->setData(url, HistoryModel::UrlRole);
		item->setData(title, HistoryModel::TitleRole);
		item->setIcon(icon);
	}

	m_instance->scheduleSave();
}

HistoryManager* HistoryManager::getInstance()
{
	return m_instance;
}

HistoryModel* HistoryManager::getBrowsingHistoryModel()
{
	if (!m_browsingHistoryModel)
	{
		m_browsingHistoryModel = new HistoryModel(SessionsManager::getWritableDataPath(QLatin1String("browsingHistory.json")), m_instance);
	}

	return m_browsingHistoryModel;
}

HistoryModel* HistoryManager::getTypedHistoryModel()
{
	if (!m_typedHistoryModel && m_instance)
	{
		m_typedHistoryModel = new HistoryModel(SessionsManager::getWritableDataPath(QLatin1String("typedHistory.json")), m_instance);
	}

	return m_typedHistoryModel;
}

QIcon HistoryManager::getIcon(const QUrl &url)
{
	if (url.scheme() == QLatin1String("about"))
	{
		if (url.path() == QLatin1String("bookmarks"))
		{
			return Utils::getIcon(QLatin1String("bookmarks"));
		}

		if (url.path() == QLatin1String("cache"))
		{
			return Utils::getIcon(QLatin1String("cache"));
		}

		if (url.path() == QLatin1String("config"))
		{
			return Utils::getIcon(QLatin1String("configuration"));
		}

		if (url.path() == QLatin1String("cookies"))
		{
			return Utils::getIcon(QLatin1String("cookies"));
		}

		if (url.path() == QLatin1String("history"))
		{
			return Utils::getIcon(QLatin1String("view-history"));
		}

		if (url.path() == QLatin1String("notes"))
		{
			return Utils::getIcon(QLatin1String("notes"));
		}

		if (url.path() == QLatin1String("transfers"))
		{
			return Utils::getIcon(QLatin1String("transfers"));
		}

		return Utils::getIcon(QLatin1String("text-html"));
	}

///TODO

	return Utils::getIcon(QLatin1String("text-html"));
}

HistoryEntryItem* HistoryManager::getEntry(quint64 identifier)
{
	if (!m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	return m_browsingHistoryModel->getEntry(identifier);
}

QList<HistoryModel::HistoryEntryMatch> HistoryManager::findEntries(const QString &prefix)
{
	if (!m_typedHistoryModel)
	{
		getTypedHistoryModel();
	}

	if (!m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	QList<HistoryModel::HistoryEntryMatch> entries;
	entries.append(m_typedHistoryModel->findEntries(prefix, true));
	entries.append(m_browsingHistoryModel->findEntries(prefix));

	return entries;
}

quint64 HistoryManager::addEntry(const QUrl &url, const QString &title, const QIcon &icon, bool isTypedIn)
{
	if (!m_isEnabled || !url.isValid() || !SettingsManager::getValue(QLatin1String("History/RememberBrowsing"), url).toBool())
	{
		return 0;
	}

	if (!m_browsingHistoryModel)
	{
		getBrowsingHistoryModel();
	}

	const quint64 identifier = m_browsingHistoryModel->addEntry(url, title, icon, QDateTime::currentDateTime())->data(HistoryModel::IdentifierRole).toULongLong();

	if (isTypedIn)
	{
		if (!m_typedHistoryModel)
		{
			getTypedHistoryModel();
		}

		m_typedHistoryModel->addEntry(url, title, icon, QDateTime::currentDateTime());
	}

	const int limit = SettingsManager::getValue(QLatin1String("History/BrowsingLimitAmountGlobal")).toInt();

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
