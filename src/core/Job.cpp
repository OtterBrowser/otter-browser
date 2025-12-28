/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

Job::Job(QObject *parent) : QObject(parent),
	m_progress(-1)
{
}

void Job::setProgress(int progress)
{
	if (progress != m_progress)
	{
		m_progress = progress;

		emit progressChanged(progress);
	}
}

int Job::getProgress() const
{
	return m_progress;
}

FetchJob::FetchJob(const QUrl &url, QObject *parent) : Job(parent),
	m_reply(nullptr),
	m_url(url),
	m_sizeLimit(-1),
	m_timeoutTimer(0),
	m_isFinished(false),
	m_isPrivate(false),
	m_isSuccess(true)
{
}

FetchJob::~FetchJob()
{
	m_reply->deleteLater();
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

void FetchJob::start()
{
	if (m_reply)
	{
		return;
	}

	m_reply = NetworkManagerFactory::createRequest(m_url, QNetworkAccessManager::GetOperation, m_isPrivate);

	connect(m_reply, &QNetworkReply::downloadProgress, this, [&](qint64 bytesReceived, qint64 bytesTotal)
	{
		if (m_sizeLimit >= 0 && ((bytesReceived > m_sizeLimit) || (bytesTotal > m_sizeLimit)))
		{
			cancel();
		}

		if (bytesTotal > 0)
		{
			setProgress(qRound(Utils::calculatePercent(bytesReceived, bytesTotal)));
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

void FetchJob::cancel()
{
	m_reply->blockSignals(true);
	m_reply->abort();

	deleteLater();

	emit jobFinished(false);
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

void FetchJob::setPrivate(bool isPrivate)
{
	m_isPrivate = isPrivate;
}

QUrl FetchJob::getUrl() const
{
	return (m_reply ? m_reply->request().url() : m_url);
}

bool FetchJob::isRunning() const
{
	return (m_reply != nullptr);
}

DataFetchJob::DataFetchJob(const QUrl &url, QObject *parent) : FetchJob(url, parent),
	m_reply(nullptr)
{
}

void DataFetchJob::handleSuccessfulReply(QNetworkReply *reply)
{
	m_reply = reply;

	markAsFinished();
}

QIODevice* DataFetchJob::getData() const
{
	return m_reply;
}

QMap<QByteArray, QByteArray> DataFetchJob::getHeaders() const
{
	QMap<QByteArray, QByteArray> headers;
	const QList<QNetworkReply::RawHeaderPair> rawHeaders(m_reply->rawHeaderPairs());

	for (const QNetworkReply::RawHeaderPair &rawHeader: rawHeaders)
	{
		headers[rawHeader.first] = rawHeader.second;
	}

	return headers;
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

}
