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

#include "UserAgentsManagerDialog.h"
#include "../core/Utils.h"

#include "ui_UserAgentsManagerDialog.h"

namespace Otter
{

UserAgentsManagerDialog::UserAgentsManagerDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::UserAgentsManagerDialog)
{
	m_ui->setupUi(this);
	m_ui->moveDownButton->setIcon(Utils::getIcon(QLatin1String("arrow-down")));
	m_ui->moveUpButton->setIcon(Utils::getIcon(QLatin1String("arrow-up")));

	connect(m_ui->userAgentsView, SIGNAL(canMoveDownChanged(bool)), m_ui->moveDownButton, SLOT(setEnabled(bool)));
	connect(m_ui->userAgentsView, SIGNAL(canMoveUpChanged(bool)), m_ui->moveUpButton, SLOT(setEnabled(bool)));
	connect(m_ui->userAgentsView, SIGNAL(needsActionsUpdate()), this, SLOT(updateKeyboardProfleActions()));
	connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(addUserAgent()));
	connect(m_ui->editButton, SIGNAL(clicked()), this, SLOT(editUserAgent()));
	connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(removeUserAgent()));
	connect(m_ui->moveDownButton, SIGNAL(clicked()), m_ui->userAgentsView, SLOT(moveDownRow()));
	connect(m_ui->moveUpButton, SIGNAL(clicked()), m_ui->userAgentsView, SLOT(moveUpRow()));
}

UserAgentsManagerDialog::~UserAgentsManagerDialog()
{
	delete m_ui;
}

void UserAgentsManagerDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void UserAgentsManagerDialog::addUserAgent()
{

}

void UserAgentsManagerDialog::editUserAgent()
{

}

void UserAgentsManagerDialog::removeUserAgent()
{

}

}
