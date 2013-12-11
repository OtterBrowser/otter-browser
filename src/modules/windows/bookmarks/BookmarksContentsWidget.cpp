#include "BookmarksContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/Utils.h"
#include "../../../core/WebBackend.h"
#include "../../../core/WebBackendsManager.h"
#include "../../../ui/BookmarkPropertiesDialog.h"

#include "ui_BookmarksContentsWidget.h"

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

	QMenu *addMenu = new QMenu(m_ui->addButton);
	addMenu->addAction(Utils::getIcon("inode-directory"), tr("Add Folder"), this, SLOT(addFolder()));
	addMenu->addAction(tr("Add Bookmark"), this, SLOT(addBookmark()));
	addMenu->addAction(tr("Add Separator"), this, SLOT(addSeparator()));

	m_ui->addButton->setMenu(addMenu);

	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(updateFolder(int)));
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
			item = new QStandardItem("------------------------");

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

void BookmarksContentsWidget::bookmarkProperties()
{
	BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(m_ui->bookmarksView->currentIndex().data(Qt::UserRole).value<void*>());

	if (bookmark)
	{
		BookmarkPropertiesDialog dialog(bookmark, -1, this);
		dialog.exec();
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

void BookmarksContentsWidget::print(QPrinter *printer)
{
	m_ui->bookmarksView->render(printer);
}

void BookmarksContentsWidget::triggerAction(WindowAction action, bool checked)
{
	Q_UNUSED(action)
	Q_UNUSED(checked)
}

void BookmarksContentsWidget::setHistory(const HistoryInformation &history)
{
	Q_UNUSED(history)
}

void BookmarksContentsWidget::setZoom(int zoom)
{
	Q_UNUSED(zoom)
}

void BookmarksContentsWidget::setUrl(const QUrl &url)
{
	Q_UNUSED(url)
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

ContentsWidget* BookmarksContentsWidget::clone(Window *window)
{
	Q_UNUSED(window)

	return NULL;
}

QAction* BookmarksContentsWidget::getAction(WindowAction action)
{
	Q_UNUSED(action)

	return NULL;
}

QUndoStack* BookmarksContentsWidget::getUndoStack()
{
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

QPixmap BookmarksContentsWidget::getThumbnail() const
{
	return QPixmap();
}

HistoryInformation BookmarksContentsWidget::getHistory() const
{
	HistoryEntry entry;
	entry.url = getUrl().toString();
	entry.title = getTitle();
	entry.position = QPoint(0, 0);
	entry.zoom = 100;

	HistoryInformation information;
	information.index = 0;
	information.entries.append(entry);

	return information;
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

int BookmarksContentsWidget::getZoom() const
{
	return 100;
}

bool BookmarksContentsWidget::canZoom() const
{
	return false;
}

bool BookmarksContentsWidget::isClonable() const
{
	return false;
}

bool BookmarksContentsWidget::isLoading() const
{
	return false;
}

bool BookmarksContentsWidget::isPrivate() const
{
	return false;
}

}
