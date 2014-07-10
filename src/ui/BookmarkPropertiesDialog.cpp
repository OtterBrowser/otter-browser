/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "BookmarkPropertiesDialog.h"
#include "../core/Utils.h"

#include "ui_BookmarkPropertiesDialog.h"

#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QTreeView>

namespace Otter
{

BookmarkPropertiesDialog::BookmarkPropertiesDialog(BookmarkInformation *bookmark, int folder, QWidget *parent) : QDialog(parent),
	m_bookmark(bookmark),
	m_model(new QStandardItemModel(this)),
	m_folder((folder < 0) ? bookmark->parent : folder),
	m_ui(new Ui::BookmarkPropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->titleLineEdit->setText(m_bookmark->title);
	m_ui->addressLineEdit->setText(m_bookmark->url);
	m_ui->addressLineEdit->setVisible(m_bookmark->type == UrlBookmark);
	m_ui->addressLabel->setVisible(m_bookmark->type == UrlBookmark);
	m_ui->descriptionTextEdit->setPlainText(m_bookmark->description);
	m_ui->keywordLineEdit->setText(m_bookmark->keyword);
	m_ui->addedLabelWidget->setText(m_bookmark->added.isValid() ? m_bookmark->added.toString() : tr("Unknown"));
	m_ui->modifiedLabelWidget->setText(m_bookmark->modified.isValid() ? m_bookmark->modified.toString() : tr("Unknown"));

	if (m_bookmark->type == UrlBookmark)
	{
		m_ui->lastVisitLabelWidget->setText(m_bookmark->visited.isValid() ? m_bookmark->visited.toString() : tr("Unknown"));
		m_ui->visitsLabelWidget->setText(QString::number(m_bookmark->visits));
	}
	else
	{
		m_ui->visitsLabel->hide();
		m_ui->visitsLabelWidget->hide();
		m_ui->lastVisitLabel->hide();
		m_ui->lastVisitLabelWidget->hide();
	}

	if (bookmark->parent < 0)
	{
		setWindowTitle(tr("Add Bookmark"));

		m_ui->visitsLabel->hide();
		m_ui->visitsLabelWidget->hide();
		m_ui->lastVisitLabel->hide();
		m_ui->lastVisitLabelWidget->hide();
		m_ui->addedLabel->hide();
		m_ui->addedLabelWidget->hide();
		m_ui->modifiedLabel->hide();
		m_ui->modifiedLabelWidget->hide();
	}
	else
	{
		setWindowTitle(tr("Edit Bookmark"));
	}

	QTreeView *view = new QTreeView(m_ui->folderComboBox);

	m_ui->folderComboBox->setView(view);
	m_ui->folderComboBox->setModel(m_model);

	view->setHeaderHidden(true);
	view->setItemsExpandable(false);
	view->setRootIsDecorated(false);
	view->setStyleSheet(QLatin1String("QTreeView::branch {border-image: url(invalid.png);}"));

	reloadFolders();

	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(reloadFolders()));
	connect(view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(folderChanged(QModelIndex)));
	connect(m_ui->newFolderButton, SIGNAL(clicked()), this, SLOT(createFolder()));
	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(saveBookmark()));
	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
}

BookmarkPropertiesDialog::~BookmarkPropertiesDialog()
{
	delete m_ui;
}

void BookmarkPropertiesDialog::changeEvent(QEvent *event)
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

void BookmarkPropertiesDialog::populateFolder(const QList<BookmarkInformation*> bookmarks, QStandardItem *parent)
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

void BookmarkPropertiesDialog::folderChanged(const QModelIndex &index)
{
	m_folder = index.data(Qt::UserRole).toInt();
}

void BookmarkPropertiesDialog::reloadFolders()
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

void BookmarkPropertiesDialog::saveBookmark()
{
	const QString newKeyword = m_ui->keywordLineEdit->text();

	if (m_bookmark->keyword != newKeyword && BookmarksManager::getBookmark(newKeyword))
	{
		QMessageBox::critical(this, tr("Error"), tr("Bookmark with this keyword already exists."), QMessageBox::Close);

		return;
	}

	m_bookmark->url = m_ui->addressLineEdit->text();
	m_bookmark->title = m_ui->titleLineEdit->text();
	m_bookmark->description = m_ui->descriptionTextEdit->toPlainText();
	m_bookmark->keyword = newKeyword;

	if (m_bookmark->parent >= 0)
	{
		BookmarksManager::updateBookmark(m_bookmark);
	}
	else
	{
		disconnect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(reloadFolders()));

		if (!BookmarksManager::addBookmark(m_bookmark, m_ui->folderComboBox->view()->currentIndex().data(Qt::UserRole).toInt()))
		{
			QMessageBox::critical(this, tr("Error"), tr("Failed to save bookmark."), QMessageBox::Close);

			connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(reloadFolders()));

			return;
		}
	}

	accept();
}

void BookmarkPropertiesDialog::createFolder()
{
	const QString title = QInputDialog::getText(this, tr("Folder Name"), tr("Select name of new folder:"));

	if (title.isEmpty())
	{
		return;
	}

	BookmarkInformation *bookmark = new BookmarkInformation();
	bookmark->title = title;

	disconnect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(reloadFolders()));

	if (BookmarksManager::addBookmark(bookmark, m_ui->folderComboBox->view()->currentIndex().data(Qt::UserRole).toInt()))
	{
		m_folder = bookmark->identifier;

		reloadFolders();
	}
	else
	{
		delete bookmark;
	}

	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(reloadFolders()));
}

}
