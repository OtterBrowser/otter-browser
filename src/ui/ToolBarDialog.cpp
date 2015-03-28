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

#include "ToolBarDialog.h"
#include "../core/ActionsManager.h"
#include "../core/Utils.h"

#include "ui_ToolBarDialog.h"

namespace Otter
{

ToolBarDialog::ToolBarDialog(const QString &identifier, QWidget *parent) : QDialog(parent),
	m_identifier(identifier),
	m_ui(new Ui::ToolBarDialog)
{
	m_ui->setupUi(this);
	m_ui->removeButton->setIcon(Utils::getIcon(QLatin1String("go-previous")));
	m_ui->addButton->setIcon(Utils::getIcon(QLatin1String("go-next")));
	m_ui->moveUpButton->setIcon(Utils::getIcon(QLatin1String("go-up")));
	m_ui->moveDownButton->setIcon(Utils::getIcon(QLatin1String("go-down")));
}

ToolBarDialog::~ToolBarDialog()
{
	delete m_ui;
}

void ToolBarDialog::changeEvent(QEvent *event)
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

}
