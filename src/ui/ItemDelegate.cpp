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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "ItemDelegate.h"

#include <QtGui/QPainter>
#include <QtWidgets/QApplication>

namespace Otter
{

ItemDelegate::ItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator"))
	{
		if (option.state.testFlag(QStyle::State_Selected))
		{
			painter->fillRect(option.rect, option.palette.brush((option.state.testFlag(QStyle::State_Enabled) ? (option.state.testFlag(QStyle::State_Active) ? QPalette::Normal : QPalette::Inactive) : QPalette::Disabled), QPalette::Highlight));
		}

		QStyleOptionFrame frameOption;
		frameOption.palette = option.palette;
		frameOption.rect = option.rect;
		frameOption.state = option.state;
		frameOption.frameShape = QFrame::HLine;

		QApplication::style()->drawControl(QStyle::CE_ShapedFrame, &frameOption, painter);

		return;
	}

	if (index.data(Qt::FontRole).isValid())
	{
		QStyleOptionViewItem mutableOption(option);
		mutableOption.font = index.data(Qt::FontRole).value<QFont>();

		QStyledItemDelegate::paint(painter, mutableOption, index);

		return;
	}

	QStyledItemDelegate::paint(painter, option, index);
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.data(Qt::SizeHintRole).isValid())
	{
		return index.data(Qt::SizeHintRole).toSize();
	}

	const int height((index.data(Qt::FontRole).isValid() ? QFontMetrics(index.data(Qt::FontRole).value<QFont>()) : option.fontMetrics).height());
	QSize size(index.data(Qt::SizeHintRole).toSize());

	if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator"))
	{
		size.setHeight(height * 0.75);
	}
	else
	{
		size.setHeight(height * 1.25);
	}

	return size;
}

}
