/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "QtWebEnginePage.h"
#include "QtWebEngineTransfer.h"
#include "QtWebEngineWebWidget.h"
#include "../../../../core/ContentFiltersManager.h"
#include "../../../../core/HandlersManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/NotificationsManager.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/SpellCheckManager.h"
#include "../../../../core/TransfersManager.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/TransferDialog.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtWebEngineWidgets/QWebEngineSettings>

namespace Otter
{

QString QtWebEngineWebBackend::m_engineVersion;
QHash<QString, QString> QtWebEngineWebBackend::m_userAgentComponents;
QMap<QString, QString> QtWebEngineWebBackend::m_userAgents;

QtWebEngineWebBackend::QtWebEngineWebBackend(QObject *parent) : WebBackend(parent),
	m_isInitialized(false)
{
	const QString userAgent(QWebEngineProfile::defaultProfile()->httpUserAgent());

	m_engineVersion = QRegularExpression(QLatin1String("Chrome/([\\d\\.]+)")).match(userAgent).captured(1);

	m_userAgentComponents = {{QLatin1String("platform"), QRegularExpression(QLatin1String(R"((\([^\)]+\)))")).match(userAgent).captured(1)}, {QLatin1String("engineVersion"), QLatin1String("AppleWebKit/537.36 (KHTML, like Gecko) Chrome/") + m_engineVersion}, {QLatin1String("applicationVersion"), QCoreApplication::applicationName() + QLatin1Char('/') + QCoreApplication::applicationVersion()}};

	qputenv("QTWEBENGINE_DICTIONARIES_PATH", SpellCheckManager::getDictionariesPath().toLatin1());
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

	QtWebEnginePage *page(qobject_cast<QtWebEnginePage*>(item->page()));

	if (page && page->getWebWidget())
	{
		page->getWebWidget()->startTransfer(transfer);

		return;
	}

	const HandlersManager::MimeTypeHandlerDefinition handler(HandlersManager::getMimeTypeHandler(transfer->getMimeType()));

	switch (handler.transferMode)
	{
		case HandlersManager::MimeTypeHandlerDefinition::IgnoreTransfer:
			transfer->cancel();
			transfer->deleteLater();

			break;
		case HandlersManager::MimeTypeHandlerDefinition::AskTransfer:
			TransferDialog(transfer).exec();

			break;
		case HandlersManager::MimeTypeHandlerDefinition::OpenTransfer:
			transfer->setOpenCommand(handler.openCommand);

			TransfersManager::addTransfer(transfer);

			break;
		case HandlersManager::MimeTypeHandlerDefinition::SaveTransfer:
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
		case HandlersManager::MimeTypeHandlerDefinition::SaveAsTransfer:
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
	QWebEngineProfile *profile(QWebEngineProfile::defaultProfile());
	QWebEngineSettings *settings(QWebEngineSettings::globalSettings());

	switch (identifier)
	{
		case SettingsManager::Browser_PrintElementBackgroundsOption:
			settings->setAttribute(QWebEngineSettings::PrintElementBackgrounds, SettingsManager::getOption(SettingsManager::Browser_PrintElementBackgroundsOption).toBool());

			return;
		case SettingsManager::Interface_EnableSmoothScrollingOption:
			settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, SettingsManager::getOption(SettingsManager::Interface_EnableSmoothScrollingOption).toBool());

			return;
		case SettingsManager::Network_AcceptLanguageOption:
			profile->setHttpAcceptLanguage(NetworkManagerFactory::getAcceptLanguage());

			return;
		case SettingsManager::Network_EnableDnsPrefetchOption:
			settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, SettingsManager::getOption(SettingsManager::Network_EnableDnsPrefetchOption).toBool());

			return;
		case SettingsManager::Network_UserAgentOption:
			profile->setHttpUserAgent(getUserAgent());

			return;
		default:
			break;
	}

	const QString optionName(SettingsManager::getOptionName(identifier));

	if (optionName.startsWith(QLatin1String("Content/")))
	{
		settings->setFontSize(QWebEngineSettings::DefaultFontSize, SettingsManager::getOption(SettingsManager::Content_DefaultFontSizeOption).toInt());
		settings->setFontSize(QWebEngineSettings::DefaultFixedFontSize, SettingsManager::getOption(SettingsManager::Content_DefaultFixedFontSizeOption).toInt());
		settings->setFontSize(QWebEngineSettings::MinimumFontSize, SettingsManager::getOption(SettingsManager::Content_MinimumFontSizeOption).toInt());
		settings->setFontFamily(QWebEngineSettings::StandardFont, SettingsManager::getOption(SettingsManager::Content_StandardFontOption).toString());
		settings->setFontFamily(QWebEngineSettings::FixedFont, SettingsManager::getOption(SettingsManager::Content_FixedFontOption).toString());
		settings->setFontFamily(QWebEngineSettings::SerifFont, SettingsManager::getOption(SettingsManager::Content_SerifFontOption).toString());
		settings->setFontFamily(QWebEngineSettings::SansSerifFont, SettingsManager::getOption(SettingsManager::Content_SansSerifFontOption).toString());
		settings->setFontFamily(QWebEngineSettings::CursiveFont, SettingsManager::getOption(SettingsManager::Content_CursiveFontOption).toString());
		settings->setFontFamily(QWebEngineSettings::FantasyFont, SettingsManager::getOption(SettingsManager::Content_FantasyFontOption).toString());
	}
	else if (optionName.startsWith(QLatin1String("Permissions/")))
	{
		settings->setAttribute(QWebEngineSettings::AutoLoadImages, (SettingsManager::getOption(SettingsManager::Permissions_EnableImagesOption).toString() != QLatin1String("onlyCached")));
		settings->setAttribute(QWebEngineSettings::PluginsEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnablePluginsOption).toString() != QLatin1String("disabled"));
		settings->setAttribute(QWebEngineSettings::JavascriptEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnableJavaScriptOption).toBool());
		settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanAccessClipboardOption).toBool());
		settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, (SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption).toString() != QLatin1String("blockAll")));
		settings->setAttribute(QWebEngineSettings::WebGLEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnableWebglOption).toBool());
		settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnableLocalStorageOption).toBool());
	}
}

void QtWebEngineWebBackend::showNotification(std::unique_ptr<QWebEngineNotification> nativeNotification)
{
	Notification::Message message;
	message.title = nativeNotification->title();
	message.message = nativeNotification->message();
	message.image = nativeNotification->icon();
	message.event = NotificationsManager::WebPageNotificationEvent;

	const Notification *notification(NotificationsManager::createNotification(message));
	const QUrl origin(nativeNotification->origin());

	if (origin.isValid())
	{
		connect(notification, &Notification::clicked, [=]()
		{
			SessionsManager::hasUrl(origin, true);
		});
	}

	connect(notification, &Notification::shown, nativeNotification.get(), &QWebEngineNotification::show);
	connect(notification, &Notification::clicked, nativeNotification.get(), &QWebEngineNotification::click);
	connect(notification, &Notification::ignored, nativeNotification.get(), &QWebEngineNotification::close);
	connect(nativeNotification.get(), &QWebEngineNotification::closed, notification, &Notification::requestClose);
}

WebWidget* QtWebEngineWebBackend::createWidget(const QVariantMap &parameters, ContentsWidget *parent)
{
	return new QtWebEngineWebWidget(parameters, this, parent);
}

QWebEngineProfile* QtWebEngineWebBackend::getDefaultProfile()
{
	QWebEngineProfile *profile(QWebEngineProfile::defaultProfile());

	if (!m_isInitialized)
	{
		m_isInitialized = true;

		ContentFiltersManager::initialize();

		QWebEngineSettings *settings(QWebEngineSettings::globalSettings());

		profile->setHttpAcceptLanguage(NetworkManagerFactory::getAcceptLanguage());
		profile->setHttpUserAgent(getUserAgent());
		profile->setDownloadPath(SettingsManager::getOption(SettingsManager::Paths_DownloadsOption).toString());
		profile->setNotificationPresenter(&QtWebEngineWebBackend::showNotification);

		settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, SettingsManager::getOption(SettingsManager::Network_EnableDnsPrefetchOption).toBool());
		settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
		settings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, false);
		settings->setAttribute(QWebEngineSettings::XSSAuditingEnabled, true);
		settings->setAttribute(QWebEngineSettings::PrintElementBackgrounds, SettingsManager::getOption(SettingsManager::Browser_PrintElementBackgroundsOption).toBool());
		settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, SettingsManager::getOption(SettingsManager::Interface_EnableSmoothScrollingOption).toBool());

		const QString cachePath(SessionsManager::getCachePath());

		if (cachePath.isEmpty())
		{
			profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
		}
		else
		{
			Utils::ensureDirectoryExists(cachePath);

			profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
			profile->setHttpCacheMaximumSize(SettingsManager::getOption(SettingsManager::Cache_DiskCacheLimitOption).toInt() * 1024);
			profile->setCachePath(cachePath);
			profile->setPersistentStoragePath(cachePath + QLatin1String("/persistentStorage/"));
		}

		handleOptionChanged(SettingsManager::Content_DefaultFontSizeOption);
		handleOptionChanged(SettingsManager::Permissions_EnableFullScreenOption);

		connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &QtWebEngineWebBackend::handleOptionChanged);
		connect(profile, &QWebEngineProfile::downloadRequested, this, &QtWebEngineWebBackend::handleDownloadRequested);
	}

	return profile;
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

		const QString userAgent(Utils::substitutePlaceholders(pattern, m_userAgentComponents));

		m_userAgents[pattern] = ((pattern == userAgent) ? QString() : userAgent);

		return userAgent;
	}

	const QString userAgent(NetworkManagerFactory::getUserAgent(SettingsManager::getOption(SettingsManager::Network_UserAgentOption).toString()).value);

	return ((userAgent.isEmpty()) ? QString() : getUserAgent(userAgent));
}

QUrl QtWebEngineWebBackend::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

WebBackend::CapabilityScopes QtWebEngineWebBackend::getCapabilityScopes(WebBackend::BackendCapability capability) const
{
	switch (capability)
	{
		case CookiesPolicyCapability:
		case ContentFilteringCapability:
		case ProxyCapability:
		case UserScriptsCapability:
			return GlobalScope;
		case DoNotTrackCapability:
		case ReferrerCapability:
		case UserAgentCapability:
			return AllScopes;
		default:
			break;
	}

	return NoScope;
}

bool QtWebEngineWebBackend::hasSslSupport() const
{
	return true;
}

}
