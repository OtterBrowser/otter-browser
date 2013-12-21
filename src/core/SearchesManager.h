#ifndef OTTER_SEARCHESMANAGER_H
#define OTTER_SEARCHESMANAGER_H

#include <QtCore/QFile>
#include <QtCore/QUrlQuery>
#include <QtGui/QIcon>
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
	static SearchInformation* readSearch(QIODevice *device, const QString &identifier);
	static SearchesManager* getInstance();
	static SearchInformation* getEngine(const QString &identifier);
	static QStringList getEngines();
	static QStringList getShortcuts();
	static bool writeSearch(QIODevice *device, SearchInformation *search);
	static bool setupQuery(const QString &query, const QString &engine, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body);
	static bool setEngines(QList<SearchInformation*> engines);

private:
	explicit SearchesManager(QObject *parent = NULL);

	static SearchesManager *m_instance;
	static QStringList m_order;
	static QStringList m_shortcuts;
	static QHash<QString, SearchInformation*> m_engines;

signals:
	void searchEnginesModified();
};

}

#endif
