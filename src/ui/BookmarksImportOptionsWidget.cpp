/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "BookmarksImportOptionsWidget.h"

#include "ui_BookmarksImportOptionsWidget.h"

namespace Otter
{

BookmarksImportOptionsWidget::BookmarksImportOptionsWidget(QWidget *parent) : QWidget(parent),
	m_ui(new Ui::BookmarksImportOptionsWidget)
{
	m_ui->setupUi(this);

	handleRemoveExistingChanged(m_ui->removeExistingCheckBox->checkState());
	handleImportToSubfolderChanged(m_ui->importToSubfolderCheckBox->checkState());

	connect(m_ui->newFolderButton, &QPushButton::clicked, m_ui->folderComboBox, &BookmarksComboBoxWidget::createFolder);
	connect(m_ui->removeExistingCheckBox, &QCheckBox::toggled, this, &BookmarksImportOptionsWidget::handleRemoveExistingChanged);
	connect(m_ui->importToSubfolderCheckBox, &QCheckBox::toggled, this, &BookmarksImportOptionsWidget::handleImportToSubfolderChanged);
}

BookmarksImportOptionsWidget::~BookmarksImportOptionsWidget()
{
	delete m_ui;
}

void BookmarksImportOptionsWidget::handleRemoveExistingChanged(bool isChecked)
{
	m_ui->removeDependentStackedWidget->setCurrentWidget(isChecked ? m_ui->noRemovePage : m_ui->removePage);
}

void BookmarksImportOptionsWidget::handleImportToSubfolderChanged(bool isChecked)
{
	m_ui->subfolderNameLineEditWidget->setEnabled(isChecked);
}

BookmarksModel::Bookmark* BookmarksImportOptionsWidget::getTargetFolder() const
{
	return m_ui->folderComboBox->getCurrentFolder();
}

QString BookmarksImportOptionsWidget::getSubfolderName() const
{
	return m_ui->subfolderNameLineEditWidget->text();
}

bool BookmarksImportOptionsWidget::hasToRemoveExisting() const
{
	return m_ui->removeExistingCheckBox->isChecked();
}

bool BookmarksImportOptionsWidget::areDuplicatesAllowed() const
{
	return m_ui->allowDuplicatesCheckBox->isChecked();
}

bool BookmarksImportOptionsWidget::isImportingIntoSubfolder() const
{
	return m_ui->importToSubfolderCheckBox->isChecked();
}

}
