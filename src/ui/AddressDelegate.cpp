/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "AddressDelegate.h"

#include <QtWidgets/QApplication>

namespace Otter
{

AddressDelegate::AddressDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void AddressDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	drawBackground(painter, option, index);

	QRect titleReactangle = option.rect;

	if (!index.data(Qt::DecorationRole).value<QIcon>().isNull())
	{
		QRect decorationRectangle = option.rect;
		decorationRectangle.setRight(option.rect.height());
		decorationRectangle = decorationRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

		index.data(Qt::DecorationRole).value<QIcon>().paint(painter, decorationRectangle, option.decorationAlignment);
	}

	titleReactangle.setLeft(option.rect.height());

	drawDisplay(painter, option, titleReactangle, index.data(Qt::DisplayRole).toString());
}

}
