/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_UPDATECHECKERDIALOG_H
#define OTTER_UPDATECHECKERDIALOG_H

#include "Dialog.h"
#include "../core/UpdateChecker.h"

#include <QtWidgets/QAbstractButton>

namespace Otter
{

namespace Ui
{
	class UpdateCheckerDialog;
}

class UpdateCheckerDialog : public Dialog
{
	Q_OBJECT

public:
	explicit UpdateCheckerDialog(QWidget *parent = nullptr, const QVector<UpdateChecker::UpdateInformation> &availableUpdates = QVector<UpdateChecker::UpdateInformation>());
	~UpdateCheckerDialog();

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void buttonClicked(QAbstractButton *button);
	void updateCheckFinished(const QVector<UpdateChecker::UpdateInformation> &availableUpdates);
	void updateProgress(const int progress);
	void showDetails();
	void transferFinished(const bool success);
	void downloadUpdate();
	void readyToInstall();

private:
	Ui::UpdateCheckerDialog *m_ui;
};

}

#endif
