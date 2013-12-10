#ifndef OTTER_PROGRESSBARDELEGATE_H
#define OTTER_PROGRESSBARDELEGATE_H

#include <QtWidgets/QStyledItemDelegate>

namespace Otter
{

class ProgressBarDelegate : public QStyledItemDelegate
{
public:
	explicit ProgressBarDelegate(QObject *parent);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

}

#endif
