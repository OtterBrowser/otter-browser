#include "CookiesContentsWidget.h"

#include "ui_CookiesContentsWidget.h"

namespace Otter
{

CookiesContentsWidget::CookiesContentsWidget(Window *window) : ContentsWidget(window),
	m_ui(new Ui::CookiesContentsWidget)
{
	m_ui->setupUi(this);
}

CookiesContentsWidget::~CookiesContentsWidget()
{
	delete m_ui;
}

void CookiesContentsWidget::changeEvent(QEvent *event)
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

void CookiesContentsWidget::print(QPrinter *printer)
{
	render(printer);
}

void CookiesContentsWidget::triggerAction(WindowAction action, bool checked)
{
	Q_UNUSED(action)
	Q_UNUSED(checked)
}

void CookiesContentsWidget::setHistory(const HistoryInformation &history)
{
	Q_UNUSED(history)
}

void CookiesContentsWidget::setZoom(int zoom)
{
	Q_UNUSED(zoom)
}

void CookiesContentsWidget::setUrl(const QUrl &url)
{
	Q_UNUSED(url)
}

ContentsWidget *CookiesContentsWidget::clone(Window *window)
{
	Q_UNUSED(window)

	return NULL;
}

QAction *CookiesContentsWidget::getAction(WindowAction action)
{
	Q_UNUSED(action)

	return NULL;
}

QUndoStack *CookiesContentsWidget::getUndoStack()
{
	return NULL;
}

QString CookiesContentsWidget::getTitle() const
{
	return tr("Cookies Manager");
}

QString CookiesContentsWidget::getType() const
{
	return "cookies";
}

QUrl CookiesContentsWidget::getUrl() const
{
	return QUrl("about:cookies");
}

QIcon CookiesContentsWidget::getIcon() const
{
	return QIcon(":/icons/cookies.png");
}

QPixmap CookiesContentsWidget::getThumbnail() const
{
	return QPixmap();
}

HistoryInformation CookiesContentsWidget::getHistory() const
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

int CookiesContentsWidget::getZoom() const
{
	return 100;
}

bool CookiesContentsWidget::canZoom() const
{
	return false;
}

bool CookiesContentsWidget::isClonable() const
{
	return false;
}

bool CookiesContentsWidget::isLoading() const
{
	return false;
}

bool CookiesContentsWidget::isPrivate() const
{
	return false;
}

}
