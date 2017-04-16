/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QFile>
#include <QtCore/QUrlQuery>
#include <QtGui/QIcon>
#include <QtGui/QStandardItemModel>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

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

	struct SearchUrl
	{
		QString url;
		QString enctype;
		QString method = QLatin1String("get");
		QUrlQuery parameters;
	};

	struct SearchEngineDefinition
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

		bool isValid() const
		{
			return !identifier.isEmpty();
		}
	};

	static void createInstance(QObject *parent = nullptr);
	static void loadSearchEngines();
	static void addSearchEngine(const SearchEngineDefinition &searchEngine);
	static void setupQuery(const QString &query, const SearchUrl &searchUrl, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body);
	static SearchEngineDefinition loadSearchEngine(QIODevice *device, const QString &identifier, bool checkKeyword = true);
	static SearchEnginesManager* getInstance();
	static QStandardItemModel* getSearchEnginesModel();
	static SearchEngineDefinition getSearchEngine(const QString &identifier = QString(), bool byKeyword = false);
	static QStringList getSearchEngines();
	static QStringList getSearchKeywords();
	static bool hasSearchEngine(const QUrl &url);
	static bool saveSearchEngine(const SearchEngineDefinition &searchEngine);
	static bool setupSearchQuery(const QString &query, const QString &identifier, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body);

protected:
	explicit SearchEnginesManager(QObject *parent = nullptr);

	static void ensureInitialized();
	static void updateSearchEnginesModel();
	static void updateSearchEnginesOptions();

protected slots:
	void handleOptionChanged(int identifier);

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

class SearchEngineFetchJob : public QObject
{
	Q_OBJECT

public:
	explicit SearchEngineFetchJob(const QUrl &url, const QString &identifier = QString(), bool saveSearchEngine = true, QObject *parent = nullptr);
	~SearchEngineFetchJob();

	SearchEnginesManager::SearchEngineDefinition getSearchEngine() const;

public slots:
	void cancel();

protected slots:
	void handleRequestFailed();
	void handleRequestFinished();

private:
	QNetworkReply *m_reply;
	SearchEnginesManager::SearchEngineDefinition m_searchEngine;
	bool m_isFetchingIcon;
	bool m_needsToSaveSearchEngine;

signals:
	void jobFinished(bool isSuccess);
};

}

#endif
