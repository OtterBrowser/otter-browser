/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_SELECTPASSWORDDIALOG_H
#define OTTER_SELECTPASSWORDDIALOG_H

#include "../../../ui/Dialog.h"
#include "../../../core/PasswordsManager.h"

namespace Otter
{

namespace Ui
{
	class SelectPasswordDialog;
}

class SelectPasswordDialog final : public Dialog
{
	Q_OBJECT

public:
	explicit SelectPasswordDialog(const QVector<PasswordsManager::PasswordInformation> &passwords, QWidget *parent = nullptr);
	~SelectPasswordDialog();

	PasswordsManager::PasswordInformation getPassword() const;

protected:
	void changeEvent(QEvent *event) override;
	int getCurrentSet() const;

protected slots:
	void removePassword();
	void updateActions();

private:
	QVector<PasswordsManager::PasswordInformation> m_passwords;
	Ui::SelectPasswordDialog *m_ui;
};

}

#endif
