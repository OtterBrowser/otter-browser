/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_OPENADDRESSDIALOG_H
#define OTTER_OPENADDRESSDIALOG_H

#include "Dialog.h"
#include "../core/ActionExecutor.h"
#include "../core/InputInterpreter.h"

namespace Otter
{

namespace Ui
{
	class OpenAddressDialog;
}

class AddressWidget;

class OpenAddressDialog final : public Dialog
{
	Q_OBJECT

public:
	explicit OpenAddressDialog(ActionExecutor::Object executor, QWidget *parent = nullptr);
	~OpenAddressDialog();

	void setText(const QString &text);
	InputInterpreter::InterpreterResult getResult() const;

protected:
	void changeEvent(QEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

protected slots:
	void handleUserInput();

private:
	AddressWidget *m_addressWidget;
	ActionExecutor::Object m_executor;
	InputInterpreter::InterpreterResult m_result;
	Ui::OpenAddressDialog *m_ui;
};

}

#endif
