/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "QtWebKitWebBackend.h"
#include "QtWebKitHistoryInterface.h"
#include "QtWebKitPage.h"
#include "QtWebKitWebWidget.h"
#include "../../../../core/Application.h"
#include "../../../../core/BookmarksManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/SettingsManager.h"

#include <QtCore/QDir>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtWebKit/QWebHistoryInterface>
#include <QtWebKit/QWebSettings>
#include <QtWebKitWidgets/QWebFrame>

namespace Otter
{

QtWebKitWebBackend* QtWebKitWebBackend::m_instance(nullptr);
QPointer<WebWidget> QtWebKitWebBackend::m_activeWidget(nullptr);
QHash<QString, QString> QtWebKitWebBackend::m_userAgentComponents;
QMap<QString, QString> QtWebKitWebBackend::m_userAgents;
int QtWebKitWebBackend::m_enableMediaOption(-1);
int QtWebKitWebBackend::m_enableMediaSourceOption(-1);
int QtWebKitWebBackend::m_enableSiteSpecificQuirksOption(-1);
int QtWebKitWebBackend::m_enableWebSecurityOption(-1);

QtWebKitWebBackend::QtWebKitWebBackend(QObject *parent) : WebBackend(parent),
	m_isInitialized(false)
{
	m_instance = this;
	m_enableMediaOption = SettingsManager::registerOption(QLatin1String("QtWebKitBackend/EnableMedia"), SettingsManager::BooleanType, true);
	m_enableMediaSourceOption = SettingsManager::registerOption(QLatin1String("QtWebKitBackend/EnableMediaSource"), SettingsManager::BooleanType, false);
	m_enableSiteSpecificQuirksOption = SettingsManager::registerOption(QLatin1String("QtWebKitBackend/EnableSiteSpecificQuirks"), SettingsManager::BooleanType, true);
	m_enableWebSecurityOption = SettingsManager::registerOption(QLatin1String("QtWebKitBackend/EnableWebSecurity"), SettingsManager::BooleanType, true);

	const QString cachePath(SessionsManager::getCachePath());

	if (!cachePath.isEmpty())
	{
		QDir().mkpath(cachePath);

		QWebSettings::setIconDatabasePath(cachePath);
		QWebSettings::setOfflineStoragePath(cachePath + QLatin1String("/offlineStorage/"));
		QWebSettings::setOfflineWebApplicationCachePath(cachePath + QLatin1String("/offlineWebApplicationCache/"));
		QWebSettings::globalSettings()->setLocalStoragePath(cachePath + QLatin1String("/localStorage/"));
	}

	QtWebKitPage *page(new QtWebKitPage());

	m_userAgentComponents = {{QLatin1String("platform"), QRegularExpression(QLatin1String(R"((\([^\)]+\)))")).match(page->userAgentForUrl({})).captured(1)}, {QLatin1String("engineVersion"), QLatin1String("AppleWebKit/") + qWebKitVersion() + QLatin1String(" (KHTML, like Gecko)")}, {QLatin1String("applicationVersion"), QCoreApplication::applicationName() + QLatin1Char('/') + QCoreApplication::applicationVersion()}};

	page->deleteLater();
}

void QtWebKitWebBackend::handleOptionChanged(int identifier)
{
	QWebSettings *settings(QWebSettings::globalSettings());

	switch (identifier)
	{
		case SettingsManager::Browser_OfflineStorageLimitOption:
			QWebSettings::setOfflineStorageDefaultQuota(SettingsManager::getOption(SettingsManager::Browser_OfflineStorageLimitOption).toInt() * 1024);

			return;
		case SettingsManager::Browser_OfflineWebApplicationCacheLimitOption:
			QWebSettings::setOfflineWebApplicationCacheQuota(SettingsManager::getOption(SettingsManager::Browser_OfflineWebApplicationCacheLimitOption).toInt() * 1024);

			return;
		case SettingsManager::Browser_PrintElementBackgroundsOption:
			settings->setAttribute(QWebSettings::PrintElementBackgrounds, SettingsManager::getOption(SettingsManager::Browser_PrintElementBackgroundsOption).toBool());

			return;
		case SettingsManager::Cache_PagesInMemoryLimitOption:
			QWebSettings::setMaximumPagesInCache(SettingsManager::getOption(SettingsManager::Cache_PagesInMemoryLimitOption).toInt());

			return;
		case SettingsManager::Interface_EnableSmoothScrollingOption:
			settings->setAttribute(QWebSettings::ScrollAnimatorEnabled, SettingsManager::getOption(SettingsManager::Interface_EnableSmoothScrollingOption).toBool());

			return;

		case SettingsManager::Network_EnableDnsPrefetchOption:
			settings->setAttribute(QWebSettings::DnsPrefetchEnabled, SettingsManager::getOption(SettingsManager::Network_EnableDnsPrefetchOption).toBool());

			return;
		default:
			break;
	}

	const QString optionName(SettingsManager::getOptionName(identifier));

	if (optionName.startsWith(QLatin1String("Content/")))
	{
		settings->setAttribute(QWebSettings::ZoomTextOnly, SettingsManager::getOption(SettingsManager::Content_ZoomTextOnlyOption).toBool());
		settings->setFontSize(QWebSettings::DefaultFontSize, SettingsManager::getOption(SettingsManager::Content_DefaultFontSizeOption).toInt());
		settings->setFontSize(QWebSettings::DefaultFixedFontSize, SettingsManager::getOption(SettingsManager::Content_DefaultFixedFontSizeOption).toInt());
		settings->setFontSize(QWebSettings::MinimumFontSize, SettingsManager::getOption(SettingsManager::Content_MinimumFontSizeOption).toInt());
		settings->setFontFamily(QWebSettings::StandardFont, SettingsManager::getOption(SettingsManager::Content_StandardFontOption).toString());
		settings->setFontFamily(QWebSettings::FixedFont, SettingsManager::getOption(SettingsManager::Content_FixedFontOption).toString());
		settings->setFontFamily(QWebSettings::SerifFont, SettingsManager::getOption(SettingsManager::Content_SerifFontOption).toString());
		settings->setFontFamily(QWebSettings::SansSerifFont, SettingsManager::getOption(SettingsManager::Content_SansSerifFontOption).toString());
		settings->setFontFamily(QWebSettings::CursiveFont, SettingsManager::getOption(SettingsManager::Content_CursiveFontOption).toString());
		settings->setFontFamily(QWebSettings::FantasyFont, SettingsManager::getOption(SettingsManager::Content_FantasyFontOption).toString());
	}
	else if (optionName.startsWith(QLatin1String("Permissions/")))
	{
#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
		const bool arePluginsEnabled(SettingsManager::getOption(SettingsManager::Permissions_EnablePluginsOption).toString() != QLatin1String("disabled"));

		settings->setAttribute(QWebSettings::PluginsEnabled, arePluginsEnabled);
		settings->setAttribute(QWebSettings::JavaEnabled, arePluginsEnabled);
#endif
		settings->setAttribute(QWebSettings::AutoLoadImages, (SettingsManager::getOption(SettingsManager::Permissions_EnableImagesOption).toString() != QLatin1String("onlyCached")));
		settings->setAttribute(QWebSettings::JavascriptEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnableJavaScriptOption).toBool());
		settings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanAccessClipboardOption).toBool());
		settings->setAttribute(QWebSettings::JavascriptCanOpenWindows, (SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption).toString() != QLatin1String("blockAll")));
		settings->setAttribute(QWebSettings::WebGLEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnableWebglOption).toBool());
		settings->setAttribute(QWebSettings::LocalStorageEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnableLocalStorageOption).toBool());
		settings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnableOfflineStorageDatabaseOption).toBool());
		settings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, SettingsManager::getOption(SettingsManager::Permissions_EnableOfflineWebApplicationCacheOption).toBool());
	}
}

WebWidget* QtWebKitWebBackend::createWidget(const QVariantMap &parameters, ContentsWidget *parent)
{
	if (!m_isInitialized)
	{
		m_isInitialized = true;

		QWebHistoryInterface::setDefaultInterface(new QtWebKitHistoryInterface(this));

		QWebSettings *settings(QWebSettings::globalSettings());
		settings->setAttribute(QWebSettings::DnsPrefetchEnabled, SettingsManager::getOption(SettingsManager::Network_EnableDnsPrefetchOption).toBool());
		settings->setAttribute(QWebSettings::FullScreenSupportEnabled, true);
		settings->setAttribute(QWebSettings::XSSAuditingEnabled, true);
		settings->setAttribute(QWebSettings::PrintElementBackgrounds, SettingsManager::getOption(SettingsManager::Browser_PrintElementBackgroundsOption).toBool());
		settings->setAttribute(QWebSettings::ScrollAnimatorEnabled, SettingsManager::getOption(SettingsManager::Interface_EnableSmoothScrollingOption).toBool());

#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
		QStringList pluginSearchPaths(QWebSettings::pluginSearchPaths());
		pluginSearchPaths.append(QDir::toNativeSeparators(Application::getApplicationDirectoryPath()));

		QWebSettings::setPluginSearchPaths(pluginSearchPaths);
#endif
		QWebSettings::setMaximumPagesInCache(SettingsManager::getOption(SettingsManager::Cache_PagesInMemoryLimitOption).toInt());
		QWebSettings::setOfflineStorageDefaultQuota(SettingsManager::getOption(SettingsManager::Browser_OfflineStorageLimitOption).toInt() * 1024);
		QWebSettings::setOfflineWebApplicationCacheQuota(SettingsManager::getOption(SettingsManager::Browser_OfflineWebApplicationCacheLimitOption).toInt() * 1024);

		handleOptionChanged(SettingsManager::Content_DefaultFontSizeOption);
		handleOptionChanged(SettingsManager::Permissions_EnableFullScreenOption);

		connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &QtWebKitWebBackend::handleOptionChanged);
	}

	QtWebKitWebWidget *widget(new QtWebKitWebWidget(parameters, this, nullptr, parent));

	connect(widget, &QtWebKitWebWidget::widgetActivated, this, [&](WebWidget *widget)
	{
		m_activeWidget = widget;

		emit activeDictionaryChanged(getActiveDictionary());
	});

	return widget;
}

BookmarksImportJob* QtWebKitWebBackend::createBookmarksImportJob(BookmarksModel::Bookmark *folder, const QString &path, bool areDuplicatesAllowed)
{
	return new QtWebKitBookmarksImportJob(folder, path, areDuplicatesAllowed, this);
}

WebPageThumbnailJob* QtWebKitWebBackend::createPageThumbnailJob(const QUrl &url, const QSize &size)
{
	return new QtWebKitWebPageThumbnailJob(url, size, this);
}

QtWebKitWebBackend* QtWebKitWebBackend::getInstance()
{
	return m_instance;
}

QString QtWebKitWebBackend::getName() const
{
	return QLatin1String("qtwebkit");
}

QString QtWebKitWebBackend::getTitle() const
{
	return tr("WebKit Backend");
}

QString QtWebKitWebBackend::getDescription() const
{
	return tr("Backend utilizing QtWebKit module");
}

QString QtWebKitWebBackend::getVersion() const
{
	return QCoreApplication::applicationVersion();
}

QString QtWebKitWebBackend::getEngineVersion() const
{
	return qWebKitVersion();
}

QString QtWebKitWebBackend::getSslVersion() const
{
	return (QSslSocket::supportsSsl() ? QSslSocket::sslLibraryVersionString() : QString());
}

QString QtWebKitWebBackend::getUserAgent(const QString &pattern) const
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

	return (userAgent.isEmpty() ? QString() : getUserAgent(userAgent));
}

QUrl QtWebKitWebBackend::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QString QtWebKitWebBackend::getActiveDictionary()
{
	if (m_activeWidget && m_activeWidget->getOption(SettingsManager::Browser_EnableSpellCheckOption, m_activeWidget->getUrl()).toBool())
	{
		const QString dictionary(m_activeWidget->getOption(SettingsManager::Browser_SpellCheckDictionaryOption, m_activeWidget->getUrl()).toString());

		return (dictionary.isEmpty() ? SpellCheckManager::getDefaultDictionary() : dictionary);
	}

	return {};
}

WebBackend::CapabilityScopes QtWebKitWebBackend::getCapabilityScopes(WebBackend::BackendCapability capability) const
{
	switch (capability)
	{
		case BookmarksImportCapability:
		case CacheManagementCapability:
		case CookiesManagementCapability:
		case PasswordsManagementCapability:
			return GlobalScope;
		case HistoryMetaDataCapability:
			return TabScope;
		case CookiesPolicyCapability:
		case ContentFilteringCapability:
		case DoNotTrackCapability:
		case FindInPageHighlightAllCapability:
#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
		case PluginsOnDemandCapability:
#endif
		case ProxyCapability:
		case ReferrerCapability:
		case UserAgentCapability:
		case UserScriptsCapability:
		case UserStyleSheetsCapability:
			return AllScopes;
		default:
			break;
	}

	return NoScope;
}

int QtWebKitWebBackend::getOptionIdentifier(QtWebKitWebBackend::OptionIdentifier identifier)
{
	switch (identifier)
	{
		case QtWebKitBackend_EnableMediaOption:
			return m_enableMediaOption;
		case QtWebKitBackend_EnableMediaSourceOption:
			return m_enableMediaSourceOption;
		case QtWebKitBackend_EnableSiteSpecificQuirksOption:
			return m_enableSiteSpecificQuirksOption;
		case QtWebKitBackend_EnableWebSecurityOption:
			return m_enableWebSecurityOption;
	}

	return -1;
}

bool QtWebKitWebBackend::hasSslSupport() const
{
	return QSslSocket::supportsSsl();
}

QtWebKitBookmarksImportJob::QtWebKitBookmarksImportJob(BookmarksModel::Bookmark *folder, const QString &path, bool areDuplicatesAllowed, QObject *parent) : BookmarksImportJob(folder, areDuplicatesAllowed, parent),
	m_path(path),
	m_currentAmount(0),
	m_totalAmount(-1),
	m_isRunning(false)
{
}

void QtWebKitBookmarksImportJob::processElement(const QWebElement &element)
{
	QWebElement entryElement(element.findFirst(QLatin1String("dt, hr")));

	while (!entryElement.isNull())
	{
		if (entryElement.tagName().toLower() == QLatin1String("hr"))
		{
			BookmarksManager::addBookmark(BookmarksModel::SeparatorBookmark, {}, getCurrentFolder());

			++m_currentAmount;

			emit importProgress(DataExchanger::BookmarksExchange, m_totalAmount, m_currentAmount);
		}
		else
		{
			BookmarksModel::BookmarkType type(BookmarksModel::UnknownBookmark);
			QWebElement matchedElement(entryElement.findFirst(QLatin1String("dt > h3")));

			if (matchedElement.isNull())
			{
				matchedElement = entryElement.findFirst(QLatin1String("dt > a"));

				if (!matchedElement.isNull())
				{
					type = (matchedElement.hasAttribute(QLatin1String("FEEDURL")) ? BookmarksModel::FeedBookmark : BookmarksModel::UrlBookmark);
				}
			}
			else
			{
				type = BookmarksModel::FolderBookmark;
			}

			if (type != BookmarksModel::UnknownBookmark && !matchedElement.isNull())
			{
				QMap<int, QVariant> metaData({{BookmarksModel::TitleRole, matchedElement.toPlainText()}});
				const bool isUrlBookmark(type == BookmarksModel::UrlBookmark || type == BookmarksModel::FeedBookmark);

				if (isUrlBookmark)
				{
					const QUrl url(matchedElement.attribute(QLatin1String("HREF")));

					if (!areDuplicatesAllowed() && BookmarksManager::hasBookmark(url))
					{
						entryElement = entryElement.nextSibling();

						continue;
					}

					metaData[BookmarksModel::UrlRole] = url;
				}

				if (matchedElement.hasAttribute(QLatin1String("SHORTCUTURL")))
				{
					const QString keyword(matchedElement.attribute(QLatin1String("SHORTCUTURL")));

					if (!keyword.isEmpty() && !BookmarksManager::hasKeyword(keyword))
					{
						metaData[BookmarksModel::KeywordRole] = keyword;
					}
				}

				if (matchedElement.hasAttribute(QLatin1String("ADD_DATE")))
				{
					const QDateTime dateTime(getDateTime(matchedElement.attribute(QLatin1String("ADD_DATE"))));

					if (dateTime.isValid())
					{
						metaData[BookmarksModel::TimeAddedRole] = dateTime;
						metaData[BookmarksModel::TimeModifiedRole] = dateTime;
					}
				}

				if (matchedElement.hasAttribute(QLatin1String("LAST_MODIFIED")))
				{
					const QDateTime dateTime(getDateTime(matchedElement.attribute(QLatin1String("LAST_MODIFIED"))));

					if (dateTime.isValid())
					{
						metaData[BookmarksModel::TimeModifiedRole] = dateTime;
					}
				}

				if (isUrlBookmark && matchedElement.hasAttribute(QLatin1String("LAST_VISITED")))
				{
					const QDateTime dateTime(getDateTime(matchedElement.attribute(QLatin1String("LAST_VISITED"))));

					if (dateTime.isValid())
					{
						metaData[BookmarksModel::TimeVisitedRole] = dateTime;
					}
				}

				BookmarksModel::Bookmark *bookmark(BookmarksManager::addBookmark(type, metaData, getCurrentFolder()));

				++m_currentAmount;

				emit importProgress(DataExchanger::BookmarksExchange, m_totalAmount, m_currentAmount);

				if (type == BookmarksModel::FolderBookmark)
				{
					setCurrentFolder(bookmark);
					processElement(entryElement);
				}

				if (entryElement.nextSibling().tagName().toLower() == QLatin1String("dd"))
				{
					bookmark->setItemData(entryElement.nextSibling().toPlainText(), BookmarksModel::DescriptionRole);

					entryElement = entryElement.nextSibling();
				}
			}
		}

		entryElement = entryElement.nextSibling();
	}

	goToParent();
}

void QtWebKitBookmarksImportJob::start()
{
	QFile file(m_path);

	if (!file.open(QIODevice::ReadOnly))
	{
		emit importFinished(DataExchanger::BookmarksExchange, DataExchanger::FailedOperation, 0);
		emit jobFinished(false);

		deleteLater();

		return;
	}

	m_isRunning = true;

	QWebPage page;
	page.settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
	page.mainFrame()->setHtml(QString::fromLatin1(file.readAll()));

	m_totalAmount = page.mainFrame()->findAllElements(QLatin1String("dt, hr")).count();

	emit importStarted(DataExchanger::BookmarksExchange, m_totalAmount);

	BookmarksManager::getModel()->beginImport(getImportFolder(), page.mainFrame()->findAllElements(QLatin1String("a[href]")).count(), page.mainFrame()->findAllElements(QLatin1String("a[shortcuturl]")).count());

	processElement(page.mainFrame()->documentElement().findFirst(QLatin1String("dl")));

	BookmarksManager::getModel()->endImport();

	emit importFinished(DataExchanger::BookmarksExchange, DataExchanger::SuccessfullOperation, m_totalAmount);
	emit jobFinished(true);

	file.close();

	m_isRunning = false;

	deleteLater();
}

void QtWebKitBookmarksImportJob::cancel()
{
}

bool QtWebKitBookmarksImportJob::isRunning() const
{
	return m_isRunning;
}

QtWebKitWebPageThumbnailJob::QtWebKitWebPageThumbnailJob(const QUrl &url, const QSize &size, QObject *parent) : WebPageThumbnailJob(url, size, parent),
	m_page(nullptr),
	m_url(url),
	m_size(size)
{
}

void QtWebKitWebPageThumbnailJob::start()
{
	if (!m_page)
	{
		m_page = new QtWebKitPage(m_url);
		m_page->setParent(this);

		connect(m_page, &QtWebKitPage::loadFinished, this, &QtWebKitWebPageThumbnailJob::handlePageLoadFinished);
	}
}

void QtWebKitWebPageThumbnailJob::cancel()
{
	if (m_page)
	{
		m_page->triggerAction(QWebPage::Stop);
	}

	deleteLater();
}

void QtWebKitWebPageThumbnailJob::handlePageLoadFinished(bool result)
{
	if (!result)
	{
		deleteLater();

		emit jobFinished(false);

		return;
	}

	m_title = m_page->mainFrame()->title();

	QSize contentsSize(m_page->mainFrame()->contentsSize());

	if (contentsSize.isNull())
	{
		m_page->setViewportSize({1280, 760});

		contentsSize = m_page->mainFrame()->contentsSize();
	}

	if (m_size.isNull() || contentsSize.isNull())
	{
		return;
	}

	if (contentsSize.width() < m_size.width())
	{
		contentsSize.setWidth(m_size.width());
	}
	else if (contentsSize.width() > 2000)
	{
		contentsSize.setWidth(2000);
	}

	contentsSize.setHeight(qRound(m_size.height() * (static_cast<qreal>(contentsSize.width()) / m_size.width())));

	if (contentsSize.isNull())
	{
		deleteLater();

		emit jobFinished(true);
	}
	else
	{
		m_page->setViewportSize(contentsSize);

		QTimer::singleShot(1000, this, [=]()
		{
			m_pixmap = QPixmap(contentsSize);
			m_pixmap.fill(Qt::white);

			QPainter painter(&m_pixmap);

			m_page->mainFrame()->render(&painter, QWebFrame::ContentsLayer, QRegion({{0, 0}, contentsSize}));

			painter.end();

			if (m_pixmap.size() != m_size)
			{
				m_pixmap = m_pixmap.scaled(m_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			}

			deleteLater();

			emit jobFinished(true);
		});
	}
}

QString QtWebKitWebPageThumbnailJob::getTitle() const
{
	return m_title;
}

QPixmap QtWebKitWebPageThumbnailJob::getThumbnail() const
{
	return m_pixmap;
}

bool QtWebKitWebPageThumbnailJob::isRunning() const
{
	return (m_page != nullptr);
}

}
