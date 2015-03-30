/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ContentBlockingIntervalDelegate.h"

#include <QtWidgets/QSpinBox>

namespace Otter
{

ContentBlockingIntervalDelegate::ContentBlockingIntervalDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void ContentBlockingIntervalDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	const int updateInterval = index.data(Qt::EditRole).toInt();

	drawBackground(painter, option, index);
	drawDisplay(painter, option, option.rect, ((updateInterval > 0) ? tr("%n day(s)", "", updateInterval) : tr("Never")));
}

void ContentBlockingIntervalDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	editor->setGeometry(option.rect);
}

void ContentBlockingIntervalDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor);

	if (spinBox)
	{
		model->setData(index, spinBox->value());
	}
}

QWidget* ContentBlockingIntervalDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	QSpinBox *spinBox = new QSpinBox(parent);
	spinBox->setSuffix(tr(" day(s)"));
	spinBox->setSpecialValueText(tr("Never"));
	spinBox->setMinimum(0);
	spinBox->setMaximum(365);
	spinBox->setValue(index.data(Qt::EditRole).toInt());

	return spinBox;
}

}
