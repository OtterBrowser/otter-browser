/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "BookmarksImporterWidget.h"

#include "ui_BookmarksImporterWidget.h"

namespace Otter
{

BookmarksImporterWidget::BookmarksImporterWidget(QWidget *parent) : QWidget(parent),
	m_ui(new Ui::BookmarksImporterWidget)
{
	m_ui->setupUi(this);

	handleRemoveExistingChanged(m_ui->removeExistingCheckBox->checkState());
	handleImportToSubfolderChanged(m_ui->importToSubfolderCheckBox->checkState());

	connect(m_ui->newFolderButton, &QPushButton::clicked, m_ui->folderComboBox, &BookmarksComboBoxWidget::createFolder);
	connect(m_ui->removeExistingCheckBox, &QCheckBox::toggled, this, &BookmarksImporterWidget::handleRemoveExistingChanged);
	connect(m_ui->importToSubfolderCheckBox, &QCheckBox::toggled, this, &BookmarksImporterWidget::handleImportToSubfolderChanged);
}

BookmarksImporterWidget::~BookmarksImporterWidget()
{
	delete m_ui;
}

void BookmarksImporterWidget::handleRemoveExistingChanged(bool isChecked)
{
	if (isChecked)
	{
		m_ui->removeDependentStackedWidget->setCurrentWidget(m_ui->noRemovePage);
	}
	else
	{
		m_ui->removeDependentStackedWidget->setCurrentWidget(m_ui->removePage);
	}
}

void BookmarksImporterWidget::handleImportToSubfolderChanged(bool isChecked)
{
	m_ui->subfolderNameLineEditWidget->setEnabled(isChecked);
}

BookmarksModel::Bookmark* BookmarksImporterWidget::getTargetFolder() const
{
	return m_ui->folderComboBox->getCurrentFolder();
}

QString BookmarksImporterWidget::getSubfolderName() const
{
	return m_ui->subfolderNameLineEditWidget->text();
}

bool BookmarksImporterWidget::hasToRemoveExisting() const
{
	return m_ui->removeExistingCheckBox->isChecked();
}

bool BookmarksImporterWidget::areDuplicatesAllowed() const
{
	return m_ui->allowDuplicatesCheckBox->isChecked();
}

bool BookmarksImporterWidget::isImportingIntoSubfolder() const
{
	return m_ui->importToSubfolderCheckBox->isChecked();
}

}
