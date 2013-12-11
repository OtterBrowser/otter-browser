#include "BookmarkPropertiesDialog.h"
#include "../core/Utils.h"

#include "ui_BookmarkPropertiesDialog.h"

#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QTreeView>

namespace Otter
{

BookmarkPropertiesDialog::BookmarkPropertiesDialog(Bookmark *bookmark, QWidget *parent) : QDialog(parent),
	m_bookmark(bookmark),
	m_model(new QStandardItemModel(this)),
	m_folder(bookmark->parent),
	m_ui(new Ui::BookmarkPropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->titleLineEdit->setText(m_bookmark->title);
	m_ui->addressLineEdit->setText(m_bookmark->url);
	m_ui->descriptionTextEdit->setPlainText(m_bookmark->description);

	if (bookmark->parent < 0)
	{
		setWindowTitle(tr("Add Bookmark"));
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
	view->setStyleSheet("QTreeView::branch {border-image: url(invalid.png);}");

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

void BookmarkPropertiesDialog::populateFolder(const QList<Bookmark*> bookmarks, QStandardItem *parent)
{
	for (int i = 0; i < bookmarks.count(); ++i)
	{
		if (bookmarks.at(i)->type == FolderBookmark)
		{
			const QString title = (bookmarks.at(i)->title.isEmpty() ? tr("(Untitled)") : bookmarks.at(i)->title);
			QStandardItem *item = new QStandardItem(Utils::getIcon("inode-directory"), title);
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

	QStandardItem *item = new QStandardItem(Utils::getIcon("inode-directory"), tr("Bookmarks"));
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
	m_bookmark->url = m_ui->addressLineEdit->text();
	m_bookmark->title = m_ui->titleLineEdit->text();
	m_bookmark->description = m_ui->descriptionTextEdit->toPlainText();

	if (m_bookmark->parent >= 0)
	{
///TODO
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

	Bookmark *bookmark = new Bookmark();
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
