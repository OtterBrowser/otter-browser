#ifndef OTTER_SEARCHWIDGET_H
#define OTTER_SEARCHWIDGET_H

#include <QtGui/QStandardItemModel>
#include <QtWidgets/QComboBox>

namespace Otter
{

class SearchWidget : public QComboBox
{
	Q_OBJECT

public:
	explicit SearchWidget(QWidget *parent = NULL);

protected slots:
	void optionChanged(const QString &option);
	void currentSearchChanged(int index);
	void queryChanged(const QString &query);
	void sendRequest();

private:
	QStandardItemModel *m_model;
	QString m_query;
	int m_index;

signals:
	void requestedSearch(QString query, QString engine);
};

}

#endif
