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

FetchJob::FetchJob(const QUrl &url, QObject *parent) : Job(parent),
	m_sizeLimit(-1),
	m_timeoutTimer(0),
	m_isFinished(false),
	m_isSuccess(true)
{
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());

	m_reply = NetworkManagerFactory::getNetworkManager()->get(request);

	connect(m_reply, &QNetworkReply::downloadProgress, this, [&](qint64 bytesReceived, qint64 bytesTotal)
	{
		Q_UNUSED(bytesReceived)

		if (bytesTotal > 0 && m_sizeLimit >= 0 && bytesTotal > m_sizeLimit)
		{
			cancel();
		}
	});
	connect(m_reply, &QNetworkReply::finished, this, [&]()
	{
		const bool isSuccess(m_reply->error() == QNetworkReply::NoError);

		if (isSuccess && (m_sizeLimit < 0 || m_reply->size() <= m_sizeLimit))
		{
			handleSuccessfulReply(m_reply);
		}

		if (!isSuccess || m_isFinished)
		{
			deleteLater();

			emit jobFinished(isSuccess && m_isSuccess);
		}
	});
}

FetchJob::~FetchJob()
{
	m_reply->deleteLater();
}

void FetchJob::cancel()
{
	m_reply->blockSignals(true);
	m_reply->abort();

	deleteLater();

	emit jobFinished(false);
}

void FetchJob::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_timeoutTimer)
	{
		cancel();
		killTimer(m_timeoutTimer);

		m_timeoutTimer = 0;
	}
}

void FetchJob::markAsFailure()
{
	m_isSuccess = false;
}

void FetchJob::markAsFinished()
{
	m_isFinished = true;
}

void FetchJob::setTimeout(int seconds)
{
	if (m_timeoutTimer != 0)
	{
		killTimer(m_timeoutTimer);
	}

	m_timeoutTimer = startTimer(seconds * 1000);
}

void FetchJob::setSizeLimit(qint64 limit)
{
	m_sizeLimit = limit;
}

QUrl FetchJob::getUrl() const
{
	return m_reply->request().url();
}

IconFetchJob::IconFetchJob(const QUrl &url, QObject *parent) : FetchJob(url, parent)
{
	setSizeLimit(20480);
	setTimeout(5);
}

void IconFetchJob::handleSuccessfulReply(QNetworkReply *reply)
{
	QPixmap pixmap;

	if (pixmap.loadFromData(reply->readAll()))
	{
		m_icon = QIcon(pixmap);
	}

	if (m_icon.isNull())
	{
		markAsFailure();
	}

	markAsFinished();
}

QIcon IconFetchJob::getIcon() const
{
	return m_icon;
}

SearchEngineFetchJob::SearchEngineFetchJob(const QUrl &url, const QString &identifier, bool saveSearchEngine, QObject *parent) : FetchJob(url, parent),
	m_needsToSaveSearchEngine(saveSearchEngine)
{
	m_searchEngine.identifier = (identifier.isEmpty() ? Utils::createIdentifier({}, SearchEnginesManager::getSearchEngines()) : identifier);
}

SearchEnginesManager::SearchEngineDefinition SearchEngineFetchJob::getSearchEngine() const
{
	return m_searchEngine;
}

void SearchEngineFetchJob::handleSuccessfulReply(QNetworkReply *reply)
{
	m_searchEngine = SearchEnginesManager::loadSearchEngine(reply, m_searchEngine.identifier);

	if (!m_searchEngine.isValid())
	{
		markAsFailure();

		return;
	}

	if (m_searchEngine.selfUrl.isEmpty())
	{
		m_searchEngine.selfUrl = reply->request().url();
	}

	if (m_needsToSaveSearchEngine)
	{
		SearchEnginesManager::addSearchEngine(m_searchEngine);
	}

	if (m_searchEngine.iconUrl.isValid())
	{
		const IconFetchJob *job(new IconFetchJob(m_searchEngine.iconUrl, this));

		connect(job, &IconFetchJob::jobFinished, this, [=]()
		{
			m_searchEngine.icon = job->getIcon();

			if (m_needsToSaveSearchEngine)
			{
				SearchEnginesManager::addSearchEngine(m_searchEngine);
			}

			deleteLater();

			emit jobFinished(true);
		});
	}
	else
	{
		markAsFinished();
	}
}

}
