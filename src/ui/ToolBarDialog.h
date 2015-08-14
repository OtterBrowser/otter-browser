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

#ifndef OTTER_TOOLBARDIALOG_H
#define OTTER_TOOLBARDIALOG_H

#include <QtWidgets/QDialog>
#include <QtGui/QStandardItemModel>

#include "../core/ToolBarsManager.h"

namespace Otter
{

namespace Ui
{
	class ToolBarDialog;
}

class ToolBarDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ToolBarDialog(int identifier = -1, QWidget *parent = NULL);
	~ToolBarDialog();

	ToolBarDefinition getDefinition();

protected:
	void changeEvent(QEvent *event);
	QStandardItem* createEntry(const QString &identifier);

protected slots:
	void addEntry();
	void addBookmark(QAction *action);
	void restoreDefaults();
	void updateActions();

private:
	ToolBarDefinition m_definition;
	Ui::ToolBarDialog *m_ui;
};

}

#endif
