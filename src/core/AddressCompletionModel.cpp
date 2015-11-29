/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "AddressCompletionModel.h"
#include "AddonsManager.h"
#include "BookmarksManager.h"
#include "HistoryManager.h"
#include "SettingsManager.h"

#include <QtCore/QCoreApplication>

namespace Otter
{

AddressCompletionModel::AddressCompletionModel(QObject *parent) : QAbstractListModel(parent),
	m_types(UnknownCompletionType),
	m_updateTimer(0)
{
}

void AddressCompletionModel::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_updateTimer)
	{
		killTimer(m_updateTimer);

		m_updateTimer = 0;

		QList<CompletionEntry> completions;

		if (m_types.testFlag(SearchSuggestionsCompletionType))
		{
			CompletionEntry completionEntry(QUrl(), m_defaultSearchEngine.title, QString(), m_defaultSearchEngine.icon, SearchSuggestionType);
			completionEntry.text = m_filter;

			completions.append(completionEntry);
		}

		if (m_types.testFlag(BookmarksCompletionType))
		{
			const QList<BookmarksModel::BookmarkMatch> bookmarks = BookmarksManager::findBookmarks(m_filter);

			for (int i = 0; i < bookmarks.count(); ++i)
			{
				CompletionEntry completionEntry(bookmarks.at(i).bookmark->data(BookmarksModel::UrlRole).toUrl(), bookmarks.at(i).bookmark->data(BookmarksModel::TitleRole).toString(), bookmarks.at(i).match, bookmarks.at(i).bookmark->data(Qt::DecorationRole).value<QIcon>(), BookmarkType);
				completionEntry.text = bookmarks.at(i).bookmark->data(BookmarksModel::TitleRole).toString();
				completionEntry.keyword = bookmarks.at(i).bookmark->data(BookmarksModel::KeywordRole).toString();

				if (completionEntry.keyword.startsWith(m_filter))
				{
					completionEntry.match = completionEntry.keyword;
				}

				completions.append(completionEntry);
			}
		}

		if (m_types.testFlag(HistoryCompletionType))
		{
			const QList<HistoryModel::HistoryEntryMatch> entries = HistoryManager::findEntries(m_filter);

			for (int i = 0; i < entries.count(); ++i)
			{
				CompletionEntry completionEntry(entries.at(i).entry->data(HistoryModel::UrlRole).toUrl(), entries.at(i).entry->data(HistoryModel::TitleRole).toString(), entries.at(i).match, entries.at(i).entry->data(Qt::DecorationRole).value<QIcon>(), (entries.at(i).entry->data(HistoryModel::TypedInRole).toBool() ? TypedInHistoryType : HistoryType));
				completionEntry.text = entries.at(i).entry->data(HistoryModel::TitleRole).toString();

				completions.append(completionEntry);
			}
		}

		if (m_types.testFlag(SpecialPageCompletionType))
		{
			const QStringList specialPages = AddonsManager::getSpecialPages();

			for (int i = 0; i < specialPages.count(); ++i)
			{
				const AddonsManager::SpecialPageInformation information = AddonsManager::getSpecialPage(specialPages.at(i));

				if (information.url.toString().startsWith(m_filter))
				{
					completions.append(CompletionEntry(information.url, information.getTitle(), QString(), information.icon, SpecialPageType));
				}
			}
		}

		m_completions = completions;

		endResetModel();

		emit completionReady(m_filter);
	}
}

void AddressCompletionModel::setFilter(const QString &filter)
{
	if (m_filter.isEmpty() && !filter.isEmpty())
	{
		m_types = UnknownCompletionType;

		if (SettingsManager::getValue(QLatin1String("AddressField/SuggestBookmarks")).toBool())
		{
			m_types |= BookmarksCompletionType;
		}

		if (SettingsManager::getValue(QLatin1String("AddressField/SuggestHistory")).toBool())
		{
			m_types |= HistoryCompletionType;
		}

		if (SettingsManager::getValue(QLatin1String("AddressField/SuggestSearch")).toBool())
		{
			m_types |= SearchSuggestionsCompletionType;

			m_defaultSearchEngine = SearchesManager::getSearchEngine();
		}

		if (SettingsManager::getValue(QLatin1String("AddressField/SuggestSpecialPages")).toBool())
		{
			m_types |= SpecialPageCompletionType;
		}
	}

	m_filter = filter;

	if (m_filter.isEmpty())
	{
		beginResetModel();

		m_completions.clear();

		endResetModel();
	}
	else if (m_updateTimer == 0)
	{
		m_updateTimer = startTimer(50);
	}
}

QVariant AddressCompletionModel::data(const QModelIndex &index, int role) const
{
	if (index.column() == 0 && index.row() >= 0 && index.row() < m_completions.count())
	{
		switch (role)
		{
			case UrlRole:
				return m_completions.at(index.row()).url;
			case TitleRole:
				return m_completions.at(index.row()).title;
			case KeywordRole:
				return m_completions.at(index.row()).keyword;
			case MatchRole:
				return (m_completions.at(index.row()).match.isEmpty() ? m_completions.at(index.row()).url.toString() : m_completions.at(index.row()).match);
		}

		return (m_completions.at(index.row()).text.isEmpty() ? m_completions.at(index.row()).url.toDisplayString(QUrl::RemoveScheme) : m_completions.at(index.row()).text);
	}

	return QVariant();
}

QVariant AddressCompletionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(section)
	Q_UNUSED(orientation)
	Q_UNUSED(role)

	return QVariant();
}

int AddressCompletionModel::rowCount(const QModelIndex &index) const
{
	return (index.isValid() ? 0 : m_completions.count());
}

}
