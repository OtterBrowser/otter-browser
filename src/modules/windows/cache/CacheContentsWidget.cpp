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

#include "CacheContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/NetworkAccessManager.h"
#include "../../../core/NetworkCache.h"
#include "../../../core/Utils.h"
#include "../../../core/WebBackend.h"
#include "../../../core/WebBackendsManager.h"
#include "../../../ui/ItemDelegate.h"

#include "ui_CacheContentsWidget.h"

#include <QtCore/QMimeDatabase>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtWidgets/QMenu>

namespace Otter
{

CacheContentsWidget::CacheContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_isLoading(true),
	m_ui(new Ui::CacheContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->previewLabel->hide();

	QTimer::singleShot(100, this, SLOT(populateCache()));

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterCache(QString)));
	connect(m_ui->cacheView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openEntry(QModelIndex)));
	connect(m_ui->cacheView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(removeDomainEntriesOrEntry()));
}

CacheContentsWidget::~CacheContentsWidget()
{
	delete m_ui;
}

void CacheContentsWidget::changeEvent(QEvent *event)
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

void CacheContentsWidget::print(QPrinter *printer)
{
	m_ui->cacheView->render(printer);
}

void CacheContentsWidget::populateCache()
{
	NetworkCache *cache = NetworkAccessManager::getCache();
	QStringList labels;
	labels << tr("Address") << tr("Type") << tr("Size") << tr("Last Modified") << tr("Expires");

	m_model->setHorizontalHeaderLabels(labels);
	m_model->setSortRole(Qt::DisplayRole);

	const QList<QUrl> entries = cache->getEntries();

	for (int i = 0; i < entries.count(); ++i)
	{
		addEntry(entries.at(i));
	}

	m_model->sort(0);

	m_ui->cacheView->setModel(m_model);
	m_ui->cacheView->setItemDelegate(new ItemDelegate(this));
	m_ui->cacheView->header()->setTextElideMode(Qt::ElideRight);
	m_ui->cacheView->header()->setSectionResizeMode(0, QHeaderView::Stretch);

	m_isLoading = false;

	emit loadingChanged(false);

	connect(cache, SIGNAL(cleared()), this, SLOT(clearEntries()));
	connect(cache, SIGNAL(entryAdded(QUrl)), this, SLOT(addEntry(QUrl)));
	connect(cache, SIGNAL(entryRemoved(QUrl)), this, SLOT(removeEntry(QUrl)));
	connect(m_model, SIGNAL(modelReset()), this, SLOT(updateActions()));
	connect(m_ui->cacheView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
}

void CacheContentsWidget::filterCache(const QString &filter)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *domainItem = m_model->item(i, 0);

		if (!domainItem)
		{
			continue;
		}

		bool found = (filter.isEmpty() && domainItem->rowCount() > 0);

		for (int j = 0; j < domainItem->rowCount(); ++j)
		{
			QStandardItem *entryItem = domainItem->child(j, 0);

			if (!entryItem)
			{
				continue;
			}

			const bool match = (filter.isEmpty() || entryItem->data(Qt::UserRole).toString().contains(filter, Qt::CaseInsensitive) || domainItem->child(j, 1)->text().contains(filter, Qt::CaseInsensitive));

			if (match)
			{
				found = true;
			}

			m_ui->cacheView->setRowHidden(j, domainItem->index(), !match);
		}

		m_ui->cacheView->setRowHidden(i, m_model->invisibleRootItem()->index(), !found);
		m_ui->cacheView->setExpanded(domainItem->index(), !filter.isEmpty());
	}
}

void CacheContentsWidget::clearEntries()
{
	m_model->clear();
}

void CacheContentsWidget::addEntry(const QUrl &entry)
{
	const QString domain = entry.host();
	QStandardItem *domainItem = findDomain(domain);

	if (domainItem)
	{
		for (int i = 0; i < domainItem->rowCount(); ++i)
		{
			QStandardItem *entryItem = domainItem->child(i, 0);

			if (entry == entryItem->data(Qt::UserRole).toUrl())
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
		m_model->setItem(domainItem->row(), 2, new QStandardItem(QString()));

		if (sender())
		{
			m_model->sort(0);
		}
	}

	NetworkCache *cache = NetworkAccessManager::getCache();
	QIODevice *device = cache->data(entry);
	const QNetworkCacheMetaData metaData = cache->metaData(entry);
	const QList<QPair<QByteArray, QByteArray> > headers = metaData.rawHeaders();
	QString type;

	for (int i = 0; i < headers.count(); ++i)
	{
		if (headers.at(i).first == QStringLiteral("Content-Type").toLatin1())
		{
			type = QString(headers.at(i).second);

			break;
		}
	}

	const QMimeType mimeType = ((type.isEmpty() && device) ? QMimeDatabase().mimeTypeForData(device) : QMimeDatabase().mimeTypeForName(type));
	QList<QStandardItem*> entryItems;
	entryItems.append(new QStandardItem(entry.path()));
	entryItems.append(new QStandardItem(mimeType.name()));
	entryItems.append(new QStandardItem(device ? Utils::formatUnit(device->size()) : QString()));
	entryItems.append(new QStandardItem(metaData.lastModified().toString()));
	entryItems.append(new QStandardItem(metaData.expirationDate().toString()));
	entryItems[0]->setData(entry, Qt::UserRole);

	QStandardItem *sizeItem = m_model->item(domainItem->row(), 2);

	if (sizeItem && device)
	{
		sizeItem->setData((sizeItem->data(Qt::UserRole).toLongLong() + device->size()), Qt::UserRole);
		sizeItem->setText(Utils::formatUnit(sizeItem->data(Qt::UserRole).toLongLong()));
	}

	if (device)
	{
		device->deleteLater();
	}

	domainItem->appendRow(entryItems);
	domainItem->setText(QString("%1 (%2)").arg(domain).arg(domainItem->rowCount()));

	if (sender())
	{
		domainItem->sortChildren(0, Qt::DescendingOrder);
	}

	if (!m_ui->filterLineEdit->text().isEmpty())
	{
		filterCache(m_ui->filterLineEdit->text());
	}
}

void CacheContentsWidget::removeEntry(const QUrl &entry)
{
	QStandardItem *entryItem = findEntry(entry);

	if (entryItem)
	{
		QStandardItem *domainItem = entryItem->parent();

		if (domainItem)
		{
			m_model->removeRow(entryItem->row(), domainItem->index());

			if (domainItem->rowCount() == 0)
			{
				m_model->invisibleRootItem()->removeRow(domainItem->row());
			}
		}
	}
}

void CacheContentsWidget::removeEntry()
{
	const QUrl entry = getEntry(m_ui->cacheView->currentIndex());

	if (entry.isValid())
	{
		NetworkAccessManager::getCache()->remove(entry);
	}
}

void CacheContentsWidget::removeDomainEntries()
{
	const QModelIndex index = m_ui->cacheView->currentIndex();
	QStandardItem *domainItem = ((index.isValid() && index.parent() == m_model->invisibleRootItem()->index()) ? findDomain(index.sibling(index.row(), 0).data(Qt::ToolTipRole).toString()) : findEntry(getEntry(index)));

	if (!domainItem)
	{
		return;
	}

	NetworkCache *cache = NetworkAccessManager::getCache();

	for (int i = (domainItem->rowCount() - 1); i >= 0; --i)
	{
		QStandardItem *entryItem = domainItem->child(i, 0);

		if (entryItem)
		{
			cache->remove(entryItem->data(Qt::UserRole).toUrl());
		}
	}
}

void CacheContentsWidget::removeDomainEntriesOrEntry()
{
	const QUrl entry = getEntry(m_ui->cacheView->currentIndex());

	if (entry.isValid())
	{
		NetworkAccessManager::getCache()->remove(entry);
	}
	else
	{
		removeDomainEntries();
	}
}

void CacheContentsWidget::openEntry(const QModelIndex &index)
{
	const QModelIndex entryIndex = (index.isValid() ? index : m_ui->cacheView->currentIndex());

	if (!entryIndex.isValid() || entryIndex.parent() == m_model->invisibleRootItem()->index())
	{
		return;
	}

	const QUrl url = entryIndex.sibling(entryIndex.row(), 0).data(Qt::UserRole).toUrl();

	if (url.isValid())
	{
		QAction *action = qobject_cast<QAction*>(sender());

		emit requestedOpenUrl(url, false, (action && action->objectName().contains(QLatin1String("background"))), (action && action->objectName().contains(QLatin1String("window"))));
	}
}

void CacheContentsWidget::copyEntryLink()
{
	QStandardItem *entryItem = findEntry(getEntry(m_ui->cacheView->currentIndex()));

	if (entryItem)
	{
		QApplication::clipboard()->setText(entryItem->data(Qt::UserRole).toString());
	}
}

void CacheContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index = m_ui->cacheView->indexAt(point);
	const QUrl entry = getEntry(index);
	QMenu menu(this);

	if (entry.isValid())
	{
		menu.addAction(Utils::getIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openEntry()));
		menu.addAction(tr("Open in New Tab"), this, SLOT(openEntry()))->setObjectName(QLatin1String("new-tab"));
		menu.addAction(tr("Open in New Background Tab"), this, SLOT(openEntry()))->setObjectName(QLatin1String("new-background-tab"));
		menu.addSeparator();
		menu.addAction(tr("Open in New Window"), this, SLOT(openEntry()))->setObjectName(QLatin1String("new-window"));
		menu.addAction(tr("Open in New Background Window"), this, SLOT(openEntry()))->setObjectName(QLatin1String("new-background-window"));
		menu.addSeparator();
		menu.addAction(tr("Copy Link to Clipboard"), this, SLOT(copyEntryLink()));
		menu.addSeparator();
		menu.addAction(tr("Remove Entry"), this, SLOT(removeEntry()));
	}

	if (entry.isValid() || (index.isValid() && index.parent() == m_model->invisibleRootItem()->index()))
	{
		menu.addAction(tr("Remove All Entries from This Domain"), this, SLOT(removeDomainEntries()));
		menu.addSeparator();
	}

	menu.addAction(ActionsManager::getAction(QLatin1String("ClearHistory")));
	menu.exec(m_ui->cacheView->mapToGlobal(point));
}

void CacheContentsWidget::updateActions()
{
	const QModelIndex index = (m_ui->cacheView->selectionModel()->hasSelection() ? m_ui->cacheView->selectionModel()->currentIndex() : QModelIndex());
	const QUrl entry = getEntry(index);
	const QString domain = ((index.isValid() && index.parent() == m_model->invisibleRootItem()->index()) ? index.sibling(index.row(), 0).data(Qt::ToolTipRole).toString() : entry.host());

	m_ui->locationLabelWidget->setText(QString());
	m_ui->previewLabel->hide();
	m_ui->previewLabel->setPixmap(QPixmap());
	m_ui->deleteButton->setEnabled(!domain.isEmpty());

	if (entry.isValid())
	{
		NetworkCache *cache = NetworkAccessManager::getCache();
		QIODevice *device = cache->data(entry);
		const QNetworkCacheMetaData metaData = cache->metaData(entry);
		const QList<QPair<QByteArray, QByteArray> > headers = metaData.rawHeaders();
		QString type;

		for (int i = 0; i < headers.count(); ++i)
		{
			if (headers.at(i).first == QStringLiteral("Content-Type").toLatin1())
			{
				type = QString(headers.at(i).second);

				break;
			}
		}

		const QMimeType mimeType = ((type.isEmpty() && device) ? QMimeDatabase().mimeTypeForData(device) : QMimeDatabase().mimeTypeForName(type));
		QPixmap preview;
		const int size = (m_ui->formWidget->contentsRect().height() - 10);

		if (mimeType.name().startsWith(QLatin1String("image")) && device)
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
			preview = QIcon::fromTheme(mimeType.iconName(), Utils::getIcon(QLatin1String("unknown"))).pixmap(64, 64);
		}

		m_ui->addressLabelWidget->setText(entry.toString(QUrl::FullyDecoded | QUrl::PreferLocalFile));
		m_ui->locationLabelWidget->setText(cache->getPathForUrl(entry));
		m_ui->typeLabelWidget->setText(mimeType.name());
		m_ui->sizeLabelWidget->setText(device ? Utils::formatUnit(device->size(), false, 2) : tr("Unknown"));
		m_ui->lastModifiedLabelWidget->setText(metaData.lastModified().toString());
		m_ui->expiresLabelWidget->setText(metaData.expirationDate().toString());

		if (!preview.isNull())
		{
			m_ui->previewLabel->show();
			m_ui->previewLabel->setPixmap(preview);
		}

		QStandardItem *typeItem = m_model->itemFromIndex(index.sibling(index.row(), 1));

		if (typeItem && typeItem->text().isEmpty())
		{
			typeItem->setText(mimeType.name());
		}

		QStandardItem *lastModifiedItem = m_model->itemFromIndex(index.sibling(index.row(), 3));

		if (lastModifiedItem && lastModifiedItem->text().isEmpty())
		{
			lastModifiedItem->setText(metaData.lastModified().toString());
		}

		QStandardItem *expiresItem = m_model->itemFromIndex(index.sibling(index.row(), 4));

		if (expiresItem && expiresItem->text().isEmpty())
		{
			expiresItem->setText(metaData.expirationDate().toString());
		}

		if (device)
		{
			QStandardItem *sizeItem = m_model->itemFromIndex(index.sibling(index.row(), 2));

			if (sizeItem && sizeItem->text().isEmpty())
			{
				sizeItem->setText(Utils::formatUnit(device->size()));

				QStandardItem *domainSizeItem = (sizeItem->parent() ? m_model->item(sizeItem->parent()->row(), 2) : NULL);

				if (domainSizeItem)
				{
					domainSizeItem->setData((domainSizeItem->data(Qt::UserRole).toLongLong() + device->size()), Qt::UserRole);
					domainSizeItem->setText(Utils::formatUnit(domainSizeItem->data(Qt::UserRole).toLongLong()));
				}
			}

			device->deleteLater();
		}
	}
	else
	{
		m_ui->addressLabelWidget->setText(QString());
		m_ui->typeLabelWidget->setText(QString());
		m_ui->sizeLabelWidget->setText(QString());
		m_ui->lastModifiedLabelWidget->setText(QString());
		m_ui->expiresLabelWidget->setText(QString());

		if (!domain.isEmpty())
		{
			m_ui->addressLabelWidget->setText(domain);
		}
	}
}

QStandardItem* CacheContentsWidget::findDomain(const QString &domain)
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

QStandardItem* CacheContentsWidget::findEntry(const QUrl &entry)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *domainItem = m_model->item(i, 0);

		if (domainItem)
		{
			for (int j = 0; j < domainItem->rowCount(); ++j)
			{
				QStandardItem *entryItem = domainItem->child(j, 0);

				if (entryItem && entry == entryItem->data(Qt::UserRole).toUrl())
				{
					return entryItem;
				}
			}
		}
	}

	return NULL;
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
	return QUrl(QLatin1String("about:cache"));
}

QIcon CacheContentsWidget::getIcon() const
{
	return Utils::getIcon(QLatin1String("cache"), false);
}

QUrl CacheContentsWidget::getEntry(const QModelIndex &index) const
{
	return ((index.isValid() && index.parent().isValid() && index.parent().parent() == m_model->invisibleRootItem()->index()) ? index.sibling(index.row(), 0).data(Qt::UserRole).toUrl() : QUrl());
}

bool CacheContentsWidget::isLoading() const
{
	return m_isLoading;
}

}
