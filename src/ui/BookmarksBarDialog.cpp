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

#include "BookmarksBarDialog.h"
#include "../core/BookmarksModel.h"
#include "../core/ToolBarsManager.h"

#include "ui_BookmarksBarDialog.h"

namespace Otter
{

BookmarksBarDialog::BookmarksBarDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::BookmarksBarDialog)
{
	m_ui->setupUi(this);

	adjustSize();
}

BookmarksBarDialog::~BookmarksBarDialog()
{
	delete m_ui;
}

void BookmarksBarDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		adjustSize();
	}
}

ToolBarDefinition BookmarksBarDialog::getDefinition() const
{
	ToolBarDefinition definition;
	definition.title = m_ui->titleLineEdit->text();
	definition.bookmarksPath = QLatin1Char('#') + QString::number(m_ui->folderComboBox->getCurrentFolder()->data(BookmarksModel::IdentifierRole).toULongLong());

	return definition;
}

}
