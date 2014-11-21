/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2009 Jakub Wieczorek <faw217@gmail.com>
* Copyright (C) 2009 by Benjamin C. Meyer <ben@meyerhome.net>
* Copyright (C) 2010-2011 by Matthieu Gicquel <matgic78@gmail.com>
* Copyright (C) 2014 Martin Rejda <rejdi@otter.ksp.sk>
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

#include "QtWebKitWebPluginFactory.h"
#include "LoadPluginWidget.h"
#include "../../../../core/SettingsManager.h"

namespace Otter
{

QtWebKitWebPluginFactory::QtWebKitWebPluginFactory(QWebPage *parent) : QWebPluginFactory(parent),
	m_page(parent),
	m_pluginIsLoaded(false)
{
	connect(this, SIGNAL(pluginIsLoaded(bool)), this, SLOT(setPluginIsLoaded(bool)));
}

void QtWebKitWebPluginFactory::setPluginIsLoaded(bool isLoaded)
{
	m_pluginIsLoaded = isLoaded;
}

void QtWebKitWebPluginFactory::setBaseUrl(const QUrl url)
{
	m_baseUrl = url;
}

QObject* QtWebKitWebPluginFactory::create(const QString &mimeType, const QUrl &url, const QStringList &argumentNames, const QStringList &argumentValues) const
{
	Q_UNUSED(argumentNames);
	Q_UNUSED(argumentValues);

	//TODO: add support to recognize unsupported plugins and show apropriate message
	if (SettingsManager::getValue(QLatin1String("Browser/EnablePlugins"), m_baseUrl).toString() == QLatin1String("onDemand") && !m_pluginIsLoaded)
	{
		LoadPluginWidget *plugin = new LoadPluginWidget(m_page, mimeType, url);

		connect(plugin, SIGNAL(pluginLoaded()), this, SLOT(setPluginIsLoaded()));

		return plugin;
	}
	else
	{
		emit pluginIsLoaded(false);
	}

	return NULL;
}

QList<QWebPluginFactory::Plugin> QtWebKitWebPluginFactory::plugins() const
{
	return QList<QWebPluginFactory::Plugin>();
}

}
