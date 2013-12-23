#include "NetworkAccessManager.h"
#include "CookieJar.h"
#include "LocalListingNetworkReply.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "../ui/AuthenticationDialog.h"
#include "../ui/ContentsWidget.h"

#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QMessageBox>

namespace Otter
{

CookieJar* NetworkAccessManager::m_cookieJar = NULL;
QNetworkCookieJar* NetworkAccessManager::m_privateCookieJar = NULL;
QNetworkDiskCache* NetworkAccessManager::m_cache = NULL;

NetworkAccessManager::NetworkAccessManager(bool privateWindow, bool simpleMode, ContentsWidget *widget) : QNetworkAccessManager(widget),
	m_widget(widget),
	m_mainReply(NULL),
	m_speed(0),
	m_bytesReceivedDifference(0),
	m_bytesReceived(0),
	m_bytesTotal(0),
	m_finishedRequests(0),
	m_startedRequests(0),
	m_updateTimer(0),
	m_simpleMode(simpleMode)
{
	QNetworkCookieJar *cookieJar = getCookieJar(privateWindow);

	setCookieJar(cookieJar);

	cookieJar->setParent(QCoreApplication::instance());

	if (!privateWindow)
	{
		QNetworkDiskCache *cache = getCache();

		setCache(cache);

		cache->setParent(QCoreApplication::instance());
	}

	connect(this, SIGNAL(finished(QNetworkReply*)), SLOT(requestFinished(QNetworkReply*)));
	connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(handleAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
	connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(handleSslErrors(QNetworkReply*,QList<QSslError>)));
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

void NetworkAccessManager::clearCookies(int period)
{
	if (!m_cookieJar)
	{
		m_cookieJar = new CookieJar(QCoreApplication::instance());
	}

	m_cookieJar->clearCookies(period);
}

void NetworkAccessManager::clearCache(int period)
{
	QNetworkDiskCache *cache = getCache();

	if (period <= 0)
	{
		cache->clear();

		return;
	}

	const QDir cacheDirectory(cache->cacheDirectory());
	const QStringList directories = cacheDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

	for (int i = 0; i < directories.count(); ++i)
	{
		const QDir cacheSubDirectory(cacheDirectory.absoluteFilePath(directories.at(i)));
		const QStringList subDirectories = cacheSubDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

		for (int j = 0; j < subDirectories.count(); ++j)
		{
			const QDir cacheFilesDirectory(cacheSubDirectory.absoluteFilePath(subDirectories.at(j)));
			const QStringList files = cacheFilesDirectory.entryList(QDir::Files);

			for (int k = 0; k < files.count(); ++k)
			{
				const QNetworkCacheMetaData metaData = cache->fileMetaData(cacheFilesDirectory.absoluteFilePath(files.at(k)));

				if (metaData.isValid() && metaData.lastModified().isValid() && metaData.lastModified().secsTo(QDateTime::currentDateTime()) > (period * 3600))
				{
					cache->remove(metaData.url());
				}
			}
		}
	}
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

	const qint64 difference = (bytesReceived - m_replies[reply].first);

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
	if (!m_simpleMode)
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

	if (reply)
	{
		disconnect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));

		if (!m_simpleMode)
		{
			reply->deleteLater();
		}
	}
}

void NetworkAccessManager::handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
	if (m_widget)
	{
		AuthenticationDialog dialog(reply->url(), authenticator, m_widget);
		QEventLoop eventLoop;

		m_widget->showDialog(&dialog);

		connect(&dialog, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();

		m_widget->hideDialog(&dialog);
	}
	else
	{
		AuthenticationDialog dialog(reply->url(), authenticator, SessionsManager::getActiveWindow());
		dialog.exec();
	}
}

void NetworkAccessManager::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
	QStringList messages;

	for (int i = 0; i < errors.count(); ++i)
	{
		if (errors.at(i).error() != QSslError::NoError)
		{
			messages.append(errors.at(i).errorString());
		}
	}

	if (messages.isEmpty())
	{
		reply->ignoreSslErrors(errors);

		return;
	}

	if (m_widget)
	{
		QMessageBox dialog;
		dialog.setModal(false);
		dialog.setWindowTitle(tr("Warning"));
		dialog.setText(tr("SSL errors occured:\n\n%1\n\nDo you want to continue?").arg(messages.join('\n')));
		dialog.setIcon(QMessageBox::Warning);
		dialog.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

		QEventLoop eventLoop;

		m_widget->showDialog(&dialog);

		connect(&dialog, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();

		m_widget->hideDialog(&dialog);

		if (dialog.buttonRole(dialog.clickedButton()) == QMessageBox::YesRole)
		{
			reply->ignoreSslErrors(errors);
		}
	}
	else if (QMessageBox::warning(SessionsManager::getActiveWindow(), tr("Warning"), tr("SSL errors occured:\n\n%1\n\nDo you want to continue?").arg(messages.join('\n')), (QMessageBox::Yes | QMessageBox::No)) == QMessageBox::Yes)
	{
		reply->ignoreSslErrors(errors);
	}
}

QNetworkReply *NetworkAccessManager::createRequest(QNetworkAccessManager::Operation operation, const QNetworkRequest &request, QIODevice *outgoingData)
{
	if (!m_simpleMode)
	{
		++m_startedRequests;
	}

	if (operation == GetOperation && request.url().isLocalFile() && QFileInfo(request.url().toLocalFile()).isDir())
	{
		return new LocalListingNetworkReply(this, request);
	}

	QNetworkRequest mutableRequest(request);

	if (SettingsManager::getValue("Network/WorkOffline", false).toBool())
	{
		mutableRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
	}

	QNetworkReply *reply = QNetworkAccessManager::createRequest(operation, mutableRequest, outgoingData);

	if (!m_mainReply)
	{
		m_mainReply = reply;
	}

	if (!m_simpleMode)
	{
		m_replies[reply] = qMakePair(0, false);

		connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));

		if (m_updateTimer == 0)
		{
			m_updateTimer = startTimer(500);
		}
	}

	return reply;
}

QNetworkCookieJar* NetworkAccessManager::getCookieJar(bool privateCookieJar)
{
	if (!m_cookieJar && !privateCookieJar)
	{
		m_cookieJar = new CookieJar(QCoreApplication::instance());
	}

	if (!m_privateCookieJar && privateCookieJar)
	{
		m_privateCookieJar = new QNetworkCookieJar(QCoreApplication::instance());
	}

	return (privateCookieJar ? m_privateCookieJar : m_cookieJar);
}

QNetworkDiskCache *NetworkAccessManager::getCache()
{
	if (!m_cache)
	{
		m_cache = new QNetworkDiskCache(QCoreApplication::instance());
		m_cache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
	}

	return m_cache;
}

}
