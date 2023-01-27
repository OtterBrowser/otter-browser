/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014, 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2023 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OperaSearchEnginesImportDataExchanger.h"
#include "../../../core/IniSettings.h"
#include "../../../core/SearchEnginesManager.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/Utils.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>

namespace Otter
{

OperaSearchEnginesImportDataExchanger::OperaSearchEnginesImportDataExchanger(QObject *parent) : ImportDataExchanger(parent),
	m_optionsWidget(nullptr)
{
}

QWidget* OperaSearchEnginesImportDataExchanger::createOptionsWidget(QWidget *parent)
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new QCheckBox(tr("Remove existing search engines"), parent);
	}

	return m_optionsWidget;
}

QString OperaSearchEnginesImportDataExchanger::getName() const
{
	return QLatin1String("opera-search-engines");
}

QString OperaSearchEnginesImportDataExchanger::getTitle() const
{
	return tr("Import Opera search engines");
}

QString OperaSearchEnginesImportDataExchanger::getDescription() const
{
	return tr("Imports search engines from Opera Browser version 12 or earlier");
}

QString OperaSearchEnginesImportDataExchanger::getVersion() const
{
	return QLatin1String("0.8");
}

QString OperaSearchEnginesImportDataExchanger::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty())
	{
		if (QFileInfo(path).isDir())
		{
			return QDir(path).filePath(QLatin1String("search.ini"));
		}

		return path;
	}

#if !defined(Q_OS_DARWIN) && defined(Q_OS_UNIX)
	const QString homePath(Utils::getStandardLocation(QStandardPaths::HomeLocation));

	if (!homePath.isEmpty())
	{
		return QDir(homePath).filePath(QLatin1String(".opera/search.ini"));
	}
#endif

	return path;
}

QString OperaSearchEnginesImportDataExchanger::getGroup() const
{
	return QLatin1String("opera");
}

QUrl OperaSearchEnginesImportDataExchanger::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList OperaSearchEnginesImportDataExchanger::getFileFilters() const
{
	return {tr("Opera search engines files (search.ini)")};
}

DataExchanger::ExchangeType OperaSearchEnginesImportDataExchanger::getExchangeType() const
{
	return SearchEnginesExchange;
}

bool OperaSearchEnginesImportDataExchanger::hasOptions() const
{
	return true;
}

bool OperaSearchEnginesImportDataExchanger::importData(const QString &path)
{
	IniSettings settings(getSuggestedPath(path), this);

	if (settings.hasError())
	{
		emit exchangeFinished(SearchEnginesExchange, FailedOperation, 0);

		return false;
	}

	emit exchangeStarted(SearchEnginesExchange, -1);

	if (m_optionsWidget->isChecked())
	{
		SettingsManager::setOption(SettingsManager::Search_SearchEnginesOrderOption, QStringList());
	}

	const QStringList groups(settings.getGroups());
	QStringList keywords(SearchEnginesManager::getSearchKeywords());

	settings.beginGroup(QLatin1String("Options"));

	const QVariant defaultSearchEngine(settings.getValue(QLatin1String("Default Search")));
	int totalAmount(0);

	for (int i = 0; i < groups.count(); ++i)
	{
		if (!groups.at(i).startsWith(QLatin1String("Search Engine ")))
		{
			continue;
		}

		settings.beginGroup(groups.at(i));

		if (settings.getValue(QLatin1String("Deleted")).toInt())
		{
			continue;
		}

		SearchEnginesManager::SearchEngineDefinition searchEngine;
		searchEngine.title = settings.getValue(QLatin1String("Name")).toString();
		searchEngine.keyword = Utils::createIdentifier(settings.getValue(QLatin1String("Key")).toString(), keywords);
		searchEngine.encoding = settings.getValue(QLatin1String("Encoding")).toString();
		searchEngine.resultsUrl.url = settings.getValue(QLatin1String("URL")).toString().replace(QLatin1String("%s"), QLatin1String("{searchTerms}"));
		searchEngine.identifier = searchEngine.createIdentifier();

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

		if (SearchEnginesManager::addSearchEngine(searchEngine))
		{
			if (m_optionsWidget->isChecked() && settings.getValue(QLatin1String("UNIQUEID")) == defaultSearchEngine)
			{
				SettingsManager::setOption(SettingsManager::Search_DefaultSearchEngineOption, searchEngine.identifier);
				SettingsManager::setOption(SettingsManager::Search_DefaultQuickSearchEngineOption, searchEngine.identifier);
			}

			keywords.append(searchEngine.keyword);

			++totalAmount;
		}
	}

	emit exchangeFinished(SearchEnginesExchange, SuccessfullOperation, totalAmount);

	return true;
}

}
