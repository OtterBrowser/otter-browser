#include "SearchDelegate.h"

#include <QtWidgets/QApplication>

namespace Otter
{

SearchDelegate::SearchDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void SearchDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
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

	const int shortcutWidth = ((option.rect.width() > 150) ? 40 : 0);
	QRect titleReactangle = option.rect;

	if (!index.data(Qt::DecorationRole).value<QIcon>().isNull())
	{
		QRect decorationRectangle = option.rect;
		decorationRectangle.setRight(option.rect.left() + option.rect.height());

		index.data(Qt::DecorationRole).value<QIcon>().paint(painter, decorationRectangle, option.decorationAlignment);

		titleReactangle.setLeft(option.rect.left() + option.rect.height());
	}

	if (shortcutWidth > 0)
	{
		titleReactangle.setRight(titleReactangle.right() - (shortcutWidth + 5));
	}

	drawDisplay(painter, option, titleReactangle, index.data(Qt::UserRole).toString());

	if (shortcutWidth > 0)
	{
		QRect shortcutReactangle = option.rect;
		shortcutReactangle.setLeft(option.rect.right() - shortcutWidth);

		drawDisplay(painter, option, shortcutReactangle, index.data(Qt::UserRole + 2).toString());
	}
}

}
