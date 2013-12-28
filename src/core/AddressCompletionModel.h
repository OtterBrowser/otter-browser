#ifndef OTTER_ADDRESSCOMPLETIONMODEL_H
#define OTTER_ADDRESSCOMPLETIONMODEL_H

#include <QtCore/QAbstractListModel>

namespace Otter
{

class AddressCompletionModel : public QAbstractListModel
{
	Q_OBJECT

public:
	static AddressCompletionModel* getInstance();
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	int rowCount(const QModelIndex &index = QModelIndex()) const;

protected:
	void timerEvent(QTimerEvent *event);

protected slots:
	void optionChanged(const QString &option);
	void updateCompletion();

private:
	explicit AddressCompletionModel(QObject *parent = NULL);

	QStringList m_urls;
	int m_updateTimer;

	static AddressCompletionModel *m_instance;
};

}

#endif
