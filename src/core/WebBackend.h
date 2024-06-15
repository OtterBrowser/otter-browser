/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_WEBBACKEND_H
#define OTTER_WEBBACKEND_H

#include "AddonsManager.h"
#include "BookmarksModel.h"
#include "DataExchanger.h"
#include "SpellCheckManager.h"

namespace Otter
{

class CookieJar;
class ContentsWidget;
class WebWidget;

class WebPageThumbnailJob : public Job
{
	Q_OBJECT

public:
	explicit WebPageThumbnailJob(const QUrl &url, const QSize &size, QObject *parent = nullptr);

	virtual QString getTitle() const = 0;
	virtual QPixmap getThumbnail() const = 0;
};

class WebBackend : public QObject, public Addon
{
	Q_OBJECT

public:
	enum BackendCapability
	{
		UnknownCapability = 0,
		BookmarksImportCapability,
		CacheManagementCapability,
		CookiesManagementCapability,
		CookiesPolicyCapability,
		ContentFilteringCapability,
		DoNotTrackCapability,
		FindInPageHighlightAllCapability,
		FindInPageExactMatchCapability,
		HistoryMetaDataCapability,
		PasswordsManagementCapability,
		PluginsOnDemandCapability,
		ProxyCapability,
		ReferrerCapability,
		UserScriptsCapability,
		UserStyleSheetsCapability,
		UserAgentCapability
	};

	enum CapabilityScope
	{
		NoScope = 0,
		GlobalScope = 1,
		DomainScope = 2,
		TabScope = 4,
		AllScopes = (GlobalScope | DomainScope | TabScope)
	};

	Q_DECLARE_FLAGS(CapabilityScopes, CapabilityScope)

	struct CookieJarInformation final
	{
		CookieJar *cookieJar = nullptr;
		QString title;
		QString path;
		bool isPrivate = false;
	};

	explicit WebBackend(QObject *parent = nullptr);

	virtual WebWidget* createWidget(const QVariantMap &parameters, ContentsWidget *parent = nullptr) = 0;
	virtual BookmarksImportJob* createBookmarksImportJob(BookmarksModel::Bookmark *folder, const QString &path, bool areDuplicatesAllowed);
	virtual WebPageThumbnailJob* createPageThumbnailJob(const QUrl &url, const QSize &size);
	virtual CookieJarInformation getCookieJar() const;
	virtual QString getEngineVersion() const = 0;
	virtual QString getSslVersion() const = 0;
	virtual QString getUserAgent(const QString &pattern = {}) const = 0;
	virtual QVector<SpellCheckManager::DictionaryInformation> getDictionaries() const;
	AddonType getType() const override;
	virtual CapabilityScopes getCapabilityScopes(BackendCapability capability) const;
	bool hasCapability(BackendCapability capability) const;
	virtual bool hasSslSupport() const = 0;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::WebBackend::CapabilityScopes)

#endif
