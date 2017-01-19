/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../core/Utils.h"

#include "ui_BookmarksImporterWidget.h"

#include <QtWidgets/QInputDialog>
#include <QtWidgets/QTreeView>

namespace Otter
{

BookmarksImporterWidget::BookmarksImporterWidget(QWidget *parent) : QWidget(parent),
	m_ui(new Ui::BookmarksImporterWidget)
{
	m_ui->setupUi(this);

	removeStateChanged(m_ui->removeCheckBox->checkState());
	toSubfolderChanged(m_ui->toSubfolderCheckBox->checkState());

	connect(m_ui->newFolderButton, SIGNAL(clicked()), m_ui->folderComboBox, SLOT(createFolder()));
	connect(m_ui->removeCheckBox, SIGNAL(toggled(bool)), this, SLOT(removeStateChanged(bool)));
	connect(m_ui->toSubfolderCheckBox, SIGNAL(toggled(bool)), this, SLOT(toSubfolderChanged(bool)));
}

BookmarksImporterWidget::~BookmarksImporterWidget()
{
	delete m_ui;
}

void BookmarksImporterWidget::removeStateChanged(bool checked)
{
	if (checked)
	{
		m_ui->removeDependentStackedWidget->setCurrentWidget(m_ui->noRemovePage);
	}
	else
	{
		m_ui->removeDependentStackedWidget->setCurrentWidget(m_ui->removePage);
	}
}

void BookmarksImporterWidget::toSubfolderChanged(bool checked)
{
	m_ui->subfolderNameLineEdit->setEnabled(checked);
}

BookmarksItem* BookmarksImporterWidget::getTargetFolder() const
{
	return m_ui->folderComboBox->getCurrentFolder();
}

QString BookmarksImporterWidget::getSubfolderName() const
{
	return m_ui->subfolderNameLineEdit->text();
}

bool BookmarksImporterWidget::hasToRemoveExisting() const
{
	return m_ui->removeCheckBox->isChecked();
}

bool BookmarksImporterWidget::allowDuplicates() const
{
	return m_ui->allowDuplicatesCheckBox->isChecked();
}

bool BookmarksImporterWidget::isImportingIntoSubfolder() const
{
	return m_ui->toSubfolderCheckBox->isChecked();
}

}
