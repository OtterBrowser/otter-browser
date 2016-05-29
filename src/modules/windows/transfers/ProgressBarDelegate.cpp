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

#include "ProgressBarDelegate.h"

#include <QtWidgets/QApplication>

namespace Otter
{

ProgressBarDelegate::ProgressBarDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void ProgressBarDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionProgressBar progressBarOption;
	progressBarOption.fontMetrics = option.fontMetrics;
	progressBarOption.palette = option.palette;
	progressBarOption.rect = option.rect;
	progressBarOption.state = option.state;
	progressBarOption.minimum = 0;
	progressBarOption.maximum = 100;
	progressBarOption.textAlignment = Qt::AlignCenter;
	progressBarOption.textVisible = true;
	progressBarOption.progress = index.data(Qt::DisplayRole).toInt();
	progressBarOption.text = QStringLiteral("%1%").arg(progressBarOption.progress);

	QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter, 0);
}

QSize ProgressBarDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size(index.data(Qt::SizeHintRole).toSize());
	size.setHeight(option.fontMetrics.height() * 1.25);

	return size;
}

}
