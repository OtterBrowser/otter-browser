/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "JsonSettings.h"
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
#include <QtCore/QTextStream>

namespace Otter
{

Addon::Addon() :
	m_isEnabled(true)
{
}

Addon::~Addon() = default;

void Addon::setEnabled(bool isEnabled)
{
	m_isEnabled = isEnabled;
}

QString Addon::getDescription() const
{
	return {};
}

QString Addon::getVersion() const
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

JsonAddon::JsonAddon() : Addon(),
	m_isModified(false)
{
}

void JsonAddon::loadMetaData(const QString &path)
{
	const JsonSettings settings(path);
	const QStringList comments(settings.getComment().split(QLatin1Char('\n')));

	for (int i = 0; i < comments.count(); ++i)
	{
		const QString comment(comments.at(i));
		const QString key(comment.section(QLatin1Char(':'), 0, 0).trimmed());
		const QString value(comment.section(QLatin1Char(':'), 1).trimmed());

		if (key == QLatin1String("Title"))
		{
			m_metaData.title = value;
		}
		else if (key == QLatin1String("Description"))
		{
			m_metaData.description = value;
		}
		else if (key == QLatin1String("Author"))
		{
			m_metaData.author = value;
		}
		else if (key == QLatin1String("Version"))
		{
			m_metaData.version = value;
		}
		else if (key == QLatin1String("HomePage"))
		{
			m_metaData.homePage = QUrl(value);
		}
	}
}

void JsonAddon::setTitle(const QString &title)
{
	if (title != m_metaData.title)
	{
		m_metaData.title = title;
		m_isModified = true;
	}
}

void JsonAddon::setDescription(const QString &description)
{
	if (description != m_metaData.description)
	{
		m_metaData.description = description;
		m_isModified = true;
	}
}

void JsonAddon::setAuthor(const QString &author)
{
	if (author != m_metaData.author)
	{
		m_metaData.author = author;
		m_isModified = true;
	}
}

void JsonAddon::setVersion(const QString &version)
{
	if (version != m_metaData.version)
	{
		m_metaData.version = version;
		m_isModified = true;
	}
}

void JsonAddon::setMetaData(const MetaData &metaData)
{
	if (metaData.title != m_metaData.title || metaData.description != m_metaData.description || metaData.author != m_metaData.author || metaData.version != m_metaData.version || metaData.homePage != m_metaData.homePage)
	{
		m_metaData = metaData;
		m_isModified = true;
	}
}

void JsonAddon::setModified(bool isModified)
{
	m_isModified = isModified;
}

QString JsonAddon::formatComment(const QString &type)
{
	QString comment;
	QTextStream stream(&comment);
	stream << QLatin1String("Title: ") << (m_metaData.title.isEmpty() ? QT_TR_NOOP("(Untitled)") : m_metaData.title) << QLatin1Char('\n');
	stream << QLatin1String("Description: ") << m_metaData.description << QLatin1Char('\n');
	stream << QLatin1String("Type: ") << type << QLatin1String("\n");
	stream << QLatin1String("Author: ") << m_metaData.author << QLatin1Char('\n');
	stream << QLatin1String("Version: ") << m_metaData.version;

	if (m_metaData.homePage.isValid())
	{
		stream << QLatin1Char('\n') << QLatin1String("HomePage: ") << m_metaData.homePage.toString();
	}

	return comment;
}

QString JsonAddon::getTitle() const
{
	return (m_metaData.title.isEmpty() ? QCoreApplication::translate("Otter::JsonAddon", "(Untitled)") : m_metaData.title);
}

QString JsonAddon::getDescription() const
{
	return m_metaData.description;
}

QString JsonAddon::getAuthor() const
{
	return m_metaData.author;
}

QString JsonAddon::getVersion() const
{
	return m_metaData.version;
}

QUrl JsonAddon::getHomePage() const
{
	return m_metaData.homePage;
}

Addon::MetaData JsonAddon::getMetaData() const
{
	return m_metaData;
}

bool JsonAddon::isModified() const
{
	return m_isModified;
}

AddonsManager* AddonsManager::m_instance(nullptr);
QMap<QString, UserScript*> AddonsManager::m_userScripts;
QMap<QString, WebBackend*> AddonsManager::m_webBackends;
QMap<QString, AddonsManager::SpecialPageInformation> AddonsManager::m_specialPages;
bool AddonsManager::m_areUserScriptsInitialized(false);

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

	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Actions"), {}, QUrl(QLatin1String("about:actions")), ThemesManager::createIcon(QLatin1String("edit-cut"), false), SpecialPageInformation::UniversalType), QLatin1String("actions"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Addons"), {}, QUrl(QLatin1String("about:addons")), ThemesManager::createIcon(QLatin1String("preferences-plugin"), false), SpecialPageInformation::UniversalType), QLatin1String("addons"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Bookmarks"), {}, QUrl(QLatin1String("about:bookmarks")), ThemesManager::createIcon(QLatin1String("bookmarks"), false), SpecialPageInformation::UniversalType), QLatin1String("bookmarks"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Cache"), {}, QUrl(QLatin1String("about:cache")), ThemesManager::createIcon(QLatin1String("cache"), false), SpecialPageInformation::UniversalType), QLatin1String("cache"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Advanced Configuration"), {}, QUrl(QLatin1String("about:config")), ThemesManager::createIcon(QLatin1String("configuration"), false), SpecialPageInformation::UniversalType), QLatin1String("config"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Content Filters"), {}, QUrl(QLatin1String("about:content-filters")), ThemesManager::createIcon(QLatin1String("content-blocking"), false), SpecialPageInformation::UniversalType), QLatin1String("content-filters"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Cookies"), {}, QUrl(QLatin1String("about:cookies")), ThemesManager::createIcon(QLatin1String("cookies"), false), SpecialPageInformation::UniversalType), QLatin1String("cookies"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Feeds"), {}, QUrl(QLatin1String("about:feeds")), ThemesManager::createIcon(QLatin1String("feeds"), false), SpecialPageInformation::UniversalType), QLatin1String("feeds"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "History"), {}, QUrl(QLatin1String("about:history")), ThemesManager::createIcon(QLatin1String("view-history"), false), SpecialPageInformation::UniversalType), QLatin1String("history"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Links"), {}, QUrl(QLatin1String("about:links")), ThemesManager::createIcon(QLatin1String("links"), false), SpecialPageInformation::SidebarPanelType, false), QLatin1String("links"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Notes"), {}, QUrl(QLatin1String("about:notes")), ThemesManager::createIcon(QLatin1String("notes"), false), SpecialPageInformation::UniversalType), QLatin1String("notes"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Page Information"), {}, {}, ThemesManager::createIcon(QLatin1String("view-information"), false), SpecialPageInformation::SidebarPanelType), QLatin1String("pageInformation"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Passwords"), {}, QUrl(QLatin1String("about:passwords")), ThemesManager::createIcon(QLatin1String("dialog-password"), false), SpecialPageInformation::UniversalType), QLatin1String("passwords"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Preferences"), {}, QUrl(QLatin1String("about:preferences")), ThemesManager::createIcon(QLatin1String("configuration"), false), SpecialPageInformation::StandaloneType), QLatin1String("preferences"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Tab History"), {}, {}, ThemesManager::createIcon(QLatin1String("tab-history"), false), SpecialPageInformation::SidebarPanelType), QLatin1String("tabHistory"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Downloads"), {}, QUrl(QLatin1String("about:transfers")), ThemesManager::createIcon(QLatin1String("transfers"), false), SpecialPageInformation::UniversalType, false), QLatin1String("transfers"));
	registerSpecialPage(SpecialPageInformation(QT_TRANSLATE_NOOP("addons", "Windows and Tabs"), {}, QUrl(QLatin1String("about:windows")), ThemesManager::createIcon(QLatin1String("window"), false), SpecialPageInformation::UniversalType, false), QLatin1String("windows"));
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
	qDeleteAll(m_userScripts);

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

	const QList<QFileInfo> scriptFiles(QDir(SessionsManager::getWritableDataPath(QLatin1String("scripts"))).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot));

	for (int i = 0; i < scriptFiles.count(); ++i)
	{
		const QFileInfo scriptFile(scriptFiles.at(i));
		const QString path(QDir(scriptFile.absoluteFilePath()).filePath(scriptFile.fileName() + QLatin1String(".js")));

		if (!QFile::exists(path))
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to find User Script file: %1").arg(path), Console::OtherCategory, Console::WarningLevel);

			continue;
		}

		const QJsonObject scriptObject(metaData.value(scriptFile.fileName()));
		UserScript *script(new UserScript(path, QUrl(scriptObject.value(QLatin1String("downloadUrl")).toString()), m_instance));
		script->setEnabled(scriptObject.value(QLatin1String("isEnabled")).toBool(false));

		m_userScripts[scriptFile.fileName()] = script;

		connect(script, &UserScript::metaDataChanged, m_instance, [=]()
		{
			emit m_instance->userScriptModified(script->getName());
		});
	}

	m_areUserScriptsInitialized = true;
}

AddonsManager* AddonsManager::getInstance()
{
	return m_instance;
}

UserScript* AddonsManager::getUserScript(const QString &name)
{
	if (!m_areUserScriptsInitialized)
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

		return m_webBackends.value(m_webBackends.firstKey(), nullptr);
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

QStringList AddonsManager::getAddons(Addon::AddonType type)
{
	switch (type)
	{
		case Addon::UserScriptType:
			if (!m_areUserScriptsInitialized)
			{
				loadUserScripts();
			}

			return m_userScripts.keys();

		case Addon::WebBackendType:
			return m_webBackends.keys();

		default:
			break;
	}

	return {};
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

bool AddonsManager::isSpecialPage(const QUrl &url)
{
	if (url.scheme() != QLatin1String("about"))
	{
		return false;
	}

	const QString identifier(url.path());

	return (m_specialPages.contains(identifier) && m_specialPages[identifier].types.testFlag(SpecialPageInformation::StandaloneType));
}

}
