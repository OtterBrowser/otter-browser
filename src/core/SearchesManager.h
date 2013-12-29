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
	QString shortcut;
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
	static void createInstance(QObject *parent = NULL);
	static void setupQuery(const QString &query, const SearchUrl &searchUrl, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body);
	static SearchInformation* readSearch(QIODevice *device, const QString &identifier);
	static SearchesManager* getInstance();
	static SearchInformation* getSearchEngine(const QString &identifier);
	static QStandardItemModel* getSearchEnginesModel();
	static QStringList getSearchEngines();
	static QStringList getSearchShortcuts();
	static bool writeSearch(QIODevice *device, SearchInformation *search);
	static bool setupSearchQuery(const QString &query, const QString &engine, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body);
	static bool setSearchEngines(const QList<SearchInformation*> &engines);

protected:
	void updateSearchEnginesModel();

private:
	explicit SearchesManager(QObject *parent = NULL);

	static SearchesManager *m_instance;
	static QStandardItemModel *m_searchEnginesModel;
	static QStringList m_searchEnginesOrder;
	static QStringList m_searchShortcuts;
	static QHash<QString, SearchInformation*> m_searchEngines;

signals:
	void searchEnginesModified();
};

}

#endif
