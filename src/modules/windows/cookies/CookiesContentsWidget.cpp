#include "CookiesContentsWidget.h"
#include "CookiePropertiesDialog.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/CookieJar.h"
#include "../../../core/NetworkAccessManager.h"
#include "../../../core/WebBackend.h"
#include "../../../core/WebBackendsManager.h"

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
	connect(m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteCookies()));
	connect(m_ui->propertiesButton, SIGNAL(clicked()), this, SLOT(cookieProperties()));
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

	connect(m_ui->cookiesView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
	connect(m_ui->cookiesView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(cookieProperties()));
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
	cookieItem->setData(cookie.domain(), (Qt::UserRole + 1));
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

void CookiesContentsWidget::deleteCookies()
{
	const QModelIndexList indexes = m_ui->cookiesView->selectionModel()->selectedIndexes();

	if (indexes.isEmpty())
	{
		return;
	}

	QNetworkCookieJar *cookieJar = NetworkAccessManager::getCookieJar();

	for (int i = (indexes.count() - 1); i >= 0; --i)
	{
		if (!indexes.at(i).isValid())
		{
			continue;
		}

		if (indexes.at(i).data(Qt::UserRole).toString().isEmpty())
		{
			QStandardItem *domainItem = m_model->itemFromIndex(indexes.at(i));

			if (!domainItem)
			{
				continue;
			}

			for (int j = 0; j < domainItem->rowCount(); ++j)
			{
				QStandardItem *cookieItem = domainItem->child(j, 0);

				if (cookieItem)
				{
					QNetworkCookie cookie(cookieItem->text().toUtf8());
					cookie.setDomain(cookieItem->data(Qt::UserRole + 1).toString());
					cookie.setPath(cookieItem->data(Qt::UserRole).toString());

					cookieJar->deleteCookie(cookie);
				}
			}
		}
		else
		{
			QStandardItem *cookieItem = m_model->itemFromIndex(indexes.at(i));

			if (cookieItem)
			{
				QNetworkCookie cookie(cookieItem->text().toUtf8());
				cookie.setDomain(cookieItem->data(Qt::UserRole + 1).toString());
				cookie.setPath(cookieItem->data(Qt::UserRole).toString());

				cookieJar->deleteCookie(cookie);
			}
		}
	}
}

void CookiesContentsWidget::cookieProperties()
{
	const QModelIndex index = m_ui->cookiesView->currentIndex();
	QUrl url;
	url.setScheme("http");
	url.setHost(index.parent().data(Qt::DisplayRole).toString());
	url.setPath(index.data(Qt::UserRole).toString());

	QList<QNetworkCookie> cookies = NetworkAccessManager::getCookieJar()->cookiesForUrl(url);

	for (int i = 0; i < cookies.count(); ++i)
	{
		if (cookies.at(i).name() == index.data(Qt::DisplayRole).toString())
		{
			CookiePropertiesDialog dialog(cookies.at(i), this);
			dialog.exec();

			break;
		}
	}
}

void CookiesContentsWidget::print(QPrinter *printer)
{
	m_ui->cookiesView->render(printer);
}

void CookiesContentsWidget::triggerAction(WindowAction action, bool checked)
{
	Q_UNUSED(checked)

	switch (action)
	{
		case SelectAllAction:
			m_ui->cookiesView->selectAll();

			break;
		case DeleteAction:
			deleteCookies();

			break;
		default:
			break;
	}
}

void CookiesContentsWidget::triggerAction()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		triggerAction(static_cast<WindowAction>(action->data().toInt()));
	}
}

void CookiesContentsWidget::updateActions()
{
	const QModelIndexList indexes = m_ui->cookiesView->selectionModel()->selectedIndexes();

	m_ui->deleteButton->setEnabled(!indexes.isEmpty());
	m_ui->propertiesButton->setEnabled(indexes.count() == 1 && !indexes.first().data(Qt::UserRole).toString().isEmpty());

	if (m_ui->deleteButton->isEnabled() != getAction(DeleteAction)->isEnabled())
	{
		getAction(DeleteAction)->setEnabled(m_ui->deleteButton->isEnabled());

		emit actionsChanged();
	}
}

void CookiesContentsWidget::filterCookies(const QString &filter)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		m_ui->cookiesView->setRowHidden(i, m_model->invisibleRootItem()->index(), (!filter.isEmpty() && !m_model->item(i, 0)->data(Qt::DisplayRole).toString().contains(filter, Qt::CaseInsensitive)));
	}
}

QStandardItem* CookiesContentsWidget::findDomain(const QString &domain)
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

QAction* CookiesContentsWidget::getAction(WindowAction action)
{
	if (m_actions.contains(action))
	{
		return m_actions[action];
	}

	QAction *actionObject = new QAction(this);
	actionObject->setData(action);

	connect(actionObject, SIGNAL(triggered()), this, SLOT(triggerAction()));

	switch (action)
	{
		case SelectAllAction:
			ActionsManager::setupLocalAction(actionObject, "SelectAll");

			break;
		case DeleteAction:
			ActionsManager::setupLocalAction(actionObject, "Delete");

			break;
		default:
			actionObject->deleteLater();
			actionObject = NULL;

			break;
	}

	if (actionObject)
	{
		m_actions[action] = actionObject;
	}

	return actionObject;
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

}
