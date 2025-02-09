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

#ifndef OTTER_ADDRESSCOMPLETIONMODEL_H
#define OTTER_ADDRESSCOMPLETIONMODEL_H

#include "../../../core/SearchEnginesManager.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QUrl>

namespace Otter
{

class AddressCompletionModel final : public QAbstractListModel
{
	Q_OBJECT

public:
	enum CompletionType
	{
		NoCompletionType = 0,
		BookmarksCompletionType = 1,
		HistoryCompletionType = 2,
		TypedHistoryCompletionType = 4,
		SearchSuggestionsCompletionType = 8,
		SpecialPagesCompletionType = 16,
		LocalPathSuggestionsCompletionType = 32
	};

	Q_DECLARE_FLAGS(CompletionTypes, CompletionType)

	enum EntryRole
	{
		TextRole = Qt::DisplayRole,
		UrlRole = Qt::StatusTipRole,
		TitleRole = Qt::UserRole,
		HistoryIdentifierRole,
		IsRemovableRole,
		MatchRole,
		KeywordRole,
		TypeRole,
		TimeVisitedRole
	};

	struct CompletionEntry final
	{
		enum EntryType
		{
			UnknownType = 0,
			HeaderType,
			BookmarkType,
			HistoryType,
			TypedHistoryType,
			SearchSuggestionType,
			SpecialPageType,
			LocalPathType
		};

		QString text;
		QString title;
		QString match;
		QString keyword;
		QUrl url;
		QIcon icon;
		QDateTime timeVisited;
		quint64 historyIdentifier = 0;
		EntryType type = UnknownType;

		explicit CompletionEntry(const QUrl &urlValue, const QString &titleValue, const QString &matchValue, const QIcon &iconValue, const QDateTime &timeVisitedValue, EntryType typeValue, quint64 historyIdentifierValue = 0) : title(titleValue), match(matchValue), url(urlValue), icon(iconValue), timeVisited(timeVisitedValue), historyIdentifier(historyIdentifierValue), type(typeValue)
		{
		}

		CompletionEntry() = default;
	};

	explicit AddressCompletionModel(QObject *parent = nullptr);

	void setTypes(CompletionTypes types, bool force = false);
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &index = {}) const override;
	bool event(QEvent *event) override;

public slots:
	void setFilter(const QString &filter = {});

protected:
	void timerEvent(QTimerEvent *event) override;
	void updateModel();

private:
	QVector<CompletionEntry> m_completions;
	QString m_filter;
	SearchEnginesManager::SearchEngineDefinition m_defaultSearchEngine;
	AddressCompletionModel::CompletionTypes m_types;
	int m_updateTimer;
	bool m_showCompletionCategories;

signals:
	void completionReady(const QString &filter);
};

}

#endif
