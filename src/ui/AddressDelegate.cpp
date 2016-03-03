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
#include "../core/Utils.h"

namespace Otter
{

AddressDelegate::AddressDelegate(bool isAddressField, QObject *parent) : QItemDelegate(parent),
	m_isAddressField(isAddressField)
{
}

void AddressDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	drawBackground(painter, option, index);

	QRect titleRectangle = option.rect;
	titleRectangle.setLeft(m_isAddressField ? 33 : (option.rect.height() + 1));

	QRect decorationRectangle = option.rect;
	decorationRectangle.setRight(titleRectangle.left());
	decorationRectangle = decorationRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

	QIcon icon(index.data(Qt::DecorationRole).value<QIcon>());

	if (icon.isNull())
	{
		icon = Utils::getIcon(QLatin1String("tab"));
	}

	icon.paint(painter, decorationRectangle, option.decorationAlignment);

	const QString url(index.data(Qt::DisplayRole).toString());

	if (m_isAddressField)
	{
		QStyleOptionViewItem mutableOption(option);
		mutableOption.palette.setColor(QPalette::Text, option.palette.color(QPalette::Link));

		drawDisplay(painter, mutableOption, titleRectangle, url);

		if (!index.data(Qt::UserRole + 1).isNull())
		{
			const int urlLength = option.fontMetrics.width(url + QLatin1Char(' '));

			if (urlLength < titleRectangle.width())
			{
				titleRectangle.setLeft(titleRectangle.left() + urlLength);

				drawDisplay(painter, option, titleRectangle, QLatin1String("- ") + index.data(Qt::UserRole + 1).toString());
			}
		}
	}
	else
	{
		drawDisplay(painter, option, titleRectangle, url);
	}
}

QSize AddressDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size = index.data(Qt::SizeHintRole).toSize();
	size.setHeight(option.fontMetrics.height() * 1.25);

	return size;
}

}
