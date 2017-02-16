/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "AddonsManager.h"
#include "BookmarksManager.h"
#include "HistoryManager.h"
#include "SettingsManager.h"
#include "ThemesManager.h"
#include "Utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtWidgets/QFileIconProvider>

namespace Otter
{

AddressCompletionModel::AddressCompletionModel(QObject *parent) : QAbstractListModel(parent),
	m_types(UnknownCompletionType),
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
	QList<CompletionEntry> completions;

	if (m_types.testFlag(SearchSuggestionsCompletionType))
	{
		const QString keyword(m_filter.section(QLatin1Char(' '), 0, 0));
		const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(keyword, true));
		QString title(m_defaultSearchEngine.title);
		QString text(m_filter);
		QIcon icon(m_defaultSearchEngine.icon);

		if (!searchEngine.identifier.isEmpty())
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
			icon = ThemesManager::getIcon(QLatin1String("edit-find"));
		}

		if (m_showCompletionCategories)
		{
			completions.append(CompletionEntry(QUrl(), tr("Search with %1").arg(title), QString(), QIcon(), QDateTime(), HeaderType));

			title = QString();
		}

		CompletionEntry completionEntry(QUrl(), title, QString(), icon, QDateTime(), SearchSuggestionType);
		completionEntry.text = text;

		completions.append(completionEntry);
	}

	if (m_types.testFlag(BookmarksCompletionType))
	{
		const QList<BookmarksModel::BookmarkMatch> bookmarks(BookmarksManager::findBookmarks(m_filter));

		if (m_showCompletionCategories && !bookmarks.isEmpty())
		{
			completions.append(CompletionEntry(QUrl(), tr("Bookmarks"), QString(), QIcon(), QDateTime(), HeaderType));
		}

		for (int i = 0; i < bookmarks.count(); ++i)
		{
			CompletionEntry completionEntry(bookmarks.at(i).bookmark->data(BookmarksModel::UrlRole).toUrl(), bookmarks.at(i).bookmark->data(BookmarksModel::TitleRole).toString(), bookmarks.at(i).match, bookmarks.at(i).bookmark->data(Qt::DecorationRole).value<QIcon>(), QDateTime(), BookmarkType);
			completionEntry.keyword = bookmarks.at(i).bookmark->data(BookmarksModel::KeywordRole).toString();

			if (completionEntry.keyword.startsWith(m_filter))
			{
				completionEntry.match = completionEntry.keyword;
			}

			completions.append(completionEntry);
		}
	}

	if (m_types.testFlag(LocalPathSuggestionsCompletionType) && m_filter.contains(QDir::separator()))
	{
		const QString directory(m_filter.section(QDir::separator(), 0, -2) + QDir::separator());
		const QString prefix(m_filter.section(QDir::separator(), -1, -1));
		const QList<QFileInfo> entries(QDir(Utils::normalizePath(directory)).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot));
		const QFileIconProvider iconProvider;
		bool wasAdded(!m_showCompletionCategories);

		for (int i = 0; i < entries.count(); ++i)
		{
			if (entries.at(i).fileName().startsWith(prefix, Qt::CaseInsensitive))
			{
				const QString path(directory + entries.at(i).fileName());
				const QMimeType type(QMimeDatabase().mimeTypeForFile(entries.at(i), QMimeDatabase::MatchExtension));

				if (!wasAdded)
				{
					completions.append(CompletionEntry(QUrl(), tr("Local files"), QString(), QIcon(), QDateTime(), HeaderType));

					wasAdded = true;
				}

				completions.append(CompletionEntry(QUrl::fromLocalFile(QDir::toNativeSeparators(path)), path, path, QIcon::fromTheme(type.iconName(), iconProvider.icon(entries.at(i))), QDateTime(), LocalPathType));
			}
		}
	}

	if (m_types.testFlag(HistoryCompletionType))
	{
		const QList<HistoryModel::HistoryEntryMatch> entries(HistoryManager::findEntries(m_filter));

		if (m_showCompletionCategories && !entries.isEmpty())
		{
			completions.append(CompletionEntry(QUrl(), tr("History"), QString(), QIcon(), QDateTime(), HeaderType));
		}

		for (int i = 0; i < entries.count(); ++i)
		{
			completions.append(CompletionEntry(entries.at(i).entry->data(HistoryModel::UrlRole).toUrl(), entries.at(i).entry->data(HistoryModel::TitleRole).toString(), entries.at(i).match, entries.at(i).entry->data(Qt::DecorationRole).value<QIcon>(), entries.at(i).entry->data(HistoryModel::TimeVisitedRole).toDateTime(), (entries.at(i).isTypedIn ? TypedInHistoryType : HistoryType)));
		}
	}

	if (m_types.testFlag(TypedHistoryCompletionType))
	{
		const QList<HistoryModel::HistoryEntryMatch> entries(HistoryManager::findEntries(QString(), true));

		if (m_showCompletionCategories && !entries.isEmpty())
		{
			completions.append(CompletionEntry(QUrl(), tr("Typed history"), QString(), QIcon(), QDateTime(), HeaderType));
		}

		for (int i = 0; i < entries.count(); ++i)
		{
			completions.append(CompletionEntry(entries.at(i).entry->data(HistoryModel::UrlRole).toUrl(), entries.at(i).entry->data(HistoryModel::TitleRole).toString(), entries.at(i).match, entries.at(i).entry->data(Qt::DecorationRole).value<QIcon>(), entries.at(i).entry->data(HistoryModel::TimeVisitedRole).toDateTime(), TypedInHistoryType));
		}
	}

	if (m_types.testFlag(SpecialPagesCompletionType))
	{
		const QStringList specialPages(AddonsManager::getSpecialPages());
		bool wasAdded(!m_showCompletionCategories);

		for (int i = 0; i < specialPages.count(); ++i)
		{
			const AddonsManager::SpecialPageInformation information(AddonsManager::getSpecialPage(specialPages.at(i)));

			if (information.url.toString().startsWith(m_filter))
			{
				if (!wasAdded)
				{
					completions.append(CompletionEntry(QUrl(), tr("Special pages"), QString(), QIcon(), QDateTime(), HeaderType));

					wasAdded = true;
				}

				completions.append(CompletionEntry(information.url, information.getTitle(), QString(), information.icon, QDateTime(), SpecialPageType));
			}
		}
	}

	beginResetModel();

	m_completions = completions;

	endResetModel();
}

void AddressCompletionModel::setFilter(const QString &filter, CompletionTypes types)
{
	if (types.testFlag(TypedHistoryCompletionType))
	{
		m_types = types;
		m_filter.clear();

		updateModel();

		return;
	}
 
	if (m_filter.isEmpty() && !filter.isEmpty())
	{
		m_types = types;

		if (SettingsManager::getValue(SettingsManager::AddressField_SuggestBookmarksOption).toBool())
		{
			m_types |= BookmarksCompletionType;
		}

		if (SettingsManager::getValue(SettingsManager::AddressField_SuggestHistoryOption).toBool())
		{
			m_types |= HistoryCompletionType;
		}

		if (SettingsManager::getValue(SettingsManager::AddressField_SuggestSearchOption).toBool())
		{
			m_types |= SearchSuggestionsCompletionType;

			m_defaultSearchEngine = SearchEnginesManager::getSearchEngine();
		}

		if (SettingsManager::getValue(SettingsManager::AddressField_SuggestSpecialPagesOption).toBool())
		{
			m_types |= SpecialPagesCompletionType;
		}

		if (SettingsManager::getValue(SettingsManager::AddressField_SuggestLocalPathsOption).toBool())
		{
			m_types |= LocalPathSuggestionsCompletionType;
		}
	}

	m_filter = filter;
	m_showCompletionCategories = SettingsManager::getValue(SettingsManager::AddressField_ShowCompletionCategoriesOption).toBool();

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

		emit completionReady(QString());
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
			case Qt::DecorationRole:
				return m_completions.at(index.row()).icon;
			case TextRole:
				return (m_completions.at(index.row()).text.isEmpty() ? m_completions.at(index.row()).url.toString() : m_completions.at(index.row()).text);
			case UrlRole:
				return m_completions.at(index.row()).url;
			case TitleRole:
				return m_completions.at(index.row()).title;
			case KeywordRole:
				return m_completions.at(index.row()).keyword;
			case MatchRole:
				return (m_completions.at(index.row()).match.isEmpty() ? m_completions.at(index.row()).url.toString() : m_completions.at(index.row()).match);
			case TimeVisitedRole:
				return m_completions.at(index.row()).timeVisited;
			case TypeRole:
				return static_cast<int>(m_completions.at(index.row()).type);
		}
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

Qt::ItemFlags AddressCompletionModel::flags(const QModelIndex &index) const
{
	if (!index.isValid() || m_completions.at(index.row()).type == AddressCompletionModel::HeaderType)
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
