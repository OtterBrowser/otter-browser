#include "BookmarksContentsWidget.h"
#include "../../../core/ActionsManager.h"
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
