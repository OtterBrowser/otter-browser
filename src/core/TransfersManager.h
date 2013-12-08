#ifndef OTTER_TRANSFERSMANAGER_H
#define OTTER_TRANSFERSMANAGER_H

#include "NetworkAccessManager.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace Otter
{

struct TransferInformation
{
	QIODevice *device;
	QString source;
	QString target;
	QDateTime started;
	QDateTime finished;
	qint64 speed;
	qint64 bytesReceivedDifference;
	qint64 bytesReceived;
	qint64 bytesTotal;
	bool running;

	TransferInformation() : device(NULL), speed(0), bytesReceivedDifference(0), bytesReceived(0), bytesTotal(-1), running(false) {}
};

class TransfersManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static TransfersManager* getInstance();
	static TransferInformation* startTransfer(const QString &source, const QString &target = QString());
	static TransferInformation* startTransfer(const QNetworkRequest &request, const QString &target = QString());
	static TransferInformation* startTransfer(QNetworkReply *reply, const QString &target = QString());
	static QList<TransferInformation*> getTransfers();
	static bool resumeTransfer(TransferInformation *transfer);
	static bool removeTransfer(TransferInformation *transfer, bool keepFile = true);
	static bool stopTransfer(TransferInformation *transfer);

protected:
	void timerEvent(QTimerEvent *event);
	void startUpdates();

protected slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadData(QNetworkReply *reply = NULL);
	void downloadFinished(QNetworkReply *reply = NULL);

private:
	explicit TransfersManager(QObject *parent = NULL);

	int m_updateTimer;

	static TransfersManager *m_instance;
	static NetworkAccessManager *m_networkAccessManager;
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
