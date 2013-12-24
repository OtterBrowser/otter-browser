#include "CacheContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/NetworkAccessManager.h"
#include "../../../core/NetworkCache.h"
#include "../../../core/Utils.h"
#include "../../../ui/ItemDelegate.h"

#include "ui_CacheContentsWidget.h"

#include <QtGui/QClipboard>
#include <QtWidgets/QMenu>

namespace Otter
{

CacheContentsWidget::CacheContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::CacheContentsWidget)
{
	m_ui->setupUi(this);

	NetworkCache *cache = NetworkAccessManager::getCache();
	QStringList labels;
	labels << tr("Address") << tr("Type") << tr("Size") << tr("Last Modified") << tr("Expires");

	m_model->setHorizontalHeaderLabels(labels);
	m_model->setSortRole(Qt::DisplayRole);

	m_ui->cacheView->setModel(m_model);
	m_ui->cacheView->setItemDelegate(new ItemDelegate(this));
	m_ui->cacheView->header()->setTextElideMode(Qt::ElideRight);
	m_ui->cacheView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_ui->cacheView->expand(m_model->index(0, 0));

	connect(cache, SIGNAL(cleared()), this, SLOT(clearEntries()));
	connect(cache, SIGNAL(entryAdded(QUrl)), this, SLOT(addEntry(QUrl)));
	connect(cache, SIGNAL(entryRemoved(QUrl)), this, SLOT(removeEntry(QUrl)));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterCache(QString)));
	connect(m_ui->cacheView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openEntry(QModelIndex)));
	connect(m_ui->cacheView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
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

void CacheContentsWidget::filterCache(const QString &filter)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem = m_model->item(i, 0);

		if (!groupItem)
		{
			continue;
		}

		bool found = (filter.isEmpty() && groupItem->rowCount() > 0);

		for (int j = 0; j < groupItem->rowCount(); ++j)
		{
			QStandardItem *entryItem = groupItem->child(j, 0);

			if (!entryItem)
			{
				continue;
			}

			const bool match = (filter.isEmpty() || entryItem->text().contains(filter, Qt::CaseInsensitive) || groupItem->child(j, 1)->text().contains(filter, Qt::CaseInsensitive));

			if (match)
			{
				found = true;
			}

			m_ui->cacheView->setRowHidden(j, groupItem->index(), !match);
		}

		m_ui->cacheView->setRowHidden(i, m_model->invisibleRootItem()->index(), !found);
		m_ui->cacheView->setExpanded(groupItem->index(), !filter.isEmpty());
	}
}

void CacheContentsWidget::clearEntries()
{
	m_model->clear();
}

void CacheContentsWidget::addEntry(const QUrl &entry, bool sort)
{
	Q_UNUSED(entry)
	Q_UNUSED(sort)

//TODO
}

void CacheContentsWidget::removeEntry(const QUrl &entry)
{
	QStandardItem *entryItem = findEntry(entry);

	if (entryItem)
	{
		QStandardItem *groupItem = entryItem->parent();

		if (groupItem)
		{
			m_model->removeRow(entryItem->row(), groupItem->index());

			if (groupItem->rowCount() == 0)
			{
//TODO
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
	QStandardItem *domainItem = findEntry(getEntry(m_ui->cacheView->currentIndex()));

	if (!domainItem)
	{
		return;
	}

	const QString host = QUrl(domainItem->text()).host();

	for (int i = (m_model->rowCount() - 1); i >= 0; --i)
	{
		QStandardItem *groupItem = m_model->item(i, 0);

		if (!groupItem)
		{
			continue;
		}

		for (int j = (groupItem->rowCount() - 1); j >= 0; --j)
		{
			QStandardItem *entryItem = groupItem->child(j, 0);

			if (entryItem && host == QUrl(entryItem->text()).host())
			{
//TODO
			}
		}
	}
}

void CacheContentsWidget::openEntry(const QModelIndex &index)
{
	const QModelIndex entryIndex = (index.isValid() ? index : m_ui->cacheView->currentIndex());

	if (!entryIndex.isValid() || entryIndex.parent() == m_model->invisibleRootItem()->index())
	{
		return;
	}

	const QUrl url(entryIndex.sibling(entryIndex.row(), 0).data(Qt::DisplayRole).toString());

	if (url.isValid())
	{
		QAction *action = qobject_cast<QAction*>(sender());

		emit requestedOpenUrl(url, false, (action && action->objectName().contains("background")), (action && action->objectName().contains("window")));
	}
}

void CacheContentsWidget::copyEntryLink()
{
	QStandardItem *entryItem = findEntry(getEntry(m_ui->cacheView->currentIndex()));

	if (entryItem)
	{
		QApplication::clipboard()->setText(entryItem->text());
	}
}

void CacheContentsWidget::showContextMenu(const QPoint &point)
{
	const QUrl entry = getEntry(m_ui->cacheView->indexAt(point));
	QMenu menu(this);

	if (entry.isValid())
	{
		menu.addAction(Utils::getIcon("document-open"), tr("Open"), this, SLOT(openEntry()));
		menu.addAction(tr("Open in New Tab"), this, SLOT(openEntry()))->setObjectName("new-tab");
		menu.addAction(tr("Open in New Background Tab"), this, SLOT(openEntry()))->setObjectName("new-background-tab");
		menu.addSeparator();
		menu.addAction(tr("Open in New Window"), this, SLOT(openEntry()))->setObjectName("new-window");
		menu.addAction(tr("Open in New Background Window"), this, SLOT(openEntry()))->setObjectName("new-background-window");
		menu.addSeparator();
		menu.addAction(tr("Copy Link to Clipboard"), this, SLOT(copyEntryLink()));
		menu.addSeparator();
		menu.addAction(tr("Remove Entry"), this, SLOT(removeEntry()));
		menu.addAction(tr("Remove All Entries from This Domain"), this, SLOT(removeDomainEntries()));
		menu.addSeparator();
	}

	menu.addAction(ActionsManager::getAction("Clearcache"));
	menu.exec(m_ui->cacheView->mapToGlobal(point));
}

QStandardItem* CacheContentsWidget::findEntry(const QUrl &entry)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem = m_model->item(i, 0);

		if (groupItem)
		{
			for (int j = 0; j < groupItem->rowCount(); ++j)
			{
				QStandardItem *entryItem = groupItem->child(j, 0);

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

QString CacheContentsWidget::getType() const
{
	return "cache";
}

QUrl CacheContentsWidget::getUrl() const
{
	return QUrl("about:cache");
}

QIcon CacheContentsWidget::getIcon() const
{
	return QIcon(":/icons/cache.png");
}

QUrl CacheContentsWidget::getEntry(const QModelIndex &index) const
{
	return ((index.isValid() && index.parent().isValid() && index.parent().parent() == m_model->invisibleRootItem()->index()) ? index.sibling(index.row(), 0).data(Qt::UserRole).toUrl() : QUrl());
}

}
