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

#ifndef OTTER_WEBBACKENDSMANAGER_H
#define OTTER_WEBBACKENDSMANAGER_H

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QStringList>

namespace Otter
{

class WebBackend;

class WebBackendsManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void registerBackend(WebBackend *backend, const QString &name);
	static WebBackend* getBackend(const QString &backend = QString());
	static QStringList getBackends();

protected:
	explicit WebBackendsManager(QObject *parent = NULL);

private:
	static WebBackendsManager *m_instance;
	static QHash<QString, WebBackend*> m_backends;
};

}

#endif
