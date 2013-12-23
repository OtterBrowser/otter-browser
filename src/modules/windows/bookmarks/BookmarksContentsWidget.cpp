#include "BookmarksContentsWidget.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/Utils.h"
#include "../../../core/WebBackend.h"
#include "../../../core/WebBackendsManager.h"
#include "../../../ui/BookmarkPropertiesDialog.h"
#include "../../../ui/ItemDelegate.h"

#include "ui_BookmarksContentsWidget.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

namespace Otter
{

BookmarksContentsWidget::BookmarksContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::BookmarksContentsWidget)
{
	m_ui->setupUi(this);

	const QList<BookmarkInformation*> bookmarks = BookmarksManager::getFolder();

	for (int i = 0; i < bookmarks.count(); ++i)
	{
		addBookmark(bookmarks.at(i), m_model->invisibleRootItem());
	}

	m_ui->bookmarksView->setModel(m_model);
	m_ui->bookmarksView->setItemDelegate(new ItemDelegate(this));

	QMenu *addMenu = new QMenu(m_ui->addButton);
	addMenu->addAction(Utils::getIcon("inode-directory"), tr("Add Folder"), this, SLOT(addFolder()));
	addMenu->addAction(tr("Add Bookmark"), this, SLOT(addBookmark()));
	addMenu->addAction(tr("Add Separator"), this, SLOT(addSeparator()));

	m_ui->addButton->setMenu(addMenu);

	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(updateFolder(int)));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterBookmarks(QString)));
	connect(m_ui->bookmarksView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openBookmark(QModelIndex)));
	connect(m_ui->bookmarksView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
	connect(m_ui->propertiesButton, SIGNAL(clicked()), this, SLOT(bookmarkProperties()));
	connect(m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteBookmark()));
	connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(addBookmark()));
}

BookmarksContentsWidget::~BookmarksContentsWidget()
{
	delete m_ui;
}

void BookmarksContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void BookmarksContentsWidget::addBookmark(BookmarkInformation *bookmark, QStandardItem *parent)
{
	if (!bookmark)
	{
		return;
	}

	if (!parent)
	{
		parent = findFolder(bookmark->parent);
	}

	if (!parent)
	{
		return;
	}

	QStandardItem *item = NULL;

	switch (bookmark->type)
	{
		case FolderBookmark:
			item = new QStandardItem(Utils::getIcon("inode-directory"), (bookmark->title.isEmpty() ? tr("(Untitled)") : bookmark->title));

			for (int i = 0; i < bookmark->children.count(); ++i)
			{
				addBookmark(bookmark->children.at(i), item);
			}

			break;
		case UrlBookmark:
			item = new QStandardItem(WebBackendsManager::getBackend()->getIconForUrl(QUrl(bookmark->url)), (bookmark->title.isEmpty() ? tr("(Untitled)") : bookmark->title));

			break;
		default:
			item = new QStandardItem();
			item->setData("separator", Qt::AccessibleDescriptionRole);

			break;
	}

	item->setData(qVariantFromValue((void*) bookmark), Qt::UserRole);

	parent->appendRow(item);
}

void BookmarksContentsWidget::addBookmark()
{
	BookmarkInformation *bookmark = new BookmarkInformation();
	bookmark->type = UrlBookmark;

	BookmarkPropertiesDialog dialog(bookmark, findFolder(m_ui->bookmarksView->currentIndex()), this);

	if (dialog.exec() == QDialog::Rejected)
	{
		delete bookmark;
	}
}

void BookmarksContentsWidget::addFolder()
{
	BookmarkInformation *bookmark = new BookmarkInformation();
	bookmark->type = FolderBookmark;

	BookmarkPropertiesDialog dialog(bookmark, findFolder(m_ui->bookmarksView->currentIndex()), this);

	if (dialog.exec() == QDialog::Rejected)
	{
		delete bookmark;
	}
}

void BookmarksContentsWidget::addSeparator()
{
	BookmarkInformation *bookmark = new BookmarkInformation();
	bookmark->type = SeparatorBookmark;
	bookmark->parent = findFolder(m_ui->bookmarksView->currentIndex());

	BookmarksManager::addBookmark(bookmark, bookmark->parent);
}

void BookmarksContentsWidget::deleteBookmark()
{
	BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(m_ui->bookmarksView->currentIndex().data(Qt::UserRole).value<void*>());

	if (bookmark && (bookmark->type == SeparatorBookmark || QMessageBox::question(this, tr("Question"), ((bookmark->type == FolderBookmark) ? tr("Do you really want to delete this folder and all its children?") : tr("Do you really want to delete this bookmark?")), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Yes))
	{
		BookmarksManager::deleteBookmark(bookmark);
	}
}

void BookmarksContentsWidget::openBookmark(const QModelIndex &index)
{
	BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(index.data(Qt::UserRole).value<void*>());

	if (!bookmark || bookmark->type == SeparatorBookmark)
	{
		return;
	}

	if (bookmark->type == UrlBookmark)
	{
		emit requestedOpenUrl(QUrl(bookmark->url));

		return;
	}

	gatherBookmarks(bookmark->identifier);

	if (m_bookmarksToOpen.isEmpty())
	{
		return;
	}

	if (m_bookmarksToOpen.count() > 1 && SettingsManager::getValue("Choices/WarnOpenBookmarkFolder", true).toBool())
	{
		QMessageBox messageBox;
		messageBox.setWindowTitle(tr("Question"));
		messageBox.setText(tr("You are about to open %n bookmarks.", "", m_bookmarksToOpen.count()));
		messageBox.setInformativeText("Do you want to continue?");
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		messageBox.setDefaultButton(QMessageBox::Yes);
		messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

		if (messageBox.exec() == QMessageBox::Cancel)
		{
			m_bookmarksToOpen.clear();
		}

		SettingsManager::setValue("Choices/WarnOpenBookmarkFolder", !messageBox.checkBox()->isChecked());
	}

	for (int i = 0; i < m_bookmarksToOpen.count(); ++i)
	{
		emit requestedOpenUrl(QUrl(m_bookmarksToOpen.at(i)));
	}

	m_bookmarksToOpen.clear();
}

void BookmarksContentsWidget::bookmarkProperties()
{
	BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(m_ui->bookmarksView->currentIndex().data(Qt::UserRole).value<void*>());

	if (bookmark)
	{
		BookmarkPropertiesDialog dialog(bookmark, -1, this);
		dialog.exec();
	}
}

void BookmarksContentsWidget::gatherBookmarks(int folder)
{
	const QList<BookmarkInformation*> bookmarks = BookmarksManager::getFolder(folder);

	for (int i = 0; i < bookmarks.count(); ++i)
	{
		if (bookmarks.at(i)->type == FolderBookmark)
		{
			gatherBookmarks(bookmarks.at(i)->identifier);
		}
		else if (bookmarks.at(i)->type == UrlBookmark)
		{
			m_bookmarksToOpen.append(bookmarks.at(i)->url);
		}
	}
}

void BookmarksContentsWidget::updateFolder(int folder)
{
	QStandardItem *item = findFolder(folder);

	if (!item)
	{
		return;
	}

	item->removeRows(0, item->rowCount());

	const QList<BookmarkInformation*> bookmarks = BookmarksManager::getFolder(folder);

	for (int i = 0; i < bookmarks.count(); ++i)
	{
		addBookmark(bookmarks.at(i), item);
	}
}

void BookmarksContentsWidget::updateActions()
{
	const bool hasSelecion = !m_ui->bookmarksView->selectionModel()->selectedIndexes().isEmpty();
	BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(m_ui->bookmarksView->currentIndex().data(Qt::UserRole).value<void*>());

	m_ui->propertiesButton->setEnabled((hasSelecion && bookmark && bookmark->type != SeparatorBookmark));
	m_ui->deleteButton->setEnabled(hasSelecion);
}

bool BookmarksContentsWidget::filterBookmarks(const QString &filter, QStandardItem *branch)
{
	if (!branch)
	{
		if (sender())
		{
			branch = m_model->invisibleRootItem();
		}
		else
		{
			return false;
		}
	}

	bool found = filter.isEmpty();

	for (int i = 0; i < branch->rowCount(); ++i)
	{
		QStandardItem *item = branch->child(i, 0);

		if (item && filterBookmarks(filter, item))
		{
			found = true;
		}
	}

	if (!found)
	{
		BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(branch->data(Qt::UserRole).value<void*>());

		if (bookmark && bookmark->type != SeparatorBookmark && (bookmark->url.contains(filter, Qt::CaseInsensitive) || bookmark->title.contains(filter, Qt::CaseInsensitive) || bookmark->description.contains(filter, Qt::CaseInsensitive)))
		{
			found = true;
		}
	}

	m_ui->bookmarksView->setRowHidden(branch->row(), branch->index().parent(), !found);
	m_ui->bookmarksView->setExpanded(branch->index(), (found && !filter.isEmpty()));

	return found;
}

void BookmarksContentsWidget::print(QPrinter *printer)
{
	m_ui->bookmarksView->render(printer);
}

QStandardItem *BookmarksContentsWidget::findFolder(int folder, QStandardItem *item)
{
	if (folder == 0)
	{
		return m_model->invisibleRootItem();
	}

	if (!item)
	{
		item = m_model->invisibleRootItem();
	}

	for (int i = 0; i < item->rowCount(); ++i)
	{
		QStandardItem *child = item->child(i, 0);

		if (!child)
		{
			continue;
		}

		BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(child->data(Qt::UserRole).value<void*>());

		if (bookmark && bookmark->type == FolderBookmark && bookmark->identifier == folder)
		{
			return child;
		}

		QStandardItem *result = findFolder(folder, child);

		if (result)
		{
			return result;
		}
	}

	return NULL;
}

QString BookmarksContentsWidget::getTitle() const
{
	return tr("Bookmarks Manager");
}

QString BookmarksContentsWidget::getType() const
{
	return "bookmarks";
}

QUrl BookmarksContentsWidget::getUrl() const
{
	return QUrl("about:bookmarks");
}

QIcon BookmarksContentsWidget::getIcon() const
{
	return QIcon(":/icons/bookmarks.png");
}

int BookmarksContentsWidget::findFolder(const QModelIndex &index)
{
	if (index.isValid())
	{
		BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(index.data(Qt::UserRole).value<void*>());

		if (bookmark)
		{
			return ((bookmark->type == FolderBookmark) ? bookmark->identifier : bookmark->parent);
		}
	}

	return 0;
}

}
