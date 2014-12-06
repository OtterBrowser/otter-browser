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

#ifndef OTTER_QTWEBKITWEBPLUGINFACTORY_H
#define OTTER_QTWEBKITWEBPLUGINFACTORY_H

#include <QtWebKit/QWebPluginFactory>

namespace Otter
{

class QtWebKitWebWidget;

class QtWebKitWebPluginFactory : public QWebPluginFactory
{
	Q_OBJECT

public:
	explicit QtWebKitWebPluginFactory(QtWebKitWebWidget *parent);

	QObject* create(const QString &mimeType, const QUrl &url, const QStringList &argumentNames, const QStringList &argumentValues) const;
	QList<QWebPluginFactory::Plugin> plugins() const;

private:
	QtWebKitWebWidget *m_widget;
};

}

#endif
