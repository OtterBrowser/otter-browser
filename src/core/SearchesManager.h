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
};

class SearchesManager : public QObject
{
	Q_OBJECT

public:
	~SearchesManager();

	static void createInstance(QObject *parent = NULL);
	static void setupQuery(const QString &query, const SearchUrl &searchUrl, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body);
	static SearchInformation* readSearch(QIODevice *device, const QString &identifier);
	static SearchesManager* getInstance();
	static SearchInformation* getSearchEngine(const QString &identifier = QString(), bool byKeyword = false);
	static QStandardItemModel* getSearchEnginesModel();
	static QStringList getSearchEngines();
	static QStringList getSearchKeywords();
	static bool writeSearch(QIODevice *device, SearchInformation *search);
	static bool setupSearchQuery(const QString &query, const QString &engine, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body);
	static bool setSearchEngines(const QList<SearchInformation*> &engines);

protected:
	explicit SearchesManager(QObject *parent = NULL);

	void initialize();
	void updateSearchEnginesModel();

private:
	static SearchesManager *m_instance;
	static QStandardItemModel *m_searchEnginesModel;
	static QStringList m_searchEnginesOrder;
	static QStringList m_searchKeywords;
	static QHash<QString, SearchInformation*> m_searchEngines;
	static bool m_initialized;

signals:
	void searchEnginesModified();
	void searchEnginesModelModified();
};

}

#endif
