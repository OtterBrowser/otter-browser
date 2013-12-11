#include "BookmarksContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/Utils.h"
#include "../../../core/WebBackend.h"
#include "../../../core/WebBackendsManager.h"

#include "ui_BookmarksContentsWidget.h"

namespace Otter
{

BookmarksContentsWidget::BookmarksContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::BookmarksContentsWidget)
{
	m_ui->setupUi(this);

	const QList<Bookmark*> bookmarks = BookmarksManager::getFolder();

	for (int i = 0; i < bookmarks.count(); ++i)
	{
		addBookmark(bookmarks.at(i), m_model->invisibleRootItem());
	}

	m_ui->bookmarksView->setModel(m_model);
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

void BookmarksContentsWidget::addBookmark(Bookmark *bookmark, QStandardItem *parent)
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
			item->setData(bookmark->identifier, Qt::UserRole);

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

	parent->appendRow(item);
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
	if (!item)
	{
		item = m_model->invisibleRootItem();
	}

	for (int i = 0; i < item->rowCount(); ++i)
	{
		if (!item->child(i, 0)->data(Qt::UserRole).isValid())
		{
			continue;
		}

		if (item->child(i, 0)->data(Qt::UserRole).toInt() == folder)
		{
			return m_model->item(i, 0);
		}

		QStandardItem *result = findFolder(folder, m_model->item(i, 0));

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
