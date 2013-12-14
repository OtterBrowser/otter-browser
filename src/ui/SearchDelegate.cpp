#include "SearchDelegate.h"

namespace Otter
{

SearchDelegate::SearchDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void SearchDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QRect textReactangle = option.rect;
	textReactangle.setLeft(option.rect.height());

	QRect decorationRectangle = option.rect;
	decorationRectangle.setRight(option.rect.height());

	drawBackground(painter, option, index);
	drawDecoration(painter, option, decorationRectangle, qvariant_cast<QIcon>(index.data(Qt::DecorationRole)).pixmap(option.rect.height(), option.rect.height()));
	drawDisplay(painter, option, textReactangle, index.data(Qt::UserRole).toString());
}

}
