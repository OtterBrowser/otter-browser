/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "SpellCheckManager.h"

namespace Otter
{

class ContentsWidget;
class WebWidget;

class WebBackend : public Addon
{
	Q_OBJECT

public:
	enum BackendCapability
	{
		NoCapabilities = 0,
		CacheManagementCapability = 1,
		CookiesManagementCapability = 2,
		PasswordsManagementCapability = 4,
		PluginsOnDemandCapability = 8,
		UserScriptsCapability = 16,
		UserStyleSheetsCapability = 32,
		GlobalCookiesPolicyCapability = 64,
		GlobalContentFilteringCapability = 128,
		GlobalDoNotTrackCapability = 256,
		GlobalProxyCapability = 512,
		GlobalReferrerCapability = 1024,
		GlobalUserAgentCapability = 2048,
		TabCookiesPolicyCapability = 4096,
		TabContentFilteringCapability = 8192,
		TabDoNotTrackCapability = 16384,
		TabProxyCapability = 32768,
		TabReferrerCapability = 65536,
		TabUserAgentCapability = 131072
	};

	Q_DECLARE_FLAGS(BackendCapabilities, BackendCapability)

	explicit WebBackend(QObject *parent = nullptr);

	virtual WebWidget* createWidget(bool isPrivate = false, ContentsWidget *parent = nullptr) = 0;
	virtual QString getEngineVersion() const = 0;
	virtual QString getSslVersion() const = 0;
	virtual QString getUserAgent(const QString &pattern = QString()) const = 0;
	QUrl getUpdateUrl() const override;
	virtual QList<SpellCheckManager::DictionaryInformation> getDictionaries() const;
	AddonType getType() const override;
	virtual BackendCapabilities getCapabilities() const;
	virtual bool requestThumbnail(const QUrl &url, const QSize &size) = 0;

signals:
	void thumbnailAvailable(const QUrl &url, const QPixmap &thumbnail, const QString &title);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::WebBackend::BackendCapabilities)

#endif
