#include "QtWebKitWebBackend.h"
#include "QtWebKitWebWidget.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/Utils.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtWebKit/QWebSettings>

namespace Otter
{

QtWebKitWebBackend::QtWebKitWebBackend(QObject *parent) : WebBackend(parent)
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

WebWidget *QtWebKitWebBackend::createWidget(bool privateWindow, ContentsWidget *parent)
{
	return new QtWebKitWebWidget(privateWindow, parent);
}

QString QtWebKitWebBackend::getTitle() const
{
	return tr("WebKit Backend");
}

QString QtWebKitWebBackend::getDescription() const
{
	return tr("Backend utilizing QtWebKit module");
}

QIcon QtWebKitWebBackend::getIconForUrl(const QUrl &url)
{
	const QIcon icon = QWebSettings::iconForUrl(url);

	return (icon.isNull() ? Utils::getIcon("text-html") : icon);
}

}
