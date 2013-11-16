#include "WebBackendWebKit.h"
#include "WebWidgetWebKit.h"
#include "../../../core/SettingsManager.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtWebKit/QWebSettings>

namespace Otter
{

WebBackendWebKit::WebBackendWebKit(QObject *parent) : WebBackend(parent)
{
	const QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

	QDir().mkpath(cachePath);

	QWebSettings *globalSettings = QWebSettings::globalSettings();
	globalSettings->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
	globalSettings->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	globalSettings->setAttribute(QWebSettings::PluginsEnabled, SettingsManager::getValue("Browser/EnablePlugins").toBool());
	globalSettings->setAttribute(QWebSettings::JavaEnabled, SettingsManager::getValue("Browser/EnableJava").toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptEnabled, SettingsManager::getValue("Browser/EnableJavaScript").toBool());
	globalSettings->setIconDatabasePath(cachePath);
	globalSettings->setOfflineStoragePath(cachePath);
}

WebWidget *WebBackendWebKit::createWidget(QWidget *parent)
{
	return new WebWidgetWebKit(parent);
}

QString WebBackendWebKit::getTitle() const
{
	return tr("WebKit Backend");
}

QString WebBackendWebKit::getDescription() const
{
	return tr("Backend utilizing QtWebKit module");
}

QIcon WebBackendWebKit::getIconForUrl(const QUrl &url)
{
	return QWebSettings::iconForUrl(url);
}

}
