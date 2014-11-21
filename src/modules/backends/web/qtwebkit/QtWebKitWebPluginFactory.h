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

#ifndef OTTER_QTWEBKITWEBPLUGINFACTORY_H
#define OTTER_QTWEBKITWEBPLUGINFACTORY_H

#include "../../../windows/web/qtwebkit/LoadPluginWidget.h"

#include <QtCore/QList>
#include <QtWebKit/QWebPluginFactory>

namespace Otter
{
	class QtWebKitWebPluginFactory : public QWebPluginFactory
	{
		Q_OBJECT

		public:
			explicit QtWebKitWebPluginFactory(QObject *parent);

			virtual QObject *create(
					const QString &mimeType,
					const QUrl &url,
					const QStringList &argumentNames,
					const QStringList &argumentValues) const;

			virtual void setPageURL(const QUrl url);
			virtual QList<QWebPluginFactory::Plugin> plugins() const;

		signals:
			void signalLoadPlugin(bool) const;

		public slots:
			void setLoadClickToPlugin(bool load);

		private:
			QList<QWebPluginFactory::Plugin> m_pluginList;
			bool m_loadPluginClicked;
			QUrl m_mainPageUrl;
	};

}

#endif
