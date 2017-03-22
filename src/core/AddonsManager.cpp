/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

Addon::Addon(QObject *parent) : QObject(parent),
	m_isEnabled(true)
{
}

void Addon::setEnabled(bool isEnabled)
{
	m_isEnabled = isEnabled;
}

QString Addon::getName() const
{
	return QString();
}

QUrl Addon::getHomePage() const
{
	return QUrl();
}

QUrl Addon::getUpdateUrl() const
{
	return QUrl();
}

QIcon Addon::getIcon() const
{
	return QIcon();
}

Addon::AddonType Addon::getType() const
{
	return UnknownType;
}

bool Addon::isEnabled() const
{
	return m_isEnabled;
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

	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Addons"), QString(), QUrl(QLatin1String("about:addons")), ThemesManager::getIcon(QLatin1String("preferences-plugin"), false)), QLatin1String("addons"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Bookmarks"), QString(), QUrl(QLatin1String("about:bookmarks")), ThemesManager::getIcon(QLatin1String("bookmarks"), false)), QLatin1String("bookmarks"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Cache"), QString(), QUrl(QLatin1String("about:cache")), ThemesManager::getIcon(QLatin1String("cache"), false)), QLatin1String("cache"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Advanced Configuration"), QString(), QUrl(QLatin1String("about:config")), ThemesManager::getIcon(QLatin1String("configuration"), false)), QLatin1String("config"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Cookies"), QString(), QUrl(QLatin1String("about:cookies")), ThemesManager::getIcon(QLatin1String("cookies"), false)), QLatin1String("cookies"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "History"), QString(), QUrl(QLatin1String("about:history")), ThemesManager::getIcon(QLatin1String("view-history"), false)), QLatin1String("history"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Notes"), QString(), QUrl(QLatin1String("about:notes")), ThemesManager::getIcon(QLatin1String("notes"), false)), QLatin1String("notes"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Passwords"), QString(), QUrl(QLatin1String("about:passwords")), ThemesManager::getIcon(QLatin1String("dialog-password"), false)), QLatin1String("passwords"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Transfers"), QString(), QUrl(QLatin1String("about:transfers")), ThemesManager::getIcon(QLatin1String("transfers"), false)), QLatin1String("transfers"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Windows"), QString(), QUrl(QLatin1String("about:windows")), ThemesManager::getIcon(QLatin1String("window"), false)), QLatin1String("windows"));
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

void AddonsManager::loadUserScripts()
{
	qDeleteAll(m_userScripts.values());

	m_userScripts.clear();

	QHash<QString, bool> enabledScripts;
	QFile file(SessionsManager::getWritableDataPath(QLatin1String("scripts/scripts.json")));

	if (file.open(QIODevice::ReadOnly))
	{
		const QJsonObject settings(QJsonDocument::fromJson(file.readAll()).object());
		QJsonObject::const_iterator iterator;

		for (iterator = settings.constBegin(); iterator != settings.constEnd(); ++iterator)
		{
			enabledScripts[iterator.key()] = iterator.value().toObject().value(QLatin1String("isEnabled")).toBool(false);
		}

		file.close();
	}

	const QList<QFileInfo> scripts(QDir(SessionsManager::getWritableDataPath(QLatin1String("scripts"))).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot));

	for (int i = 0; i < scripts.count(); ++i)
	{
		const QString path(QDir(scripts.at(i).absoluteFilePath()).filePath(scripts.at(i).fileName() + QLatin1String(".js")));

		if (QFile::exists(path))
		{
			UserScript *script(new UserScript(path, m_instance));
			script->setEnabled(enabledScripts.value(scripts.at(i).fileName(), false));

			m_userScripts[scripts.at(i).fileName()] = script;
		}
		else
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to find User Script file: %1").arg(path), Console::OtherCategory, Console::WarningLevel);
		}
	}

	m_areUserScripsInitialized = true;
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

	return SpecialPageInformation();
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

QStringList AddonsManager::getSpecialPages()
{
	return m_specialPages.keys();
}

}
