#include "BookmarkDialog.h"

#include "ui_BookmarkDialog.h"

#include <QtGui/QStandardItemModel>
#include <QtWidgets/QTreeView>

namespace Otter
{

BookmarkDialog::BookmarkDialog(Bookmark *bookmark, QWidget *parent) : QDialog(parent),
	m_bookmark(bookmark),
	m_ui(new Ui::BookmarkDialog)
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

	QStandardItem *item = new QStandardItem(QIcon::fromTheme("inode-directory", QIcon(":/icons/inode-directory.png")), tr("Bookmarks"));
	item->setData(0);
	item->setToolTip(tr("Bookmarks"));

	QStandardItemModel *model = new QStandardItemModel(this);
	model->invisibleRootItem()->appendRow(item);

	populateFolder(BookmarksManager::getFolder(0), item);

	QTreeView *view = new QTreeView(m_ui->folderComboBox);

	m_ui->folderComboBox->setView(view);
	m_ui->folderComboBox->setModel(model);

	view->setHeaderHidden(true);
	view->setItemsExpandable(false);
	view->setRootIsDecorated(false);
	view->setStyleSheet("QTreeView::branch {border-image: url(invalid.png);}");
	view->expandAll();

	connect(m_ui->newFolderButton, SIGNAL(clicked()), this, SLOT(createFolder()));
}

BookmarkDialog::~BookmarkDialog()
{
	delete m_ui;
}

void BookmarkDialog::changeEvent(QEvent *event)
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

void BookmarkDialog::populateFolder(const QList<Bookmark*> bookmarks, QStandardItem *parent)
{
	for (int i = 0; i < bookmarks.count(); ++i)
	{
		if (bookmarks.at(i)->type == FolderBookmark)
		{
			const QString title = (bookmarks.at(i)->title.isEmpty() ? tr("(Untitled)") : bookmarks.at(i)->title);
			QStandardItem *item = new QStandardItem(QIcon::fromTheme("inode-directory", QIcon(":/icons/inode-directory.png")), title);
			item->setData(bookmarks.at(i)->identifier);
			item->setToolTip(title);

			parent->appendRow(item);

			populateFolder(BookmarksManager::getFolder(bookmarks.at(i)->identifier), item);
		}
	}
}

void BookmarkDialog::createFolder()
{
//TODO
}

}
