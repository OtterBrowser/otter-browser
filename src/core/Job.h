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

#ifndef OTTER_JOB_H
#define OTTER_JOB_H

#include <QtGui/QIcon>
#include <QtNetwork/QNetworkReply>

namespace Otter
{

class Job : public QObject
{
	Q_OBJECT

public:
	explicit Job(QObject *parent = nullptr);

	int getProgress() const;
	virtual bool isRunning() const = 0;

public slots:
	virtual void start() = 0;
	virtual void cancel() = 0;

protected:
	void setProgress(int progress);

private:
	int m_progress;

signals:
	void jobFinished(bool isSuccess);
	void progressChanged(int progress);
};

class FetchJob : public Job
{
	Q_OBJECT

public:
	explicit FetchJob(const QUrl &url, QObject *parent = nullptr);
	~FetchJob();

	void setTimeout(int seconds);
	void setSizeLimit(qint64 limit);
	void setPrivate(bool isPrivate);
	QUrl getUrl() const;
	bool isRunning() const override;

public slots:
	void start() override;
	void cancel() override;

protected:
	void timerEvent(QTimerEvent *event) override;
	void markAsFailure();
	void markAsFinished();
	virtual void handleSuccessfulReply(QNetworkReply *reply) = 0;

private:
	QNetworkReply *m_reply;
	QUrl m_url;
	qint64 m_sizeLimit;
	int m_timeoutTimer;
	bool m_isFinished;
	bool m_isPrivate;
	bool m_isSuccess;
};

class DataFetchJob final : public FetchJob
{
	Q_OBJECT

public:
	explicit DataFetchJob(const QUrl &url, QObject *parent = nullptr);

	QIODevice* getData() const;
	QMap<QByteArray, QByteArray> getHeaders() const;

protected:
	void handleSuccessfulReply(QNetworkReply *reply) override;

private:
	QNetworkReply *m_reply;
};

class IconFetchJob final : public FetchJob
{
	Q_OBJECT

public:
	explicit IconFetchJob(const QUrl &url, QObject *parent = nullptr);

	QIcon getIcon() const;

protected:
	void handleSuccessfulReply(QNetworkReply *reply) override;

private:
	QIcon m_icon;
};

}

#endif
