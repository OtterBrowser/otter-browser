#include "NetworkAccessManager.h"
#include "CookieJar.h"
#include "LocalListingNetworkReply.h"
#include "SessionsManager.h"
#include "../ui/AuthenticationDialog.h"

#include <QtCore/QFileInfo>

namespace Otter
{

CookieJar* NetworkAccessManager::m_cookieJar = NULL;
QNetworkCookieJar* NetworkAccessManager::m_privateCookieJar = NULL;

NetworkAccessManager::NetworkAccessManager(bool privateWindow, QObject *parent) : QNetworkAccessManager(parent),
	m_mainReply(NULL),
	m_speed(0),
	m_bytesReceivedDifference(0),
	m_bytesReceived(0),
	m_bytesTotal(0),
	m_finishedRequests(0),
	m_startedRequests(0),
	m_updateTimer(0)
{
	if (!m_cookieJar && !privateWindow)
	{
		m_cookieJar = new CookieJar(QCoreApplication::instance());
	}

	if (!m_privateCookieJar && privateWindow)
	{
		m_privateCookieJar = new QNetworkCookieJar(QCoreApplication::instance());
	}

	QNetworkCookieJar *cookieJar = (privateWindow ? m_privateCookieJar : m_cookieJar);

	setCookieJar(cookieJar);

	cookieJar->setParent(QCoreApplication::instance());

	connect(this, SIGNAL(finished(QNetworkReply*)), SLOT(requestFinished(QNetworkReply*)));
	connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), SLOT(authenticate(QNetworkReply*,QAuthenticator*)));
}

void NetworkAccessManager::resetStatistics()
{
	killTimer(m_updateTimer);

	updateStatus();

	m_updateTimer = 0;
	m_replies.clear();
	m_mainReply = NULL;
	m_speed = 0;
	m_bytesReceivedDifference = 0;
	m_bytesReceived = 0;
	m_bytesTotal = 0;
	m_finishedRequests = 0;
	m_startedRequests = 0;
}

void NetworkAccessManager::timerEvent(QTimerEvent *event)
{
	Q_UNUSED(event)

	updateStatus();
}

void NetworkAccessManager::updateStatus()
{
	m_speed = (m_bytesReceivedDifference * 2);
	m_bytesReceivedDifference = 0;

	emit statusChanged(m_finishedRequests, m_startedRequests, m_bytesReceived, m_bytesTotal, m_speed);
}

QNetworkReply *NetworkAccessManager::createRequest(QNetworkAccessManager::Operation operation, const QNetworkRequest &request, QIODevice *outgoingData)
{
	++m_startedRequests;

	if (operation == GetOperation && request.url().isLocalFile() && QFileInfo(request.url().toLocalFile()).isDir())
	{
		return new LocalListingNetworkReply(this, request);
	}

	QNetworkReply *reply = QNetworkAccessManager::createRequest(operation, request, outgoingData);

	if (!m_mainReply)
	{
		m_mainReply = reply;
	}

	m_replies[reply] = qMakePair(0, false);

	connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));

	if (m_updateTimer == 0)
	{
		m_updateTimer = startTimer(500);
	}

	return reply;
}

void NetworkAccessManager::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (reply && reply == m_mainReply)
	{
		if (bytesTotal > 0)
		{
			emit documentLoadProgressChanged(((bytesReceived * 1.0) / bytesTotal) * 100);
		}
		else
		{
			emit documentLoadProgressChanged(-1);
		}
	}

	if (!reply || !m_replies.contains(reply))
	{
		return;
	}

	const qreal difference = (bytesReceived - m_replies[reply].first);

	m_replies[reply].first = bytesReceived;

	if (!m_replies[reply].second && bytesTotal > 0)
	{
		m_replies[reply].second = true;

		m_bytesTotal += bytesTotal;
	}

	if (difference <= 0)
	{
		return;
	}

	m_bytesReceived += difference;
	m_bytesReceivedDifference += difference;
}

void NetworkAccessManager::requestFinished(QNetworkReply *reply)
{
	m_replies.remove(reply);

	if (m_replies.isEmpty())
	{
		killTimer(m_updateTimer);

		m_updateTimer = 0;

		updateStatus();
	}

	++m_finishedRequests;
}

void NetworkAccessManager::authenticate(QNetworkReply *reply, QAuthenticator *authenticator)
{
	AuthenticationDialog dialog(reply->url(), authenticator, SessionsManager::getActiveWindow());
	dialog.exec();
}

}
