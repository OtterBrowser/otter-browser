/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OpmlImporterWidget.h"

#include "ui_OpmlImporterWidget.h"

namespace Otter
{

OpmlImporterWidget::OpmlImporterWidget(QWidget *parent) : QWidget(parent),
	m_ui(new Ui::OpmlImporterWidget)
{
	m_ui->setupUi(this);
}

OpmlImporterWidget::~OpmlImporterWidget()
{
	delete m_ui;
}

FeedsModel::Entry* OpmlImporterWidget::getTargetFolder() const
{
	return m_ui->folderComboBox->getCurrentFolder();
}

bool OpmlImporterWidget::areDuplicatesAllowed() const
{
	return m_ui->allowDuplicatesCheckBox->isChecked();
}

}
