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

#include "NetworkManager.h"
#include "ContentBlockingManager.h"
#include "Console.h"
#include "CookieJar.h"
#include "LocalListingNetworkReply.h"
#include "NetworkCache.h"
#include "NetworkManagerFactory.h"
#include "SessionsManager.h"
#include "Utils.h"
#include "../ui/AuthenticationDialog.h"
#include "../ui/ContentsDialog.h"
#include "../ui/ContentsWidget.h"

#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtWidgets/QMessageBox>
#include <QtNetwork/QNetworkProxy>

namespace Otter
{

NetworkManager::NetworkManager(bool isPrivate, bool useSimpleMode, ContentsWidget *widget) : QNetworkAccessManager(widget),
	m_widget(widget),
	m_mainReply(NULL),
	m_speed(0),
	m_bytesReceivedDifference(0),
	m_bytesReceived(0),
	m_bytesTotal(0),
	m_finishedRequests(0),
	m_startedRequests(0),
	m_updateTimer(0),
	m_useSimpleMode(useSimpleMode)
{
	if (!isPrivate)
	{
		QNetworkCookieJar *cookieJar = NetworkManagerFactory::getCookieJar();

		setCookieJar(cookieJar);

		cookieJar->setParent(QCoreApplication::instance());

		QNetworkDiskCache *cache = NetworkManagerFactory::getCache();

		setCache(cache);

		cache->setParent(QCoreApplication::instance());
	}
	else
	{
		setCookieJar(new CookieJar(true, this));
	}

	connect(this, SIGNAL(finished(QNetworkReply*)), SLOT(requestFinished(QNetworkReply*)));
	connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(handleAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
	connect(this, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this, SLOT(handleProxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
	connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(handleSslErrors(QNetworkReply*,QList<QSslError>)));
}

void NetworkManager::timerEvent(QTimerEvent *event)
{
	Q_UNUSED(event)

	updateStatus();
}

void NetworkManager::resetStatistics()
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

void NetworkManager::updateStatus()
{
	m_speed = (m_bytesReceivedDifference * 2);
	m_bytesReceivedDifference = 0;

	emit statusChanged(m_finishedRequests, m_startedRequests, m_bytesReceived, m_bytesTotal, m_speed);
}

void NetworkManager::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (reply && reply == m_mainReply)
	{
		if (m_mainReply->hasRawHeader(QStringLiteral("Location").toLatin1()))
		{
			m_mainReply = NULL;
		}
		else
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

void NetworkManager::requestFinished(QNetworkReply *reply)
{
	if (!m_useSimpleMode)
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

	if (!m_useSimpleMode && reply)
	{
		disconnect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
	}
}

void NetworkManager::handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
	if (m_widget)
	{
		AuthenticationDialog *authenticationDialog = new AuthenticationDialog(reply->url(), authenticator, m_widget);
		authenticationDialog->setButtonsVisible(false);

		ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, m_widget);

		connect(&dialog, SIGNAL(accepted()), authenticationDialog, SLOT(accept()));

		QEventLoop eventLoop;

		m_widget->showDialog(&dialog);

		connect(&dialog, SIGNAL(closed(bool,QDialogButtonBox::StandardButton)), &eventLoop, SLOT(quit()));
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

void NetworkManager::handleProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
	if (NetworkManagerFactory::isUsingSystemProxyAuthentication())
	{
		authenticator->setUser(QString());

		return;
	}

	if (m_widget)
	{
		AuthenticationDialog *authenticationDialog = new AuthenticationDialog(proxy.hostName(), authenticator, m_widget);
		authenticationDialog->setButtonsVisible(false);

		ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, m_widget);

		connect(&dialog, SIGNAL(accepted()), authenticationDialog, SLOT(accept()));

		QEventLoop eventLoop;

		m_widget->showDialog(&dialog);

		connect(&dialog, SIGNAL(closed(bool,QDialogButtonBox::StandardButton)), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();

		m_widget->hideDialog(&dialog);
	}
	else
	{
		AuthenticationDialog dialog(QUrl(proxy.hostName()), authenticator, SessionsManager::getActiveWindow());
		dialog.exec();
	}
}

void NetworkManager::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
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
		ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-warning")), tr("Warning"), tr("SSL errors occured, do you want to continue?"), messages.join('\n'), (QDialogButtonBox::Yes | QDialogButtonBox::No), NULL, m_widget);
		QEventLoop eventLoop;

		m_widget->showDialog(&dialog);

		connect(&dialog, SIGNAL(closed(bool,QDialogButtonBox::StandardButton)), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();

		m_widget->hideDialog(&dialog);

		if (dialog.isAccepted())
		{
			reply->ignoreSslErrors(errors);
		}
	}
	else if (QMessageBox::warning(SessionsManager::getActiveWindow(), tr("Warning"), tr("SSL errors occured:\n\n%1\n\nDo you want to continue?").arg(messages.join('\n')), (QMessageBox::Yes | QMessageBox::No)) == QMessageBox::Yes)
	{
		reply->ignoreSslErrors(errors);
	}
}

void NetworkManager::setUserAgent(const QString &identifier, const QString &value)
{
	m_userAgentIdentifier = ((identifier == QLatin1String("default")) ? QString() : identifier);
	m_userAgentValue = value;
}

NetworkManager* NetworkManager::clone(ContentsWidget *parent)
{
	NetworkManager *manager = NetworkManagerFactory::createManager((cache() == NULL), m_useSimpleMode, parent);
	manager->setCookieJar(qobject_cast<CookieJar*>(cookieJar())->clone(manager));

	return manager;
}

QNetworkReply* NetworkManager::createRequest(QNetworkAccessManager::Operation operation, const QNetworkRequest &request, QIODevice *outgoingData)
{
	if (!m_useSimpleMode)
	{
		++m_startedRequests;
	}

	if (operation == GetOperation && request.url().isLocalFile() && QFileInfo(request.url().toLocalFile()).isDir())
	{
		return new LocalListingNetworkReply(this, request);
	}

	if (ContentBlockingManager::isContentBlockingEnabled() && ContentBlockingManager::isUrlBlocked(request))
	{
		Console::addMessage(QCoreApplication::translate("main", "Blocked content: %0").arg(request.url().url()), Otter::NetworkMessageCategory, LogMessageLevel);

		return QNetworkAccessManager::createRequest(QNetworkAccessManager::GetOperation, QNetworkRequest(QUrl()));
	}

	QNetworkRequest mutableRequest(request);

	if (!m_userAgentIdentifier.isEmpty())
	{
		mutableRequest.setHeader(QNetworkRequest::UserAgentHeader, m_userAgentValue);
	}

	if (!NetworkManagerFactory::canSendReferrer())
	{
		mutableRequest.setRawHeader(QStringLiteral("Referer").toLatin1(), QByteArray());
	}

	if (operation == PostOperation && mutableRequest.header(QNetworkRequest::ContentTypeHeader).isNull())
	{
		mutableRequest.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));
	}

	if (NetworkManagerFactory::isWorkingOffline())
	{
		mutableRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
	}
	else if (NetworkManagerFactory::getDoNotTrackPolicy() != NetworkManagerFactory::SkipTrackPolicy)
	{
		mutableRequest.setRawHeader(QByteArray("DNT"), QByteArray((NetworkManagerFactory::getDoNotTrackPolicy() == NetworkManagerFactory::DoNotAllowToTrackPolicy) ? "1" : "0"));
	}

	QNetworkReply *reply = QNetworkAccessManager::createRequest(operation, mutableRequest, outgoingData);

	if (!m_mainReply)
	{
		m_mainReply = reply;
	}

	if (!m_useSimpleMode)
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

QPair<QString, QString> NetworkManager::getUserAgent() const
{
	return qMakePair(m_userAgentIdentifier, m_userAgentValue);
}

}
