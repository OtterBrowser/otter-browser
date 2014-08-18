/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "BookmarksImporterWidget.h"
#include "../core/Utils.h"

#include "ui_BookmarksImporterWidget.h"

#include <QtWidgets/QInputDialog>
#include <QtWidgets/QTreeView>

namespace Otter
{

BookmarksImporterWidget::BookmarksImporterWidget(QWidget *parent) : QWidget(parent),
	m_model(new QStandardItemModel(this)),
	m_folder(0),
	m_ui(new Ui::BookmarksImporterWidget)
{
	m_ui->setupUi(this);

	QTreeView *view = new QTreeView(this);

	m_ui->folderComboBox->setView(view);
	m_ui->folderComboBox->setModel(m_model);

	view->setHeaderHidden(true);
	view->setItemsExpandable(false);
	view->setRootIsDecorated(false);
	view->setStyleSheet(QLatin1String("QTreeView::branch {border-image: url(invalid.png);}"));

	reloadFolders();
	removeStateChanged(m_ui->removeCheckBox->checkState());
	toSubfolderChanged(m_ui->toSubfolderCheckBox->checkState());

	connect(m_ui->removeCheckBox, SIGNAL(toggled(bool)), this, SLOT(removeStateChanged(bool)));
	connect(m_ui->toSubfolderCheckBox, SIGNAL(toggled(bool)), this, SLOT(toSubfolderChanged(bool)));
	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(reloadFolders()));
	connect(view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(folderChanged(QModelIndex)));
}

BookmarksImporterWidget::~BookmarksImporterWidget()
{
	delete m_ui;
}

void BookmarksImporterWidget::populateFolder(const QList<BookmarkInformation*> bookmarks, QStandardItem *parent)
{
	for (int i = 0; i < bookmarks.count(); ++i)
	{
		if (bookmarks.at(i)->type == FolderBookmark)
		{
			const QString title = (bookmarks.at(i)->title.isEmpty() ? tr("(Untitled)") : bookmarks.at(i)->title);
			QStandardItem *item = new QStandardItem(Utils::getIcon(QLatin1String("inode-directory")), title);
			item->setData(bookmarks.at(i)->identifier, Qt::UserRole);
			item->setToolTip(title);

			parent->appendRow(item);

			populateFolder(BookmarksManager::getFolder(bookmarks.at(i)->identifier), item);

			if (bookmarks.at(i)->identifier == m_folder)
			{
				m_index = item->index();
			}
		}
	}
}

void BookmarksImporterWidget::folderChanged(const QModelIndex &index)
{
	m_folder = index.data(Qt::UserRole).toInt();
}

void BookmarksImporterWidget::reloadFolders()
{
	m_model->clear();

	QStandardItem *item = new QStandardItem(Utils::getIcon(QLatin1String("inode-directory")), tr("Bookmarks"));
	item->setData(0, Qt::UserRole);
	item->setToolTip(tr("Bookmarks"));

	m_model->invisibleRootItem()->appendRow(item);

	m_index = item->index();

	populateFolder(BookmarksManager::getFolder(0), item);

	QTreeView *view = qobject_cast<QTreeView*>(m_ui->folderComboBox->view());

	if (view)
	{
		view->setCurrentIndex(m_index);
		view->expandAll();

		m_ui->folderComboBox->setRootModelIndex(m_index.parent());
		m_ui->folderComboBox->setModelColumn(0);
		m_ui->folderComboBox->setCurrentIndex(m_index.row());
		m_ui->folderComboBox->setRootModelIndex(QModelIndex());
	}
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

QString BookmarksImporterWidget::subfolderName()
{
	return m_ui->subfolderNameLineEdit->text();
}

int BookmarksImporterWidget::targetFolder()
{
	return m_ui->folderComboBox->view()->currentIndex().data(Qt::UserRole).toInt();
}

bool BookmarksImporterWidget::removeExisting()
{
	return m_ui->removeCheckBox->isChecked();
}

bool BookmarksImporterWidget::duplicate()
{
	return !m_ui->notDuplicateCheckBox->isChecked();
}

bool BookmarksImporterWidget::importIntoSubfolder()
{
	return m_ui->toSubfolderCheckBox->isChecked();
}

}
