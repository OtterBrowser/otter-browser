/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "AddonsManager.h"
#include "SettingsManager.h"
#include "WebBackend.h"
#ifdef OTTER_ENABLE_QTWEBENGINE
#include "../modules/backends/web/qtwebengine/QtWebEngineWebBackend.h"
#endif
#include "../modules/backends/web/qtwebkit/QtWebKitWebBackend.h"

namespace Otter
{

AddonsManager *AddonsManager::m_instance = NULL;
QHash<QString, WebBackend*> AddonsManager::m_webBackends;

AddonsManager::AddonsManager(QObject *parent) : QObject(parent)
{
#ifdef OTTER_ENABLE_QTWEBENGINE
	registerWebBackend(new QtWebEngineWebBackend(this), QLatin1String("qtwebengine"));
#endif
	registerWebBackend(new QtWebKitWebBackend(this), QLatin1String("qtwebkit"));
}

void AddonsManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new AddonsManager(parent);
	}
}

void AddonsManager::registerWebBackend(WebBackend *backend, const QString &name)
{
	m_webBackends[name] = backend;
}

QStringList AddonsManager::getWebBackends()
{
	return m_webBackends.keys();
}

WebBackend* AddonsManager::getWebBackend(const QString &name)
{
	if (m_webBackends.contains(name))
	{
		return m_webBackends[name];
	}

	if (name.isEmpty() && !m_webBackends.isEmpty())
	{
		const QString defaultName = SettingsManager::getValue(QLatin1String("Backends/Web")).toString();

		if (m_webBackends.contains(defaultName))
		{
			return m_webBackends[defaultName];
		}

		return m_webBackends.value(m_webBackends.keys().first(), NULL);
	}

	return NULL;
}

}

