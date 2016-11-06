/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Martin Rejda <rejdi@otter.ksp.sk>
* Copyright (C) 2014 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

namespace Otter
{

QtWebKitPluginFactory::QtWebKitPluginFactory(QtWebKitWebWidget *parent) : QWebPluginFactory(parent),
	m_widget(parent)
{
}

QObject* QtWebKitPluginFactory::create(const QString &mimeType, const QUrl &url, const QStringList &argumentNames, const QStringList &argumentValues) const
{
	const bool isActivatingPlugin(argumentNames.contains(QLatin1String("data-otter-browser")) && argumentValues.value(argumentNames.indexOf(QLatin1String("data-otter-browser"))) == m_widget->getPluginToken());

	if (m_widget->canLoadPlugins() || isActivatingPlugin)
	{
		if (isActivatingPlugin)
		{
			m_widget->clearPluginToken();
		}

		return nullptr;
	}

	return new QtWebKitPluginWidget(mimeType, url, m_widget);
}

QList<QWebPluginFactory::Plugin> QtWebKitPluginFactory::plugins() const
{
	return QList<QWebPluginFactory::Plugin>();
}

}
