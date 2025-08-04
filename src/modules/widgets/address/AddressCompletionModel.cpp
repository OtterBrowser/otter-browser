/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "AddressCompletionModel.h"
#include "../../../core/AddonsManager.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"

#include <QtCore/QDir>

namespace Otter
{

AddressCompletionModel::AddressCompletionModel(QObject *parent) : QAbstractListModel(parent),
	m_types(NoCompletionType),
	m_updateTimer(0),
	m_showCompletionCategories(true)
{
}

void AddressCompletionModel::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_updateTimer)
	{
		killTimer(m_updateTimer);

		m_updateTimer = 0;

		if (!m_filter.isEmpty())
		{
			updateModel();

			emit completionReady(m_filter);
		}
	}
}

void AddressCompletionModel::updateModel()
{
	QVector<CompletionEntry> completions;
	completions.reserve(10);

	if (m_types.testFlag(SearchSuggestionsCompletionType))
	{
		const QString keyword(m_filter.section(QLatin1Char(' '), 0, 0));
		const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(keyword, true));
		QString title(m_defaultSearchEngine.title);
		QString text(m_filter);
		QIcon icon(m_defaultSearchEngine.icon);

		if (searchEngine.isValid())
		{
			title = searchEngine.title;
			text = m_filter.section(QLatin1Char(' '), 1, -1);
			icon = searchEngine.icon;
		}
		else if (keyword == QLatin1String("?"))
		{
			text = m_filter.section(QLatin1Char(' '), 1, -1);
		}

		if (icon.isNull())
		{
			icon = ThemesManager::createIcon(QLatin1String("edit-find"));
		}

		if (m_showCompletionCategories)
		{
			completions.append(CompletionEntry({}, tr("Search with %1").arg(title), {}, {}, {}, CompletionEntry::HeaderType));

			title.clear();
		}

		CompletionEntry completionEntry({}, title, {}, icon, {}, CompletionEntry::SearchSuggestionType);
		completionEntry.text = text;
		completionEntry.keyword = keyword;

		completions.append(completionEntry);
	}

	if (m_types.testFlag(BookmarksCompletionType))
	{
		const QVector<BookmarksModel::BookmarkMatch> bookmarks(BookmarksManager::findBookmarks(m_filter));

		if (m_showCompletionCategories && !bookmarks.isEmpty())
		{
			completions.append(CompletionEntry({}, tr("Bookmarks"), {}, {}, {}, CompletionEntry::HeaderType));
		}

		for (int i = 0; i < bookmarks.count(); ++i)
		{
			const BookmarksModel::BookmarkMatch bookmark(bookmarks.at(i));
			CompletionEntry completionEntry(bookmark.bookmark->getUrl(), bookmark.bookmark->getTitle(), bookmark.match, bookmark.bookmark->getIcon(), {}, CompletionEntry::BookmarkType);
			completionEntry.keyword = bookmark.bookmark->getKeyword();

			if (completionEntry.keyword.startsWith(m_filter))
			{
				completionEntry.match = completionEntry.keyword;
			}

			completions.append(completionEntry);
		}
	}

	if (m_types.testFlag(LocalPathSuggestionsCompletionType) && (m_filter == QString(QLatin1Char('~')) || m_filter.contains(QDir::separator())))
	{
		const QString directory((m_filter == QString(QLatin1Char('~'))) ? QDir::homePath() : m_filter.section(QDir::separator(), 0, -2) + QDir::separator());
		const QString prefix(m_filter.contains(QDir::separator()) ? m_filter.section(QDir::separator(), -1, -1) : QString());
		const QStringList entries(QDir(Utils::normalizePath(directory)).entryList(QDir::AllEntries | QDir::NoDotAndDotDot));
		bool headerWasAdded(!m_showCompletionCategories);

		for (int i = 0; i < entries.count(); ++i)
		{
			const QString entry(entries.at(i));

			if (entry.startsWith(prefix, Qt::CaseInsensitive))
			{
				const QString path(directory + entry);

				if (!headerWasAdded)
				{
					completions.append(CompletionEntry({}, tr("Local files"), {}, {}, {}, CompletionEntry::HeaderType));

					headerWasAdded = true;
				}

				completions.append(CompletionEntry(QUrl::fromLocalFile(QDir::toNativeSeparators(path)), path, path, ThemesManager::getFileTypeIcon(path), {}, CompletionEntry::LocalPathType));
			}
		}
	}

	if (m_types.testFlag(HistoryCompletionType))
	{
		const QVector<HistoryModel::HistoryEntryMatch> entries(HistoryManager::findEntries(m_filter));

		if (m_showCompletionCategories && !entries.isEmpty())
		{
			completions.append(CompletionEntry({}, tr("History"), {}, {}, {}, CompletionEntry::HeaderType));
		}

		for (int i = 0; i < entries.count(); ++i)
		{
			const HistoryModel::HistoryEntryMatch match(entries.at(i));
			HistoryModel::Entry *entry(match.entry);

			completions.append(CompletionEntry(entry->getUrl(), entry->getTitle(), match.match, entry->getIcon(), entry->getTimeVisited(), (match.isTypedIn ? CompletionEntry::TypedHistoryType : CompletionEntry::HistoryType)));
		}
	}

	if (m_types.testFlag(TypedHistoryCompletionType))
	{
		const QVector<HistoryModel::HistoryEntryMatch> entries(HistoryManager::findEntries({}, true));

		if (m_showCompletionCategories && !entries.isEmpty())
		{
			completions.append(CompletionEntry({}, tr("Typed history"), {}, {}, {}, CompletionEntry::HeaderType));
		}

		for (int i = 0; i < entries.count(); ++i)
		{
			const HistoryModel::HistoryEntryMatch match(entries.at(i));
			HistoryModel::Entry *entry(match.entry);

			completions.append(CompletionEntry(entry->getUrl(), entry->getTitle(), match.match, entry->getIcon(), entry->getTimeVisited(), CompletionEntry::TypedHistoryType, entry->getIdentifier()));
		}
	}

	if (m_types.testFlag(SpecialPagesCompletionType))
	{
		const QStringList specialPages(AddonsManager::getSpecialPages());
		bool headerWasAdded(!m_showCompletionCategories);

		for (int i = 0; i < specialPages.count(); ++i)
		{
			const AddonsManager::SpecialPageInformation information(AddonsManager::getSpecialPage(specialPages.at(i)));

			if (information.url.toString().startsWith(m_filter))
			{
				if (!headerWasAdded)
				{
					completions.append(CompletionEntry({}, tr("Special pages"), {}, {}, {}, CompletionEntry::HeaderType));

					headerWasAdded = true;
				}

				completions.append(CompletionEntry(information.url, information.getTitle(), {}, information.icon, {}, CompletionEntry::SpecialPageType));
			}
		}
	}

	completions.squeeze();

	beginResetModel();

	m_completions = completions;

	endResetModel();
}

void AddressCompletionModel::setFilter(const QString &filter)
{
	m_filter = filter;
	m_showCompletionCategories = SettingsManager::getOption(SettingsManager::AddressField_ShowCompletionCategoriesOption).toBool();

	if (m_filter.isEmpty())
	{
		if (m_updateTimer != 0)
		{
			killTimer(m_updateTimer);

			m_updateTimer = 0;
		}

		beginResetModel();

		m_completions.clear();

		endResetModel();

		emit completionReady({});
	}
	else if (m_updateTimer == 0)
	{
		m_updateTimer = startTimer(50);
	}
}

void AddressCompletionModel::setTypes(CompletionTypes types, bool force)
{
	if (!force && types == m_types)
	{
		return;
	}

	m_types = types;

	if (types.testFlag(TypedHistoryCompletionType))
	{
		m_filter.clear();
	}

	if (m_types.testFlag(SearchSuggestionsCompletionType))
	{
		m_defaultSearchEngine = SearchEnginesManager::getSearchEngine();
	}

	updateModel();
}

QVariant AddressCompletionModel::data(const QModelIndex &index, int role) const
{
	if (index.column() != 0 || !(index.row() >= 0 && index.row() < m_completions.count()))
	{
		return {};
	}

	const CompletionEntry entry(m_completions.at(index.row()));

	switch (role)
	{
		case Qt::DecorationRole:
			return entry.icon;
		case HistoryIdentifierRole:
			return entry.historyIdentifier;
		case IsRemovableRole:
			return (entry.type == CompletionEntry::TypedHistoryType);
		case TextRole:
			return (entry.text.isEmpty() ? entry.url.toString() : entry.text);
		case UrlRole:
			return entry.url;
		case TitleRole:
			return entry.title;
		case KeywordRole:
			return entry.keyword;
		case MatchRole:
			return (entry.match.isEmpty() ? entry.url.toString() : entry.match);
		case TimeVisitedRole:
			return entry.timeVisited;
		case TypeRole:
			return static_cast<int>(entry.type);
		default:
			return {};
	}

	return {};
}

QVariant AddressCompletionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(section)
	Q_UNUSED(orientation)
	Q_UNUSED(role)

	return {};
}

Qt::ItemFlags AddressCompletionModel::flags(const QModelIndex &index) const
{
	if (!index.isValid() || m_completions.at(index.row()).type == AddressCompletionModel::CompletionEntry::HeaderType)
	{
		return Qt::ItemNeverHasChildren;
	}

	return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren);
}

int AddressCompletionModel::rowCount(const QModelIndex &index) const
{
	return (index.isValid() ? 0 : m_completions.count());
}

bool AddressCompletionModel::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange && m_completions.count() > 0)
	{
		updateModel();
	}

	return QAbstractListModel::event(event);
}

}
