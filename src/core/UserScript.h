/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_USERSCRIPT_H
#define OTTER_USERSCRIPT_H

#include "AddonsManager.h"

namespace Otter
{

class IconFetchJob;

class UserScript final : public QObject, public Addon
{
	Q_OBJECT

public:
	enum InjectionTime
	{
		AnyTime = 0,
		DocumentCreationTime,
		DocumentReadyTime,
		DeferredTime
	};

	explicit UserScript(const QString &path, const QUrl &url = {}, QObject *parent = nullptr);

	QString getName() const override;
	QString getTitle() const override;
	QString getDescription() const override;
	QString getVersion() const override;
	QString getPath() const;
	QString getSource();
	QUrl getHomePage() const override;
	QUrl getUpdateUrl() const override;
	QIcon getIcon() const override;
	QStringList getExcludeRules() const;
	QStringList getIncludeRules() const;
	QStringList getMatchRules() const;
	static QVector<UserScript*> getUserScriptsForUrl(const QUrl &url, InjectionTime injectionTime = AnyTime, bool isSubFrame = false);
	InjectionTime getInjectionTime() const;
	AddonType getType() const override;
	bool isEnabledForUrl(const QUrl &url);
	bool canRemove() const override;
	bool shouldRunOnSubFrames() const;
	bool remove() override;

public slots:
	void reload();

protected:
	QString checkUrlSubString(const QString &rule, const QString &urlSubString, QString generatedUrl, int position = 0) const;
	bool checkUrl(const QUrl &url, const QStringList &rules) const;

private:
	IconFetchJob *m_iconFetchJob;
	QString m_path;
	QString m_source;
	QString m_title;
	QString m_description;
	QString m_version;
	QUrl m_homePage;
	QUrl m_downloadUrl;
	QUrl m_iconUrl;
	QUrl m_updateUrl;
	QIcon m_icon;
	QStringList m_excludeRules;
	QStringList m_includeRules;
	QStringList m_matchRules;
	InjectionTime m_injectionTime;
	bool m_shouldRunOnSubFrames;

signals:
	void metaDataChanged();
};

}

#endif
