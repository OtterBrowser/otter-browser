/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TABLEVIEWWIDGET_H
#define OTTER_TABLEVIEWWIDGET_H

#include <QtGui/QStandardItemModel>
#include <QtWidgets/QTableView>

namespace Otter
{

class TableViewWidget : public QTableView
{
	Q_OBJECT

public:
	explicit TableViewWidget(QWidget *parent = NULL);

	void setModel(QAbstractItemModel *model);
	QStandardItemModel* getModel();
	bool canMoveUp() const;
	bool canMoveDown() const;

public slots:
	void insertRow(const QList<QStandardItem*> &items = QList<QStandardItem*>());
	void deleteRow();
	void moveUpRow();
	void moveDownRow();
	void setFilter(const QString filter = QString());

protected:
	void moveRow(bool up);

protected slots:
	void currentRowChanged(const QModelIndex &index);

private:
	QStandardItemModel *m_model;

signals:
	void canMoveUpChanged(bool available);
	void canMoveDownChanged(bool available);
	void needsActionsUpdate();
};

}

#endif
