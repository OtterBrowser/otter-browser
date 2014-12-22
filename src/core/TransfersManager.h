/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TRANSFERSMANAGER_H
#define OTTER_TRANSFERSMANAGER_H

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QMimeType>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace Otter
{

enum TransferState
{
	UnknownTransfer = 0,
	RunningTransfer = 1,
	FinishedTransfer = 2,
	ErrorTransfer = 3
};

struct TransferInformation
{
	QIODevice *device;
	QString source;
	QString target;
	QDateTime started;
	QDateTime finished;
	QMimeType mimeType;
	qint64 speed;
	qint64 bytesStart;
	qint64 bytesReceivedDifference;
	qint64 bytesReceived;
	qint64 bytesTotal;
	TransferState state;
	bool isPrivate;
	bool isHidden;

	TransferInformation() : device(NULL), speed(0), bytesStart(0), bytesReceivedDifference(0), bytesReceived(0), bytesTotal(-1), state(UnknownTransfer), isPrivate(false), isHidden(false) {}
};

class NetworkManager;

class TransfersManager : public QObject
{
	Q_OBJECT

public:
	~TransfersManager();

	static void createInstance(QObject *parent = NULL);
	static void clearTransfers(int period = 0);
	static TransfersManager* getInstance();
	static TransferInformation* startTransfer(const QString &source, const QString &target = QString(), bool privateTransfer = false, bool quickTransfer = false, bool skipTransfers = false);
	static TransferInformation* startTransfer(const QNetworkRequest &request, const QString &target = QString(), bool privateTransfer = false, bool quickTransfer = false, bool skipTransfers = false);
	static TransferInformation* startTransfer(QNetworkReply *reply, const QString &target = QString(), bool privateTransfer = false, bool quickTransfer = false, bool skipTransfers = false);
	static QString getSavePath(const QString &fileName, QString path = QString());
	static QList<TransferInformation*> getTransfers();
	static bool resumeTransfer(TransferInformation *transfer);
	static bool restartTransfer(TransferInformation *transfer);
	static bool removeTransfer(TransferInformation *transfer, bool keepFile = true);
	static bool stopTransfer(TransferInformation *transfer);
	static bool isDownloading(const QString &source, const QString &target = QString());

protected:
	explicit TransfersManager(QObject *parent = NULL);

	void timerEvent(QTimerEvent *event);
	void startUpdates();

protected slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadData(QNetworkReply *reply = NULL);
	void downloadFinished(QNetworkReply *reply = NULL);
	void downloadError(QNetworkReply::NetworkError error);
	void save();

private:
	int m_updateTimer;

	static TransfersManager *m_instance;
	static NetworkManager *m_networkManager;
	static QHash<QNetworkReply*, TransferInformation*> m_replies;
	static QList<TransferInformation*> m_transfers;

signals:
	void transferStarted(TransferInformation *transfer);
	void transferFinished(TransferInformation *transfer);
	void transferUpdated(TransferInformation *transfer);
	void transferStopped(TransferInformation *transfer);
	void transferRemoved(TransferInformation *transfer);
};

}

#endif
