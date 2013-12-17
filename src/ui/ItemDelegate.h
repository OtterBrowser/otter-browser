#ifndef OTTER_ITEMDELEGATE_H
#define OTTER_ITEMDELEGATE_H

#include <QtWidgets/QItemDelegate>

namespace Otter
{

class ItemDelegate : public QItemDelegate
{
public:
	explicit ItemDelegate(QObject *parent = NULL);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

}

#endif
