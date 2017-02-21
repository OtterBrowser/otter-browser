/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014, 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OperaSearchEnginesImporter.h"
#include "../../../core/IniSettings.h"
#include "../../../core/SearchEnginesManager.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/Utils.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>

namespace Otter
{

OperaSearchEnginesImporter::OperaSearchEnginesImporter(QObject *parent) : Importer(parent),
	m_optionsWidget(nullptr)
{
}

OperaSearchEnginesImporter::~OperaSearchEnginesImporter()
{
	if (m_optionsWidget)
	{
		m_optionsWidget->deleteLater();
	}
}

QWidget* OperaSearchEnginesImporter::getOptionsWidget()
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new QCheckBox(tr("Remove existing search engines"));
		m_optionsWidget->setChecked(true);
	}

	return m_optionsWidget;
}

QString OperaSearchEnginesImporter::getTitle() const
{
	return QString(tr("Opera search engines"));
}

QString OperaSearchEnginesImporter::getDescription() const
{
	return QString(tr("Imports search engines from Opera Browser version 12 or earlier"));
}

QString OperaSearchEnginesImporter::getVersion() const
{
	return QLatin1String("0.8");
}

QString OperaSearchEnginesImporter::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty() && QFileInfo(path).isDir())
	{
		return QDir(path).filePath(QLatin1String("search.ini"));
	}
#if !defined(Q_OS_MAC) && defined(Q_OS_UNIX)
	const QString homePath(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0));

	if (!homePath.isEmpty())
	{
		return QDir(homePath).filePath(QLatin1String(".opera/search.ini"));
	}
#endif

	return path;
}

QString OperaSearchEnginesImporter::getBrowser() const
{
	return QLatin1String("opera");
}

QUrl OperaSearchEnginesImporter::getHomePage() const
{
	return QUrl(QLatin1String("http://otter-browser.org/"));
}

QIcon OperaSearchEnginesImporter::getIcon() const
{
	return QIcon();
}

QStringList OperaSearchEnginesImporter::getFileFilters() const
{
	return QStringList(tr("Opera search engines files (search.ini)"));
}

ImportType OperaSearchEnginesImporter::getImportType() const
{
	return SearchEnginesImport;
}

bool OperaSearchEnginesImporter::import(const QString &path)
{
	IniSettings settings(getSuggestedPath(path), this);

	if (m_optionsWidget->isChecked())
	{
		SettingsManager::setValue(SettingsManager::Search_SearchEnginesOrderOption, QStringList());
	}

	const QStringList groups(settings.getGroups());
	QStringList identifiers;
	QStringList keywords(SearchEnginesManager::getSearchKeywords());

	settings.beginGroup(QLatin1String("Options"));

	const QVariant defaultEngine(settings.getValue(QLatin1String("Default Search")));
	const QList<QFileInfo> allSearchEngines(QDir(SessionsManager::getReadableDataPath(QLatin1String("searches"))).entryInfoList());

	for (int i = 0; i < allSearchEngines.count(); ++i)
	{
		identifiers.append(allSearchEngines.at(i).baseName());
	}

	for (int i = 0; i < groups.count(); ++i)
	{
		if (groups.at(i).startsWith(QLatin1String("Search Engine ")))
		{
			settings.beginGroup(groups.at(i));
		}
		else
		{
			continue;
		}

		if (settings.getValue(QLatin1String("Deleted")).toInt())
		{
			continue;
		}

		SearchEnginesManager::SearchEngineDefinition searchEngine;
		searchEngine.identifier = Utils::createIdentifier(settings.getValue(QLatin1String("UNIQUEID")).toString(), identifiers);
		searchEngine.title = settings.getValue(QLatin1String("Name")).toString();
		searchEngine.keyword = Utils::createIdentifier(settings.getValue(QLatin1String("Key")).toString(), keywords);
		searchEngine.encoding = settings.getValue(QLatin1String("Encoding")).toString();
		searchEngine.resultsUrl.url = settings.getValue(QLatin1String("URL")).toString().replace(QLatin1String("%s"), QLatin1String("{searchTerms}"));

		if (settings.getValue(QLatin1String("Is post")).toInt())
		{
			searchEngine.resultsUrl.method = QLatin1String("post");
			searchEngine.resultsUrl.enctype = QLatin1String("application/x-www-form-urlencoded");
			searchEngine.resultsUrl.parameters = QUrlQuery(settings.getValue(QLatin1String("Query")).toString().replace(QLatin1String("%s"), QLatin1String("{searchTerms}")));
		}
		else
		{
			searchEngine.resultsUrl.method = QLatin1String("get");
		}

		if (settings.getValue(QLatin1String("Suggest Protocol")).toString() == QLatin1String("JSON"))
		{
			searchEngine.suggestionsUrl.url = settings.getValue(QLatin1String("Suggest URL")).toString();
			searchEngine.suggestionsUrl.method = QLatin1String("get");
		}

		SearchEnginesManager::addSearchEngine(searchEngine);

		if (settings.getValue(QLatin1String("UNIQUEID")) == defaultEngine)
		{
			SettingsManager::setValue(SettingsManager::Search_DefaultSearchEngineOption, defaultEngine);
		}

		identifiers.append(searchEngine.identifier);
		keywords.append(searchEngine.keyword);
	}

	return true;
}

}
