/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_INLINELISTWIDGET_H
#define OTTER_INLINELISTWIDGET_H

#include <QtGui/QStandardItemModel>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeView>

namespace Otter
{

class ListEntryDelegate final : public QStyledItemDelegate
{
public:
	explicit ListEntryDelegate(QWidget *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
	QWidget *m_parent;
};

class InlineListWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit InlineListWidget(QWidget *parent = nullptr);

	QStringList getValues() const;

public slots:
	void setValues(const QStringList &values);

protected:
	void changeEvent(QEvent *event);

private:
	QTreeView *m_treeView;
	QToolButton *m_addButton;
	QToolButton *m_removeButton;
	QStandardItemModel *m_model;

signals:
	void modified();
};

}

#endif
