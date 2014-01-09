/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "CookiesContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/CookieJar.h"
#include "../../../core/NetworkAccessManager.h"
#include "../../../core/Utils.h"
#include "../../../core/WebBackend.h"
#include "../../../core/WebBackendsManager.h"

#include "ui_CookiesContentsWidget.h"

#include <QtCore/QTimer>
#include <QtWidgets/QMenu>

namespace Otter
{

CookiesContentsWidget::CookiesContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_isLoading(true),
	m_ui(new Ui::CookiesContentsWidget)
{
	m_ui->setupUi(this);

	QTimer::singleShot(100, this, SLOT(populateCookies()));

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterCookies(QString)));
	connect(m_ui->cookiesView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(removeCookies()));
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

	const QList<QNetworkCookie> cookies = cookieJar->getCookies();

	for (int i = 0; i < cookies.count(); ++i)
	{
		addCookie(cookies.at(i));
	}

	m_model->sort(0);

	m_ui->cookiesView->setModel(m_model);

	m_isLoading = false;

	emit loadingChanged(false);

	connect(cookieJar, SIGNAL(cookieAdded(QNetworkCookie)), this, SLOT(addCookie(QNetworkCookie)));
	connect(cookieJar, SIGNAL(cookieRemoved(QNetworkCookie)), this, SLOT(removeCookie(QNetworkCookie)));
	connect(m_ui->cookiesView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
}

void CookiesContentsWidget::addCookie(const QNetworkCookie &cookie)
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

void CookiesContentsWidget::removeCookie(const QNetworkCookie &cookie)
{
	const QString domain = (cookie.domain().startsWith('.') ? cookie.domain().mid(1) : cookie.domain());

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *domainItem = m_model->item(i, 0);

		if (domainItem && domainItem->text() == domain)
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

void CookiesContentsWidget::removeCookies()
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

			for (int j = (domainItem->rowCount() - 1); j >= 0; --j)
			{
				QStandardItem *cookieItem = domainItem->child(j, 0);

				if (cookieItem)
				{
					cookieJar->deleteCookie(getCookie((cookieItem->index())));
				}
			}
		}
		else
		{
			QStandardItem *cookieItem = m_model->itemFromIndex(indexes.at(i));

			if (cookieItem)
			{
				cookieJar->deleteCookie(getCookie(cookieItem->index()));
			}
		}
	}
}

void CookiesContentsWidget::removeDomainCookies()
{
	const QModelIndex index = m_ui->cookiesView->currentIndex();
	QStandardItem *domainItem = ((index.isValid() && index.parent() == m_model->invisibleRootItem()->index()) ? findDomain(index.sibling(index.row(), 0).data(Qt::DisplayRole).toString()) : m_model->itemFromIndex(index.parent()));

	if (!domainItem)
	{
		return;
	}

	QNetworkCookieJar *cookieJar = NetworkAccessManager::getCookieJar();

	for (int i = (domainItem->rowCount() - 1); i >= 0; --i)
	{
		QStandardItem *cookieItem = domainItem->child(i, 0);

		if (cookieItem)
		{
			cookieJar->deleteCookie(getCookie(cookieItem->index()));
		}
	}
}

void CookiesContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index = m_ui->cookiesView->indexAt(point);
	QMenu menu(this);

	if (index.isValid())
	{
		if (index.parent() != m_model->invisibleRootItem()->index())
		{
			menu.addAction(tr("Remove Cookie"), this, SLOT(removeCookies()));
		}

		menu.addAction(tr("Remove All Cookies from This Domain"), this, SLOT(removeDomainCookies()));
		menu.addSeparator();
	}

	menu.addAction(ActionsManager::getAction(QLatin1String("ClearHistory")));
	menu.exec(m_ui->cookiesView->mapToGlobal(point));
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
			removeCookies();

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

	if (m_ui->deleteButton->isEnabled() != getAction(DeleteAction)->isEnabled())
	{
		getAction(DeleteAction)->setEnabled(m_ui->deleteButton->isEnabled());

		emit actionsChanged();
	}

	m_ui->domainLineEdit->setText(QString());
	m_ui->nameLineEdit->setText(QString());
	m_ui->valueLineEdit->setText(QString());
	m_ui->expiresDateTimeEdit->setDateTime(QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0)));
	m_ui->secureCheckBox->setChecked(false);
	m_ui->httpOnlyCheckBox->setChecked(false);

	if (indexes.count() == 1 && !indexes.first().data(Qt::UserRole).toString().isEmpty())
	{
		const QModelIndex index = indexes.first();
		QUrl url;
		url.setScheme(QLatin1String("http"));
		url.setHost(index.parent().data(Qt::DisplayRole).toString());
		url.setPath(index.data(Qt::UserRole).toString());

		const QList<QNetworkCookie> cookies = NetworkAccessManager::getCookieJar()->cookiesForUrl(url);

		for (int i = 0; i < cookies.count(); ++i)
		{
			if (cookies.at(i).name() == index.data(Qt::DisplayRole).toString())
			{
				m_ui->domainLineEdit->setText(cookies.at(i).domain());
				m_ui->nameLineEdit->setText(QString(cookies.at(i).name()));
				m_ui->valueLineEdit->setText(QString(cookies.at(i).value()));
				m_ui->expiresDateTimeEdit->setDateTime(cookies.at(i).expirationDate());
				m_ui->secureCheckBox->setChecked(cookies.at(i).isSecure());
				m_ui->httpOnlyCheckBox->setChecked(cookies.at(i).isHttpOnly());

				break;
			}
		}
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
			ActionsManager::setupLocalAction(actionObject, QLatin1String("SelectAll"));

			break;
		case DeleteAction:
			ActionsManager::setupLocalAction(actionObject, QLatin1String("Delete"));

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

QLatin1String CookiesContentsWidget::getType() const
{
	return QLatin1String("cookies");
}

QUrl CookiesContentsWidget::getUrl() const
{
	return QUrl(QLatin1String("about:cookies"));
}

QIcon CookiesContentsWidget::getIcon() const
{
	return Utils::getIcon(QLatin1String("cookies"), false);
}

QNetworkCookie CookiesContentsWidget::getCookie(const QModelIndex &index) const
{
	QNetworkCookie cookie(index.data(Qt::DisplayRole).toString().toUtf8());
	cookie.setDomain(index.data(Qt::UserRole + 1).toString());
	cookie.setPath(index.data(Qt::UserRole).toString());

	return cookie;
}

bool CookiesContentsWidget::isLoading() const
{
	return m_isLoading;
}

}
