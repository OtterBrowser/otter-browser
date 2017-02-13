/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "ContentBlockingIntervalDelegate.h"

#include <QtCore/QCoreApplication>
#include <QtWidgets/QSpinBox>

namespace Otter
{

ContentBlockingIntervalDelegate::ContentBlockingIntervalDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void ContentBlockingIntervalDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	editor->setGeometry(option.rect);
}

void ContentBlockingIntervalDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QSpinBox *widget(qobject_cast<QSpinBox*>(editor));

	if (widget)
	{
		model->setData(index, widget->value(), Qt::DisplayRole);
	}
}

QWidget* ContentBlockingIntervalDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	QSpinBox *widget(new QSpinBox(parent));
	widget->setSuffix(QCoreApplication::translate("Otter::ContentBlockingIntervalDelegate", " day(s)"));
	widget->setSpecialValueText(QCoreApplication::translate("Otter::ContentBlockingIntervalDelegate", "Never"));
	widget->setMinimum(0);
	widget->setMaximum(365);
	widget->setValue(index.data(Qt::DisplayRole).toInt());

	return widget;
}

QString ContentBlockingIntervalDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
	Q_UNUSED(locale)

	if (value.isNull())
	{
		return QString();
	}

	const int updateInterval(value.toInt());

	return ((updateInterval > 0) ? QCoreApplication::translate("Otter::ContentBlockingIntervalDelegate", "%n day(s)", "", updateInterval) : QCoreApplication::translate("Otter::ContentBlockingIntervalDelegate", "Never"));
}

}
