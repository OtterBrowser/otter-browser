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

#ifndef OTTER_UPDATECHECKERDIALOG_H
#define OTTER_UPDATECHECKERDIALOG_H

#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class UpdateCheckerDialog;
}

class UpdateChecker;
struct UpdateInformation;

class UpdateCheckerDialog : public QDialog
{
	Q_OBJECT

public:
	explicit UpdateCheckerDialog(QWidget *parent = NULL);
	~UpdateCheckerDialog();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void updateCheckFinished(const QList<UpdateInformation> &availableUpdates);
	void runUpdate();

private:
	UpdateChecker *m_updateChecker;
	Ui::UpdateCheckerDialog *m_ui;
};

}

#endif
