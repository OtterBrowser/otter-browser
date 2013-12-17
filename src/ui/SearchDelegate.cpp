#include "SearchDelegate.h"

#include <QtWidgets/QApplication>

namespace Otter
{

SearchDelegate::SearchDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void SearchDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
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
	QRect decorationRectangle = option.rect;
	decorationRectangle.setRight(option.rect.height());

	QRect titleReactangle = option.rect;
	titleReactangle.setLeft(option.rect.height());

	if (shortcutWidth > 0)
	{
		titleReactangle.setRight(titleReactangle.right() - (shortcutWidth + 5));
	}

	drawBackground(painter, option, index);
	drawDecoration(painter, option, decorationRectangle, qvariant_cast<QIcon>(index.data(Qt::DecorationRole)).pixmap(option.rect.height(), option.rect.height()));
	drawDisplay(painter, option, titleReactangle, index.data(Qt::UserRole).toString());

	if (shortcutWidth > 0)
	{
		QRect shortcutReactangle = option.rect;
		shortcutReactangle.setLeft(option.rect.right() - shortcutWidth);

		drawDisplay(painter, option, shortcutReactangle, index.data(Qt::UserRole + 2).toString());
	}
}

}
