/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../core/HistoryManager.h"
#include "../../../core/NetworkManagerFactory.h"
#include "../../../core/ThemesManager.h"

#include "ui_CookiesContentsWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

namespace Otter
{

CookiesContentsWidget::CookiesContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_isLoading(true),
	m_ui(new Ui::CookiesContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->cookiesViewWidget->installEventFilter(this);
	m_ui->filterLineEdit->installEventFilter(this);

	if (!window)
	{
		m_ui->detailsWidget->hide();
	}

	QTimer::singleShot(100, this, SLOT(populateCookies()));

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterCookies(QString)));
	connect(m_ui->cookiesViewWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(removeCookies()));
}

CookiesContentsWidget::~CookiesContentsWidget()
{
	delete m_ui;
}

void CookiesContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void CookiesContentsWidget::populateCookies()
{
	CookieJar *cookieJar(NetworkManagerFactory::getCookieJar());
	const QList<QNetworkCookie> cookies(cookieJar->getCookies());

	for (int i = 0; i < cookies.count(); ++i)
	{
		addCookie(cookies.at(i));
	}

	m_model->sort(0);

	m_ui->cookiesViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->cookiesViewWidget->setModel(m_model);

	m_isLoading = false;

	emit loadingStateChanged(WindowsManager::FinishedLoadingState);

	connect(cookieJar, SIGNAL(cookieAdded(QNetworkCookie)), this, SLOT(addCookie(QNetworkCookie)));
	connect(cookieJar, SIGNAL(cookieRemoved(QNetworkCookie)), this, SLOT(removeCookie(QNetworkCookie)));
	connect(m_model, SIGNAL(modelReset()), this, SLOT(updateActions()));
	connect(m_ui->cookiesViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateActions()));
}

void CookiesContentsWidget::addCookie(const QNetworkCookie &cookie)
{
	const QString domain(cookie.domain().startsWith(QLatin1Char('.')) ? cookie.domain().mid(1) : cookie.domain());
	QStandardItem *domainItem(findDomain(domain));

	if (domainItem)
	{
		for (int i = 0; i < domainItem->rowCount(); ++i)
		{
			if (domainItem->child(i, 0)->text() == cookie.name() && domainItem->child(i, 0)->data(PathRole).toString() == cookie.path())
			{
				return;
			}
		}
	}
	else
	{
		domainItem = new QStandardItem(HistoryManager::getIcon(QUrl(QStringLiteral("http://%1/").arg(domain))), domain);
		domainItem->setToolTip(domain);

		m_model->appendRow(domainItem);

		if (sender())
		{
			m_model->sort(0);
		}
	}

	QStandardItem *cookieItem(new QStandardItem(QString(cookie.name())));
	cookieItem->setData(cookie.path(), PathRole);
	cookieItem->setData(cookie.domain(), DomainRole);
	cookieItem->setToolTip(cookie.name());
	cookieItem->setFlags(cookieItem->flags() | Qt::ItemNeverHasChildren);

	domainItem->appendRow(cookieItem);
	domainItem->setText(QStringLiteral("%1 (%2)").arg(domain).arg(domainItem->rowCount()));
}

void CookiesContentsWidget::removeCookie(const QNetworkCookie &cookie)
{
	const QString domain(cookie.domain().startsWith(QLatin1Char('.')) ? cookie.domain().mid(1) : cookie.domain());
	QStandardItem *domainItem(findDomain(domain));

	if (domainItem)
	{
		QPoint point;

		for (int j = 0; j < domainItem->rowCount(); ++j)
		{
			if (domainItem->child(j, 0)->text() == cookie.name() && domainItem->child(j, 0)->data(PathRole).toString() == cookie.path())
			{
				point = m_ui->cookiesViewWidget->visualRect(domainItem->child(j, 0)->index()).center();

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
			const QModelIndex index(m_ui->cookiesViewWidget->indexAt(point));

			m_ui->cookiesViewWidget->setCurrentIndex(index);
			m_ui->cookiesViewWidget->selectionModel()->select(index, QItemSelectionModel::Select);
		}
	}
}

void CookiesContentsWidget::removeCookies()
{
	const QModelIndexList indexes(m_ui->cookiesViewWidget->selectionModel()->selectedIndexes());

	if (indexes.isEmpty())
	{
		return;
	}

	CookieJar *cookieJar(NetworkManagerFactory::getCookieJar());
	QList<QNetworkCookie> cookies;

	for (int i = 0; i < indexes.count(); ++i)
	{
		if (!indexes.at(i).isValid())
		{
			continue;
		}

		if (indexes.at(i).data(PathRole).toString().isEmpty())
		{
			QStandardItem *domainItem(m_model->itemFromIndex(indexes.at(i)));

			if (!domainItem)
			{
				continue;
			}

			for (int j = 0; j < domainItem->rowCount(); ++j)
			{
				QStandardItem *cookieItem(domainItem->child(j, 0));

				if (cookieItem)
				{
					cookies.append(getCookie(cookieItem->index()));
				}
			}
		}
		else
		{
			QStandardItem *cookieItem(m_model->itemFromIndex(indexes.at(i)));

			if (cookieItem)
			{
				cookies.append(getCookie(cookieItem->index()));
			}
		}
	}

	for (int i = 0; i < cookies.count(); ++i)
	{
		cookieJar->forceDeleteCookie(cookies.at(i));
	}
}

void CookiesContentsWidget::removeDomainCookies()
{
	const QModelIndexList indexes(m_ui->cookiesViewWidget->selectionModel()->selectedIndexes());

	if (indexes.isEmpty())
	{
		return;
	}

	QList<QNetworkCookie> cookies;

	for (int i = 0; i < indexes.count(); ++i)
	{
		QStandardItem *domainItem((indexes.at(i).isValid() && indexes.at(i).parent() == m_model->invisibleRootItem()->index()) ? findDomain(indexes.at(i).sibling(indexes.at(i).row(), 0).data(Qt::ToolTipRole).toString()) : m_model->itemFromIndex(indexes.at(i).parent()));

		if (domainItem)
		{
			for (int j = 0; j < domainItem->rowCount(); ++j)
			{
				QStandardItem *cookieItem(domainItem->child(j, 0));

				if (cookieItem)
				{
					const QNetworkCookie cookie(getCookie(cookieItem->index()));

					if (!cookies.contains(cookie))
					{
						cookies.append(cookie);
					}
				}
			}
		}
	}

	if (cookies.isEmpty())
	{
		return;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("You are about to delete %n cookie(s).", "", cookies.count()));
	messageBox.setInformativeText(tr("Do you want to continue?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Yes);

	if (messageBox.exec() == QMessageBox::Yes)
	{
		CookieJar *cookieJar(NetworkManagerFactory::getCookieJar());

		for (int i = 0; i < cookies.count(); ++i)
		{
			cookieJar->forceDeleteCookie(cookies.at(i));
		}
	}
}

void CookiesContentsWidget::removeAllCookies()
{
	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("You are about to delete all cookies."));
	messageBox.setInformativeText(tr("Do you want to continue?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Yes);

	if (messageBox.exec() == QMessageBox::Yes)
	{
		NetworkManagerFactory::getCookieJar()->clearCookies();
	}
}

void CookiesContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index(m_ui->cookiesViewWidget->indexAt(point));
	QMenu menu(this);

	if (index.isValid())
	{
		if (index.parent() != m_model->invisibleRootItem()->index())
		{
			menu.addAction(tr("Remove Cookie"), this, SLOT(removeCookies()));
		}

		menu.addAction(tr("Remove All Cookies from This Domain…"), this, SLOT(removeDomainCookies()));
	}

	menu.addAction(tr("Remove All Cookies…"), this, SLOT(removeAllCookies()))->setEnabled(m_ui->cookiesViewWidget->model()->rowCount() > 0);
	menu.addSeparator();
	menu.addAction(ActionsManager::getAction(ActionsManager::ClearHistoryAction, this));
	menu.exec(m_ui->cookiesViewWidget->mapToGlobal(point));
}

void CookiesContentsWidget::print(QPrinter *printer)
{
	m_ui->cookiesViewWidget->render(printer);
}

void CookiesContentsWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	Q_UNUSED(parameters)

	switch (identifier)
	{
		case ActionsManager::SelectAllAction:
			m_ui->cookiesViewWidget->selectAll();

			break;
		case ActionsManager::DeleteAction:
			removeCookies();

			break;
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ActivateAddressFieldAction:
			m_ui->filterLineEdit->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->cookiesViewWidget->setFocus();

			break;
		default:
			break;
	}
}

void CookiesContentsWidget::updateActions()
{
	const QModelIndexList indexes(m_ui->cookiesViewWidget->selectionModel()->selectedIndexes());

	m_ui->deleteButton->setEnabled(!indexes.isEmpty());

	if (m_ui->deleteButton->isEnabled() != getAction(ActionsManager::DeleteAction)->isEnabled())
	{
		getAction(ActionsManager::DeleteAction)->setEnabled(m_ui->deleteButton->isEnabled());
	}

	m_ui->domainLineEdit->setText(QString());
	m_ui->nameLineEdit->setText(QString());
	m_ui->valueLineEdit->setText(QString());
	m_ui->expiresDateTimeEdit->setDateTime(QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0)));
	m_ui->secureCheckBox->setChecked(false);
	m_ui->httpOnlyCheckBox->setChecked(false);

	if (indexes.count() == 1 && !indexes.first().data(PathRole).toString().isEmpty())
	{
		const QModelIndex index(indexes.first());
		QUrl url;
		url.setScheme(QLatin1String("http"));
		url.setHost(index.parent().data(Qt::ToolTipRole).toString());
		url.setPath(index.data(PathRole).toString());

		const QList<QNetworkCookie> cookies(NetworkManagerFactory::getCookieJar()->cookiesForUrl(url));

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
		m_ui->cookiesViewWidget->setRowHidden(i, m_model->invisibleRootItem()->index(), (!filter.isEmpty() && !m_model->item(i, 0)->data(Qt::DisplayRole).toString().contains(filter, Qt::CaseInsensitive)));
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

	if (identifier != ActionsManager::DeleteAction && identifier != ActionsManager::SelectAllAction)
	{
		return NULL;
	}

	Action *action(new Action(identifier, this));

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
	return ThemesManager::getIcon(QLatin1String("cookies"), false);
}

QNetworkCookie CookiesContentsWidget::getCookie(const QModelIndex &index) const
{
	QNetworkCookie cookie(index.data(Qt::DisplayRole).toString().toUtf8());
	cookie.setDomain(index.data(DomainRole).toString());
	cookie.setPath(index.data(PathRole).toString());

	return cookie;
}

WindowsManager::LoadingState CookiesContentsWidget::getLoadingState() const
{
	return (m_isLoading ? WindowsManager::OngoingLoadingState : WindowsManager::FinishedLoadingState);
}

bool CookiesContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->cookiesViewWidget && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent && keyEvent->key() == Qt::Key_Delete)
		{
			removeCookies();

			return true;
		}
	}
	else if (object == m_ui->filterLineEdit && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent->key() == Qt::Key_Escape)
		{
			m_ui->filterLineEdit->clear();
		}
	}

	return ContentsWidget::eventFilter(object, event);
}

}
