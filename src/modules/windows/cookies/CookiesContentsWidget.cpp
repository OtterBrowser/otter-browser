#include "CookiesContentsWidget.h"
#include "../../../core/NetworkAccessManager.h"
#include "../../../core/CookieJar.h"
#include "../../../backends/web/WebBackend.h"
#include "../../../backends/web/WebBackendsManager.h"

#include "ui_CookiesContentsWidget.h"

#include <QtCore/QTimer>

namespace Otter
{

CookiesContentsWidget::CookiesContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::CookiesContentsWidget)
{
	m_ui->setupUi(this);

	QTimer::singleShot(100, this, SLOT(populateCookies()));

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterCookies(QString)));
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

void CookiesContentsWidget::populateCookies()
{
	CookieJar *cookieJar = qobject_cast<CookieJar*>(NetworkAccessManager::getCookieJar());

	connect(cookieJar, SIGNAL(cookieInserted(QNetworkCookie)), this, SLOT(insertCookie(QNetworkCookie)));
	connect(cookieJar, SIGNAL(cookieDeleted(QNetworkCookie)), this, SLOT(deleteCookie(QNetworkCookie)));

	const QList<QNetworkCookie> cookies = cookieJar->getCookies();

	for (int i = 0; i < cookies.count(); ++i)
	{
		insertCookie(cookies.at(i));
	}

	m_model->sort(0);

	m_ui->cookiesView->setModel(m_model);
}

void CookiesContentsWidget::insertCookie(const QNetworkCookie &cookie)
{
	const QString domain = (cookie.domain().startsWith('.') ? cookie.domain().mid(1) : cookie.domain());
	QStandardItem *domainItem = findDomain(domain);

	if (domainItem)
	{
		for (int i = 0; i < domainItem->rowCount(); ++i)
		{
			if (domainItem->child(i, 0)->text() == cookie.name() && domainItem->child(i, 0)->data(Qt::UserRole).toString() == cookie.path())
			{
				return;
			}
		}
	}
	else
	{
		WebBackend *backend = WebBackendsManager::getBackend();

		domainItem = new QStandardItem(backend->getIconForUrl(QUrl(QString("http://%1/").arg(domain))), domain);
		domainItem->setToolTip(domain);

		m_model->appendRow(domainItem);

		if (sender())
		{
			m_model->sort(0);
		}
	}

	QStandardItem *cookieItem = new QStandardItem(QString(cookie.name()));
	cookieItem->setData(cookie.path(), Qt::UserRole);
	cookieItem->setToolTip(cookie.name());

	domainItem->appendRow(cookieItem);
}

void CookiesContentsWidget::deleteCookie(const QNetworkCookie &cookie)
{
	const QString domain = (cookie.domain().startsWith('.') ? cookie.domain().mid(1) : cookie.domain());

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *domainItem = m_model->item(i, 0);

		if (domainItem->text() == domain)
		{
			for (int j = 0; j < domainItem->rowCount(); ++j)
			{
				if (domainItem->child(j, 0)->text() == cookie.name() && domainItem->child(j, 0)->data(Qt::UserRole).toString() == cookie.path())
				{
					domainItem->removeRow(j);

					break;
				}
			}

			if (domainItem->rowCount() == 0)
			{
				m_model->invisibleRootItem()->removeRow(domainItem->row());
			}

			break;
		}
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

QStandardItem *CookiesContentsWidget::findDomain(const QString &domain)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		if (m_model->item(i, 0)->text() == domain)
		{
			return m_model->item(i, 0);
		}
	}

	return NULL;
}

void CookiesContentsWidget::filterCookies(const QString &filter)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		m_ui->cookiesView->setRowHidden(i, m_model->invisibleRootItem()->index(), (!filter.isEmpty() && !m_model->item(i, 0)->data(Qt::DisplayRole).toString().contains(filter, Qt::CaseInsensitive)));
	}
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
