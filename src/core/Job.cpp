/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "Job.h"
#include "NetworkManager.h"
#include "NetworkManagerFactory.h"
#include "Utils.h"

namespace Otter
{

Job::Job(QObject *parent) : QObject(parent)
{
}

SearchEngineFetchJob::SearchEngineFetchJob(const QUrl &url, const QString &identifier, bool saveSearchEngine, QObject *parent) : Job(parent),
	m_reply(nullptr),
	m_needsToSaveSearchEngine(saveSearchEngine)
{
	m_searchEngine.identifier = (identifier.isEmpty() ? Utils::createIdentifier({}, SearchEnginesManager::getSearchEngines()) : identifier);

	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());

	m_reply = NetworkManagerFactory::getNetworkManager()->get(request);

	connect(m_reply, &QNetworkReply::finished, this, &SearchEngineFetchJob::handleDefinitionRequestFinished);
}

SearchEngineFetchJob::~SearchEngineFetchJob()
{
	m_reply->deleteLater();
}

void SearchEngineFetchJob::cancel()
{
	m_reply->abort();

	emit jobFinished(false);
}

void SearchEngineFetchJob::handleDefinitionRequestFinished()
{
	if (m_reply->error() != QNetworkReply::NoError)
	{
		deleteLater();

		emit jobFinished(false);

		return;
	}

	m_searchEngine = SearchEnginesManager::loadSearchEngine(m_reply, m_searchEngine.identifier);

	if (!m_searchEngine.isValid())
	{
		deleteLater();

		emit jobFinished(false);

		return;
	}

	if (m_searchEngine.selfUrl.isEmpty())
	{
		m_searchEngine.selfUrl = m_reply->request().url();
	}

	if (m_needsToSaveSearchEngine)
	{
		SearchEnginesManager::addSearchEngine(m_searchEngine);
	}

	if (m_searchEngine.iconUrl.isValid())
	{
		m_reply->deleteLater();

		QNetworkRequest request(m_searchEngine.iconUrl);
		request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());

		m_reply = NetworkManagerFactory::getNetworkManager()->get(request);

		connect(m_reply, &QNetworkReply::finished, this, &SearchEngineFetchJob::handleIconRequestFinished);
	}
	else
	{
		deleteLater();

		emit jobFinished(true);
	}
}

void SearchEngineFetchJob::handleIconRequestFinished()
{
	QPixmap pixmap;

	if (m_reply->error() == QNetworkReply::NoError && pixmap.loadFromData(m_reply->readAll()))
	{
		m_searchEngine.icon = QIcon(pixmap);

		if (m_needsToSaveSearchEngine)
		{
			SearchEnginesManager::addSearchEngine(m_searchEngine);
		}
	}

	deleteLater();

	emit jobFinished(true);
}

SearchEnginesManager::SearchEngineDefinition SearchEngineFetchJob::getSearchEngine() const
{
	return m_searchEngine;
}

}
