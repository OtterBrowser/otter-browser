/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "QtWebKitWebBackend.h"
#include "QtWebKitHistoryInterface.h"
#include "QtWebKitPage.h"
#include "QtWebKitWebWidget.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/Utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtWebKit/QWebHistoryInterface>
#include <QtWebKit/QWebSettings>

namespace Otter
{

QtWebKitWebBackend* QtWebKitWebBackend::m_instance = NULL;
QPointer<WebWidget> QtWebKitWebBackend::m_activeWidget = NULL;
QMap<QString, QString> QtWebKitWebBackend::m_userAgentComponents;
QMap<QString, QString> QtWebKitWebBackend::m_userAgents;

QtWebKitWebBackend::QtWebKitWebBackend(QObject *parent) : WebBackend(parent),
	m_isInitialized(false)
{
	m_instance = this;

	QtWebKitPage *page(new QtWebKitPage());
	QRegularExpression platformExpression(QLatin1String("(\\([^\\)]+\\))"));

	m_userAgentComponents[QLatin1String("platform")] = platformExpression.match(page->getDefaultUserAgent()).captured(1);
	m_userAgentComponents[QLatin1String("engineVersion")] = QLatin1String("AppleWebKit/") + qWebKitVersion() + QLatin1String(" (KHTML, like Gecko)");
	m_userAgentComponents[QLatin1String("applicationVersion")] = QCoreApplication::applicationName() + QLatin1Char('/') + QCoreApplication::applicationVersion();

	page->deleteLater();
}

QtWebKitWebBackend::~QtWebKitWebBackend()
{
	qDeleteAll(m_thumbnailRequests.keys());

	m_thumbnailRequests.clear();
}

void QtWebKitWebBackend::optionChanged(const QString &option)
{
	if (option == QLatin1String("Cache/PagesInMemoryLimit"))
	{
		QWebSettings::setMaximumPagesInCache(SettingsManager::getValue(QLatin1String("Cache/PagesInMemoryLimit")).toInt());
	}

	if (!(option.startsWith(QLatin1String("Browser/")) || option.startsWith(QLatin1String("Content/"))))
	{
		return;
	}

	QWebSettings *globalSettings(QWebSettings::globalSettings());
	globalSettings->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
	globalSettings->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	globalSettings->setAttribute(QWebSettings::AutoLoadImages, (SettingsManager::getValue(QLatin1String("Browser/EnableImages")).toString() != QLatin1String("onlyCached")));
	globalSettings->setAttribute(QWebSettings::PluginsEnabled, SettingsManager::getValue(QLatin1String("Browser/EnablePlugins")).toString() != QLatin1String("disabled"));
	globalSettings->setAttribute(QWebSettings::JavaEnabled, SettingsManager::getValue(QLatin1String("Browser/EnableJava")).toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptEnabled, SettingsManager::getValue(QLatin1String("Browser/EnableJavaScript")).toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanAccessClipboard")).toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptCanCloseWindows, true);
	globalSettings->setAttribute(QWebSettings::JavascriptCanOpenWindows, SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanOpenWindows")).toBool());
	globalSettings->setAttribute(QWebSettings::WebGLEnabled, SettingsManager::getValue(QLatin1String("Browser/EnableWebgl")).toBool());
	globalSettings->setAttribute(QWebSettings::LocalStorageEnabled, SettingsManager::getValue(QLatin1String("Browser/EnableLocalStorage")).toBool());
	globalSettings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, SettingsManager::getValue(QLatin1String("Browser/EnableOfflineStorageDatabase")).toBool());
	globalSettings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, SettingsManager::getValue(QLatin1String("Browser/EnableOfflineWebApplicationCache")).toBool());
	globalSettings->setAttribute(QWebSettings::ZoomTextOnly, SettingsManager::getValue(QLatin1String("Content/ZoomTextOnly")).toBool());
	globalSettings->setFontSize(QWebSettings::DefaultFontSize, SettingsManager::getValue(QLatin1String("Content/DefaultFontSize")).toInt());
	globalSettings->setFontSize(QWebSettings::DefaultFixedFontSize, SettingsManager::getValue(QLatin1String("Content/DefaultFixedFontSize")).toInt());
	globalSettings->setFontSize(QWebSettings::MinimumFontSize, SettingsManager::getValue(QLatin1String("Content/MinimumFontSize")).toInt());
	globalSettings->setFontFamily(QWebSettings::StandardFont, SettingsManager::getValue(QLatin1String("Content/StandardFont")).toString());
	globalSettings->setFontFamily(QWebSettings::FixedFont, SettingsManager::getValue(QLatin1String("Content/FixedFont")).toString());
	globalSettings->setFontFamily(QWebSettings::SerifFont, SettingsManager::getValue(QLatin1String("Content/SerifFont")).toString());
	globalSettings->setFontFamily(QWebSettings::SansSerifFont, SettingsManager::getValue(QLatin1String("Content/SansSerifFont")).toString());
	globalSettings->setFontFamily(QWebSettings::CursiveFont, SettingsManager::getValue(QLatin1String("Content/CursiveFont")).toString());
	globalSettings->setFontFamily(QWebSettings::FantasyFont, SettingsManager::getValue(QLatin1String("Content/FantasyFont")).toString());
	globalSettings->setOfflineStorageDefaultQuota(SettingsManager::getValue(QLatin1String("Browser/OfflineStorageLimit")).toInt() * 1024);
	globalSettings->setOfflineWebApplicationCacheQuota(SettingsManager::getValue(QLatin1String("Content/OfflineWebApplicationCacheLimit")).toInt() * 1024);
}

void QtWebKitWebBackend::pageLoaded(bool success)
{
	QtWebKitPage *page(qobject_cast<QtWebKitPage*>(sender()));

	if (!page)
	{
		return;
	}

	if (success)
	{
		QPixmap pixmap;

		if (!m_thumbnailRequests[page].second.isEmpty())
		{
			QSize contentsSize(page->mainFrame()->contentsSize());

			page->setViewportSize(contentsSize);

			if (contentsSize.width() > 2000)
			{
				contentsSize.setWidth(2000);
			}

			contentsSize.setHeight(m_thumbnailRequests[page].second.height() * (qreal(contentsSize.width()) / m_thumbnailRequests[page].second.width()));

			pixmap = QPixmap(contentsSize);
			pixmap.fill(Qt::white);

			QPainter painter(&pixmap);

			page->mainFrame()->render(&painter, QWebFrame::ContentsLayer, QRegion(QRect(QPoint(0, 0), contentsSize)));

			painter.end();

			pixmap = pixmap.scaled(m_thumbnailRequests[page].second, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}

		emit thumbnailAvailable(m_thumbnailRequests[page].first, pixmap, page->mainFrame()->title());
	}
	else
	{
		emit thumbnailAvailable(m_thumbnailRequests[page].first, QPixmap(), QString());
	}

	m_thumbnailRequests.remove(page);

	page->deleteLater();
}

void QtWebKitWebBackend::setActiveWidget(WebWidget *widget)
{
	m_activeWidget = widget;

	emit activeDictionaryChanged(getActiveDictionary());
}

WebWidget* QtWebKitWebBackend::createWidget(bool isPrivate, ContentsWidget *parent)
{
	if (!m_isInitialized)
	{
		m_isInitialized = true;

		QWebHistoryInterface::setDefaultInterface(new QtWebKitHistoryInterface(this));

		QWebSettings *globalSettings(QWebSettings::globalSettings());
		globalSettings->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
		globalSettings->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

		const QString cachePath(SessionsManager::getCachePath());

		if (!cachePath.isEmpty())
		{
			QDir().mkpath(cachePath);

			globalSettings->setIconDatabasePath(cachePath);
			globalSettings->setLocalStoragePath(cachePath + QLatin1String("/localStorage/"));
			globalSettings->setOfflineStoragePath(cachePath + QLatin1String("/offlineStorage/"));
			globalSettings->setOfflineWebApplicationCachePath(cachePath + QLatin1String("/offlineWebApplicationCache/"));
		}

		QWebSettings::setMaximumPagesInCache(SettingsManager::getValue(QLatin1String("Cache/PagesInMemoryLimit")).toInt());

		optionChanged(QLatin1String("Browser/"));

		connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
	}

	QtWebKitWebWidget *widget(new QtWebKitWebWidget(isPrivate, this, NULL, parent));

	connect(widget, SIGNAL(widgetActivated(WebWidget*)), this, SLOT(setActiveWidget(WebWidget*)));

	return widget;
}

QtWebKitWebBackend* QtWebKitWebBackend::getInstance()
{
	return m_instance;
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

		QString userAgent(pattern);
		QMap<QString, QString>::iterator iterator;

		for (iterator = m_userAgentComponents.begin(); iterator != m_userAgentComponents.end(); ++iterator)
		{
			userAgent = userAgent.replace(QStringLiteral("{%1}").arg(iterator.key()), iterator.value());
		}

		m_userAgents[pattern] = ((pattern == userAgent) ? QString() : userAgent);

		return userAgent;
	}

	const UserAgentInformation userAgent = NetworkManagerFactory::getUserAgent(SettingsManager::getValue(QLatin1String("Network/UserAgent")).toString());

	return ((userAgent.value.isEmpty()) ? QString() : getUserAgent(userAgent.value));
}

QUrl QtWebKitWebBackend::getHomePage() const
{
	return QUrl(QLatin1String("http://otter-browser.org/"));
}

QIcon QtWebKitWebBackend::getIcon() const
{
	return QIcon();
}

QString QtWebKitWebBackend::getActiveDictionary()
{
	if (m_activeWidget && m_activeWidget->getOption(QLatin1String("Browser/EnableSpellCheck"), m_activeWidget->getUrl()).toBool())
	{
		const QString dictionary(m_activeWidget->getOption(QLatin1String("Browser/SpellCheckDictionary"), m_activeWidget->getUrl()).toString());

		return (dictionary.isEmpty() ? SpellCheckManager::getDefaultDictionary() : dictionary);
	}

	return QString();
}

QList<SpellCheckManager::DictionaryInformation> QtWebKitWebBackend::getDictionaries() const
{
	return SpellCheckManager::getDictionaries();
}

bool QtWebKitWebBackend::requestThumbnail(const QUrl &url, const QSize &size)
{
	QtWebKitPage *page(new QtWebKitPage());

	m_thumbnailRequests[page] = qMakePair(url, size);

	connect(page, SIGNAL(loadFinished(bool)), this, SLOT(pageLoaded(bool)));

	page->setParent(this);
	page->settings()->setAttribute(QWebSettings::JavaEnabled, false);
	page->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
	page->settings()->setAttribute(QWebSettings::PluginsEnabled, false);
	page->mainFrame()->setUrl(url);

	return true;
}

}
