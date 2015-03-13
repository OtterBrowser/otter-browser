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
#include "../../../core/AddonsManager.h"
#include "../../../core/CookieJar.h"
#include "../../../core/NetworkManagerFactory.h"
#include "../../../core/Utils.h"
#include "../../../core/WebBackend.h"

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
	CookieJar *cookieJar = qobject_cast<CookieJar*>(NetworkManagerFactory::getCookieJar());

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
	connect(m_model, SIGNAL(modelReset()), this, SLOT(updateActions()));
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
		WebBackend *backend = AddonsManager::getWebBackend();

		domainItem = new QStandardItem(backend->getIconForUrl(QUrl(QStringLiteral("http://%1/").arg(domain))), domain);
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
	domainItem->setText(QStringLiteral("%1 (%2)").arg(domain).arg(domainItem->rowCount()));
}

void CookiesContentsWidget::removeCookie(const QNetworkCookie &cookie)
{
	const QString domain = (cookie.domain().startsWith('.') ? cookie.domain().mid(1) : cookie.domain());
	QStandardItem *domainItem = findDomain(domain);

	if (domainItem)
	{
		QPoint point;

		for (int j = 0; j < domainItem->rowCount(); ++j)
		{
			if (domainItem->child(j, 0)->text() == cookie.name() && domainItem->child(j, 0)->data(Qt::UserRole).toString() == cookie.path())
			{
				point = m_ui->cookiesView->visualRect(domainItem->child(j, 0)->index()).center();

				domainItem->removeRow(j);

				break;
			}
		}

		if (domainItem->rowCount() == 0)
		{
			m_model->invisibleRootItem()->removeRow(domainItem->row());
		}
		else
		{
			domainItem->setText(QStringLiteral("%1 (%2)").arg(domain).arg(domainItem->rowCount()));
		}

		if (!point.isNull())
		{
			const QModelIndex index = m_ui->cookiesView->indexAt(point);

			m_ui->cookiesView->setCurrentIndex(index);
			m_ui->cookiesView->selectionModel()->select(index, QItemSelectionModel::Select);
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

	QNetworkCookieJar *cookieJar = NetworkManagerFactory::getCookieJar();
	QList<QNetworkCookie> cookies;

	for (int i = 0; i < indexes.count(); ++i)
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
					cookies.append(getCookie((cookieItem->index())));
				}
			}
		}
		else
		{
			QStandardItem *cookieItem = m_model->itemFromIndex(indexes.at(i));

			if (cookieItem)
			{
				cookies.append(getCookie(cookieItem->index()));
			}
		}
	}

	for (int i = 0; i < cookies.count(); ++i)
	{
		cookieJar->deleteCookie(cookies.at(i));
	}
}

void CookiesContentsWidget::removeDomainCookies()
{
	const QModelIndex index = m_ui->cookiesView->currentIndex();
	QStandardItem *domainItem = ((index.isValid() && index.parent() == m_model->invisibleRootItem()->index()) ? findDomain(index.sibling(index.row(), 0).data(Qt::ToolTipRole).toString()) : m_model->itemFromIndex(index.parent()));

	if (!domainItem)
	{
		return;
	}

	QNetworkCookieJar *cookieJar = NetworkManagerFactory::getCookieJar();
	QList<QNetworkCookie> cookies;

	for (int i = 0; i < domainItem->rowCount(); ++i)
	{
		QStandardItem *cookieItem = domainItem->child(i, 0);

		if (cookieItem)
		{
			cookies.append(getCookie(cookieItem->index()));
		}
	}

	for (int i = 0; i < cookies.count(); ++i)
	{
		cookieJar->deleteCookie(cookies.at(i));
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

	menu.addAction(ActionsManager::getAction(Action::ClearHistoryAction, this));
	menu.exec(m_ui->cookiesView->mapToGlobal(point));
}

void CookiesContentsWidget::print(QPrinter *printer)
{
	m_ui->cookiesView->render(printer);
}

void CookiesContentsWidget::triggerAction(int identifier, bool checked)
{
	Q_UNUSED(checked)

	switch (identifier)
	{
		case Action::SelectAllAction:
			m_ui->cookiesView->selectAll();

			break;
		case Action::DeleteAction:
			removeCookies();

			break;
		default:
			break;
	}
}

void CookiesContentsWidget::triggerAction()
{
	Action *action = qobject_cast<Action*>(sender());

	if (action)
	{
		triggerAction(action->getIdentifier());
	}
}

void CookiesContentsWidget::updateActions()
{
	const QModelIndexList indexes = m_ui->cookiesView->selectionModel()->selectedIndexes();

	m_ui->deleteButton->setEnabled(!indexes.isEmpty());

	if (m_ui->deleteButton->isEnabled() != getAction(Action::DeleteAction)->isEnabled())
	{
		getAction(Action::DeleteAction)->setEnabled(m_ui->deleteButton->isEnabled());
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
		url.setHost(index.parent().data(Qt::ToolTipRole).toString());
		url.setPath(index.data(Qt::UserRole).toString());

		const QList<QNetworkCookie> cookies = NetworkManagerFactory::getCookieJar()->cookiesForUrl(url);

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
		if (domain == m_model->item(i, 0)->toolTip())
		{
			return m_model->item(i, 0);
		}
	}

	return NULL;
}

Action* CookiesContentsWidget::getAction(int identifier)
{
	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	if (identifier != Action::DeleteAction && identifier != Action::SelectAllAction)
	{
		return NULL;
	}

	Action *action = new Action(identifier, qobject_cast<Window*>(parentWidget()), this);

	m_actions[identifier] = action;

	connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

	return action;
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
