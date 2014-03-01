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

#include "NetworkAccessManager.h"
#include "CookieJar.h"
#include "LocalListingNetworkReply.h"
#include "NetworkCache.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
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

CookieJar* NetworkAccessManager::m_cookieJar = NULL;
QNetworkCookieJar* NetworkAccessManager::m_privateCookieJar = NULL;
NetworkCache* NetworkAccessManager::m_cache = NULL;
QStringList NetworkAccessManager::m_userAgentsOrder;
QHash<QString, UserAgentInformation> NetworkAccessManager::m_userAgents;

NetworkAccessManager::NetworkAccessManager(bool privateWindow, bool simpleMode, ContentsWidget *widget) : QNetworkAccessManager(widget),
	m_widget(widget),
	m_mainReply(NULL),
	m_speed(0),
	m_bytesReceivedDifference(0),
	m_bytesReceived(0),
	m_bytesTotal(0),
	m_doNotTrackPolicy(SkipTrackPolicy),
	m_finishedRequests(0),
	m_startedRequests(0),
	m_updateTimer(0),
	m_simpleMode(simpleMode),
	m_disableReferrer(false)
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

	optionChanged(QLatin1String("Network/DoNotTrackPolicy"), SettingsManager::getValue(QLatin1String("Network/DoNotTrackPolicy")));
	optionChanged(QLatin1String("Network/EnableReferrer"), SettingsManager::getValue(QLatin1String("Network/EnableReferrer")));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(this, SIGNAL(finished(QNetworkReply*)), SLOT(requestFinished(QNetworkReply*)));
	connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(handleAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
	connect(this, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this, SLOT(handleProxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
	connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(handleSslErrors(QNetworkReply*,QList<QSslError>)));
}

void NetworkAccessManager::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Network/DoNotTrackPolicy"))
	{
		const QString policyValue = value.toString();

		if (policyValue == QLatin1String("allow"))
		{
			m_doNotTrackPolicy = AllowToTrackPolicy;
		}
		else if (policyValue == QLatin1String("doNotAllow"))
		{
			m_doNotTrackPolicy = DoNotAllowToTrackPolicy;
		}
		else
		{
			m_doNotTrackPolicy = SkipTrackPolicy;
		}
	}
	else if (option == QLatin1String("Network/EnableReferrer"))
	{
		m_disableReferrer = !value.toBool();
	}
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
	getCache()->clearCache(period);
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

	if (!m_simpleMode && reply)
	{
		disconnect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
	}
}

void NetworkAccessManager::handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
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

void NetworkAccessManager::handleProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
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

void NetworkAccessManager::setUserAgent(const QString &identifier, const QString &value)
{
	m_userAgentIdentifier = identifier;
	m_userAgentValue = value;
}

QNetworkReply* NetworkAccessManager::createRequest(QNetworkAccessManager::Operation operation, const QNetworkRequest &request, QIODevice *outgoingData)
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

	if (m_disableReferrer)
	{
		mutableRequest.setRawHeader(QStringLiteral("Referer").toLatin1(), QByteArray());
	}

	if (!m_userAgentIdentifier.isEmpty())
	{
		mutableRequest.setHeader(QNetworkRequest::UserAgentHeader, m_userAgentValue);
	}

	if (SettingsManager::getValue(QLatin1String("Network/WorkOffline")).toBool())
	{
		mutableRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);

		if (m_doNotTrackPolicy != SkipTrackPolicy)
		{
			mutableRequest.setRawHeader(QByteArray("DNT"), QByteArray((m_doNotTrackPolicy == DoNotAllowToTrackPolicy) ? "1" : "0"));
		}
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

NetworkCache* NetworkAccessManager::getCache()
{
	if (!m_cache)
	{
		m_cache = new NetworkCache(QCoreApplication::instance());
	}

	return m_cache;
}

UserAgentInformation NetworkAccessManager::getUserAgent(const QString &identifier)
{
	if (identifier.isEmpty() || !m_userAgents.contains(identifier))
	{
		UserAgentInformation userAgent;
		userAgent.identifier = QLatin1String("default");
		userAgent.title = tr("Default");
		userAgent.description = tr("Default User Agent");

		return userAgent;
	}

	return m_userAgents[identifier];
}

QPair<QString, QString> NetworkAccessManager::getUserAgent() const
{
	return qMakePair(m_userAgentIdentifier, m_userAgentValue);
}

QStringList NetworkAccessManager::getUserAgents()
{
	return m_userAgentsOrder;
}

}
