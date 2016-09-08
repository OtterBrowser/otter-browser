/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "../core/AddressCompletionModel.h"
#include "../core/ThemesManager.h"
#include "../core/Utils.h"
#include "../modules/widgets/address/AddressWidget.h"

#include <QtGui/QPainter>

namespace Otter
{

AddressDelegate::AddressDelegate(bool isAddressField, QObject *parent) : QItemDelegate(parent),
	m_displayMode((SettingsManager::getValue(SettingsManager::AddressField_CompletionDisplayModeOption).toString() == QLatin1String("columns")) ? ColumnsMode : CompactMode),
	m_isAddressField(isAddressField)
{
}

void AddressDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	drawBackground(painter, option, index);

	QRect titleRectangle(option.rect);

	if (static_cast<AddressCompletionModel::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::HeaderType)
	{
		titleRectangle = titleRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

		if (index.row() != 0)
		{
			QPen pen(Qt::lightGray);
			pen.setWidth(1);
			pen.setStyle(Qt::SolidLine);

			painter->setPen(pen);
			painter->drawLine((option.rect.left() + 5), (option.rect.top() + 3), (option.rect.right() - 5), (option.rect.top() + 3));
		}

		drawDisplay(painter, option, titleRectangle, index.data(Qt::UserRole + 1).toString());

		return;
	}

	titleRectangle.setLeft(m_isAddressField ? 33 : (option.rect.height() + 1));

	QRect decorationRectangle(option.rect);
	decorationRectangle.setRight(titleRectangle.left());
	decorationRectangle = decorationRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

	QIcon icon(index.data(Qt::DecorationRole).value<QIcon>());

	if (icon.isNull())
	{
		icon = ThemesManager::getIcon(QLatin1String("tab"));
	}

	icon.paint(painter, decorationRectangle, option.decorationAlignment);

	QString url(index.data(Qt::DisplayRole).toString());

	if (m_isAddressField)
	{
		QStyleOptionViewItem linkOption(option);

		if (static_cast<AddressCompletionModel::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) != AddressCompletionModel::SearchSuggestionType)
		{
			linkOption.palette.setColor(QPalette::Text, option.palette.color(QPalette::Link));
		}

		QString description;

		if (index.data(Qt::UserRole + 1).type() == QVariant::DateTime)
		{
			description = Utils::formatDateTime(index.data(Qt::UserRole + 1).toDateTime());
		}
		else
		{
			description = index.data(Qt::UserRole + 1).toString();
		}

		if (m_displayMode == ColumnsMode)
		{
			const int maxUrlWidth(option.rect.width() / 2);

			url = option.fontMetrics.elidedText(url, Qt::ElideRight, (maxUrlWidth - 40));

			drawDisplay(painter, linkOption, titleRectangle, url);

			if (!description.isEmpty())
			{
				titleRectangle.setLeft(maxUrlWidth);

				drawDisplay(painter, option, titleRectangle, description);
			}

			return;
		}

		drawDisplay(painter, linkOption, titleRectangle, url);

		if (!description.isEmpty())
		{
			const int urlLength(option.fontMetrics.width(url + QLatin1Char(' ')));

			if (urlLength < titleRectangle.width())
			{
				titleRectangle.setLeft(titleRectangle.left() + urlLength);

				drawDisplay(painter, option, titleRectangle, QLatin1String("- ") + description);
			}
		}

		return;
	}

	drawDisplay(painter, option, titleRectangle, url);
}

QSize AddressDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size(index.data(Qt::SizeHintRole).toSize());

	if (index.row() != 0 && static_cast<AddressCompletionModel::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::HeaderType)
	{
		size.setHeight(option.fontMetrics.height() * 1.75);
	}
	else
	{
		size.setHeight(option.fontMetrics.height() * 1.25);
	}

	return size;
}

}
