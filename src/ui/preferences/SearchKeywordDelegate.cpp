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

#include "SearchKeywordDelegate.h"

#include <QtGui/QRegularExpressionValidator>
#include <QtWidgets/QLineEdit>

namespace Otter
{

SearchKeywordDelegate::SearchKeywordDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void SearchKeywordDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	editor->setGeometry(option.rect);
}

void SearchKeywordDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QLineEdit *widget(qobject_cast<QLineEdit*>(editor));

	if (widget)
	{
		model->setData(index, widget->text());
	}
}

QWidget* SearchKeywordDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	QStringList keywords;

	for (int i = 0; i < index.model()->rowCount(); ++i)
	{
		const QString keyword(index.model()->index(i, 1).data(Qt::DisplayRole).toString());

		if (index.row() != i && !keyword.isEmpty())
		{
			keywords.append(keyword);
		}
	}

	QLineEdit *widget(new QLineEdit(index.data(Qt::DisplayRole).toString(), parent));
	widget->setValidator(new QRegularExpressionValidator(QRegularExpression((keywords.isEmpty() ? QString() : QStringLiteral("(?!\\b(%1)\\b)").arg(keywords.join('|'))) + "[a-z0-9]*"), widget));

	return widget;
}

QSize SearchKeywordDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size(index.data(Qt::SizeHintRole).toSize());
	size.setHeight(option.fontMetrics.height() * 1.25);

	return size;
}

}
