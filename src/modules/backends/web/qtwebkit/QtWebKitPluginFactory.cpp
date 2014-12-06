/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Martin Rejda <rejdi@otter.ksp.sk>
* Copyright (C) 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QtWebKitPluginFactory.h"
#include "QtWebKitPluginWidget.h"
#include "QtWebKitWebWidget.h"
#include "../../../../core/SettingsManager.h"

namespace Otter
{

QtWebKitPluginFactory::QtWebKitPluginFactory(QtWebKitWebWidget *parent) : QWebPluginFactory(parent),
	m_widget(parent)
{
}

QObject* QtWebKitPluginFactory::create(const QString &mimeType, const QUrl &url, const QStringList &argumentNames, const QStringList &argumentValues) const
{
	//TODO: add support to recognize unsupported plugins and show apropriate message
	if (SettingsManager::getValue(QLatin1String("Browser/EnablePlugins"), m_widget->getUrl()).toString() == QLatin1String("onDemand") && (!argumentNames.contains(QLatin1String("data-otter-browser")) || argumentValues.value(argumentNames.indexOf(QLatin1String("data-otter-browser"))) != m_widget->getPluginToken()))
	{
		m_widget->clearPluginToken();

		return new QtWebKitPluginWidget(mimeType, url, m_widget);
	}

	return NULL;
}

QList<QWebPluginFactory::Plugin> QtWebKitPluginFactory::plugins() const
{
	return QList<QWebPluginFactory::Plugin>();
}

}
