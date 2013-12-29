#ifndef OTTER_SEARCHWIDGET_H
#define OTTER_SEARCHWIDGET_H

#include <QtWidgets/QComboBox>
#include <QtWidgets/QCompleter>

namespace Otter
{

class SearchSuggester;

class SearchWidget : public QComboBox
{
	Q_OBJECT

public:
	explicit SearchWidget(QWidget *parent = NULL);

	void hidePopup();
	QString getCurrentSearchEngine() const;

public slots:
	void setCurrentSearchEngine(const QString &engine = QString());

protected:
	void wheelEvent(QWheelEvent *event);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void currentSearchEngineChanged(int index);
	void searchEngineSelected(int index);
	void queryChanged(const QString &query);
	void sendRequest(const QString &query = QString());

private:
	QCompleter *m_completer;
	SearchSuggester *m_suggester;
	QString m_query;
	int m_index;
	bool m_sendRequest;

signals:
	void requestedSearch(QString query, QString engine);
};

}

#endif
