/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "HistoryContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"

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

	const QStringList groups({tr("Today"), tr("Yesterday"), tr("Earlier This Week"), tr("Previous Week"), tr("Earlier This Month"), tr("Earlier This Year"), tr("Older")});

	for (int i = 0; i < groups.count(); ++i)
	{
		m_model->appendRow(new QStandardItem(ThemesManager::getIcon(QLatin1String("inode-directory")), groups.at(i)));
	}

	m_model->setHorizontalHeaderLabels(QStringList({tr("Address"), tr("Title"), tr("Date")}));
	m_model->setSortRole(Qt::DisplayRole);

	m_ui->historyViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->historyViewWidget->setModel(m_model, true);
	m_ui->historyViewWidget->installEventFilter(this);
	m_ui->historyViewWidget->viewport()->installEventFilter(this);
	m_ui->filterLineEdit->installEventFilter(this);

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		m_ui->historyViewWidget->setRowHidden(i, m_model->invisibleRootItem()->index(), true);
	}

	QTimer::singleShot(100, this, SLOT(populateEntries()));

	connect(HistoryManager::getBrowsingHistoryModel(), SIGNAL(cleared()), this, SLOT(populateEntries()));
	connect(HistoryManager::getBrowsingHistoryModel(), SIGNAL(entryAdded(HistoryEntryItem*)), this, SLOT(addEntry(HistoryEntryItem*)));
	connect(HistoryManager::getBrowsingHistoryModel(), SIGNAL(entryModified(HistoryEntryItem*)), this, SLOT(modifyEntry(HistoryEntryItem*)));
	connect(HistoryManager::getBrowsingHistoryModel(), SIGNAL(entryRemoved(HistoryEntryItem*)), this, SLOT(removeEntry(HistoryEntryItem*)));
	connect(HistoryManager::getInstance(), SIGNAL(dayChanged()), this, SLOT(populateEntries()));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), m_ui->historyViewWidget, SLOT(setFilterString(QString)));
	connect(m_ui->historyViewWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openEntry(QModelIndex)));
	connect(m_ui->historyViewWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
}

HistoryContentsWidget::~HistoryContentsWidget()
{
	delete m_ui;
}

void HistoryContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void HistoryContentsWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	Q_UNUSED(parameters)

	switch (identifier)
	{
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ActivateAddressFieldAction:
			m_ui->filterLineEdit->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->historyViewWidget->setFocus();

			break;
		default:
			break;
	}
}

void HistoryContentsWidget::print(QPrinter *printer)
{
	m_ui->historyViewWidget->render(printer);
}

void HistoryContentsWidget::populateEntries()
{
	const QDate date(QDate::currentDate());
	const QList<QDate> dates({date, date.addDays(-1), date.addDays(-7), date.addDays(-14), date.addDays(-30), date.addDays(-365)});

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem(m_model->item(i, 0));

		if (groupItem)
		{
			groupItem->setData(dates.value(i, QDate()), Qt::UserRole);
			groupItem->removeRows(0, groupItem->rowCount());
		}
	}

	HistoryModel *model(HistoryManager::getBrowsingHistoryModel());

	for (int i = 0; i < model->rowCount(); ++i)
	{
		addEntry(dynamic_cast<HistoryEntryItem*>(model->item(i, 0)));
	}

	const QString expandBranches(SettingsManager::getOption(SettingsManager::History_ExpandBranchesOption).toString());

	if (expandBranches == QLatin1String("first"))
	{
		for (int i = 0; i < m_model->rowCount(); ++i)
		{
			const QModelIndex index(m_model->index(i, 0));

			if (m_model->rowCount(index) > 0)
			{
				m_ui->historyViewWidget->expand(m_ui->historyViewWidget->getProxyModel()->mapFromSource(index));

				break;
			}
		}
	}
	else if (expandBranches == QLatin1String("all"))
	{
		m_ui->historyViewWidget->expandAll();
	}

	m_isLoading = false;

	emit loadingStateChanged(WindowsManager::FinishedLoadingState);
}

void HistoryContentsWidget::addEntry(HistoryEntryItem *entry)
{
	if (!entry || entry->data(HistoryModel::IdentifierRole).toULongLong() == 0 || findEntry(entry->data(HistoryModel::IdentifierRole).toULongLong()))
	{
		return;
	}

	QStandardItem *groupItem(nullptr);

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		groupItem = m_model->item(i, 0);

		if (groupItem && (entry->data(HistoryModel::TimeVisitedRole).toDateTime().date() >= groupItem->data(Qt::UserRole).toDate() || !groupItem->data(Qt::UserRole).toDate().isValid()))
		{
			break;
		}

		groupItem = nullptr;
	}

	if (!groupItem)
	{
		return;
	}

	QList<QStandardItem*> entryItems({new QStandardItem((entry->icon().isNull() ? ThemesManager::getIcon(QLatin1String("text-html")) : entry->icon()), entry->data(HistoryModel::UrlRole).toUrl().toDisplayString().replace(QLatin1String("%23"), QString(QLatin1Char('#')))), new QStandardItem(entry->data(HistoryModel::TitleRole).isNull() ? tr("(Untitled)") : entry->data(HistoryModel::TitleRole).toString()), new QStandardItem(Utils::formatDateTime(entry->data(HistoryModel::TimeVisitedRole).toDateTime()))});
	entryItems[0]->setData(entry->data(HistoryModel::IdentifierRole).toULongLong(), Qt::UserRole);
	entryItems[0]->setFlags(entryItems[0]->flags() | Qt::ItemNeverHasChildren);
	entryItems[1]->setFlags(entryItems[1]->flags() | Qt::ItemNeverHasChildren);
	entryItems[2]->setFlags(entryItems[2]->flags() | Qt::ItemNeverHasChildren);

	groupItem->appendRow(entryItems);

	m_ui->historyViewWidget->setRowHidden(groupItem->row(), groupItem->index().parent(), false);

	if (sender() && groupItem->rowCount() == 1 && SettingsManager::getOption(SettingsManager::History_ExpandBranchesOption).toString() == QLatin1String("first"))
	{
		for (int i = 0; i < m_model->rowCount(); ++i)
		{
			const QModelIndex index(m_model->index(i, 0));

			if (m_model->rowCount(index) > 0)
			{
				m_ui->historyViewWidget->expand(m_ui->historyViewWidget->getProxyModel()->mapFromSource(index));

				break;
			}
		}
	}
}

void HistoryContentsWidget::modifyEntry(HistoryEntryItem *entry)
{
	if (!entry || entry->data(HistoryModel::IdentifierRole).toULongLong() == 0)
	{
		return;
	}

	QStandardItem *entryItem(findEntry(entry->data(HistoryModel::IdentifierRole).toULongLong()));

	if (!entryItem)
	{
		addEntry(entry);

		return;
	}

	entryItem->setIcon(entry->icon().isNull() ? ThemesManager::getIcon(QLatin1String("text-html")) : entry->icon());
	entryItem->setText(entry->data(HistoryModel::UrlRole).toUrl().toDisplayString());
	entryItem->parent()->child(entryItem->row(), 1)->setText(entry->data(HistoryModel::TitleRole).isNull() ? tr("(Untitled)") : entry->data(HistoryModel::TitleRole).toString());
	entryItem->parent()->child(entryItem->row(), 2)->setText(Utils::formatDateTime(entry->data(HistoryModel::TimeVisitedRole).toDateTime()));
}

void HistoryContentsWidget::removeEntry(HistoryEntryItem *entry)
{
	if (!entry || entry->data(HistoryModel::IdentifierRole).toULongLong() == 0)
	{
		return;
	}

	QStandardItem *entryItem(findEntry(entry->data(HistoryModel::IdentifierRole).toULongLong()));

	if (entryItem)
	{
		QStandardItem *groupItem(entryItem->parent());

		if (groupItem)
		{
			m_model->removeRow(entryItem->row(), groupItem->index());

			if (groupItem->rowCount() == 0)
			{
				m_ui->historyViewWidget->setRowHidden(groupItem->row(), m_model->invisibleRootItem()->index(), true);
			}
		}
	}
}

void HistoryContentsWidget::removeEntry()
{
	const quint64 entry(getEntry(m_ui->historyViewWidget->currentIndex()));

	if (entry > 0)
	{
		HistoryManager::removeEntry(entry);
	}
}

void HistoryContentsWidget::removeDomainEntries()
{
	QStandardItem *domainItem(findEntry(getEntry(m_ui->historyViewWidget->currentIndex())));

	if (!domainItem)
	{
		return;
	}

	const QString host(QUrl(domainItem->text()).host());
	QList<quint64> entries;

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem(m_model->item(i, 0));

		if (!groupItem)
		{
			continue;
		}

		for (int j = (groupItem->rowCount() - 1); j >= 0; --j)
		{
			QStandardItem *entryItem(groupItem->child(j, 0));

			if (entryItem && host == QUrl(entryItem->text()).host())
			{
				entries.append(entryItem->data(Qt::UserRole).toULongLong());
			}
		}
	}

	HistoryManager::removeEntries(entries);
}

void HistoryContentsWidget::openEntry(const QModelIndex &index)
{
	const QModelIndex entryIndex(index.isValid() ? index : m_ui->historyViewWidget->currentIndex());

	if (!entryIndex.isValid() || entryIndex.parent() == m_model->invisibleRootItem()->index())
	{
		return;
	}

	const QUrl url(entryIndex.sibling(entryIndex.row(), 0).data(Qt::DisplayRole).toString());

	if (url.isValid())
	{
		QAction *action(qobject_cast<QAction*>(sender()));

		emit requestedOpenUrl(url, (action ? static_cast<WindowsManager::OpenHints>(action->data().toInt()) : WindowsManager::DefaultOpen));
	}
}

void HistoryContentsWidget::bookmarkEntry()
{
	QStandardItem *entryItem(findEntry(getEntry(m_ui->historyViewWidget->currentIndex())));

	if (entryItem)
	{
		emit requestedAddBookmark(QUrl(entryItem->text()), m_ui->historyViewWidget->currentIndex().sibling(m_ui->historyViewWidget->currentIndex().row(), 1).data(Qt::DisplayRole).toString(), QString());
	}
}

void HistoryContentsWidget::copyEntryLink()
{
	QStandardItem *entryItem(findEntry(getEntry(m_ui->historyViewWidget->currentIndex())));

	if (entryItem)
	{
		QApplication::clipboard()->setText(entryItem->text());
	}
}

void HistoryContentsWidget::showContextMenu(const QPoint &point)
{
	const quint64 entry(getEntry(m_ui->historyViewWidget->indexAt(point)));
	QMenu menu(this);

	if (entry > 0)
	{
		menu.addAction(ThemesManager::getIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openEntry()));
		menu.addAction(tr("Open in New Tab"), this, SLOT(openEntry()))->setData(WindowsManager::NewTabOpen);
		menu.addAction(tr("Open in New Background Tab"), this, SLOT(openEntry()))->setData(static_cast<int>(WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen));
		menu.addSeparator();
		menu.addAction(tr("Open in New Window"), this, SLOT(openEntry()))->setData(WindowsManager::NewWindowOpen);
		menu.addAction(tr("Open in New Background Window"), this, SLOT(openEntry()))->setData(static_cast<int>(WindowsManager::NewWindowOpen | WindowsManager::BackgroundOpen));
		menu.addSeparator();
		menu.addAction(tr("Add to Bookmarks…"), this, SLOT(bookmarkEntry()));
		menu.addAction(tr("Copy Link to Clipboard"), this, SLOT(copyEntryLink()));
		menu.addSeparator();
		menu.addAction(tr("Remove Entry"), this, SLOT(removeEntry()));
		menu.addAction(tr("Remove All Entries from This Domain"), this, SLOT(removeDomainEntries()));
		menu.addSeparator();
	}

	menu.addAction(ActionsManager::getAction(ActionsManager::ClearHistoryAction, this));
	menu.exec(m_ui->historyViewWidget->mapToGlobal(point));
}

QStandardItem* HistoryContentsWidget::findEntry(quint64 identifier)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem(m_model->item(i, 0));

		if (groupItem)
		{
			for (int j = 0; j < groupItem->rowCount(); ++j)
			{
				QStandardItem *entryItem(groupItem->child(j, 0));

				if (entryItem && entryItem->data(Qt::UserRole).toULongLong() == identifier)
				{
					return entryItem;
				}
			}
		}
	}

	return nullptr;
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
	return ThemesManager::getIcon(QLatin1String("view-history"), false);
}

WindowsManager::LoadingState HistoryContentsWidget::getLoadingState() const
{
	return (m_isLoading ? WindowsManager::OngoingLoadingState : WindowsManager::FinishedLoadingState);
}

quint64 HistoryContentsWidget::getEntry(const QModelIndex &index) const
{
	return ((index.isValid() && index.parent().isValid() && index.parent().parent() == m_model->invisibleRootItem()->index()) ? index.sibling(index.row(), 0).data(Qt::UserRole).toULongLong() : -1);
}

bool HistoryContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->historyViewWidget && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return))
		{
			openEntry();

			return true;
		}

		if (keyEvent && keyEvent->key() == Qt::Key_Delete)
		{
			removeEntry();

			return true;
		}
	}
	else if (object == m_ui->historyViewWidget->viewport() && event->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent && ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton))
		{
			const QModelIndex entryIndex(m_ui->historyViewWidget->currentIndex());

			if (!entryIndex.isValid() || entryIndex.parent() == m_model->invisibleRootItem()->index())
			{
				return ContentsWidget::eventFilter(object, event);
			}

			const QUrl url(entryIndex.sibling(entryIndex.row(), 0).data(Qt::DisplayRole).toString());

			if (url.isValid())
			{
				emit requestedOpenUrl(url, WindowsManager::calculateOpenHints(WindowsManager::NewTabOpen, mouseEvent->button(), mouseEvent->modifiers()));

				return true;
			}
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
