/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "AddonsManager.h"
#include "Console.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "ThemesManager.h"
#include "UserScript.h"
#include "WebBackend.h"
#ifdef OTTER_ENABLE_QTWEBENGINE
#include "../modules/backends/web/qtwebengine/QtWebEngineWebBackend.h"
#endif
#ifdef OTTER_ENABLE_QTWEBKIT
#include "../modules/backends/web/qtwebkit/QtWebKitWebBackend.h"
#endif

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Otter
{

Addon::Addon() : m_isEnabled(true)
{
}

Addon::~Addon()
{
}

void Addon::setEnabled(bool isEnabled)
{
	m_isEnabled = isEnabled;
}

QString Addon::getName() const
{
	return {};
}

QUrl Addon::getHomePage() const
{
	return {};
}

QUrl Addon::getUpdateUrl() const
{
	return {};
}

QIcon Addon::getIcon() const
{
	return {};
}

Addon::AddonType Addon::getType() const
{
	return UnknownType;
}

bool Addon::isEnabled() const
{
	return m_isEnabled;
}

bool Addon::canRemove() const
{
	return false;
}

bool Addon::remove()
{
	return false;
}

AddonsManager* AddonsManager::m_instance(nullptr);
QMap<QString, UserScript*> AddonsManager::m_userScripts;
QMap<QString, WebBackend*> AddonsManager::m_webBackends;
QMap<QString, AddonsManager::SpecialPageInformation> AddonsManager::m_specialPages;
bool AddonsManager::m_areUserScripsInitialized(false);

AddonsManager::AddonsManager(QObject *parent) : QObject(parent)
{
#ifdef OTTER_ENABLE_QTWEBENGINE
	registerWebBackend(new QtWebEngineWebBackend(this), QLatin1String("qtwebengine"));
#endif
#ifdef OTTER_ENABLE_QTWEBKIT
	registerWebBackend(new QtWebKitWebBackend(this), QLatin1String("qtwebkit"));
#endif

	SettingsManager::OptionDefinition backends(SettingsManager::getOptionDefinition(SettingsManager::Backends_WebOption));
	backends.choices.clear();
	backends.choices.reserve(m_webBackends.count());

	QMap<QString, WebBackend*>::iterator iterator;

	for (iterator = m_webBackends.begin(); iterator != m_webBackends.end(); ++iterator)
	{
		backends.choices.append({iterator.value()->getTitle(), iterator.key(), iterator.value()->getIcon()});
	}

	SettingsManager::updateOptionDefinition(SettingsManager::Backends_WebOption, backends);

	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Addons"), {}, QUrl(QLatin1String("about:addons")), ThemesManager::createIcon(QLatin1String("preferences-plugin"), false), SpecialPageInformation::UniversalType), QLatin1String("addons"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Bookmarks"), {}, QUrl(QLatin1String("about:bookmarks")), ThemesManager::createIcon(QLatin1String("bookmarks"), false), SpecialPageInformation::UniversalType), QLatin1String("bookmarks"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Cache"), {}, QUrl(QLatin1String("about:cache")), ThemesManager::createIcon(QLatin1String("cache"), false), SpecialPageInformation::UniversalType), QLatin1String("cache"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Advanced Configuration"), {}, QUrl(QLatin1String("about:config")), ThemesManager::createIcon(QLatin1String("configuration"), false), SpecialPageInformation::UniversalType), QLatin1String("config"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Cookies"), {}, QUrl(QLatin1String("about:cookies")), ThemesManager::createIcon(QLatin1String("cookies"), false), SpecialPageInformation::UniversalType), QLatin1String("cookies"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Feeds"), {}, QUrl(QLatin1String("about:feeds")), ThemesManager::createIcon(QLatin1String("feeds"), false), SpecialPageInformation::UniversalType), QLatin1String("feeds"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "History"), {}, QUrl(QLatin1String("about:history")), ThemesManager::createIcon(QLatin1String("view-history"), false), SpecialPageInformation::UniversalType), QLatin1String("history"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Links"), {}, QUrl(QLatin1String("about:links")), ThemesManager::createIcon(QLatin1String("links"), false), SpecialPageInformation::SidebarPanelType), QLatin1String("links"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Notes"), {}, QUrl(QLatin1String("about:notes")), ThemesManager::createIcon(QLatin1String("notes"), false), SpecialPageInformation::UniversalType), QLatin1String("notes"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Page Information"), {}, {}, ThemesManager::createIcon(QLatin1String("view-information"), false), SpecialPageInformation::SidebarPanelType), QLatin1String("pageInformation"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Passwords"), {}, QUrl(QLatin1String("about:passwords")), ThemesManager::createIcon(QLatin1String("dialog-password"), false), SpecialPageInformation::UniversalType), QLatin1String("passwords"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Tab History"), {}, {}, ThemesManager::createIcon(QLatin1String("tab-history"), false), SpecialPageInformation::SidebarPanelType), QLatin1String("tabHistory"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Downloads"), {}, QUrl(QLatin1String("about:transfers")), ThemesManager::createIcon(QLatin1String("transfers"), false), SpecialPageInformation::UniversalType), QLatin1String("transfers"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Windows and Tabs"), {}, QUrl(QLatin1String("about:windows")), ThemesManager::createIcon(QLatin1String("window"), false), SpecialPageInformation::UniversalType), QLatin1String("windows"));
}

void AddonsManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new AddonsManager(QCoreApplication::instance());
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

void AddonsManager::loadUserScripts()
{
	qDeleteAll(m_userScripts.values());

	m_userScripts.clear();

	QHash<QString, QJsonObject> metaData;
	QFile file(SessionsManager::getWritableDataPath(QLatin1String("scripts/scripts.json")));

	if (file.open(QIODevice::ReadOnly))
	{
		const QJsonObject settingsObject(QJsonDocument::fromJson(file.readAll()).object());
		QJsonObject::const_iterator iterator;

		for (iterator = settingsObject.constBegin(); iterator != settingsObject.constEnd(); ++iterator)
		{
			metaData[iterator.key()] = iterator.value().toObject();
		}

		file.close();
	}

	const QList<QFileInfo> scripts(QDir(SessionsManager::getWritableDataPath(QLatin1String("scripts"))).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot));

	for (int i = 0; i < scripts.count(); ++i)
	{
		const QString path(QDir(scripts.at(i).absoluteFilePath()).filePath(scripts.at(i).fileName() + QLatin1String(".js")));

		if (QFile::exists(path))
		{
			const QJsonObject scriptObject(metaData.value(scripts.at(i).fileName()));
			UserScript *script(new UserScript(path, QUrl(scriptObject.value(QLatin1String("downloadUrl")).toString()), m_instance));
			script->setEnabled(scriptObject.value(QLatin1String("isEnabled")).toBool(false));

			m_userScripts[scripts.at(i).fileName()] = script;

			connect(script, &UserScript::metaDataChanged, m_instance, [=]()
			{
				emit m_instance->userScriptModified(script->getName());
			});
		}
		else
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to find User Script file: %1").arg(path), Console::OtherCategory, Console::WarningLevel);
		}
	}

	m_areUserScripsInitialized = true;
}

AddonsManager* AddonsManager::getInstance()
{
	return m_instance;
}

UserScript* AddonsManager::getUserScript(const QString &name)
{
	if (!m_areUserScripsInitialized)
	{
		loadUserScripts();
	}

	if (m_userScripts.contains(name))
	{
		return m_userScripts[name];
	}

	return nullptr;
}

WebBackend* AddonsManager::getWebBackend(const QString &name)
{
	if (m_webBackends.contains(name))
	{
		return m_webBackends[name];
	}

	if (name.isEmpty() && !m_webBackends.isEmpty())
	{
		const QString defaultName(SettingsManager::getOption(SettingsManager::Backends_WebOption).toString());

		if (m_webBackends.contains(defaultName))
		{
			return m_webBackends[defaultName];
		}

		return m_webBackends.value(m_webBackends.keys().first(), nullptr);
	}

	return nullptr;
}

AddonsManager::SpecialPageInformation AddonsManager::getSpecialPage(const QString &name)
{
	if (m_specialPages.contains(name))
	{
		return m_specialPages[name];
	}

	return {};
}

QStringList AddonsManager::getUserScripts()
{
	if (!m_areUserScripsInitialized)
	{
		loadUserScripts();
	}

	return m_userScripts.keys();
}

QStringList AddonsManager::getWebBackends()
{
	return m_webBackends.keys();
}

QStringList AddonsManager::getSpecialPages(SpecialPageInformation::PageTypes types)
{
	if (types == SpecialPageInformation::UniversalType)
	{
		return m_specialPages.keys();
	}

	QMap<QString, SpecialPageInformation>::const_iterator iterator;
	QStringList pages;
	pages.reserve(m_specialPages.count());

	for (iterator = m_specialPages.constBegin(); iterator != m_specialPages.constEnd(); ++iterator)
	{
		if (iterator.value().types & types)
		{
			pages.append(iterator.key());
		}
	}

	return pages;
}

}
