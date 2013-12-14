#ifndef OTTER_SEARCHWIDGET_H
#define OTTER_SEARCHWIDGET_H

#include <QtWidgets/QComboBox>

namespace Otter
{

class SearchWidget : public QComboBox
{
	Q_OBJECT

public:
	explicit SearchWidget(QWidget *parent = NULL);

protected slots:
	void currentSearchChanged(int index);
	void queryChanged(const QString &query);
	void sendRequest();

private:
	QString m_query;

signals:
	void requestedSearch(QString query, QString engine);
};

}

#endif
