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

#include "KeyboardShortcutDelegate.h"

#include <QtWidgets/QKeySequenceEdit>

namespace Otter
{

KeyboardShortcutDelegate::KeyboardShortcutDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void KeyboardShortcutDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QKeySequenceEdit *widget(qobject_cast<QKeySequenceEdit*>(editor));

	if (widget)
	{
		model->setData(index, widget->keySequence().toString());
	}
}

QWidget* KeyboardShortcutDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	return new QKeySequenceEdit(QKeySequence(index.data().toString()), parent);
}

QSize KeyboardShortcutDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size(index.data(Qt::SizeHintRole).toSize());
	size.setHeight(option.fontMetrics.height() * 1.25);

	return size;
}

}
