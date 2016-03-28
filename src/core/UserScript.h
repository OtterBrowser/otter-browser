/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

	explicit UserScript(const QString &path, QObject *parent = NULL);

	QString getTitle() const;
	QString getDescription() const;
	QString getVersion() const;
	QString getSource();
	QUrl getHomePage() const;
	QUrl getUpdateUrl() const;
	QIcon getIcon() const;
	InjectionTime getInjectionTime() const;
	bool shouldRunOnSubFrames() const;

private:
	QString m_path;
	QString m_source;
	QString m_title;
	QString m_description;
	QString m_version;
	QUrl m_homePage;
	QUrl m_updateUrl;
	QIcon  m_icon;
	InjectionTime m_injectionTime;
	bool m_shouldRunOnSubFrames;

};

}

#endif
