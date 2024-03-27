/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_SEARCHENGINESMANAGER_H
#define OTTER_SEARCHENGINESMANAGER_H

#include "Job.h"
#include "Utils.h"

#include <QtCore/QUrlQuery>
#include <QtGui/QIcon>
#include <QtGui/QStandardItemModel>
#include <QtNetwork/QNetworkAccessManager>

namespace Otter
{

class SearchEnginesManager final : public QObject
{
	Q_OBJECT

public:
	enum SearchEngineRole
	{
		TitleRole = Qt::UserRole,
		IdentifierRole,
		KeywordRole
	};

	struct SearchQuery final
	{
		QByteArray body;
		QNetworkRequest request;
		QNetworkAccessManager::Operation method = QNetworkAccessManager::PostOperation;

		bool isValid() const
		{
			return (!body.isEmpty() || !request.url().isEmpty());
		}
	};

	struct SearchUrl final
	{
		QString url;
		QString enctype;
		QString method = QLatin1String("get");
		QUrlQuery parameters;
	};

	struct SearchEngineDefinition final
	{
		QString identifier;
		QString title;
		QString description;
		QString keyword;
		QString encoding = QLatin1String("UTF-8");
		QUrl formUrl;
		QUrl iconUrl;
		QUrl selfUrl;
		SearchUrl resultsUrl;
		SearchUrl suggestionsUrl;
		QIcon icon;

		QString createIdentifier(const QStringList &exclude = {}) const
		{
			return Utils::createIdentifier(QUrl(resultsUrl.url).host(), (exclude.isEmpty() ? SearchEnginesManager::getSearchEngines() : exclude));
		}

		bool isValid() const
		{
			return !identifier.isEmpty();
		}
	};

	static void createInstance();
	static void loadSearchEngines();
	static SearchQuery setupQuery(const QString &query, const SearchUrl &searchUrl);
	static SearchQuery setupSearchQuery(const QString &query, const QString &identifier);
	static SearchEngineDefinition loadSearchEngine(QIODevice *device, const QString &identifier, bool checkKeyword = true);
	static SearchEnginesManager* getInstance();
	static QStandardItemModel* getSearchEnginesModel();
	static SearchEngineDefinition getSearchEngine(const QString &identifier = {}, bool byKeyword = false);
	static QStringList getSearchEngines();
	static QStringList getSearchKeywords();
	static bool hasSearchEngine(const QUrl &url);
	static bool addSearchEngine(const SearchEngineDefinition &searchEngine);
	static bool saveSearchEngine(const SearchEngineDefinition &searchEngine);

protected:
	explicit SearchEnginesManager(QObject *parent);

	static void ensureInitialized();
	static void updateSearchEnginesModel();
	static void updateSearchEnginesOptions();

private:
	static SearchEnginesManager *m_instance;
	static QStandardItemModel *m_searchEnginesModel;
	static QStringList m_searchEnginesOrder;
	static QStringList m_searchKeywords;
	static QHash<QString, SearchEngineDefinition> m_searchEngines;
	static bool m_isInitialized;

signals:
	void searchEnginesModified();
	void searchEnginesModelModified();
};

class SearchEngineFetchJob final : public FetchJob
{
	Q_OBJECT

public:
	explicit SearchEngineFetchJob(const QUrl &url, const QString &identifier = {}, bool saveSearchEngine = true, QObject *parent = nullptr);

	SearchEnginesManager::SearchEngineDefinition getSearchEngine() const;

protected:
	void handleSuccessfulReply(QNetworkReply *reply) override;

private:
	SearchEnginesManager::SearchEngineDefinition m_searchEngine;
	bool m_needsToSaveSearchEngine;
};

}

#endif
