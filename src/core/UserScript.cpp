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
#include "Console.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>

namespace Otter
{

UserScript::UserScript(const QString &path, QObject *parent) : Addon(parent),
	m_path(path),
	m_injectionTime(DocumentReadyTime),
	m_shouldRunOnSubFrames(true)
{
	QFile file(path);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to open user script file: %1").arg(file.errorString()), Otter::OtherMessageCategory, ErrorMessageLevel, path);

		return;
	}

	QTextStream stream(&file);
	bool hasHeader(false);

	while (!stream.atEnd())
	{
		QString line(stream.readLine().trimmed());

		if (!line.startsWith(QLatin1String("//")))
		{
			continue;
		}

		line = line.mid(2).trimmed();

		if (line.startsWith(QLatin1String("==UserScript==")))
		{
			hasHeader = true;

			continue;
		}

		if (!line.startsWith(QLatin1Char('@')))
		{
			continue;
		}

		line = line.mid(1);

		const QString keyword(line.section(QLatin1Char(' '), 0, 0));

		if (keyword == QLatin1String("description"))
		{
			m_description = line.section(QLatin1Char(' '), 0, -1);
		}
		else if (keyword == QLatin1String("exclude"))
		{
			m_excludeRules.append(line.section(QLatin1Char(' '), 0, -1));
		}
		else if (keyword == QLatin1String("include"))
		{
			m_includeRules.append(line.section(QLatin1Char(' '), 0, -1));
		}
		else if (keyword == QLatin1String("match"))
		{
			m_matchRules.append(line.section(QLatin1Char(' '), 0, -1));
		}
		else if (keyword == QLatin1String("name"))
		{
			m_title = line.section(QLatin1Char(' '), 0, -1);
		}
		else if (keyword == QLatin1String("noframes"))
		{
			m_shouldRunOnSubFrames = true;
		}
		else if (keyword == QLatin1String("run-at"))
		{
			const QString injectionTime(line.section(QLatin1Char(' '), 0, -1));

			if (injectionTime == QLatin1String("document-start"))
			{
				m_injectionTime = DocumentCreationTime;
			}
			else if (injectionTime == QLatin1String("document-idle"))
			{
				m_injectionTime = DeferredTime;
			}
			else
			{
				m_injectionTime = DocumentReadyTime;
			}
		}
		else if (keyword == QLatin1String("updateURL"))
		{
			m_updateUrl = QUrl(line.section(QLatin1Char(' '), 0, -1));
		}
		else if (keyword == QLatin1String("version"))
		{
			m_version = line.section(QLatin1Char(' '), 0, -1);
		}
	}

	file.close();

	if (!hasHeader)
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to locate header of user script file"), Otter::OtherMessageCategory, WarningMessageLevel, path);
	}
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
	return QUrl();
}

QUrl UserScript::getUpdateUrl() const
{
	return m_updateUrl;
}

QIcon UserScript::getIcon() const
{
	return m_icon;
}

QStringList UserScript::getExcludeRules() const
{
	return m_excludeRules;
}

QStringList UserScript::getIncludeRules() const
{
	return m_includeRules;
}

QStringList UserScript::getMatchRules() const
{
	return m_matchRules;
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
