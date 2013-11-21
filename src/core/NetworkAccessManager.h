#ifndef OTTER_NETWORKACCESSMANAGER_H
#define OTTER_NETWORKACCESSMANAGER_H

#include <QtNetwork/QNetworkAccessManager>

namespace Otter
{

class NetworkAccessManager : public QNetworkAccessManager
{
	Q_OBJECT

public:
	explicit NetworkAccessManager(QObject *parent = NULL);

	void resetStatistics();

protected:
	void timerEvent(QTimerEvent *event);
	void updateStatus();
	QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoingData);

protected slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void requestFinished(QNetworkReply *reply);

private:
	QNetworkReply* m_mainReply;
	QHash<QNetworkReply*, QPair<qint64, bool> > m_replies;
	qint64 m_speed;
	qint64 m_bytesReceivedDifference;
	qint64 m_bytesReceived;
	qint64 m_bytesTotal;
	int m_finishedRequests;
	int m_startedRequests;
	int m_updateTimer;

signals:
	void messageChanged(const QString &message = QString());
	void documentLoadProgressChanged(int progress);
	void statusChanged(int finishedRequests, int startedReuests, qint64 bytesReceived, qint64 bytesTotal, qint64 speed);
};

}

#endif
