#include "SearchSuggester.h"
#include "NetworkAccessManager.h"
#include "SearchesManager.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkReply>

namespace Otter
{

SearchSuggester::SearchSuggester(const QString &engine, QObject *parent) : QObject(parent),
	m_networkAccessManager(new NetworkAccessManager(true, true, NULL)),
	m_currentReply(NULL),
	m_engine(engine)
{
	connect(m_networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
}

void SearchSuggester::setEngine(const QString &engine)
{
	const QString query = m_query;

	m_engine = engine;
	m_query = QString();

	setQuery(query);
}

void SearchSuggester::setQuery(const QString &query)
{
	if (query != m_query)
	{
		m_query = query;

		if (m_currentReply)
		{
			m_currentReply->abort();

			QTimer::singleShot(250, m_currentReply, SLOT(deleteLater()));

			m_currentReply = NULL;
		}

		SearchInformation *engine = SearchesManager::getSearchEngine(m_engine);

		if (!engine || engine->suggestionsUrl.url.isEmpty())
		{
			return;
		}

		QNetworkRequest request;
		QNetworkAccessManager::Operation method;
		QByteArray body;

		SearchesManager::setupQuery(query, engine->suggestionsUrl, &request, &method, &body);

		if (method == QNetworkAccessManager::PostOperation)
		{
			m_currentReply = m_networkAccessManager->post(request, body);
		}
		else
		{
			m_currentReply = m_networkAccessManager->get(request);
		}
	}
}

void SearchSuggester::replyFinished(QNetworkReply *reply)
{
	if (reply == m_currentReply)
	{
		m_currentReply = NULL;
	}

	const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());

	if (!document.isEmpty() && document.isArray() && document.array().count() > 1 && document.array().at(0).toString() == m_query)
	{
		const QJsonArray completionsArray = document.array().at(1).toArray();
		const QJsonArray descriptionsArray = document.array().at(2).toArray();
		const QJsonArray urlsArray = document.array().at(3).toArray();
		QList<SearchSuggestion> suggestions;

		for (int i = 0; i < completionsArray.count(); ++i)
		{
			SearchSuggestion suggestion;
			suggestion.completion = completionsArray.at(i).toString();
			suggestion.description = descriptionsArray.at(i).toString();
			suggestion.url = urlsArray.at(i).toString();

			suggestions.append(suggestion);
		}

		emit suggestionsChanged(suggestions);
	}

	QTimer::singleShot(250, reply, SLOT(deleteLater()));
}

}
