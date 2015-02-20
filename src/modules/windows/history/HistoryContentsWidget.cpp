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

#include "HistoryContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/ItemDelegate.h"

#include "ui_HistoryContentsWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QMenu>

namespace Otter
{

HistoryContentsWidget::HistoryContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_isLoading(true),
	m_ui(new Ui::HistoryContentsWidget)
{
	m_ui->setupUi(this);

	QStringList groups;
	groups << tr("Today") << tr("Yesterday") << tr("Earlier This Week") << tr("Previous Week") << tr("Earlier This Month") << tr("Earlier This Year") << tr("Older");

	for (int i = 0; i < groups.count(); ++i)
	{
		m_model->appendRow(new QStandardItem(Utils::getIcon(QLatin1String("inode-directory")), groups.at(i)));
	}

	QStringList labels;
	labels << tr("Address") << tr("Title") << tr("Date");

	m_model->setHorizontalHeaderLabels(labels);
	m_model->setSortRole(Qt::DisplayRole);

	m_ui->historyView->setModel(m_model);
	m_ui->historyView->setItemDelegate(new ItemDelegate(this));
	m_ui->historyView->header()->setTextElideMode(Qt::ElideRight);
	m_ui->historyView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_ui->historyView->viewport()->installEventFilter(this);

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		m_ui->historyView->setRowHidden(i, m_model->invisibleRootItem()->index(), true);
	}

	const QString expandBranches = SettingsManager::getValue(QLatin1String("History/ExpandBranches")).toString();

	if (expandBranches == QLatin1String("first"))
	{
		m_ui->historyView->expand(m_model->index(0, 0));
	}
	else if (expandBranches == QLatin1String("all"))
	{
		m_ui->historyView->expandAll();
	}

	QTimer::singleShot(100, this, SLOT(populateEntries()));

	connect(HistoryManager::getInstance(), SIGNAL(cleared()), this, SLOT(populateEntries()));
	connect(HistoryManager::getInstance(), SIGNAL(entryAdded(qint64)), this, SLOT(addEntry(qint64)));
	connect(HistoryManager::getInstance(), SIGNAL(entryUpdated(qint64)), this, SLOT(updateEntry(qint64)));
	connect(HistoryManager::getInstance(), SIGNAL(entryRemoved(qint64)), this, SLOT(removeEntry(qint64)));
	connect(HistoryManager::getInstance(), SIGNAL(dayChanged()), this, SLOT(populateEntries()));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterHistory(QString)));
	connect(m_ui->historyView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openEntry(QModelIndex)));
	connect(m_ui->historyView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
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

void HistoryContentsWidget::print(QPrinter *printer)
{
	m_ui->historyView->render(printer);
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

void HistoryContentsWidget::populateEntries()
{
	const QDate date = QDate::currentDate();
	QList<QDate> dates;
	dates << date << date.addDays(-1) << date.addDays(-7) << date.addDays(-14) << date.addDays(-30) << date.addDays(-365);

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem = m_model->item(i, 0);

		if (groupItem)
		{
			groupItem->setData(dates.value(i, QDate()), Qt::UserRole);
			groupItem->removeRows(0, groupItem->rowCount());
		}
	}

	const QList<HistoryEntry> entries = HistoryManager::getEntries();

	for (int i = 0; i < entries.count(); ++i)
	{
		addEntry(entries.at(i));
	}

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem = m_model->item(i, 0);

		if (groupItem)
		{
			groupItem->sortChildren(2, Qt::DescendingOrder);
		}
	}

	m_isLoading = false;

	emit loadingChanged(false);
}

void HistoryContentsWidget::addEntry(qint64 entry)
{
	addEntry(HistoryManager::getEntry(entry));
}

void HistoryContentsWidget::addEntry(const HistoryEntry &entry)
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
	entryItems.append(new QStandardItem((entry.icon.isNull() ? Utils::getIcon(QLatin1String("text-html")) : entry.icon), entry.url.toString().replace(QLatin1String("%23"), QString(QLatin1Char('#')))));
	entryItems.append(new QStandardItem(entry.title.isEmpty() ? tr("(Untitled)") : entry.title));
	entryItems.append(new QStandardItem(entry.time.toString()));
	entryItems[0]->setData(entry.identifier, Qt::UserRole);

	groupItem->appendRow(entryItems);

	m_ui->historyView->setRowHidden(groupItem->row(), groupItem->index().parent(), false);

	if (sender())
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

	entryItem->setIcon(historyEntry.icon.isNull() ? Utils::getIcon(QLatin1String("text-html")) : historyEntry.icon);
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

void HistoryContentsWidget::removeEntry()
{
	const qint64 entry = getEntry(m_ui->historyView->currentIndex());

	if (entry >= 0)
	{
		HistoryManager::removeEntry(entry);
	}
}

void HistoryContentsWidget::removeDomainEntries()
{
	QStandardItem *domainItem = findEntry(getEntry(m_ui->historyView->currentIndex()));

	if (!domainItem)
	{
		return;
	}

	const QString host = QUrl(domainItem->text()).host();
	QList<qint64> entries;

	for (int i = 0; i < m_model->rowCount(); ++i)
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
				entries.append(entryItem->data(Qt::UserRole).toLongLong());
			}
		}
	}

	HistoryManager::removeEntries(entries);
}

void HistoryContentsWidget::openEntry(const QModelIndex &index)
{
	const QModelIndex entryIndex = (index.isValid() ? index : m_ui->historyView->currentIndex());

	if (!entryIndex.isValid() || entryIndex.parent() == m_model->invisibleRootItem()->index())
	{
		return;
	}

	const QUrl url(entryIndex.sibling(entryIndex.row(), 0).data(Qt::DisplayRole).toString());

	if (url.isValid())
	{
		QAction *action = qobject_cast<QAction*>(sender());

		emit requestedOpenUrl(url, (action ? static_cast<OpenHints>(action->data().toInt()) : DefaultOpen));
	}
}

void HistoryContentsWidget::bookmarkEntry()
{
	QStandardItem *entryItem = findEntry(getEntry(m_ui->historyView->currentIndex()));

	if (entryItem)
	{
		emit requestedAddBookmark(QUrl(entryItem->text()), m_ui->historyView->currentIndex().sibling(m_ui->historyView->currentIndex().row(), 1).data(Qt::DisplayRole).toString(), QString());
	}
}

void HistoryContentsWidget::copyEntryLink()
{
	QStandardItem *entryItem = findEntry(getEntry(m_ui->historyView->currentIndex()));

	if (entryItem)
	{
		QApplication::clipboard()->setText(entryItem->text());
	}
}

void HistoryContentsWidget::showContextMenu(const QPoint &point)
{
	const qint64 entry = getEntry(m_ui->historyView->indexAt(point));
	QMenu menu(this);

	if (entry >= 0)
	{
		menu.addAction(Utils::getIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openEntry()));
		menu.addAction(tr("Open in New Tab"), this, SLOT(openEntry()))->setData(NewTabOpen);
		menu.addAction(tr("Open in New Background Tab"), this, SLOT(openEntry()))->setData(NewBackgroundTabOpen);
		menu.addSeparator();
		menu.addAction(tr("Open in New Window"), this, SLOT(openEntry()))->setData(NewWindowOpen);
		menu.addAction(tr("Open in New Background Window"), this, SLOT(openEntry()))->setData(NewBackgroundWindowOpen);
		menu.addSeparator();
		menu.addAction(tr("Add to Bookmarks..."), this, SLOT(bookmarkEntry()));
		menu.addAction(tr("Copy Link to Clipboard"), this, SLOT(copyEntryLink()));
		menu.addSeparator();
		menu.addAction(tr("Remove Entry"), this, SLOT(removeEntry()));
		menu.addAction(tr("Remove All Entries from This Domain"), this, SLOT(removeDomainEntries()));
		menu.addSeparator();
	}

	menu.addAction(ActionsManager::getAction(Action::ClearHistoryAction, this));
	menu.exec(m_ui->historyView->mapToGlobal(point));
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

QLatin1String HistoryContentsWidget::getType() const
{
	return QLatin1String("history");
}

QUrl HistoryContentsWidget::getUrl() const
{
	return QUrl(QLatin1String("about:history"));
}

QIcon HistoryContentsWidget::getIcon() const
{
	return Utils::getIcon(QLatin1String("view-history"), false);
}

qint64 HistoryContentsWidget::getEntry(const QModelIndex &index) const
{
	return ((index.isValid() && index.parent().isValid() && index.parent().parent() == m_model->invisibleRootItem()->index()) ? index.sibling(index.row(), 0).data(Qt::UserRole).toLongLong() : -1);
}

bool HistoryContentsWidget::isLoading() const
{
	return m_isLoading;
}

bool HistoryContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonRelease && object == m_ui->historyView->viewport())
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton))
		{
			const QModelIndex entryIndex = m_ui->historyView->currentIndex();

			if (!entryIndex.isValid() || entryIndex.parent() == m_model->invisibleRootItem()->index())
			{
				return ContentsWidget::eventFilter(object, event);
			}

			const QUrl url(entryIndex.sibling(entryIndex.row(), 0).data(Qt::DisplayRole).toString());

			if (url.isValid())
			{
				emit requestedOpenUrl(url, WindowsManager::calculateOpenHints(mouseEvent->modifiers(), mouseEvent->button(), NewTabOpen));

				return true;
			}
		}
	}

	return ContentsWidget::eventFilter(object, event);
}

}
