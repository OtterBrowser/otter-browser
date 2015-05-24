/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_TRANSFER_H
#define OTTER_TRANSFER_H

#include <QtCore/QIODevice>
#include <QtCore/QMimeType>
#include <QtCore/QPointer>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkReply>

namespace Otter
{

class NetworkManager;

class Transfer : public QObject
{
	Q_OBJECT

public:
	enum TransferState
	{
		UnknownState = 0,
		RunningState = 1,
		FinishedState = 2,
		ErrorState = 3,
		CancelledState = 4
	};

	explicit Transfer(QObject *parent);
	Transfer(const QSettings &settings, QObject *parent);
	Transfer(const QUrl &source, const QString &target, bool quickTransfer, QObject *parent);
	Transfer(const QNetworkRequest &request, const QString &target, bool quickTransfer, QObject *parent);
	Transfer(QNetworkReply *reply, const QString &target, bool quickTransfer, QObject *parent);

	virtual void setUpdateInterval(int interval);
	virtual QUrl getSource() const;
	virtual QString getTarget() const;
	virtual QDateTime getTimeStarted() const;
	virtual QDateTime getTimeFinished() const;
	virtual QMimeType getMimeType() const;
	virtual qint64 getSpeed() const;
	virtual qint64 getBytesReceived() const;
	virtual qint64 getBytesTotal() const;
	virtual TransferState getState() const;

public slots:
	void openTarget();
	virtual void stop();
	virtual bool resume();
	virtual bool restart();

protected:
	void timerEvent(QTimerEvent *event);
	void start(QNetworkReply *reply, const QString &target, bool quickTransfer);

protected slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadData();
	void downloadFinished();
	void downloadError(QNetworkReply::NetworkError error);

private:
	QPointer<QNetworkReply> m_reply;
	QPointer<QIODevice> m_device;
	QUrl m_source;
	QString m_target;
	QDateTime m_timeStarted;
	QDateTime m_timeFinished;
	QMimeType m_mimeType;
	qint64 m_speed;
	qint64 m_bytesStart;
	qint64 m_bytesReceivedDifference;
	qint64 m_bytesReceived;
	qint64 m_bytesTotal;
	TransferState m_state;
	int m_updateTimer;
	int m_updateInterval;

signals:
	void started();
	void finished();
	void changed();
	void stopped();
};

}

#endif
