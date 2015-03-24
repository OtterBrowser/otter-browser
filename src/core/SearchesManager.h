/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_SEARCHESMANAGER_H
#define OTTER_SEARCHESMANAGER_H

#include <QtCore/QFile>
#include <QtCore/QUrlQuery>
#include <QtGui/QIcon>
#include <QtGui/QStandardItemModel>
#include <QtNetwork/QNetworkAccessManager>

namespace Otter
{

struct SearchUrl
{
	QString url;
	QString enctype;
	QString method;
	QUrlQuery parameters;

	SearchUrl() : method(QLatin1String("get")) {}
};

struct SearchInformation
{
	QString identifier;
	QString title;
	QString description;
	QString keyword;
	QString encoding;
	QString selfUrl;
	SearchUrl resultsUrl;
	SearchUrl suggestionsUrl;
	QIcon icon;

	SearchInformation() : encoding(QLatin1String("UTF-8")) {}
};

class SearchesManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void loadSearchEngines();
	static void addSearchEngine(const SearchInformation &engine, bool isDefault = false);
	static void setupQuery(const QString &query, const SearchUrl &searchUrl, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body);
	static SearchInformation loadSearchEngine(QIODevice *device, const QString &identifier);
	static SearchesManager* getInstance();
	static QStandardItemModel* getSearchEnginesModel();
	static SearchInformation getSearchEngine(const QString &identifier = QString(), bool byKeyword = false);
	static QStringList getSearchEngines();
	static QStringList getSearchKeywords();
	static bool saveSearchEngine(const SearchInformation &engine);
	static bool setupSearchQuery(const QString &query, const QString &engine, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body);

protected:
	explicit SearchesManager(QObject *parent = NULL);

	static void initialize();
	static void updateSearchEnginesModel();

protected slots:
	void optionChanged(const QString &key);

private:
	static SearchesManager *m_instance;
	static QStandardItemModel *m_searchEnginesModel;
	static QStringList m_searchEnginesOrder;
	static QStringList m_searchKeywords;
	static QHash<QString, SearchInformation> m_searchEngines;
	static bool m_isInitialized;

signals:
	void searchEnginesModified();
	void searchEnginesModelModified();
};

}

#endif
