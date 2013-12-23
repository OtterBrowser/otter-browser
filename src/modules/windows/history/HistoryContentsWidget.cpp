#include "HistoryContentsWidget.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/ItemDelegate.h"

#include "ui_HistoryContentsWidget.h"

#include <QtCore/QSettings>

namespace Otter
{

HistoryContentsWidget::HistoryContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::HistoryContentsWidget)
{
	m_ui->setupUi(this);

	QStringList groups;
	groups << tr("Today") << tr("Yesterday") << tr("Earlier This Week") << tr("Previous Week") << tr("Earlier This Month") << tr("Earlier This Year") << tr("Older");

	for (int i = 0; i < groups.count(); ++i)
	{
		m_model->appendRow(new QStandardItem(Utils::getIcon("inode-directory"), groups.at(i)));
	}

	updateGroups();

	QStringList labels;
	labels << tr("Address") << tr("Title") << tr("Date");

	m_model->setHorizontalHeaderLabels(labels);
	m_model->setSortRole(Qt::DisplayRole);

	m_ui->historyView->setModel(m_model);
	m_ui->historyView->setItemDelegate(new ItemDelegate(this));
	m_ui->historyView->header()->setTextElideMode(Qt::ElideRight);
	m_ui->historyView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_ui->historyView->expand(m_model->index(0, 0));

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		m_ui->historyView->setRowHidden(i, m_model->invisibleRootItem()->index(), true);

		QStandardItem *groupItem = m_model->item(i, 0);

		if (groupItem)
		{
			groupItem->sortChildren(2, Qt::DescendingOrder);
		}
	}

	const QList<HistoryEntry> entries = HistoryManager::getEntries();

	for (int i = 0; i < entries.count(); ++i)
	{
		addEntry(entries.at(i), false);
	}

	connect(HistoryManager::getInstance(), SIGNAL(cleared()), this, SLOT(clearEntries()));
	connect(HistoryManager::getInstance(), SIGNAL(entryAdded(qint64)), this, SLOT(addEntry(qint64)));
	connect(HistoryManager::getInstance(), SIGNAL(entryUpdated(qint64)), this, SLOT(updateEntry(qint64)));
	connect(HistoryManager::getInstance(), SIGNAL(entryRemoved(qint64)), this, SLOT(removeEntry(qint64)));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterHistory(QString)));
	connect(m_ui->historyView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openEntry(QModelIndex)));
}

HistoryContentsWidget::~HistoryContentsWidget()
{
	delete m_ui;
}

void HistoryContentsWidget::changeEvent(QEvent *event)
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

void HistoryContentsWidget::updateGroups()
{
//TODO call each midnight, update each entry (except older)
	const QDate date = QDate::currentDate();
	QList<QDate> dates;
	dates << date << date.addDays(-1) << date.addDays(-7) << date.addDays(-14) << date.addDays(-30) << date.addDays(-365);

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem = m_model->item(i, 0);

		if (groupItem)
		{
			groupItem->setData(dates.value(i, QDate()), Qt::UserRole);
		}
	}
}

void HistoryContentsWidget::print(QPrinter *printer)
{
	m_ui->historyView->render(printer);
}

void HistoryContentsWidget::triggerAction(WindowAction action, bool checked)
{
	Q_UNUSED(action)
	Q_UNUSED(checked)
}

void HistoryContentsWidget::setHistory(const WindowHistoryInformation &history)
{
	Q_UNUSED(history)
}

void HistoryContentsWidget::setZoom(int zoom)
{
	Q_UNUSED(zoom)
}

void HistoryContentsWidget::setUrl(const QUrl &url)
{
	Q_UNUSED(url)
}

void HistoryContentsWidget::filterHistory(const QString &filter)
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

			m_ui->historyView->setRowHidden(j, groupItem->index(), !match);
		}

		m_ui->historyView->setRowHidden(i, m_model->invisibleRootItem()->index(), !found);
		m_ui->historyView->setExpanded(groupItem->index(), !filter.isEmpty());
	}
}

void HistoryContentsWidget::clearEntries()
{
	m_model->clear();
}

void HistoryContentsWidget::addEntry(qint64 entry)
{
	addEntry(HistoryManager::getEntry(entry));
}

void HistoryContentsWidget::addEntry(const HistoryEntry &entry, bool sort)
{
	if (entry.identifier < 0)
	{
		return;
	}

	QStandardItem *groupItem = NULL;

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		groupItem = m_model->item(i, 0);

		if (groupItem && (entry.time.date() >= groupItem->data(Qt::UserRole).toDate() || !groupItem->data(Qt::UserRole).toDate().isValid()))
		{
			break;
		}

		groupItem = NULL;
	}

	if (!groupItem)
	{
		return;
	}

	QList<QStandardItem*> entryItems;
	entryItems.append(new QStandardItem((entry.icon.isNull() ? Utils::getIcon("text-html") : entry.icon), entry.url.toString()));
	entryItems.append(new QStandardItem(entry.title.isEmpty() ? tr("(Untitled)") : entry.title));
	entryItems.append(new QStandardItem(entry.time.toString()));
	entryItems[0]->setData(entry.identifier, Qt::UserRole);

	groupItem->appendRow(entryItems);

	m_ui->historyView->setRowHidden(groupItem->row(), groupItem->index().parent(), false);

	if (sort)
	{
		groupItem->sortChildren(2, Qt::DescendingOrder);
	}

	if (!m_ui->filterLineEdit->text().isEmpty())
	{
		filterHistory(m_ui->filterLineEdit->text());
	}
}

void HistoryContentsWidget::updateEntry(qint64 entry)
{
	QStandardItem *entryItem = findEntry(entry);

	if (!entryItem)
	{
		addEntry(entry);

		return;
	}

	HistoryEntry historyEntry = HistoryManager::getEntry(entry);

	entryItem->setIcon(historyEntry.icon.isNull() ? Utils::getIcon("text-html") : historyEntry.icon);
	entryItem->setText(historyEntry.url.toString());
	entryItem->parent()->child(entryItem->row(), 1)->setText(historyEntry.title.isEmpty() ? tr("(Untitled)") : historyEntry.title);
	entryItem->parent()->child(entryItem->row(), 2)->setText(historyEntry.time.toString());

	if (!m_ui->filterLineEdit->text().isEmpty())
	{
		filterHistory(m_ui->filterLineEdit->text());
	}
}

void HistoryContentsWidget::removeEntry(qint64 entry)
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
				m_ui->historyView->setRowHidden(groupItem->row(), m_model->invisibleRootItem()->index(), true);
			}
		}
	}
}

void HistoryContentsWidget::openEntry(const QModelIndex &index)
{
	if (!index.isValid() || index.parent() == m_model->invisibleRootItem()->index())
	{
		return;
	}

	const QUrl url(index.sibling(index.row(), 0).data(Qt::DisplayRole).toString());

	if (url.isValid())
	{
		emit requestedOpenUrl(url);
	}
}

ContentsWidget* HistoryContentsWidget::clone(Window *window)
{
	Q_UNUSED(window)

	return NULL;
}

QAction* HistoryContentsWidget::getAction(WindowAction action)
{
	Q_UNUSED(action)

	return NULL;
}

QUndoStack* HistoryContentsWidget::getUndoStack()
{
	return NULL;
}

QStandardItem* HistoryContentsWidget::findEntry(qint64 entry)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem = m_model->item(i, 0);

		if (groupItem)
		{
			for (int j = 0; j < groupItem->rowCount(); ++j)
			{
				QStandardItem *entryItem = groupItem->child(j, 0);

				if (entryItem && entry == entryItem->data(Qt::UserRole).toLongLong())
				{
					return entryItem;
				}
			}
		}
	}

	return NULL;
}

QString HistoryContentsWidget::getTitle() const
{
	return tr("History");
}

QString HistoryContentsWidget::getType() const
{
	return "history";
}

QUrl HistoryContentsWidget::getUrl() const
{
	return QUrl("about:history");
}

QIcon HistoryContentsWidget::getIcon() const
{
	return QIcon(":/icons/view-history.png");
}

QPixmap HistoryContentsWidget::getThumbnail() const
{
	return QPixmap();
}

WindowHistoryInformation HistoryContentsWidget::getHistory() const
{
	WindowHistoryEntry entry;
	entry.url = getUrl().toString();
	entry.title = getTitle();
	entry.position = QPoint(0, 0);
	entry.zoom = 100;

	WindowHistoryInformation information;
	information.index = 0;
	information.entries.append(entry);

	return information;
}

int HistoryContentsWidget::getZoom() const
{
	return 100;
}

bool HistoryContentsWidget::canZoom() const
{
	return false;
}

bool HistoryContentsWidget::isClonable() const
{
	return false;
}

bool HistoryContentsWidget::isLoading() const
{
	return false;
}

bool HistoryContentsWidget::isPrivate() const
{
	return false;
}

}
