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
	globalSettings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, SettingsManager::getValue("Browser/JavaSriptCanAccessClipboard").toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptCanCloseWindows, SettingsManager::getValue("Browser/JavaSriptCanCloseWindows").toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptCanOpenWindows, SettingsManager::getValue("Browser/JavaScriptCanOpenWindows").toBool());
	globalSettings->setFontSize(QWebSettings::DefaultFontSize, SettingsManager::getValue("Browser/DefaultFontSize").toInt());
	globalSettings->setFontSize(QWebSettings::DefaultFixedFontSize, SettingsManager::getValue("Browser/DefaultFixedFontSize").toInt());
	globalSettings->setFontSize(QWebSettings::MinimumFontSize, SettingsManager::getValue("Browser/MinimumFontSize").toInt());
	globalSettings->setIconDatabasePath(cachePath);
	globalSettings->setOfflineStoragePath(cachePath);

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

void QtWebKitWebBackend::optionChanged(const QString &option)
{
	if (!option.startsWith("Browser/"))
	{
		return;
	}

	QWebSettings *globalSettings = QWebSettings::globalSettings();
	globalSettings->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
	globalSettings->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	globalSettings->setAttribute(QWebSettings::PluginsEnabled, SettingsManager::getValue("Browser/EnablePlugins").toBool());
	globalSettings->setAttribute(QWebSettings::JavaEnabled, SettingsManager::getValue("Browser/EnableJava").toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptEnabled, SettingsManager::getValue("Browser/EnableJavaScript").toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, SettingsManager::getValue("Browser/JavaSriptCanAccessClipboard").toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptCanCloseWindows, SettingsManager::getValue("Browser/JavaSriptCanCloseWindows").toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptCanOpenWindows, SettingsManager::getValue("Browser/JavaScriptCanOpenWindows").toBool());
	globalSettings->setFontSize(QWebSettings::DefaultFontSize, SettingsManager::getValue("Browser/DefaultFontSize").toInt());
	globalSettings->setFontSize(QWebSettings::DefaultFixedFontSize, SettingsManager::getValue("Browser/DefaultFixedFontSize").toInt());
	globalSettings->setFontSize(QWebSettings::MinimumFontSize, SettingsManager::getValue("Browser/MinimumFontSize").toInt());
}

WebWidget* QtWebKitWebBackend::createWidget(bool privateWindow, ContentsWidget *parent)
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
