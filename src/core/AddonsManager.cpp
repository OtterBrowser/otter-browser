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
#include "Utils.h"
#include "WebBackend.h"
#ifdef OTTER_ENABLE_QTWEBENGINE
#include "../modules/backends/web/qtwebengine/QtWebEngineWebBackend.h"
#endif
#include "../modules/backends/web/qtwebkit/QtWebKitWebBackend.h"

namespace Otter
{

AddonsManager *AddonsManager::m_instance = NULL;
QHash<QString, WebBackend*> AddonsManager::m_webBackends;
QHash<QString, AddonsManager::SpecialPageInformation> AddonsManager::m_specialPages;

AddonsManager::AddonsManager(QObject *parent) : QObject(parent)
{
#ifdef OTTER_ENABLE_QTWEBENGINE
	registerWebBackend(new QtWebEngineWebBackend(this), QLatin1String("qtwebengine"));
#endif
	registerWebBackend(new QtWebKitWebBackend(this), QLatin1String("qtwebkit"));

	registerSpecialPage(SpecialPageInformation(tr("Bookmarks Manager"), QString(), QUrl(QLatin1String("about:bookmarks")), Utils::getIcon(QLatin1String("bookmarks"), false)), QLatin1String("bookmarks"));
	registerSpecialPage(SpecialPageInformation(tr("Cache Manager"), QString(), QUrl(QLatin1String("about:cache")), Utils::getIcon(QLatin1String("cache"), false)), QLatin1String("cache"));
	registerSpecialPage(SpecialPageInformation(tr("Configuration Manager"), QString(), QUrl(QLatin1String("about:config")), Utils::getIcon(QLatin1String("configuration"), false)), QLatin1String("configuration"));
	registerSpecialPage(SpecialPageInformation(tr("Cookies Manager"), QString(), QUrl(QLatin1String("about:cookies")), Utils::getIcon(QLatin1String("cookies"), false)), QLatin1String("cookies"));
	registerSpecialPage(SpecialPageInformation(tr("History Manager"), QString(), QUrl(QLatin1String("about:history")), Utils::getIcon(QLatin1String("view-history"), false)), QLatin1String("history"));
	registerSpecialPage(SpecialPageInformation(tr("Notes Manager"), QString(), QUrl(QLatin1String("about:notes")), Utils::getIcon(QLatin1String("notes"), false)), QLatin1String("notes"));
	registerSpecialPage(SpecialPageInformation(tr("Transfers Manager"), QString(), QUrl(QLatin1String("about:transfers")), Utils::getIcon(QLatin1String("transfers"), false)), QLatin1String("transfers"));

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

void AddonsManager::registerSpecialPage(const AddonsManager::SpecialPageInformation &information, const QString &name)
{
	m_specialPages[name] = information;
}

QStringList AddonsManager::getWebBackends()
{
	return m_webBackends.keys();
}

QStringList AddonsManager::getSpecialPages()
{
	return m_specialPages.keys();
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

AddonsManager::SpecialPageInformation AddonsManager::getSpecialPage(const QString &name)
{
	if (m_specialPages.contains(name))
	{
		return m_specialPages[name];
	}

	return SpecialPageInformation();
}

}

