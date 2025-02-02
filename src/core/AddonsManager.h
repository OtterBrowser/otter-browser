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

#ifndef OTTER_ADDONSMANAGER_H
#define OTTER_ADDONSMANAGER_H

#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

namespace Otter
{

class AddonsManager;
class UserScript;
class WebBackend;

class Addon
{
public:
	struct MetaData
	{
		QString title;
		QString description;
		QString author;
		QString version;
		QUrl homePage;
	};

	enum AddonType
	{
		UnknownType = 0,
		ColorSchemeType,
		DictionaryType,
		ExporterType,
		ExtensionType,
		ImporterType,
		ModuleType,
		PasswordsStorageBackendType,
		ProtocolType,
		ScriptletType,
		UserScriptType,
		UserStylesheetType,
		WebBackendType
	};

	explicit Addon();
	virtual ~Addon();

	virtual QString getName() const = 0;
	virtual QString getTitle() const = 0;
	virtual QString getDescription() const;
	virtual QString getVersion() const;
	virtual QUrl getHomePage() const;
	virtual QUrl getUpdateUrl() const;
	virtual QIcon getIcon() const;
	virtual AddonType getType() const;
	virtual bool isEnabled() const;
	virtual bool canRemove() const;
	virtual bool remove();

protected:
	void setEnabled(bool isEnabled);

private:
	bool m_isEnabled;

friend class AddonsManager;
};

class JsonAddon : public Addon
{
public:
	explicit JsonAddon();

	void setTitle(const QString &title);
	void setDescription(const QString &description);
	void setAuthor(const QString &author);
	void setVersion(const QString &version);
	void setMetaData(const MetaData &metaData);
	void setModified(bool isModified);
	QString getTitle() const override;
	QString getDescription() const override;
	QString getAuthor() const;
	QString getVersion() const override;
	QUrl getHomePage() const override;
	MetaData getMetaData() const;
	bool isModified() const;

protected:
	void loadMetaData(const QString &path);
	QString formatComment(const QString &type);

private:
	MetaData m_metaData;
	bool m_isModified;
};

class AddonsManager final : public QObject
{
	Q_OBJECT

public:
	struct AddonInformation
	{
		QString title;
		QString description;
		QIcon icon;

		explicit AddonInformation(const QString &titleValue, const QString &descriptionValue, const QIcon &iconValue) : title(titleValue), description(descriptionValue), icon(iconValue)
		{
		}

		AddonInformation() = default;

		QString getTitle() const
		{
			return QCoreApplication::translate("addons", title.toUtf8().constData());
		}

		QString getDescription() const
		{
			return QCoreApplication::translate("addons", description.toUtf8().constData());
		}
	};

	struct SpecialPageInformation final : public AddonInformation
	{
		enum PageType
		{
			UnknownType = 0,
			StandaloneType = 1,
			SidebarPanelType = 2,
			UniversalType = (StandaloneType | SidebarPanelType)
		};

		Q_DECLARE_FLAGS(PageTypes, PageType)

		QUrl url;
		PageTypes types;
		bool isSingleton = true;

		explicit SpecialPageInformation(const QString &titleValue, const QString &descriptionValue, const QUrl &urlValue, const QIcon &iconValue, PageTypes typesValue, bool isSingletonValue = true) : AddonInformation(titleValue, descriptionValue, iconValue), url(urlValue), types(typesValue), isSingleton(isSingletonValue)
		{
		}

		SpecialPageInformation() = default;
	};

	static void createInstance();
	static void registerWebBackend(WebBackend *backend, const QString &name);
	static void registerSpecialPage(const SpecialPageInformation &information, const QString &name);
	static void loadUserScripts();
	static AddonsManager* getInstance();
	static UserScript* getUserScript(const QString &name);
	static WebBackend* getWebBackend(const QString &name = {});
	static SpecialPageInformation getSpecialPage(const QString &name);
	static QStringList getAddons(Addon::AddonType type);
	static QStringList getSpecialPages(SpecialPageInformation::PageTypes types = SpecialPageInformation::StandaloneType);
	static bool isSpecialPage(const QUrl &url);

protected:
	explicit AddonsManager(QObject *parent);

private:
	static AddonsManager *m_instance;
	static QMap<QString, UserScript*> m_userScripts;
	static QMap<QString, WebBackend*> m_webBackends;
	static QMap<QString, SpecialPageInformation> m_specialPages;
	static bool m_areUserScriptsInitialized;

signals:
	void userScriptModified(const QString &name);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::AddonsManager::SpecialPageInformation::PageTypes)

#endif
