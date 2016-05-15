/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

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

	if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator"))
	{
		QStyleOptionFrame frameOption;
		frameOption.palette = option.palette;
		frameOption.rect = option.rect;
		frameOption.state = option.state;
		frameOption.frameShape = QFrame::HLine;

		QApplication::style()->drawControl(QStyle::CE_ShapedFrame, &frameOption, painter, 0);

		return;
	}

	const bool configureEntry(index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("configure"));
	const int shortcutWidth((!configureEntry && option.rect.width() > 150) ? 40 : 0);
	QRect titleRectangle(option.rect);

	if (!index.data(Qt::DecorationRole).value<QIcon>().isNull())
	{
		QRect decorationRectangle(option.rect);
		decorationRectangle.setRight(option.rect.height());
		decorationRectangle = decorationRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

		index.data(Qt::DecorationRole).value<QIcon>().paint(painter, decorationRectangle, option.decorationAlignment);
	}

	titleRectangle.setLeft(option.rect.height());

	if (shortcutWidth > 0)
	{
		titleRectangle.setRight(titleRectangle.right() - (shortcutWidth + 5));
	}

	drawDisplay(painter, option, titleRectangle, index.data(configureEntry ? Qt::DisplayRole : Qt::UserRole).toString());

	if (shortcutWidth > 0)
	{
		QRect shortcutReactangle(option.rect);
		shortcutReactangle.setLeft(option.rect.right() - shortcutWidth);

		drawDisplay(painter, option, shortcutReactangle, index.data(Qt::UserRole + 2).toString());
	}
}

QSize SearchDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size(index.data(Qt::SizeHintRole).toSize());

	if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator"))
	{
		return size;
	}

	size.setHeight(option.fontMetrics.height() * 1.25);

	return size;
}

}
