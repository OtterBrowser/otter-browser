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
	explicit ToolBarDialog(const QString &identifier, QWidget *parent = NULL);
	~ToolBarDialog();

protected:
	void changeEvent(QEvent *event);

private:
	QString m_identifier;
	Ui::ToolBarDialog *m_ui;
};

}

#endif
