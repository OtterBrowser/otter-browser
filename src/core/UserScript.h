/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

class UserScript : public Addon
{
	Q_OBJECT

public:
	enum InjectionTime
	{
		DocumentCreationTime = 0,
		DocumentReadyTime,
		DeferredTime
	};

	explicit UserScript(const QString &path, QObject *parent = nullptr);

	QString getName() const;
	QString getTitle() const;
	QString getDescription() const;
	QString getVersion() const;
	QString getSource();
	QUrl getHomePage() const;
	QUrl getUpdateUrl() const;
	QIcon getIcon() const;
	QStringList getExcludeRules() const;
	QStringList getIncludeRules() const;
	QStringList getMatchRules() const;
	InjectionTime getInjectionTime() const;
	AddonType getType() const;
	bool isEnabledForUrl(const QUrl &url);
	bool shouldRunOnSubFrames() const;

public slots:
	void reload();

protected:
	QString checkUrlSubString(const QString &rule, const QString &urlSubString, QString generatedUrl, int position = 0) const;
	bool checkUrl(const QUrl &url, const QStringList &rules) const;

private:
	QString m_path;
	QString m_source;
	QString m_title;
	QString m_description;
	QString m_version;
	QUrl m_homePage;
	QUrl m_updateUrl;
	QIcon m_icon;
	QStringList m_excludeRules;
	QStringList m_includeRules;
	QStringList m_matchRules;
	InjectionTime m_injectionTime;
	bool m_shouldRunOnSubFrames;

};

}

#endif
