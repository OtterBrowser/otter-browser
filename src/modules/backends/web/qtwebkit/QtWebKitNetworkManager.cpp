/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "QtWebKitNetworkManager.h"
#include "QtWebKitCookieJar.h"
#include "QtWebKitFtpListingNetworkReply.h"
#include "../../../../core/AddonsManager.h"
#include "../../../../core/Console.h"
#include "../../../../core/CookieJar.h"
#include "../../../../core/LocalListingNetworkReply.h"
#include "../../../../core/NetworkCache.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/PasswordsManager.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/ThemesManager.h"
#include "../../../../core/WebBackend.h"
#include "../../../../ui/AuthenticationDialog.h"
#include "../../../../ui/ContentsDialog.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkReply>

namespace Otter
{

WebBackend* QtWebKitNetworkManager::m_backend = NULL;

QtWebKitNetworkManager::QtWebKitNetworkManager(bool isPrivate, QtWebKitCookieJar *cookieJarProxy, QtWebKitWebWidget *parent) : QNetworkAccessManager(parent),
	m_widget(parent),
	m_cookieJar(NULL),
	m_cookieJarProxy(cookieJarProxy),
	m_baseReply(NULL),
	m_contentState(WindowsManager::UnknownContentState),
	m_doNotTrackPolicy(NetworkManagerFactory::SkipTrackPolicy),
	m_bytesReceivedDifference(0),
	m_securityState(UnknownState),
	m_loadingSpeedTimer(0),
	m_areImagesEnabled(true),
	m_canSendReferrer(true)
{
	NetworkManagerFactory::initialize();

	if (!isPrivate)
	{
		m_cookieJar = NetworkManagerFactory::getCookieJar();
		m_cookieJar->setParent(QCoreApplication::instance());

		QNetworkDiskCache *cache(NetworkManagerFactory::getCache());

		setCache(cache);

		cache->setParent(QCoreApplication::instance());
	}
	else
	{
		m_cookieJar = new CookieJar(true, this);
	}

	if (m_cookieJarProxy)
	{
		m_cookieJarProxy->setParent(this);
	}
	else
	{
		m_cookieJarProxy = new QtWebKitCookieJar(m_cookieJar, parent);
	}

	setCookieJar(m_cookieJarProxy);

	connect(this, SIGNAL(finished(QNetworkReply*)), SLOT(requestFinished(QNetworkReply*)));
	connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(handleAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
	connect(this, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this, SLOT(handleProxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
	connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(handleSslErrors(QNetworkReply*,QList<QSslError>)));
	connect(NetworkManagerFactory::getInstance(), SIGNAL(onlineStateChanged(bool)), this, SLOT(handleOnlineStateChanged(bool)));
}

void QtWebKitNetworkManager::timerEvent(QTimerEvent *event)
{
	Q_UNUSED(event)

	updateLoadingSpeed();
}

void QtWebKitNetworkManager::addContentBlockingException(const QUrl &url, NetworkManager::ResourceType resourceType)
{
	m_contentBlockingExceptions.insert(url);

	if (resourceType == NetworkManager::ImageType && m_widget->getOption(SettingsManager::Browser_EnableImagesOption, m_widget->getUrl()).toString() == QLatin1String("onlyCached"))
	{
		m_areImagesEnabled = false;
	}
}

void QtWebKitNetworkManager::resetStatistics()
{
	killTimer(m_loadingSpeedTimer);

	const QList<WebWidget::PageInformation> keys(m_pageInformation.keys());

	m_sslInformation = WebWidget::SslInformation();
	m_loadingSpeedTimer = 0;
	m_blockedElements.clear();
	m_contentBlockingProfiles.clear();
	m_contentBlockingExceptions.clear();
	m_blockedRequests.clear();
	m_replies.clear();
	m_pageInformation.clear();
	m_pageInformation[WebWidget::BytesReceivedInformation] = quint64(0);
	m_pageInformation[WebWidget::BytesTotalInformation] = quint64(0);
	m_pageInformation[WebWidget::RequestsFinishedInformation] = 0;
	m_pageInformation[WebWidget::RequestsStartedInformation] = 0;
	m_baseReply = NULL;
	m_contentState = WindowsManager::UnknownContentState;
	m_bytesReceivedDifference = 0;
	m_securityState = UnknownState;

	updateLoadingSpeed();

	for (int i = 0; i < keys.count(); ++i)
	{
		emit pageInformationChanged(keys.at(i), m_pageInformation.value(keys.at(i)));
	}

	emit contentStateChanged(m_contentState);
}

void QtWebKitNetworkManager::registerTransfer(QNetworkReply *reply)
{
	if (reply && !reply->isFinished())
	{
		m_transfers.append(reply);

		setParent(NULL);

		connect(reply, SIGNAL(finished()), this, SLOT(transferFinished()));
	}
}

void QtWebKitNetworkManager::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	QNetworkReply *reply(qobject_cast<QNetworkReply*>(sender()));

	if (reply && reply == m_baseReply)
	{
		if (m_baseReply->hasRawHeader(QStringLiteral("Location").toLatin1()))
		{
			m_baseReply = NULL;
		}
		else
		{
			setPageInformation(WebWidget::DocumentLoadingProgressInformation, ((bytesTotal > 0) ? (((bytesReceived * 1.0) / bytesTotal) * 100) : -1));
		}
	}

	if (!reply || !m_replies.contains(reply))
	{
		return;
	}

	const QUrl url(reply->url());

	if (url.isValid() && url.scheme() != QLatin1String("data"))
	{
		setPageInformation(WebWidget::LoadingMessageInformation, tr("Receiving data from %1…").arg(reply->url().host().isEmpty() ? QLatin1String("localhost") : reply->url().host()));
	}

	const qint64 difference(bytesReceived - m_replies[reply].first);

	m_replies[reply].first = bytesReceived;

	if (!m_replies[reply].second && bytesTotal > 0)
	{
		m_replies[reply].second = true;

		m_pageInformation[WebWidget::BytesTotalInformation] = (m_pageInformation[WebWidget::BytesTotalInformation].toULongLong() + bytesTotal);
	}

	if (difference <= 0)
	{
		return;
	}

	m_bytesReceivedDifference += difference;

	setPageInformation(WebWidget::BytesReceivedInformation, (m_pageInformation[WebWidget::BytesReceivedInformation].toULongLong() + difference));
}

void QtWebKitNetworkManager::requestFinished(QNetworkReply *reply)
{
	if (!reply || !m_replies.contains(reply))
	{
		return;
	}

	const QUrl url(reply->url());

	m_replies.remove(reply);

	setPageInformation(WebWidget::RequestsFinishedInformation, (m_pageInformation[WebWidget::RequestsFinishedInformation].toInt() + 1));

	if (reply == m_baseReply)
	{
		if (reply->sslConfiguration().isNull())
		{
			m_sslInformation.certificates = QList<QSslCertificate>();
			m_sslInformation.cipher = QSslCipher();
		}
		else
		{
			m_sslInformation.certificates = reply->sslConfiguration().peerCertificateChain();
			m_sslInformation.cipher = reply->sslConfiguration().sessionCipher();
		}
	}

	if (url.isValid() && url.scheme() != QLatin1String("data"))
	{
		setPageInformation(WebWidget::LoadingMessageInformation, tr("Completed request to %1").arg(url.host().isEmpty() ? QLatin1String("localhost") : reply->url().host()));
	}

	disconnect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
}

void QtWebKitNetworkManager::transferFinished()
{
	QNetworkReply *reply(qobject_cast<QNetworkReply*>(sender()));

	if (reply)
	{
		m_transfers.removeAll(reply);

		reply->deleteLater();

		if (m_transfers.isEmpty())
		{
			if (m_widget)
			{
				setParent(m_widget);
			}
			else
			{
				deleteLater();
			}
		}
	}
}

void QtWebKitNetworkManager::handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
	setPageInformation(WebWidget::LoadingMessageInformation, tr("Waiting for authentication…"));

	AuthenticationDialog *authenticationDialog(new AuthenticationDialog(reply->url(), authenticator, m_widget));
	authenticationDialog->setButtonsVisible(false);

	ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, m_widget);

	connect(&dialog, SIGNAL(accepted()), authenticationDialog, SLOT(accept()));
	connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));
	connect(NetworkManagerFactory::getInstance(), SIGNAL(authenticated(QAuthenticator*,bool)), authenticationDialog, SLOT(authenticated(QAuthenticator*,bool)));

	m_widget->showDialog(&dialog);

	NetworkManagerFactory::notifyAuthenticated(authenticator, dialog.isAccepted());
}

void QtWebKitNetworkManager::handleProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
	if (NetworkManagerFactory::isUsingSystemProxyAuthentication())
	{
		authenticator->setUser(QString());

		return;
	}

	setPageInformation(WebWidget::LoadingMessageInformation, tr("Waiting for authentication…"));

	AuthenticationDialog *authenticationDialog(new AuthenticationDialog(proxy.hostName(), authenticator, m_widget));
	authenticationDialog->setButtonsVisible(false);

	ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, m_widget);

	connect(&dialog, SIGNAL(accepted()), authenticationDialog, SLOT(accept()));
	connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));
	connect(NetworkManagerFactory::getInstance(), SIGNAL(authenticated(QAuthenticator*,bool)), authenticationDialog, SLOT(authenticated(QAuthenticator*,bool)));

	m_widget->showDialog(&dialog);

	NetworkManagerFactory::notifyAuthenticated(authenticator, dialog.isAccepted());
}

void QtWebKitNetworkManager::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
	if (errors.isEmpty())
	{
		reply->ignoreSslErrors(errors);

		return;
	}

	QStringList ignoredErrors(m_widget->getOption(SettingsManager::Security_IgnoreSslErrorsOption, m_widget->getUrl()).toStringList());
	QStringList messages;
	QList<QSslError> errorsToIgnore;

	for (int i = 0; i < errors.count(); ++i)
	{
		if (errors.at(i).error() != QSslError::NoError)
		{
			m_sslInformation.errors.append(qMakePair(reply->url(), errors.at(i)));

			if (ignoredErrors.contains(errors.at(i).certificate().digest().toBase64()))
			{
				errorsToIgnore.append(errors.at(i));
			}
			else
			{
				messages.append(errors.at(i).errorString());
			}
		}
	}

	if (!errorsToIgnore.isEmpty())
	{
		reply->ignoreSslErrors(errorsToIgnore);
	}

	if (messages.isEmpty())
	{
		return;
	}

	ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-warning")), tr("Warning"), tr("SSL errors occured, do you want to continue?"), messages.join('\n'), (QDialogButtonBox::Yes | QDialogButtonBox::No), NULL, m_widget);

	if (!m_widget->getUrl().isEmpty())
	{
		dialog.setCheckBox(tr("Do not show this message again"), false);
	}

	connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	m_widget->showDialog(&dialog);

	if (dialog.isAccepted())
	{
		reply->ignoreSslErrors(errors);

		if (!m_widget->getUrl().isEmpty() && dialog.getCheckBoxState())
		{
			for (int i = 0; i < errors.count(); ++i)
			{
				const QString digest(errors.at(i).certificate().digest().toBase64());

				if (!ignoredErrors.contains(digest))
				{
					ignoredErrors.append(digest);
				}
			}

			SettingsManager::setValue(SettingsManager::Security_IgnoreSslErrorsOption, ignoredErrors, m_widget->getUrl());
		}
	}
}

void QtWebKitNetworkManager::handleOnlineStateChanged(bool isOnline)
{
	if (isOnline)
	{
		setNetworkAccessible(QNetworkAccessManager::Accessible);
	}
}

void QtWebKitNetworkManager::handleLoadingFinished()
{
	setPageInformation(WebWidget::LoadingFinishedInformation, QDateTime::currentDateTime());
	setPageInformation(WebWidget::LoadingMessageInformation, tr("Loading finished"));
	setPageInformation(WebWidget::LoadingSpeedInformation, 0);
	killTimer(m_loadingSpeedTimer);

	m_loadingSpeedTimer = 0;

	if ((m_securityState == SecureState || (m_securityState == UnknownState && m_contentState.testFlag(WindowsManager::SecureContentState))) && m_sslInformation.errors.isEmpty())
	{
		m_contentState = WindowsManager::SecureContentState;
	}

	emit contentStateChanged(m_contentState);
}

void QtWebKitNetworkManager::updateLoadingSpeed()
{
	setPageInformation(WebWidget::LoadingSpeedInformation, (m_bytesReceivedDifference * 2));

	m_bytesReceivedDifference = 0;
}

void QtWebKitNetworkManager::updateOptions(const QUrl &url)
{
	if (!m_widget)
	{
		return;
	}

	if (!m_backend)
	{
		m_backend = AddonsManager::getWebBackend(QLatin1String("qtwebkit"));
	}

	if (m_widget->getOption(SettingsManager::ContentBlocking_EnableContentBlockingOption, url).toBool())
	{
		m_contentBlockingProfiles = ContentBlockingManager::getProfileList(m_widget->getOption(SettingsManager::ContentBlocking_ProfilesOption, url).toStringList());
	}
	else
	{
		m_contentBlockingProfiles.clear();
	}

	QString acceptLanguage(m_widget->getOption(SettingsManager::Network_AcceptLanguageOption, url).toString());
	acceptLanguage = ((acceptLanguage.isEmpty()) ? QLatin1String(" ") : acceptLanguage.replace(QLatin1String("system"), QLocale::system().bcp47Name()));

	m_acceptLanguage = ((acceptLanguage == NetworkManagerFactory::getAcceptLanguage()) ? QString() : acceptLanguage);
	m_userAgent = m_backend->getUserAgent(NetworkManagerFactory::getUserAgent(m_widget->getOption(SettingsManager::Network_UserAgentOption, url).toString()).value);

	const QString doNotTrackPolicyValue(m_widget->getOption(SettingsManager::Network_DoNotTrackPolicyOption, url).toString());

	if (doNotTrackPolicyValue == QLatin1String("allow"))
	{
		m_doNotTrackPolicy = NetworkManagerFactory::AllowToTrackPolicy;
	}
	else if (doNotTrackPolicyValue == QLatin1String("doNotAllow"))
	{
		m_doNotTrackPolicy = NetworkManagerFactory::DoNotAllowToTrackPolicy;
	}
	else
	{
		m_doNotTrackPolicy = NetworkManagerFactory::SkipTrackPolicy;
	}

	m_areImagesEnabled = (m_widget->getOption(SettingsManager::Browser_EnableImagesOption, url).toString() != QLatin1String("disabled"));
	m_canSendReferrer = m_widget->getOption(SettingsManager::Network_EnableReferrerOption, url).toBool();

	const QString generalCookiesPolicyValue(m_widget->getOption(SettingsManager::Network_CookiesPolicyOption, url).toString());
	CookieJar::CookiesPolicy generalCookiesPolicy(CookieJar::AcceptAllCookies);

	if (generalCookiesPolicyValue == QLatin1String("ignore"))
	{
		generalCookiesPolicy = CookieJar::IgnoreCookies;
	}
	else if (generalCookiesPolicyValue == QLatin1String("readOnly"))
	{
		generalCookiesPolicy = CookieJar::ReadOnlyCookies;
	}
	else if (generalCookiesPolicyValue == QLatin1String("acceptExisting"))
	{
		generalCookiesPolicy = CookieJar::AcceptExistingCookies;
	}

	const QString thirdPartyCookiesPolicyValue(m_widget->getOption(SettingsManager::Network_ThirdPartyCookiesPolicyOption, url).toString());
	CookieJar::CookiesPolicy thirdPartyCookiesPolicy(CookieJar::AcceptAllCookies);

	if (thirdPartyCookiesPolicyValue == QLatin1String("ignore"))
	{
		thirdPartyCookiesPolicy = CookieJar::IgnoreCookies;
	}
	else if (thirdPartyCookiesPolicyValue == QLatin1String("readOnly"))
	{
		thirdPartyCookiesPolicy = CookieJar::ReadOnlyCookies;
	}
	else if (thirdPartyCookiesPolicyValue == QLatin1String("acceptExisting"))
	{
		thirdPartyCookiesPolicy = CookieJar::AcceptExistingCookies;
	}

	const QString keepModeValue(m_widget->getOption(SettingsManager::Network_CookiesKeepModeOption, url).toString());
	CookieJar::KeepMode keepMode(CookieJar::KeepUntilExpiresMode);

	if (keepModeValue == QLatin1String("keepUntilExit"))
	{
		keepMode = CookieJar::KeepUntilExitMode;
	}
	else if (keepModeValue == QLatin1String("ask"))
	{
		keepMode = CookieJar::AskIfKeepMode;
	}

	m_cookieJarProxy->setup(m_widget->getOption(SettingsManager::Network_ThirdPartyCookiesAcceptedHostsOption, url).toStringList(), m_widget->getOption(SettingsManager::Network_ThirdPartyCookiesRejectedHostsOption, url).toStringList(), generalCookiesPolicy, thirdPartyCookiesPolicy, keepMode);
}

void QtWebKitNetworkManager::setPageInformation(WebWidget::PageInformation key, const QVariant &value)
{
	if (m_loadingSpeedTimer != 0 || key != WebWidget::LoadingMessageInformation)
	{
		m_pageInformation[key] = value;

		emit pageInformationChanged(key, value);
	}
}

void QtWebKitNetworkManager::setFormRequest(const QUrl &url)
{
	m_formRequestUrl = url;
}

void QtWebKitNetworkManager::setWidget(QtWebKitWebWidget *widget)
{
	setParent(widget);

	m_widget = widget;

	m_cookieJarProxy->setWidget(widget);
}

QtWebKitNetworkManager* QtWebKitNetworkManager::clone()
{
	return new QtWebKitNetworkManager((cache() == NULL), m_cookieJarProxy->clone(NULL), NULL);
}

QNetworkReply* QtWebKitNetworkManager::createRequest(QNetworkAccessManager::Operation operation, const QNetworkRequest &request, QIODevice *outgoingData)
{
	if (request.url() == m_formRequestUrl)
	{
		m_formRequestUrl = QUrl();

		m_widget->openFormRequest(request.url(), operation, outgoingData);

		return QNetworkAccessManager::createRequest(QNetworkAccessManager::GetOperation, QNetworkRequest());
	}

	if (request.url().path() == QLatin1String("/otter-password") && request.hasRawHeader(QByteArray("X-Otter-Token")) && request.hasRawHeader(QByteArray("X-Otter-Data")))
	{
		if (QString(request.rawHeader(QByteArray("X-Otter-Token"))) == m_widget->getPasswordToken())
		{
			const QJsonObject passwordObject(QJsonDocument::fromJson(QByteArray::fromBase64(request.rawHeader(QByteArray("X-Otter-Data")))).object());
			const QJsonArray fieldsArray(passwordObject.value(QLatin1String("fields")).toArray());
			PasswordsManager::PasswordInformation password;
			password.url = QUrl(passwordObject.value(QLatin1String("url")).toString());
			password.timeAdded = QDateTime::currentDateTime();
			password.type = PasswordsManager::FormPassword;

			for (int i = 0; i < fieldsArray.count(); ++i)
			{
				const QJsonObject fieldObject(fieldsArray.at(i).toObject());
				PasswordsManager::FieldInformation field;
				field.name = fieldObject.value(QLatin1String("name")).toString();
				field.value = fieldObject.value(QLatin1String("value")).toString();
				field.type = ((fieldObject.value(QLatin1String("type")).toString() == QLatin1String("password")) ? PasswordsManager::PasswordField : PasswordsManager::TextField);

				password.fields.append(field);
			}

			PasswordsManager::PasswordMatch match(PasswordsManager::hasPassword(password));

			if (match != PasswordsManager::FullMatch)
			{
				m_widget->notifySavePasswordRequested(password, (match == PasswordsManager::PartialMatch));
			}
		}

		QUrl url;
		url.setScheme(QLatin1String("http"));

		return QNetworkAccessManager::createRequest(QNetworkAccessManager::GetOperation, QNetworkRequest(url));
	}

	if (m_contentBlockingExceptions.isEmpty() || !m_contentBlockingExceptions.contains(request.url()))
	{
		if (!m_areImagesEnabled && (request.rawHeader(QByteArray("Accept")).contains(QByteArray("image/")) || request.url().path().endsWith(QLatin1String(".png")) || request.url().path().endsWith(QLatin1String(".jpg")) || request.url().path().endsWith(QLatin1String(".gif"))))
		{
			return QNetworkAccessManager::createRequest(QNetworkAccessManager::GetOperation, QNetworkRequest(QUrl()));
		}

		if (!m_widget->isNavigating())
		{
			if (!m_contentBlockingProfiles.isEmpty())
			{
				const QByteArray acceptHeader(request.rawHeader(QByteArray("Accept")));
				const QString path(request.url().path());
				NetworkManager::ResourceType resourceType(NetworkManager::OtherType);
				bool storeBlockedUrl(true);

				if (!m_baseReply)
				{
					resourceType = NetworkManager::MainFrameType;
				}
				else if (acceptHeader.contains(QByteArray("text/html")) || acceptHeader.contains(QByteArray("application/xhtml+xml")) || acceptHeader.contains(QByteArray("application/xml")) || path.endsWith(QLatin1String(".htm")) || path.endsWith(QLatin1String(".html")))
				{
					resourceType = NetworkManager::SubFrameType;
				}
				else if (acceptHeader.contains(QByteArray("image/")) || path.endsWith(QLatin1String(".png")) || path.endsWith(QLatin1String(".jpg")) || path.endsWith(QLatin1String(".gif")))
				{
					resourceType = NetworkManager::ImageType;
				}
				else if (acceptHeader.contains(QByteArray("script/")) || path.endsWith(QLatin1String(".js")))
				{
					resourceType = NetworkManager::ScriptType;
					storeBlockedUrl = false;
				}
				else if (acceptHeader.contains(QByteArray("text/css")) || path.endsWith(QLatin1String(".css")))
				{
					resourceType = NetworkManager::StyleSheetType;
					storeBlockedUrl = false;
				}
				else if (acceptHeader.contains(QByteArray("object")))
				{
					resourceType = NetworkManager::ObjectType;
				}
				else if (request.rawHeader(QByteArray("X-Requested-With")) == QByteArray("XMLHttpRequest"))
				{
					resourceType = NetworkManager::XmlHttpRequestType;
				}

				const ContentBlockingManager::CheckResult result(ContentBlockingManager::checkUrl(m_contentBlockingProfiles, m_widget->getUrl(), request.url(), resourceType));

				if (result.isBlocked)
				{
					Console::addMessage(QCoreApplication::translate("main", "Blocked request"), Console::NetworkCategory, Console::LogLevel, request.url().toString(), -1, (m_widget ? m_widget->getWindowIdentifier() : 0));

					if (storeBlockedUrl)
					{
						m_blockedElements.append(request.url().url());
					}

					NetworkManager::ResourceInformation resource;
					resource.url = request.url();
					resource.resourceType = resourceType;
					resource.metaData[NetworkManager::ContentBlockingProfileMetaData] = result.profile;

					m_blockedRequests.append(resource);

					emit requestBlocked(resource);

					return QNetworkAccessManager::createRequest(QNetworkAccessManager::GetOperation, QNetworkRequest(QUrl()));
				}
			}
		}
	}

	setPageInformation(WebWidget::RequestsStartedInformation, (m_pageInformation[WebWidget::RequestsStartedInformation].toULongLong() + 1));

	QNetworkRequest mutableRequest(request);

	if (!m_canSendReferrer)
	{
		mutableRequest.setRawHeader(QStringLiteral("Referer").toLatin1(), QByteArray());
	}

	if (operation == PostOperation && mutableRequest.header(QNetworkRequest::ContentTypeHeader).isNull())
	{
		mutableRequest.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));
	}

	if (NetworkManagerFactory::isWorkingOffline())
	{
		mutableRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
	}
	else if (m_doNotTrackPolicy != NetworkManagerFactory::SkipTrackPolicy)
	{
		mutableRequest.setRawHeader(QStringLiteral("DNT").toLatin1(), ((m_doNotTrackPolicy == NetworkManagerFactory::DoNotAllowToTrackPolicy) ? QStringLiteral("1") : QStringLiteral("0")).toLatin1());
	}

	mutableRequest.setRawHeader(QStringLiteral("Accept-Language").toLatin1(), (m_acceptLanguage.isEmpty() ? NetworkManagerFactory::getAcceptLanguage().toLatin1() : m_acceptLanguage.toLatin1()));
	mutableRequest.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);

	setPageInformation(WebWidget::LoadingMessageInformation, tr("Sending request to %1…").arg(request.url().host()));

	QNetworkReply *reply(NULL);

	if (operation == GetOperation && request.url().isLocalFile() && QFileInfo(request.url().toLocalFile()).isDir())
	{
		reply = new LocalListingNetworkReply(this, request);
	}
	else if (operation == GetOperation && request.url().scheme() == QLatin1String("ftp"))
	{
		reply = new QtWebKitFtpListingNetworkReply(request, this);
	}
	else
	{
		reply = QNetworkAccessManager::createRequest(operation, mutableRequest, outgoingData);
	}

	if (!m_baseReply)
	{
		m_baseReply = reply;
	}

	if (m_securityState != InsecureState)
	{
		const QString scheme(reply->url().scheme());

		if (scheme == QLatin1String("https"))
		{
			m_securityState = SecureState;
		}
		else if (scheme == QLatin1String("http"))
		{
			m_securityState = InsecureState;
		}
	}

	m_replies[reply] = qMakePair(0, false);

	connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));

	if (m_loadingSpeedTimer == 0)
	{
		m_loadingSpeedTimer = startTimer(500);
	}

	return reply;
}

CookieJar* QtWebKitNetworkManager::getCookieJar()
{
	return m_cookieJar;
}

QVariant QtWebKitNetworkManager::getPageInformation(WebWidget::PageInformation key) const
{
	if (key == WebWidget::RequestsBlockedInformation)
	{
		return m_blockedRequests.count();
	}

	return m_pageInformation.value(key);
}

WebWidget::SslInformation QtWebKitNetworkManager::getSslInformation() const
{
	return m_sslInformation;
}

QString QtWebKitNetworkManager::getUserAgent() const
{
	return m_userAgent;
}

QStringList QtWebKitNetworkManager::getBlockedElements() const
{
	return m_blockedElements;
}

QList<NetworkManager::ResourceInformation> QtWebKitNetworkManager::getBlockedRequests() const
{
	return m_blockedRequests;
}

QHash<QByteArray, QByteArray> QtWebKitNetworkManager::getHeaders() const
{
	QHash<QByteArray, QByteArray> headers;

	if (m_baseReply)
	{
		const QList<QNetworkReply::RawHeaderPair> rawHeaders(m_baseReply->rawHeaderPairs());

		for (int i = 0; i < rawHeaders.count(); ++i)
		{
			headers[rawHeaders.at(i).first] = rawHeaders.at(i).second;
		}
	}

	return headers;
}

WindowsManager::ContentStates QtWebKitNetworkManager::getContentState() const
{
	return m_contentState;
}

}
