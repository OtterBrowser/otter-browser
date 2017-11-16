/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_SEARCHSUGGESTER_H
#define OTTER_SEARCHSUGGESTER_H

#include <QtCore/QObject>
#include <QtGui/QStandardItemModel>
#include <QtNetwork/QNetworkReply>

namespace Otter
{

class SearchSuggester final : public QObject
{
	Q_OBJECT

public:
	struct SearchSuggestion final
	{
		QString completion;
		QString description;
		QString url;
	};

	explicit SearchSuggester(const QString &searchEngine, QObject *parent = nullptr);

	QStandardItemModel* getModel();

public slots:
	void setSearchEngine(const QString &searchEngine);
	void setQuery(const QString &query);

protected slots:
	void handleReplyFinished();

private:
	QNetworkReply *m_networkReply;
	QStandardItemModel *m_model;
	QString m_searchEngine;
	QString m_query;

signals:
	void suggestionsChanged(const QVector<SearchSuggester::SearchSuggestion> &suggestions);
};

}

#endif
