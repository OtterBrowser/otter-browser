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

#include "UserScript.h"
#include "Console.h"
#include "Job.h"
#include "SessionsManager.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTextStream>

namespace Otter
{

UserScript::UserScript(const QString &path, const QUrl &url, QObject *parent) : QObject(parent),
	m_iconFetchJob(nullptr),
	m_path(path),
	m_downloadUrl(url),
	m_injectionTime(DocumentReadyTime),
	m_shouldRunOnSubFrames(true)
{
	reload();
}

void UserScript::reload()
{
	m_source.clear();
	m_title.clear();
	m_description.clear();
	m_version.clear();
	m_homePage.clear();
	m_iconUrl.clear();
	m_updateUrl.clear();
	m_icon = {};
	m_excludeRules.clear();
	m_includeRules.clear();
	m_matchRules.clear();
	m_injectionTime = DocumentReadyTime;
	m_shouldRunOnSubFrames = true;

	QFile file(m_path);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to open User Script file: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, m_path);

		return;
	}

	const QRegularExpression uriExpression(QLatin1String("^.+://.*/.*"));
	uriExpression.optimize();

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
		const QString value(line.section(QLatin1Char(' '), 1, -1));

		if (keyword == QLatin1String("description"))
		{
			m_description = value;
		}
		else if (keyword == QLatin1String("downloadURL") && m_downloadUrl.isEmpty())
		{
			m_downloadUrl = QUrl(value);
		}
		else if (keyword == QLatin1String("exclude"))
		{
			m_excludeRules.append(value);
		}
		else if (keyword == QLatin1String("homepage"))
		{
			m_homePage = QUrl(value);
		}
		else if (keyword == QLatin1String("icon"))
		{
			m_iconUrl = value;

			if (m_iconUrl.isRelative())
			{
				m_iconUrl = m_downloadUrl.resolved(m_iconUrl);
			}
		}
		else if (keyword == QLatin1String("include"))
		{
			m_includeRules.append(value);
		}
		else if (keyword == QLatin1String("match"))
		{
			line = value;

			if (uriExpression.match(line).hasMatch() && (!line.startsWith(QLatin1Char('*')) || line.at(1) == QLatin1Char(':')))
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
			m_title = value;
		}
		else if (keyword == QLatin1String("noframes"))
		{
			m_shouldRunOnSubFrames = false;
		}
		else if (keyword == QLatin1String("run-at"))
		{
			const QString injectionTime(value);

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
			m_updateUrl = QUrl(value);

			if (m_updateUrl.isRelative())
			{
				m_updateUrl = m_downloadUrl.resolved(m_updateUrl);
			}
		}
		else if (keyword == QLatin1String("version"))
		{
			m_version = value;
		}
	}

	file.close();

	if (m_title.isEmpty())
	{
		m_title = QFileInfo(file).completeBaseName();
	}

	if (m_iconUrl.isValid())
	{
		if (m_iconFetchJob && m_iconFetchJob->getUrl() != m_iconUrl)
		{
			m_iconFetchJob->cancel();
		}

		m_iconFetchJob = new IconFetchJob(m_iconUrl, this);

		connect(m_iconFetchJob, &IconFetchJob::destroyed, this, [&]()
		{
			m_iconFetchJob = nullptr;
		});
		connect(m_iconFetchJob, &IconFetchJob::jobFinished, this, [&](bool isSuccess)
		{
			if (isSuccess)
			{
				m_icon = m_iconFetchJob->getIcon();

				emit metaDataChanged();
			}
		});

		m_iconFetchJob->start();
	}

	if (!hasHeader)
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to locate header of User Script file"), Console::OtherCategory, Console::WarningLevel, m_path);
	}

	emit metaDataChanged();
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

QString UserScript::getPath() const
{
	return m_path;
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
	if (rule.isEmpty())
	{
		return urlSubString;
	}

	for (int i = 0; i < urlSubString.length(); ++i)
	{
		const QChar urlCharacter(urlSubString.at(i));
		const QChar ruleCharacter(rule.at(position));

		if (ruleCharacter == QLatin1Char('*'))
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
		else if (ruleCharacter != urlCharacter)
		{
			return {};
		}

		++position;

		generatedUrl += urlCharacter;

		if (position == rule.length())
		{
			return generatedUrl;
		}
	}

	return {};
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

QVector<UserScript*> UserScript::getUserScriptsForUrl(const QUrl &url, UserScript::InjectionTime injectionTime, bool isSubFrame)
{
	const QStringList scriptNames(AddonsManager::getAddons(Addon::UserScriptType));
	QVector<UserScript*> scripts;

	for (int i = 0; i < scriptNames.count(); ++i)
	{
		UserScript *script(AddonsManager::getUserScript(scriptNames.at(i)));

		if (script->isEnabled() && (injectionTime == AnyTime || script->getInjectionTime() == injectionTime) && (!isSubFrame || script->shouldRunOnSubFrames()) && script->isEnabledForUrl(url))
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

	bool isEnabled(m_includeRules.isEmpty() && m_matchRules.isEmpty());

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

bool UserScript::canRemove() const
{
	return true;
}

bool UserScript::checkUrl(const QUrl &url, const QStringList &rules) const
{
	for (int i = 0; i < rules.count(); ++i)
	{
		QString rule(rules.at(i));

		if (rule.startsWith(QLatin1Char('/')) && rule.endsWith(QLatin1Char('/')))
		{
			return QRegularExpression(rule.mid(1, rule.length() - 2)).match(url.url()).hasMatch();
		}

		if (rule.contains(QLatin1String(".tld"), Qt::CaseInsensitive))
		{
			rule.replace(QLatin1String(".tld"), Utils::getTopLevelDomain(url), Qt::CaseInsensitive);
		}

		bool useExactMatch(true);

		if (rule.endsWith(QLatin1Char('*')))
		{
			useExactMatch = false;
			rule = rule.left(rule.length() - 1);
		}

		const QString result(checkUrlSubString(rule, url.url(), {}));

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

bool UserScript::remove()
{
	const QFileInfo fileInformation(m_path);

	if (m_path.isEmpty() || !fileInformation.exists())
	{
		return false;
	}

	const QString basename(fileInformation.baseName());

	if (basename.isEmpty() || fileInformation.dir().dirName() != basename || !fileInformation.canonicalFilePath().startsWith(QFileInfo(SessionsManager::getWritableDataPath(QLatin1String("scripts"))).canonicalPath()))
	{
		return false;
	}

	return fileInformation.dir().removeRecursively();
}

}
