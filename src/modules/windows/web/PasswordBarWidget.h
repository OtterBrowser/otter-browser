/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 -2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_PASSWORDBARWIDGET_H
#define OTTER_PASSWORDBARWIDGET_H

#include "../../../core/PasswordsManager.h"

#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class PasswordBarWidget;
}

class PasswordBarWidget : public QWidget
{
	Q_OBJECT

public:
	explicit PasswordBarWidget(const PasswordsManager::PasswordInformation &password, QWidget *parent = NULL);
	~PasswordBarWidget();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void accepted();
	void rejected();

private:
	PasswordsManager::PasswordInformation m_password;
	Ui::PasswordBarWidget *m_ui;

signals:
	void requestedClose();
};

}

#endif
