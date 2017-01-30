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

#include "UserScript.h"
#include "Console.h"
#include "ThemesManager.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTextStream>

namespace Otter
{

UserScript::UserScript(const QString &path, QObject *parent) : Addon(parent),
	m_path(path),
	m_icon(ThemesManager::getIcon(QLatin1String("addon-user-script"), false)),
	m_injectionTime(DocumentReadyTime),
	m_shouldRunOnSubFrames(true)
{
	reload();
}

void UserScript::reload()
{
	m_source = QString();
	m_title = QString();
	m_description = QString();
	m_version = QString();
	m_homePage = QUrl();
	m_updateUrl = QUrl();
	m_icon = ThemesManager::getIcon(QLatin1String("addon-user-script"), false);
	m_excludeRules = QStringList();
	m_includeRules = QStringList();
	m_matchRules = QStringList();
	m_injectionTime = DocumentReadyTime;
	m_shouldRunOnSubFrames = true;

	QFile file(m_path);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to open User Script file: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, m_path);

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
			m_description = line.section(QLatin1Char(' '), 1, -1);
		}
		else if (keyword == QLatin1String("exclude"))
		{
			m_excludeRules.append(line.section(QLatin1Char(' '), 1, -1));
		}
		else if (keyword == QLatin1String("homepage"))
		{
			m_homePage = QUrl(line.section(QLatin1Char(' '), 1, -1));
		}
		else if (keyword == QLatin1String("include"))
		{
			m_includeRules.append(line.section(QLatin1Char(' '), 1, -1));
		}
		else if (keyword == QLatin1String("match"))
		{
			line = line.section(QLatin1Char(' '), 1, -1);

			if (QRegularExpression(QLatin1String("^.+://.*/.*")).match(line).hasMatch() && (!line.startsWith(QLatin1Char('*')) || line.at(1) == QLatin1Char(':')))
			{
				const QString scheme(line.left(line.indexOf(QLatin1String("://"))));

				if (scheme == QLatin1String("*") || scheme == QLatin1String("http") || scheme == QLatin1String("https") || scheme == QLatin1String("file") || scheme == QLatin1String("ftp"))
				{
					const QString pathAndDomain(line.mid(line.indexOf(QLatin1String("://")) + 3));
					const QString domain(pathAndDomain.left(pathAndDomain.indexOf(QLatin1Char('/'))));

					if (domain.indexOf(QLatin1Char('*')) < 0 || (domain.indexOf(QLatin1Char('*')) == 0 && (domain.length() == 1 || (domain.length() > 1 && domain.at(1) == QLatin1Char('.')))))
					{
						m_matchRules.append(line);

						continue;
					}
				}
			}

			Console::addMessage(QCoreApplication::translate("main", "Invalid match rule for User Script: %1").arg(line), Console::OtherCategory, Console::ErrorLevel, m_path);
		}
		else if (keyword == QLatin1String("name"))
		{
			m_title = line.section(QLatin1Char(' '), 1, -1);
		}
		else if (keyword == QLatin1String("noframes"))
		{
			m_shouldRunOnSubFrames = true;
		}
		else if (keyword == QLatin1String("run-at"))
		{
			const QString injectionTime(line.section(QLatin1Char(' '), 1, -1));

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
			m_updateUrl = QUrl(line.section(QLatin1Char(' '), 1, -1));
		}
		else if (keyword == QLatin1String("version"))
		{
			m_version = line.section(QLatin1Char(' '), 1, -1);
		}
	}

	file.close();

	if (m_title.isEmpty())
	{
		m_title = QFileInfo(file).completeBaseName();
	}

	if (!hasHeader)
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to locate header of User Script file"), Console::OtherCategory, Console::WarningLevel, m_path);
	}
}

QString UserScript::getName() const
{
	return QFileInfo(m_path).completeBaseName();
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

QString UserScript::checkUrlSubString(const QString &rule, const QString &urlSubString, QString generatedUrl, int position) const
{
	for (int i = 0; i < urlSubString.length(); ++i)
	{
		const QChar character(urlSubString.at(i));

		if (rule[position] == QLatin1Char('*'))
		{
			const QString wildcardString(urlSubString.mid(i));

			for (int j = 1; j < wildcardString.length(); ++j)
			{
				const QString result(checkUrlSubString(rule, urlSubString.right(wildcardString.length() - j), generatedUrl + wildcardString.left(j), (position + 1)));

				if (!result.isEmpty())
				{
					return result;
				}
			}
		}
		else if (character != rule[position])
		{
			return QString();
		}

		++position;

		generatedUrl += character;

		if (position == rule.length())
		{
			return generatedUrl;
		}
	}

	return QString();
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

QList<UserScript*> UserScript::getUserScriptsForUrl(const QUrl &url, bool shouldRunOnSubFrames, UserScript::InjectionTime injectionTime)
{
	const QStringList scriptNames(AddonsManager::getUserScripts());
	QList<UserScript*> scripts;

	for (int i = 0 ; i < scriptNames.count(); ++i)
	{
		UserScript *script(AddonsManager::getUserScript(scriptNames.at(i)));

		if (script->isEnabled() && script->shouldRunOnSubFrames() == shouldRunOnSubFrames && (injectionTime == AnyTime || script->getInjectionTime() == injectionTime) && script->isEnabledForUrl(url))
		{
			scripts.append(script);
		}
	}

	return scripts;
}

UserScript::InjectionTime UserScript::getInjectionTime() const
{
	return m_injectionTime;
}

Addon::AddonType UserScript::getType() const
{
	return Addon::UserScriptType;
}

bool UserScript::isEnabledForUrl(const QUrl &url)
{
	if (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https") && url.scheme() != QLatin1String("file") && url.scheme() != QLatin1String("ftp") && url.scheme() != QLatin1String("about"))
	{
		return false;
	}

	bool isEnabled(!(m_includeRules.length() > 0 || m_matchRules.length() > 0));

	if (checkUrl(url, m_matchRules))
	{
		isEnabled = true;
	}

	if (!isEnabled && checkUrl(url, m_includeRules))
	{
		isEnabled = true;
	}

	if (isEnabled && checkUrl(url, m_excludeRules))
	{
		isEnabled = false;
	}

	return isEnabled;
}

bool UserScript::checkUrl(const QUrl &url, const QStringList &rules) const
{
	for (int i = 0; i < rules.length(); ++i)
	{
		QString rule(rules[i]);

		if (rule.startsWith(QLatin1Char('/')) && rule.endsWith(QLatin1Char('/')))
		{
			return QRegularExpression(rule.mid(1, rule.length() - 2)).match(url.url()).hasMatch();
		}

		if (rule.contains(QLatin1String(".tld"), Qt::CaseInsensitive))
		{
			rule.replace(QLatin1String(".tld"), url.topLevelDomain(), Qt::CaseInsensitive);
		}

		bool useExactMatch(true);

		if (rule.endsWith(QLatin1Char('*')))
		{
			useExactMatch = false;
			rule = rule.left(rule.length() - 1);
		}

		const QString result(checkUrlSubString(rule, url.url(), QString()));

		if (!result.isEmpty() && ((useExactMatch && url.url() == result) || (!useExactMatch && url.url().startsWith(result))))
		{
			return true;
		}
	}

	return false;
}

bool UserScript::shouldRunOnSubFrames() const
{
	return m_shouldRunOnSubFrames;
}

}
