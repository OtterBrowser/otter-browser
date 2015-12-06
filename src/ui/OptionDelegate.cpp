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

#include "OptionDelegate.h"
#include "OptionWidget.h"
#include "../core/SettingsManager.h"

namespace Otter
{

OptionDelegate::OptionDelegate(bool isSimple, QObject *parent) : QItemDelegate(parent),
	m_isSimple(isSimple)
{
}

void OptionDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	editor->setGeometry(option.rect);
}

void OptionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (m_isSimple)
	{
		OptionWidget *widget = qobject_cast<OptionWidget*>(editor);

		if (widget)
		{
			model->setData(index, widget->getValue());
		}
	}
}

QWidget* OptionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	const QString typeString = index.data(Qt::UserRole + 1).toString();
	OptionWidget::OptionType type = OptionWidget::UnknownType;

	if (typeString == QLatin1String("bool"))
	{
		type = OptionWidget::BooleanType;
	}
	else if (typeString == QLatin1String("color"))
	{
		type = OptionWidget::ColorType;
	}
	else if (typeString == QLatin1String("enumeration"))
	{
		type = OptionWidget::EnumerationType;
	}
	else if (typeString == QLatin1String("font"))
	{
		type = OptionWidget::FontType;
	}
	else if (typeString == QLatin1String("icon"))
	{
		type = OptionWidget::IconType;
	}
	else if (typeString == QLatin1String("integer"))
	{
		type = OptionWidget::IntegerType;
	}
	else if (typeString == QLatin1String("path"))
	{
		type = OptionWidget::PathType;
	}
	else if (typeString == QLatin1String("string"))
	{
		type = OptionWidget::StringType;
	}

	QVariant value = index.data(Qt::EditRole);

	if (value.isNull())
	{
		value = SettingsManager::getValue(index.data(Qt::UserRole).toString());
	}

	OptionWidget *widget = new OptionWidget(index.data(Qt::UserRole).toString(), value, type, parent);
	widget->setIndex(index);
	widget->setControlsVisible(!m_isSimple);

	if (type == OptionWidget::EnumerationType)
	{
		widget->setChoices(index.data(Qt::UserRole + 2).toStringList());
	}

	connect(widget, SIGNAL(commitData(QWidget*)), this, SIGNAL(commitData(QWidget*)));

	return widget;
}

QSize OptionDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size = index.data(Qt::SizeHintRole).toSize();
	size.setHeight(option.fontMetrics.height() * 1.25);

	return size;
}

}
