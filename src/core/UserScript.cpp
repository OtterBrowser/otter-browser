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

#include "UserScript.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>

namespace Otter
{

UserScript::UserScript(const QString &path, QObject *parent) : Addon(parent),
	m_path(path),
	m_injectionTime(DocumentReadyTime),
	m_shouldRunOnSubFrames(true)
{
}

QString UserScript::getTitle() const
{
	return m_title;
}

QString UserScript::getDescription() const
{
	return m_description;
}

QString UserScript::getVersion() const
{
	return m_version;
}

QString UserScript::getSource()
{
	if (m_source.isEmpty())
	{
		QFile file(m_path);

		if (file.open(QIODevice::ReadOnly))
		{
			QTextStream stream(&file);

			m_source = stream.readAll();

			file.close();
		}
	}

	return m_source;
}

QUrl UserScript::getHomePage() const
{
	return m_homePage;
}

QUrl UserScript::getUpdateUrl() const
{
	return m_updateUrl;
}

QIcon UserScript::getIcon() const
{
	return m_icon;
}

UserScript::InjectionTime UserScript::getInjectionTime() const
{
	return m_injectionTime;
}

bool UserScript::isEnabledForUrl(const QUrl &url)
{
	Q_UNUSED(url)

///TODO

	return true;
}

bool UserScript::shouldRunOnSubFrames() const
{
	return m_shouldRunOnSubFrames;
}

}
