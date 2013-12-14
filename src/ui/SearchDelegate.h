#ifndef OTTER_SEARCHDELEGATE_H
#define OTTER_SEARCHDELEGATE_H

#include <QtWidgets/QItemDelegate>

namespace Otter
{

class SearchDelegate : public QItemDelegate
{
public:
	explicit SearchDelegate(QObject *parent = NULL);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

}

#endif
