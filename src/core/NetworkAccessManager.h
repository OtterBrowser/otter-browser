#ifndef OTTER_NETWORKACCESSMANAGER_H
#define OTTER_NETWORKACCESSMANAGER_H

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkDiskCache>

namespace Otter
{

class ContentsWidget;
class CookieJar;

class NetworkAccessManager : public QNetworkAccessManager
{
	Q_OBJECT

public:
	explicit NetworkAccessManager(bool privateWindow = false, bool simpleMode = false, ContentsWidget *widget = NULL);

	void resetStatistics();
	static void clearCookies(int period = 0);
	static void clearCache(int period = 0);
	static QNetworkCookieJar* getCookieJar(bool privateCookieJar = false);
	static QNetworkDiskCache* getCache();

protected:
	void timerEvent(QTimerEvent *event);
	void updateStatus();
	QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoingData);

protected slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void requestFinished(QNetworkReply *reply);
	void handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
	void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
	ContentsWidget *m_widget;
	QNetworkReply *m_mainReply;
	QHash<QNetworkReply*, QPair<qint64, bool> > m_replies;
	qint64 m_speed;
	qint64 m_bytesReceivedDifference;
	qint64 m_bytesReceived;
	qint64 m_bytesTotal;
	int m_finishedRequests;
	int m_startedRequests;
	int m_updateTimer;
	bool m_simpleMode;

	static CookieJar *m_cookieJar;
	static QNetworkCookieJar *m_privateCookieJar;
	static QNetworkDiskCache *m_cache;

signals:
	void messageChanged(const QString &message = QString());
	void documentLoadProgressChanged(int progress);
	void statusChanged(int finishedRequests, int startedReuests, qint64 bytesReceived, qint64 bytesTotal, qint64 speed);
};

}

#endif
