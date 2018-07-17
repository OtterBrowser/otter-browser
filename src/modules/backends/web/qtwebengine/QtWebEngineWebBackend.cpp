/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "QtWebEngineWebBackend.h"
#include "QtWebEngineTransfer.h"
#include "QtWebEngineUrlRequestInterceptor.h"
#include "QtWebEngineWebWidget.h"
#include "../../../../core/HandlersManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/TransfersManager.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/TransferDialog.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtWebEngineWidgets/QWebEngineProfile>
#include <QtWebEngineWidgets/QWebEngineSettings>

namespace Otter
{

QString QtWebEngineWebBackend::m_engineVersion;
QMap<QString, QString> QtWebEngineWebBackend::m_userAgentComponents;
QMap<QString, QString> QtWebEngineWebBackend::m_userAgents;

QtWebEngineWebBackend::QtWebEngineWebBackend(QObject *parent) : WebBackend(parent),
	m_requestInterceptor(nullptr),
	m_isInitialized(false)
{
	const QString userAgent(QWebEngineProfile::defaultProfile()->httpUserAgent());

	m_engineVersion = QRegularExpression(QLatin1String("Chrome/([\\d\\.]+)")).match(userAgent).captured(1);

	m_userAgentComponents = {{QLatin1String("platform"), QRegularExpression(QLatin1String("(\\([^\\)]+\\))")).match(userAgent).captured(1)}, {QLatin1String("engineVersion"), QLatin1String("AppleWebKit/537.36 (KHTML, like Gecko) Chrome/") + m_engineVersion}, {QLatin1String("applicationVersion"), QCoreApplication::applicationName() + QLatin1Char('/') + QCoreApplication::applicationVersion()}};
}

void QtWebEngineWebBackend::handleDownloadRequested(QWebEngineDownloadItem *item)
{
	if (item->savePageFormat() != QWebEngineDownloadItem::UnknownSaveFormat)
	{
		return;
	}

	const QWebEngineProfile *profile(qobject_cast<QWebEngineProfile*>(sender()));
	QtWebEngineTransfer *transfer(new QtWebEngineTransfer(item, (Transfer::CanNotifyOption | ((profile && profile->isOffTheRecord()) ? Transfer::IsPrivateOption : Transfer::NoOption))));

	if (transfer->getState() == Transfer::CancelledState)
	{
		transfer->deleteLater();

		return;
	}

	const HandlersManager::HandlerDefinition handler(HandlersManager::getHandler(transfer->getMimeType()));

	switch (handler.transferMode)
	{
		case HandlersManager::HandlerDefinition::IgnoreTransfer:
			transfer->cancel();
			transfer->deleteLater();

			break;
		case HandlersManager::HandlerDefinition::AskTransfer:
			TransferDialog(transfer).exec();

			break;
		case HandlersManager::HandlerDefinition::OpenTransfer:
			transfer->setOpenCommand(handler.openCommand);

			TransfersManager::addTransfer(transfer);

			break;
		case HandlersManager::HandlerDefinition::SaveTransfer:
			transfer->setTarget(handler.downloadsPath + QDir::separator() + transfer->getSuggestedFileName());

			if (transfer->getState() == Transfer::CancelledState)
			{
				TransfersManager::addTransfer(transfer);
			}
			else
			{
				transfer->deleteLater();
			}

			break;
		case HandlersManager::HandlerDefinition::SaveAsTransfer:
			{
			const QString path(Utils::getSavePath(transfer->getSuggestedFileName(), handler.downloadsPath, {}, true).path);

				if (path.isEmpty())
				{
					transfer->cancel();
					transfer->deleteLater();

					return;
				}

				transfer->setTarget(path);

				TransfersManager::addTransfer(transfer);
			}

			break;
		default:
			break;
	}
}

void QtWebEngineWebBackend::handleOptionChanged(int identifier)
{
	switch (identifier)
	{
		case SettingsManager::Browser_PrintElementBackgroundsOption:
			QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::PrintElementBackgrounds, SettingsManager::getOption(SettingsManager::Browser_PrintElementBackgroundsOption).toBool());

			return;
		case SettingsManager::Interface_EnableSmoothScrollingOption:
			QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, SettingsManager::getOption(SettingsManager::Interface_EnableSmoothScrollingOption).toBool());

			return;
		case SettingsManager::Network_AcceptLanguageOption:
			QWebEngineProfile::defaultProfile()->setHttpAcceptLanguage(NetworkManagerFactory::getAcceptLanguage());

			return;
		case SettingsManager::Network_UserAgentOption:
			QWebEngineProfile::defaultProfile()->setHttpUserAgent(getUserAgent());

			return;
		default:
			break;
	}

	if (SettingsManager::getOptionName(identifier).startsWith(QLatin1String("Content/")))
	{
		QWebEngineSettings::globalSettings()->setFontSize(QWebEngineSettings::DefaultFontSize, SettingsManager::getOption(SettingsManager::Content_DefaultFontSizeOption).toInt());
		QWebEngineSettings::globalSettings()->setFontSize(QWebEngineSettings::DefaultFixedFontSize, SettingsManager::getOption(SettingsManager::Content_DefaultFixedFontSizeOption).toInt());
		QWebEngineSettings::globalSettings()->setFontSize(QWebEngineSettings::MinimumFontSize, SettingsManager::getOption(SettingsManager::Content_MinimumFontSizeOption).toInt());
		QWebEngineSettings::globalSettings()->setFontFamily(QWebEngineSettings::StandardFont, SettingsManager::getOption(SettingsManager::Content_StandardFontOption).toString());
		QWebEngineSettings::globalSettings()->setFontFamily(QWebEngineSettings::FixedFont, SettingsManager::getOption(SettingsManager::Content_FixedFontOption).toString());
		QWebEngineSettings::globalSettings()->setFontFamily(QWebEngineSettings::SerifFont, SettingsManager::getOption(SettingsManager::Content_SerifFontOption).toString());
		QWebEngineSettings::globalSettings()->setFontFamily(QWebEngineSettings::SansSerifFont, SettingsManager::getOption(SettingsManager::Content_SansSerifFontOption).toString());
		QWebEngineSettings::globalSettings()->setFontFamily(QWebEngineSettings::CursiveFont, SettingsManager::getOption(SettingsManager::Content_CursiveFontOption).toString());
		QWebEngineSettings::globalSettings()->setFontFamily(QWebEngineSettings::FantasyFont, SettingsManager::getOption(SettingsManager::Content_FantasyFontOption).toString());
	}
	else if (SettingsManager::getOptionName(identifier).startsWith(QLatin1String("Permissions/")))
	{
		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::AutoLoadImages, (SettingsManager::getOption(SettingsManager::Permissions_EnableImagesOption).toString() != QLatin1String("onlyCached")));
		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnablePluginsOption).toString() != QLatin1String("disabled"));
		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::JavascriptEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnableJavaScriptOption).toBool());
		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanAccessClipboardOption).toBool());
		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, (SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption).toString() != QLatin1String("blockAll")));
		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::WebGLEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnableWebglOption).toBool());
		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnableLocalStorageOption).toBool());
	}
}

WebWidget* QtWebEngineWebBackend::createWidget(const QVariantMap &parameters, ContentsWidget *parent)
{
	if (!m_isInitialized)
	{
		m_isInitialized = true;
		m_requestInterceptor = new QtWebEngineUrlRequestInterceptor(this);

		QWebEngineProfile::defaultProfile()->setHttpAcceptLanguage(NetworkManagerFactory::getAcceptLanguage());
		QWebEngineProfile::defaultProfile()->setHttpUserAgent(getUserAgent());
		QWebEngineProfile::defaultProfile()->setRequestInterceptor(m_requestInterceptor);

		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, false);
		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::XSSAuditingEnabled, true);
		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::PrintElementBackgrounds, SettingsManager::getOption(SettingsManager::Browser_PrintElementBackgroundsOption).toBool());
		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, SettingsManager::getOption(SettingsManager::Interface_EnableSmoothScrollingOption).toBool());

		const QString cachePath(SessionsManager::getCachePath());

		if (cachePath.isEmpty())
		{
			QWebEngineProfile::defaultProfile()->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
		}
		else
		{
			QDir().mkpath(cachePath);

			QWebEngineProfile::defaultProfile()->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
			QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize(SettingsManager::getOption(SettingsManager::Cache_DiskCacheLimitOption).toInt() * 1024);
			QWebEngineProfile::defaultProfile()->setCachePath(cachePath);
			QWebEngineProfile::defaultProfile()->setPersistentStoragePath(cachePath + QLatin1String("/persistentStorage/"));
		}

		handleOptionChanged(SettingsManager::Content_DefaultFontSizeOption);
		handleOptionChanged(SettingsManager::Permissions_EnableFullScreenOption);

		connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &QtWebEngineWebBackend::handleOptionChanged);
		connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested, this, &QtWebEngineWebBackend::handleDownloadRequested);
	}

	return new QtWebEngineWebWidget(parameters, this, parent);
}

QString QtWebEngineWebBackend::getName() const
{
	return QLatin1String("qtwebengine");
}

QString QtWebEngineWebBackend::getTitle() const
{
	return tr("Blink Backend (experimental)");
}

QString QtWebEngineWebBackend::getDescription() const
{
	return tr("Backend utilizing QtWebEngine module");
}

QString QtWebEngineWebBackend::getVersion() const
{
	return QCoreApplication::applicationVersion();
}

QString QtWebEngineWebBackend::getEngineVersion() const
{
	return m_engineVersion;
}

QString QtWebEngineWebBackend::getSslVersion() const
{
	return QLatin1String("BoringSSL");
}

QString QtWebEngineWebBackend::getUserAgent(const QString &pattern) const
{
	if (!pattern.isEmpty())
	{
		if (m_userAgents.contains(pattern))
		{
			return (m_userAgents[pattern].isEmpty() ? pattern : m_userAgents[pattern]);
		}

		QString userAgent(pattern);
		QMap<QString, QString>::iterator iterator;

		for (iterator = m_userAgentComponents.begin(); iterator != m_userAgentComponents.end(); ++iterator)
		{
			userAgent = userAgent.replace(QLatin1Char('{') + iterator.key() + QLatin1Char('}'), iterator.value());
		}

		m_userAgents[pattern] = ((pattern == userAgent) ? QString() : userAgent);

		return userAgent;
	}

	const UserAgentDefinition userAgent(NetworkManagerFactory::getUserAgent(SettingsManager::getOption(SettingsManager::Network_UserAgentOption).toString()));

	return ((userAgent.value.isEmpty()) ? QString() : getUserAgent(userAgent.value));
}

QStringList QtWebEngineWebBackend::getBlockedElements(const QString &domain) const
{
	return (m_requestInterceptor ? m_requestInterceptor->getBlockedElements(domain) : QStringList());
}

QUrl QtWebEngineWebBackend::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

WebBackend::BackendCapabilities QtWebEngineWebBackend::getCapabilities() const
{
	return (UserScriptsCapability | GlobalCookiesPolicyCapability | GlobalContentFilteringCapability | GlobalDoNotTrackCapability | GlobalProxyCapability | GlobalReferrerCapability | GlobalUserAgentCapability);
}

bool QtWebEngineWebBackend::requestThumbnail(const QUrl &url, const QSize &size)
{
	Q_UNUSED(url)
	Q_UNUSED(size)

	return false;
}

}
