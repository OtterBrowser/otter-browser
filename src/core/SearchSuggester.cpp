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

#include "SearchSuggester.h"
#include "NetworkManager.h"
#include "NetworkManagerFactory.h"
#include "SearchEnginesManager.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

namespace Otter
{

SearchSuggester::SearchSuggester(const QString &searchEngine, QObject *parent) : QObject(parent),
	m_networkReply(nullptr),
	m_model(nullptr),
	m_searchEngine(searchEngine)
{
}

void SearchSuggester::handleReplyFinished()
{
	if (!m_networkReply)
	{
		return;
	}

	if (m_model)
	{
		m_model->clear();
	}
	else
	{
		m_model = new QStandardItemModel(this);
	}

	m_networkReply->deleteLater();

	if (m_networkReply->size() <= 0)
	{
		m_networkReply = nullptr;

		return;
	}

	const QJsonDocument document(QJsonDocument::fromJson(m_networkReply->readAll()));

	if (!document.isEmpty() && document.isArray() && document.array().count() > 1 && document.array().at(0).toString() == m_query)
	{
		const QJsonArray completionsArray(document.array().at(1).toArray());
		const QJsonArray descriptionsArray(document.array().at(2).toArray());
		const QJsonArray urlsArray(document.array().at(3).toArray());
		QVector<SearchSuggestion> suggestions;
		suggestions.reserve(completionsArray.count());

		for (int i = 0; i < completionsArray.count(); ++i)
		{
			SearchSuggestion suggestion;
			suggestion.completion = completionsArray.at(i).toString();
			suggestion.description = descriptionsArray.at(i).toString();
			suggestion.url = urlsArray.at(i).toString();

			if (m_model)
			{
				m_model->appendRow(new QStandardItem(suggestion.completion));
			}

			suggestions.append(suggestion);
		}

		emit suggestionsChanged(suggestions);
	}

	m_networkReply = nullptr;
}

void SearchSuggester::setSearchEngine(const QString &searchEngine)
{
	const QString query(m_query);

	m_searchEngine = searchEngine;
	m_query.clear();

	setQuery(query);
}

void SearchSuggester::setQuery(const QString &query)
{
	if (query != m_query)
	{
		m_query = query;

		if (m_networkReply)
		{
			m_networkReply->abort();
			m_networkReply->deleteLater();
			m_networkReply = nullptr;
		}

		const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(m_searchEngine));

		if (!searchEngine.isValid() || searchEngine.suggestionsUrl.url.isEmpty())
		{
			return;
		}

		QNetworkRequest request;
		request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());

		QNetworkAccessManager::Operation method;
		QByteArray body;

		SearchEnginesManager::setupQuery(query, searchEngine.suggestionsUrl, &request, &method, &body);

		if (method == QNetworkAccessManager::PostOperation)
		{
			m_networkReply = NetworkManagerFactory::getNetworkManager()->post(request, body);
		}
		else
		{
			m_networkReply = NetworkManagerFactory::getNetworkManager()->get(request);
		}

		connect(m_networkReply, &QNetworkReply::finished, this, &SearchSuggester::handleReplyFinished);
	}
}

QStandardItemModel* SearchSuggester::getModel()
{
	if (!m_model)
	{
		m_model = new QStandardItemModel(this);
	}

	return m_model;
}

}
