/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include <QtGui/QHelpEvent>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QToolTip>

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
		frameOption.palette.setCurrentColorGroup(QPalette::Disabled);
		frameOption.state = QStyle::State_None;
		frameOption.frameShape = QFrame::HLine;

		switch (option.viewItemPosition)
		{
			case QStyleOptionViewItem::Beginning:
				frameOption.rect = option.rect.marginsRemoved(QMargins(3, 0, 0, 0));

				break;
			case QStyleOptionViewItem::End:
				frameOption.rect = option.rect.marginsRemoved(QMargins(0, 0, 3, 0));

				break;
			case QStyleOptionViewItem::OnlyOne:
				frameOption.rect = option.rect.marginsRemoved(QMargins(3, 0, 3, 0));

				break;
			default:
				frameOption.rect = option.rect;

				break;
		}

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

void ItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(index)

	editor->setGeometry(option.rect);
}

bool ItemDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
	if (event->type() == QEvent::ToolTip && index.data(Qt::ToolTipRole).isNull())
	{
		QToolTip::showText(event->globalPos(), displayText(index.data(Qt::DisplayRole), view->locale()), view);

		return true;
	}

	return QStyledItemDelegate::helpEvent(event, view, option, index);
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.data(Qt::SizeHintRole).isValid())
	{
		return index.data(Qt::SizeHintRole).toSize();
	}

	const int height((index.data(Qt::FontRole).isValid() ? QFontMetrics(index.data(Qt::FontRole).value<QFont>()) : option.fontMetrics).lineSpacing());
	QSize size(index.data(Qt::SizeHintRole).toSize());

	if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator"))
	{
		size.setHeight(qRound(height * 0.75));
	}
	else
	{
		size.setHeight(qRound(height * 1.25));
	}

	return size;
}

}
