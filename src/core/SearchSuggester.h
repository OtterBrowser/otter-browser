#ifndef OTTER_SEARCHSUGGESTER_H
#define OTTER_SEARCHSUGGESTER_H

#include <QtCore/QObject>
#include <QtGui/QStandardItemModel>
#include <QtNetwork/QNetworkAccessManager>

namespace Otter
{

struct SearchSuggestion
{
	QString completion;
	QString description;
	QString url;
};

class NetworkAccessManager;

class SearchSuggester : public QObject
{
	Q_OBJECT

public:
	explicit SearchSuggester(const QString &engine, QObject *parent = NULL);

	QStandardItemModel* getModel();

public slots:
	void setEngine(const QString &engine);
	void setQuery(const QString &query);

protected slots:
	void replyFinished(QNetworkReply *reply);

private:
	NetworkAccessManager *m_networkAccessManager;
	QNetworkReply *m_currentReply;
	QStandardItemModel *m_model;
	QString m_engine;
	QString m_query;

signals:
	void suggestionsChanged(QList<SearchSuggestion> suggestions);
};

}

#endif
