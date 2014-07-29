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

#include "QtWebKitWebBackend.h"
#include "QtWebKitWebWidget.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/Utils.h"

#include <QtCore/QDir>
#include <QtWebKit/QWebSettings>
#include <QtWebKitWidgets/QWebPage>

namespace Otter
{

QtWebKitWebBackend::QtWebKitWebBackend(QObject *parent) : WebBackend(parent),
	m_isInitialized(false)
{
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

	QWebSettings *globalSettings = QWebSettings::globalSettings();
	globalSettings->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
	globalSettings->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	globalSettings->setAttribute(QWebSettings::AutoLoadImages, SettingsManager::getValue(QLatin1String("Browser/EnableImages")).toBool());
	globalSettings->setAttribute(QWebSettings::PluginsEnabled, SettingsManager::getValue(QLatin1String("Browser/EnablePlugins")).toBool());
	globalSettings->setAttribute(QWebSettings::JavaEnabled, SettingsManager::getValue(QLatin1String("Browser/EnableJava")).toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptEnabled, SettingsManager::getValue(QLatin1String("Browser/EnableJavaScript")).toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, SettingsManager::getValue(QLatin1String("Browser/JavaSriptCanAccessClipboard")).toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptCanCloseWindows, SettingsManager::getValue(QLatin1String("Browser/JavaSriptCanCloseWindows")).toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptCanOpenWindows, SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanOpenWindows")).toBool());
	globalSettings->setAttribute(QWebSettings::LocalStorageEnabled, SettingsManager::getValue(QLatin1String("Browser/EnableLocalStorage")).toBool());
	globalSettings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, SettingsManager::getValue(QLatin1String("Browser/EnableOfflineStorageDatabase")).toBool());
	globalSettings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, SettingsManager::getValue(QLatin1String("Browser/EnableOfflineWebApplicationCache")).toBool());
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

WebWidget* QtWebKitWebBackend::createWidget(bool isPrivate, ContentsWidget *parent)
{
	if (!m_isInitialized)
	{
		m_isInitialized = true;

		QWebSettings *globalSettings = QWebSettings::globalSettings();
		globalSettings->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
		globalSettings->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

		const QString cachePath = SessionsManager::getCachePath();

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

	return new QtWebKitWebWidget(isPrivate, this, parent);
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

QIcon QtWebKitWebBackend::getIconForUrl(const QUrl &url)
{
	const QIcon icon = QWebSettings::iconForUrl(url);

	return (icon.isNull() ? Utils::getIcon(QLatin1String("text-html")) : icon);
}

}
