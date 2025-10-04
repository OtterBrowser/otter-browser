/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "CacheContentsWidget.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/NetworkCache.h"
#include "../../../core/NetworkManagerFactory.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/Action.h"
#include "../../../ui/MainWindow.h"

#include "ui_CacheContentsWidget.h"

#include <QtCore/QMimeDatabase>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QMenu>

namespace Otter
{

CacheContentsWidget::CacheContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_model(new QStandardItemModel(this)),
	m_isLoading(true),
	m_ui(new Ui::CacheContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->cacheViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->cacheViewWidget->installEventFilter(this);
	m_ui->cacheViewWidget->viewport()->installEventFilter(this);
	m_ui->previewLabel->hide();

	if (isSidebarPanel())
	{
		m_ui->detailsWidget->hide();
	}

	QTimer::singleShot(100, this, &CacheContentsWidget::populateCache);

	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->cacheViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->cacheViewWidget, &ItemViewWidget::doubleClicked, this, &CacheContentsWidget::openEntry);
	connect(m_ui->cacheViewWidget, &ItemViewWidget::customContextMenuRequested, this, &CacheContentsWidget::showContextMenu);
	connect(m_ui->deleteButton, &QPushButton::clicked, this, &CacheContentsWidget::removeDomainEntriesOrEntry);
}

CacheContentsWidget::~CacheContentsWidget()
{
	delete m_ui;
}

void CacheContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		m_model->setHorizontalHeaderLabels({tr("Address"), tr("Type"), tr("Size"), tr("Last Modified"), tr("Expires")});
	}
}

void CacheContentsWidget::print(QPrinter *printer)
{
	m_ui->cacheViewWidget->render(printer);
}

void CacheContentsWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	switch (identifier)
	{
		case ActionsManager::DeleteAction:
			removeDomainEntriesOrEntry();

			break;
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
			m_ui->filterLineEditWidget->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->cacheViewWidget->setFocus();

			break;
		default:
			ContentsWidget::triggerAction(identifier, parameters, trigger);

			break;
	}
}

void CacheContentsWidget::populateCache()
{
	m_model->clear();
	m_model->setHorizontalHeaderLabels({tr("Address"), tr("Type"), tr("Size"), tr("Last Modified"), tr("Expires")});
	m_model->setHeaderData(0, Qt::Horizontal, 500, HeaderViewWidget::WidthRole);
	m_model->setHeaderData(2, Qt::Horizontal, 150, HeaderViewWidget::WidthRole);
	m_model->setSortRole(Qt::DisplayRole);

	const NetworkCache *cache(NetworkManagerFactory::getCache());
	const QVector<QUrl> entries(cache->getEntries());

	for (int i = 0; i < entries.count(); ++i)
	{
		handleEntryAdded(entries.at(i));
	}

	m_model->sort(0);

	if (!m_isLoading)
	{
		return;
	}

	m_ui->cacheViewWidget->setModel(m_model);
	m_ui->cacheViewWidget->setLayoutDirection(Qt::LeftToRight);
	m_ui->cacheViewWidget->setFilterRoles({Qt::DisplayRole, Qt::UserRole});

	m_isLoading = false;

	emit loadingStateChanged(WebWidget::FinishedLoadingState);

	connect(cache, &NetworkCache::cleared, this, &CacheContentsWidget::populateCache);
	connect(cache, &NetworkCache::entryAdded, this, &CacheContentsWidget::handleEntryAdded);
	connect(cache, &NetworkCache::entryRemoved, this, &CacheContentsWidget::handleEntryRemoved);
	connect(m_model, &QStandardItemModel::modelReset, this, &CacheContentsWidget::updateActions);
	connect(m_ui->cacheViewWidget, &ItemViewWidget::needsActionsUpdate, this, &CacheContentsWidget::updateActions);
}

void CacheContentsWidget::removeDomainEntries()
{
	const QModelIndex index(m_ui->cacheViewWidget->currentIndex());
	const QStandardItem *domainItem(findDomainItem((index.isValid() && index.parent() == m_model->invisibleRootItem()->index()) ? index.sibling(index.row(), 0).data(Qt::ToolTipRole).toString() : Utils::extractHost(getEntry(index))));

	if (!domainItem)
	{
		return;
	}

	NetworkCache *cache(NetworkManagerFactory::getCache());

	for (int i = (domainItem->rowCount() - 1); i >= 0; --i)
	{
		cache->remove(ItemModel::getItemData(domainItem->child(i, 0), UrlRole).toUrl());
	}
}

void CacheContentsWidget::removeDomainEntriesOrEntry()
{
	const QUrl url(getEntry(m_ui->cacheViewWidget->currentIndex()));

	if (url.isValid())
	{
		NetworkManagerFactory::getCache()->remove(url);
	}
	else
	{
		removeDomainEntries();
	}
}

void CacheContentsWidget::openEntry()
{
	const QModelIndex index(m_ui->cacheViewWidget->currentIndex());

	if (!index.isValid() || index.parent() == m_model->invisibleRootItem()->index())
	{
		return;
	}

	const QUrl url(getEntry(index));

	if (!url.isValid())
	{
		return;
	}

	const QAction *action(qobject_cast<QAction*>(sender()));
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (mainWindow)
	{
		SessionsManager::OpenHints hints(SessionsManager::DefaultOpen);

		if (action)
		{
			hints = static_cast<SessionsManager::OpenHints>(action->data().toInt());
		}

		mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(hints)}});
	}
}

void CacheContentsWidget::handleEntryAdded(const QUrl &url)
{
	const QString domain(url.host());
	QStandardItem *domainItem(findDomainItem(domain));

	if (domainItem)
	{
		for (int i = 0; i < domainItem->rowCount(); ++i)
		{
			if (ItemModel::getItemData(domainItem->child(i, 0), UrlRole).toUrl() == url)
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
		m_model->setItem(domainItem->row(), 2, new QStandardItem());

		if (!m_isLoading)
		{
			m_model->sort(0);
		}
	}

	NetworkCache *cache(NetworkManagerFactory::getCache());
	QIODevice *device(cache->data(url));
	const QNetworkCacheMetaData metaData(cache->metaData(url));
	const QList<QPair<QByteArray, QByteArray> > headers(metaData.rawHeaders());
	QString type;

	for (int i = 0; i < headers.count(); ++i)
	{
		const QPair<QByteArray, QByteArray> header(headers.at(i));

		if (header.first == QByteArrayLiteral("Content-Type"))
		{
			type = QString::fromLatin1(header.second);

			break;
		}
	}

	const QMimeType mimeType((device && type.isEmpty()) ? QMimeDatabase().mimeTypeForData(device) : QMimeDatabase().mimeTypeForName(type));
	QList<QStandardItem*> entryItems({new QStandardItem(url.path()), new QStandardItem(mimeType.name()), new QStandardItem(device ? Utils::formatUnit(device->size()) : QString()), new QStandardItem(Utils::formatDateTime(metaData.lastModified())), new QStandardItem(Utils::formatDateTime(metaData.expirationDate()))});
	entryItems[0]->setData(url, UrlRole);
	entryItems[0]->setFlags(entryItems[0]->flags() | Qt::ItemNeverHasChildren);
	entryItems[1]->setFlags(entryItems[1]->flags() | Qt::ItemNeverHasChildren);
	entryItems[2]->setData((device ? device->size() : 0), SizeRole);
	entryItems[2]->setFlags(entryItems[2]->flags() | Qt::ItemNeverHasChildren);
	entryItems[3]->setFlags(entryItems[3]->flags() | Qt::ItemNeverHasChildren);
	entryItems[4]->setFlags(entryItems[4]->flags() | Qt::ItemNeverHasChildren);

	if (device)
	{
		QStandardItem *sizeItem(m_model->item(domainItem->row(), 2));

		if (sizeItem)
		{
			sizeItem->setData((sizeItem->data(SizeRole).toLongLong() + device->size()), SizeRole);
			sizeItem->setText(Utils::formatUnit(sizeItem->data(SizeRole).toLongLong()));
		}

		device->deleteLater();
	}

	domainItem->appendRow(entryItems);
	domainItem->setText(QStringLiteral("%1 (%2)").arg(domain, QString::number(domainItem->rowCount())));

	if (!m_isLoading)
	{
		domainItem->sortChildren(0, Qt::DescendingOrder);
	}
}

void CacheContentsWidget::handleEntryRemoved(const QUrl &url)
{
	QStandardItem *domainItem(findDomainItem(Utils::extractHost(url)));

	if (!domainItem)
	{
		return;
	}

	for (int i = 0; i < domainItem->rowCount(); ++i)
	{
		QStandardItem *entryItem(domainItem->child(i, 0));

		if (!entryItem || entryItem->data(UrlRole).toUrl() != url)
		{
			continue;
		}

		const qint64 size(ItemModel::getItemData(domainItem->child(entryItem->row(), 2), SizeRole).toLongLong());

		m_model->removeRow(entryItem->row(), domainItem->index());

		if (domainItem->rowCount() == 0)
		{
			m_model->invisibleRootItem()->removeRow(domainItem->row());
		}
		else
		{
			QStandardItem *domainSizeItem(m_model->item(domainItem->row(), 2));

			if (domainSizeItem && size > 0)
			{
				domainSizeItem->setData((domainSizeItem->data(SizeRole).toLongLong() - size), SizeRole);
				domainSizeItem->setText(Utils::formatUnit(domainSizeItem->data(SizeRole).toLongLong()));
			}

			domainItem->setText(QStringLiteral("%1 (%2)").arg(url.host(), QString::number(domainItem->rowCount())));
		}

		break;
	}
}

void CacheContentsWidget::showContextMenu(const QPoint &position)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));
	const QModelIndex index(m_ui->cacheViewWidget->indexAt(position));
	const QUrl url(getEntry(index));
	QMenu menu(this);

	if (url.isValid())
	{
		menu.addAction(ThemesManager::createIcon(QLatin1String("document-open")), QCoreApplication::translate("actions", "Open"), this, &CacheContentsWidget::openEntry);
		menu.addAction(QCoreApplication::translate("actions", "Open in New Tab"), this, &CacheContentsWidget::openEntry)->setData(SessionsManager::NewTabOpen);
		menu.addAction(QCoreApplication::translate("actions", "Open in New Background Tab"), this, &CacheContentsWidget::openEntry)->setData(static_cast<int>(SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen));
		menu.addSeparator();
		menu.addAction(QCoreApplication::translate("actions", "Open in New Window"), this, &CacheContentsWidget::openEntry)->setData(SessionsManager::NewWindowOpen);
		menu.addAction(QCoreApplication::translate("actions", "Open in New Background Window"), this, &CacheContentsWidget::openEntry)->setData(static_cast<int>(SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen));
		menu.addSeparator();
		menu.addAction(tr("Copy Link to Clipboard"), this, [&]()
		{
			const QUrl url(getEntry(m_ui->cacheViewWidget->currentIndex()));

			if (url.isValid())
			{
				QGuiApplication::clipboard()->setText(url.toDisplayString());
			}
		});
		menu.addSeparator();
		menu.addAction(tr("Remove Entry"), this, [&]()
		{
			const QUrl url(getEntry(m_ui->cacheViewWidget->currentIndex()));

			if (url.isValid())
			{
				NetworkManagerFactory::getCache()->remove(url);
			}
		});
	}

	if (url.isValid() || (index.isValid() && index.parent() == m_model->invisibleRootItem()->index()))
	{
		menu.addAction(tr("Remove All Entries from This Domain"), this, &CacheContentsWidget::removeDomainEntries);
		menu.addSeparator();
	}

	menu.addAction(new Action(ActionsManager::ClearHistoryAction, {}, ActionExecutor::Object(mainWindow, mainWindow), &menu));
	menu.exec(m_ui->cacheViewWidget->mapToGlobal(position));
}

void CacheContentsWidget::updateActions()
{
	const QModelIndex index(m_ui->cacheViewWidget->getCurrentIndex());
	const QUrl url(getEntry(index));
	const QString domain((index.isValid() && index.parent() == m_model->invisibleRootItem()->index()) ? index.sibling(index.row(), 0).data(Qt::ToolTipRole).toString() : url.host());

	m_ui->locationLabelWidget->setText({});
	m_ui->locationLabelWidget->setUrl({});
	m_ui->previewLabel->hide();
	m_ui->previewLabel->setPixmap({});
	m_ui->deleteButton->setEnabled(!domain.isEmpty());

	if (!url.isValid())
	{
		m_ui->addressLabelWidget->setText({});
		m_ui->typeLabelWidget->setText({});
		m_ui->sizeLabelWidget->setText({});
		m_ui->lastModifiedLabelWidget->setText({});
		m_ui->expiresLabelWidget->setText({});

		if (!domain.isEmpty())
		{
			m_ui->addressLabelWidget->setText(domain);
		}

		emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::EditingCategory});

		return;
	}

	NetworkCache *cache(NetworkManagerFactory::getCache());
	QIODevice *device(cache->data(url));
	const QNetworkCacheMetaData metaData(cache->metaData(url));
	const QMimeDatabase mimeDatabase;
	QMimeType mimeType;

	if (device)
	{
		mimeType = mimeDatabase.mimeTypeForData(device);
	}

	if (!mimeType.isValid())
	{
		const QList<QPair<QByteArray, QByteArray> > headers(metaData.rawHeaders());

		for (int i = 0; i < headers.count(); ++i)
		{
			const QPair<QByteArray, QByteArray> header(headers.at(i));

			if (header.first == QByteArrayLiteral("Content-Type"))
			{
				mimeType = mimeDatabase.mimeTypeForName(QString::fromLatin1(header.second));

				break;
			}
		}

		if (!mimeType.isValid())
		{
			mimeType = mimeDatabase.mimeTypeForUrl(url);
		}
	}

	QPixmap preview;
	const int size(m_ui->propertiesWidget->contentsRect().height() - 10);

	if (device && mimeType.name().startsWith(QLatin1String("image")))
	{
		QImage image;
		image.load(device, "");

		if (image.size().width() > size || image.height() > size)
		{
			image = image.scaled(size, size, Qt::KeepAspectRatio);
		}

		preview = QPixmap::fromImage(image);
	}

	if (preview.isNull() && QIcon::hasThemeIcon(mimeType.iconName()))
	{
		preview = QIcon::fromTheme(mimeType.iconName(), ThemesManager::createIcon(QLatin1String("unknown"))).pixmap(64, 64);
	}

	const QUrl localUrl(cache->getPathForUrl(url));

	m_ui->addressLabelWidget->setText(url.toString(QUrl::FullyDecoded | QUrl::PreferLocalFile));
	m_ui->addressLabelWidget->setUrl(url);
	m_ui->locationLabelWidget->setText(localUrl.toString(QUrl::FullyDecoded | QUrl::PreferLocalFile));
	m_ui->locationLabelWidget->setUrl(localUrl);
	m_ui->typeLabelWidget->setText(mimeType.name());
	m_ui->sizeLabelWidget->setText(device ? Utils::formatUnit(device->size(), false, 2) : tr("Unknown"));
	m_ui->lastModifiedLabelWidget->setText(Utils::formatDateTime(metaData.lastModified()));
	m_ui->expiresLabelWidget->setText(Utils::formatDateTime(metaData.expirationDate()));

	if (!preview.isNull())
	{
		m_ui->previewLabel->show();
		m_ui->previewLabel->setPixmap(preview);
	}

	QStandardItem *typeItem(m_model->itemFromIndex(index.sibling(index.row(), 1)));

	if (typeItem && typeItem->text().isEmpty())
	{
		typeItem->setText(mimeType.name());
	}

	QStandardItem *lastModifiedItem(m_model->itemFromIndex(index.sibling(index.row(), 3)));

	if (lastModifiedItem && lastModifiedItem->text().isEmpty())
	{
		lastModifiedItem->setText(metaData.lastModified().toString());
	}

	QStandardItem *expiresItem(m_model->itemFromIndex(index.sibling(index.row(), 4)));

	if (expiresItem && expiresItem->text().isEmpty())
	{
		expiresItem->setText(metaData.expirationDate().toString());
	}

	if (device)
	{
		QStandardItem *sizeItem(m_model->itemFromIndex(index.sibling(index.row(), 2)));

		if (sizeItem && sizeItem->text().isEmpty())
		{
			sizeItem->setText(Utils::formatUnit(device->size()));
			sizeItem->setData(device->size(), SizeRole);

			QStandardItem *domainSizeItem(sizeItem->parent() ? m_model->item(sizeItem->parent()->row(), 2) : nullptr);

			if (domainSizeItem)
			{
				domainSizeItem->setData((domainSizeItem->data(SizeRole).toLongLong() + device->size()), SizeRole);
				domainSizeItem->setText(Utils::formatUnit(domainSizeItem->data(SizeRole).toLongLong()));
			}
		}

		device->deleteLater();
	}

	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::EditingCategory});
}

QStandardItem* CacheContentsWidget::findDomainItem(const QString &domain)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *domainItem(m_model->item(i, 0));

		if (domainItem && domain == domainItem->toolTip())
		{
			return domainItem;
		}
	}

	return nullptr;
}

QString CacheContentsWidget::getTitle() const
{
	return tr("Cache");
}

QLatin1String CacheContentsWidget::getType() const
{
	return QLatin1String("cache");
}

QUrl CacheContentsWidget::getUrl() const
{
	return {QLatin1String("about:cache")};
}

QIcon CacheContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("cache"), false);
}

QUrl CacheContentsWidget::getEntry(const QModelIndex &index) const
{
	if (index.isValid() && index.parent().isValid() && index.parent().parent() == m_model->invisibleRootItem()->index())
	{
		return index.sibling(index.row(), 0).data(UrlRole).toUrl();
	}

	return {};
}

ActionsManager::ActionDefinition::State CacheContentsWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	if (identifier == ActionsManager::DeleteAction)
	{
		ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());
		state.isEnabled = m_ui->deleteButton->isEnabled();

		return state;
	}

	return ContentsWidget::getActionState(identifier, parameters);
}

WebWidget::LoadingState CacheContentsWidget::getLoadingState() const
{
	return (m_isLoading ? WebWidget::OngoingLoadingState : WebWidget::FinishedLoadingState);
}

bool CacheContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->cacheViewWidget && event->type() == QEvent::KeyPress)
	{
		const QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		switch (keyEvent->key())
		{
			case Qt::Key_Delete:
				removeDomainEntriesOrEntry();

				return true;
			case Qt::Key_Enter:
			case Qt::Key_Return:
				openEntry();

				return true;
			default:
				break;
		}
	}
	else if (object == m_ui->cacheViewWidget->viewport() && event->type() == QEvent::MouseButtonRelease)
	{
		const QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton)
		{
			const QModelIndex entryIndex(m_ui->cacheViewWidget->currentIndex());

			if (!entryIndex.isValid() || entryIndex.parent() == m_model->invisibleRootItem()->index())
			{
				return ContentsWidget::eventFilter(object, event);
			}

			MainWindow *mainWindow(MainWindow::findMainWindow(this));
			const QUrl url(getEntry(entryIndex));

			if (mainWindow && url.isValid())
			{
				mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(SessionsManager::calculateOpenHints(SessionsManager::NewTabOpen, mouseEvent->button(), mouseEvent->modifiers()))}});

				return true;
			}
		}
	}

	return ContentsWidget::eventFilter(object, event);
}

}
