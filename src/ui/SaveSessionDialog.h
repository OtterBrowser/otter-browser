/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_SAVESESSIONDIALOG_H
#define OTTER_SAVESESSIONDIALOG_H

#include "Dialog.h"

namespace Otter
{

namespace Ui
{
	class SaveSessionDialog;
}

class SaveSessionDialog final : public Dialog
{
	Q_OBJECT

public:
	explicit SaveSessionDialog(QWidget *parent = nullptr);
	~SaveSessionDialog();

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void saveSession();

private:
	Ui::SaveSessionDialog *m_ui;
};

}

#endif
