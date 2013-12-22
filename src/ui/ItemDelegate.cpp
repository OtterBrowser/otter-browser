#include "ItemDelegate.h"

#include <QtWidgets/QApplication>

namespace Otter
{

ItemDelegate::ItemDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	drawBackground(painter, option, index);

	if (index.data(Qt::AccessibleDescriptionRole).toString() == "separator")
	{
		QStyleOptionFrame frameOption;
		frameOption.palette = option.palette;
		frameOption.rect = option.rect;
		frameOption.state = option.state;
		frameOption.frameShape = QFrame::HLine;

		QApplication::style()->drawControl(QStyle::CE_ShapedFrame, &frameOption, painter, 0);

		return;
	}

	QRect titleReactangle = option.rect;

	if (!index.data(Qt::DecorationRole).value<QIcon>().isNull())
	{
		QRect decorationRectangle = option.rect;
		decorationRectangle.setRight(option.rect.left() + option.rect.height());
		decorationRectangle = decorationRectangle.marginsRemoved(QMargins(1, 1, 1, 1));

		index.data(Qt::DecorationRole).value<QIcon>().paint(painter, decorationRectangle);

		titleReactangle.setLeft(option.rect.left() + option.rect.height());
	}

	QStyleOptionViewItem titleOption = option;
	titleOption.font = index.data(Qt::FontRole).value<QFont>();

	drawDisplay(painter, titleOption, titleReactangle, index.data(Qt::DisplayRole).toString());
}

}
