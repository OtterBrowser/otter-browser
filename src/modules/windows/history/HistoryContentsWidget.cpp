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

#include "HistoryContentsWidget.h"
#include "../../../core/Application.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/Action.h"
#include "../../../ui/MainWindow.h"

#include "ui_HistoryContentsWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QMenu>

namespace Otter
{

HistoryContentsWidget::HistoryContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_model(new QStandardItemModel(this)),
	m_isLoading(true),
	m_ui(new Ui::HistoryContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);

	const QStringList groups({tr("Today"), tr("Yesterday"), tr("Earlier This Week"), tr("Previous Week"), tr("Earlier This Month"), tr("Earlier This Year"), tr("Older")});
	const QIcon icon(ThemesManager::createIcon(QLatin1String("inode-directory")));

	for (int i = 0; i < groups.count(); ++i)
	{
		m_model->appendRow(new QStandardItem(icon, groups.at(i)));
	}

	m_model->setHorizontalHeaderLabels({tr("Address"), tr("Title"), tr("Date")});
	m_model->setHeaderData(0, Qt::Horizontal, 300, HeaderViewWidget::WidthRole);
	m_model->setHeaderData(1, Qt::Horizontal, 300, HeaderViewWidget::WidthRole);
	m_model->setSortRole(Qt::DisplayRole);

	m_ui->historyViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->historyViewWidget->setModel(m_model, true);
	m_ui->historyViewWidget->setSortRoleMapping({{2, TimeVisitedRole}});
	m_ui->historyViewWidget->installEventFilter(this);
	m_ui->historyViewWidget->viewport()->installEventFilter(this);

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		m_ui->historyViewWidget->setRowHidden(i, m_model->invisibleRootItem()->index(), true);
	}

	QTimer::singleShot(100, this, &HistoryContentsWidget::populateEntries);

	connect(HistoryManager::getBrowsingHistoryModel(), &HistoryModel::cleared, this, &HistoryContentsWidget::populateEntries);
	connect(HistoryManager::getBrowsingHistoryModel(), &HistoryModel::entryAdded, this, &HistoryContentsWidget::handleEntryAdded);
	connect(HistoryManager::getBrowsingHistoryModel(), &HistoryModel::entryModified, this, &HistoryContentsWidget::handleEntryModified);
	connect(HistoryManager::getBrowsingHistoryModel(), &HistoryModel::entryRemoved, this, &HistoryContentsWidget::handleEntryRemoved);
	connect(HistoryManager::getInstance(), &HistoryManager::dayChanged, this, &HistoryContentsWidget::populateEntries);
	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->historyViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->historyViewWidget, &ItemViewWidget::doubleClicked, this, &HistoryContentsWidget::openEntry);
	connect(m_ui->historyViewWidget, &ItemViewWidget::customContextMenuRequested, this, &HistoryContentsWidget::showContextMenu);
}

HistoryContentsWidget::~HistoryContentsWidget()
{
	delete m_ui;
}

void HistoryContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		const QStringList groups({tr("Today"), tr("Yesterday"), tr("Earlier This Week"), tr("Previous Week"), tr("Earlier This Month"), tr("Earlier This Year"), tr("Older")});

		for (int i = 0; i < groups.count(); ++i)
		{
			m_model->setData(m_model->index(i, 0), groups.at(i), Qt::DisplayRole);
		}

		m_model->setHorizontalHeaderLabels({tr("Address"), tr("Title"), tr("Date")});
	}
}

void HistoryContentsWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	switch (identifier)
	{
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
			m_ui->filterLineEditWidget->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->historyViewWidget->setFocus();

			break;
		default:
			ContentsWidget::triggerAction(identifier, parameters, trigger);

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
	const QVector<QDate> dates({date, date.addDays(-1), date.addDays(-7), date.addDays(-14), date.addDays(-30), date.addDays(-365)});

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem(m_model->item(i, 0));

		if (groupItem)
		{
			groupItem->setData(dates.value(i, QDate()), GroupDateRole);
			groupItem->removeRows(0, groupItem->rowCount());
		}
	}

	const HistoryModel *model(HistoryManager::getBrowsingHistoryModel());

	for (int i = 0; i < model->rowCount(); ++i)
	{
		handleEntryAdded(static_cast<HistoryModel::Entry*>(model->item(i, 0)));
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

	emit loadingStateChanged(WebWidget::FinishedLoadingState);
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
	const QStandardItem *domainItem(findEntry(getEntry(m_ui->historyViewWidget->currentIndex())));

	if (!domainItem)
	{
		return;
	}

	const QString host(QUrl(domainItem->text()).host());
	QVector<quint64> entries;

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		const QStandardItem *groupItem(m_model->item(i, 0));

		if (!groupItem)
		{
			continue;
		}

		for (int j = (groupItem->rowCount() - 1); j >= 0; --j)
		{
			const QStandardItem *entryItem(groupItem->child(j, 0));

			if (entryItem && host == QUrl(entryItem->text()).host())
			{
				entries.append(entryItem->data(IdentifierRole).toULongLong());
			}
		}
	}

	HistoryManager::removeEntries(entries);
}

void HistoryContentsWidget::openEntry()
{
	const QModelIndex index(m_ui->historyViewWidget->currentIndex());

	if (!index.isValid() || index.parent() == m_model->invisibleRootItem()->index())
	{
		return;
	}

	const QUrl url(index.sibling(index.row(), 0).data(Qt::DisplayRole).toString());

	if (url.isValid())
	{
		const QAction *action(qobject_cast<QAction*>(sender()));
		MainWindow *mainWindow(MainWindow::findMainWindow(this));

		if (action && mainWindow)
		{
			mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(static_cast<SessionsManager::OpenHints>(action->data().toInt()))}});
		}
	}
}

void HistoryContentsWidget::handleEntryAdded(HistoryModel::Entry *entry)
{
	if (!entry || !entry->isValid() || findEntry(entry->getIdentifier()))
	{
		return;
	}

	QStandardItem *groupItem(nullptr);

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		groupItem = m_model->item(i, 0);

		const QDate date(groupItem ? groupItem->data(GroupDateRole).toDate() : QDate());

		if (!date.isValid() || entry->getTimeVisited().date() >= date)
		{
			break;
		}

		groupItem = nullptr;
	}

	if (!groupItem)
	{
		return;
	}

	QList<QStandardItem*> entryItems({new QStandardItem(entry->getIcon(), entry->getUrl().toDisplayString().replace(QLatin1String("%23"), QString(QLatin1Char('#')))), new QStandardItem(entry->getTitle()), new QStandardItem(Utils::formatDateTime(entry->getTimeVisited()))});
	entryItems[0]->setData(entry->getIdentifier(), IdentifierRole);
	entryItems[0]->setFlags(entryItems[0]->flags() | Qt::ItemNeverHasChildren);
	entryItems[1]->setFlags(entryItems[1]->flags() | Qt::ItemNeverHasChildren);
	entryItems[2]->setData(entry->getTimeVisited(), TimeVisitedRole);
	entryItems[2]->setFlags(entryItems[2]->flags() | Qt::ItemNeverHasChildren);
	entryItems[2]->setToolTip(Utils::formatDateTime(entry->getTimeVisited(), {}, false));

	groupItem->appendRow(entryItems);

	m_ui->historyViewWidget->setRowHidden(groupItem->row(), groupItem->index().parent(), false);

	if (m_isLoading || groupItem->rowCount() != 1 || SettingsManager::getOption(SettingsManager::History_ExpandBranchesOption).toString() != QLatin1String("first"))
	{
		return;
	}

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

void HistoryContentsWidget::handleEntryModified(HistoryModel::Entry *entry)
{
	if (!entry || !entry->isValid())
	{
		return;
	}

	QStandardItem *entryItem(findEntry(entry->getIdentifier()));

	if (!entryItem)
	{
		handleEntryAdded(entry);

		return;
	}

	entryItem->setIcon(entry->getIcon());
	entryItem->setText(entry->getUrl().toDisplayString());
	entryItem->parent()->child(entryItem->row(), 1)->setText(entry->getTitle());
	entryItem->parent()->child(entryItem->row(), 2)->setText(Utils::formatDateTime(entry->getTimeVisited()));
}

void HistoryContentsWidget::handleEntryRemoved(HistoryModel::Entry *entry)
{
	if (!entry || !entry->isValid())
	{
		return;
	}

	QStandardItem *entryItem(findEntry(entry->getIdentifier()));

	if (!entryItem)
	{
		return;
	}

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

void HistoryContentsWidget::showContextMenu(const QPoint &position)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));
	const quint64 entry(getEntry(m_ui->historyViewWidget->indexAt(position)));
	QMenu menu(this);

	if (entry > 0)
	{
		menu.addAction(ThemesManager::createIcon(QLatin1String("document-open")), QCoreApplication::translate("actions", "Open"), this, &HistoryContentsWidget::openEntry);
		menu.addAction(QCoreApplication::translate("actions", "Open in New Tab"), this, &HistoryContentsWidget::openEntry)->setData(SessionsManager::NewTabOpen);
		menu.addAction(QCoreApplication::translate("actions", "Open in New Background Tab"), this, &HistoryContentsWidget::openEntry)->setData(static_cast<int>(SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen));
		menu.addSeparator();
		menu.addAction(QCoreApplication::translate("actions", "Open in New Window"), this, &HistoryContentsWidget::openEntry)->setData(SessionsManager::NewWindowOpen);
		menu.addAction(QCoreApplication::translate("actions", "Open in New Background Window"), this, &HistoryContentsWidget::openEntry)->setData(static_cast<int>(SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen));
		menu.addSeparator();
		menu.addAction(tr("Add to Bookmarks…"), this, [&]()
		{
			const QStandardItem *entryItem(findEntry(getEntry(m_ui->historyViewWidget->currentIndex())));

			if (entryItem)
			{
				Application::triggerAction(ActionsManager::BookmarkPageAction, {{QLatin1String("url"), entryItem->text()}, {QLatin1String("title"), m_ui->historyViewWidget->currentIndex().sibling(m_ui->historyViewWidget->currentIndex().row(), 1).data(Qt::DisplayRole).toString()}}, parentWidget());
			}
		});
		menu.addAction(tr("Copy Link to Clipboard"), this, [&]()
		{
			const QStandardItem *entryItem(findEntry(getEntry(m_ui->historyViewWidget->currentIndex())));

			if (entryItem)
			{
				QGuiApplication::clipboard()->setText(entryItem->text());
			}
		});
		menu.addSeparator();
		menu.addAction(tr("Remove Entry"), this, &HistoryContentsWidget::removeEntry);
		menu.addAction(tr("Remove All Entries from This Domain"), this, &HistoryContentsWidget::removeDomainEntries);
	}

	menu.addAction(new Action(ActionsManager::ClearHistoryAction, {}, ActionExecutor::Object(mainWindow, mainWindow), &menu));
	menu.exec(m_ui->historyViewWidget->mapToGlobal(position));
}

QStandardItem* HistoryContentsWidget::findEntry(quint64 identifier)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		const QStandardItem *groupItem(m_model->item(i, 0));

		if (groupItem)
		{
			for (int j = 0; j < groupItem->rowCount(); ++j)
			{
				QStandardItem *entryItem(groupItem->child(j, 0));

				if (entryItem && entryItem->data(IdentifierRole).toULongLong() == identifier)
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
	return {QLatin1String("about:history")};
}

QIcon HistoryContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("view-history"), false);
}

WebWidget::LoadingState HistoryContentsWidget::getLoadingState() const
{
	return (m_isLoading ? WebWidget::OngoingLoadingState : WebWidget::FinishedLoadingState);
}

quint64 HistoryContentsWidget::getEntry(const QModelIndex &index) const
{
	return ((index.isValid() && index.parent().isValid() && index.parent().parent() == m_model->invisibleRootItem()->index()) ? index.sibling(index.row(), 0).data(IdentifierRole).toULongLong() : 0);
}

bool HistoryContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->historyViewWidget && event->type() == QEvent::KeyPress)
	{
		const QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		switch (keyEvent->key())
		{
			case Qt::Key_Delete:
				removeEntry();

				return true;
			case Qt::Key_Enter:
			case Qt::Key_Return:
				openEntry();

				return true;
			default:
				break;
		}
	}
	else if (object == m_ui->historyViewWidget->viewport() && event->type() == QEvent::MouseButtonRelease)
	{
		const QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton)
		{
			const QModelIndex entryIndex(m_ui->historyViewWidget->currentIndex());

			if (!entryIndex.isValid() || entryIndex.parent() == m_model->invisibleRootItem()->index())
			{
				return ContentsWidget::eventFilter(object, event);
			}

			MainWindow *mainWindow(MainWindow::findMainWindow(this));
			const QUrl url(entryIndex.sibling(entryIndex.row(), 0).data(Qt::DisplayRole).toString());

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
