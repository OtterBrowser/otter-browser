/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "FeedsContentsWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Action.h"
#include "../../../ui/FeedPropertiesDialog.h"

#include "ui_FeedsContentsWidget.h"

#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QToolTip>

namespace Otter
{

FeedsContentsWidget::FeedsContentsWidget(const QVariantMap &parameters, QWidget *parent) : ContentsWidget(parameters, nullptr, parent),
	m_feed(nullptr),
	m_feedModel(nullptr),
	m_ui(new Ui::FeedsContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->feedsFilterLineEditWidget->setClearOnEscape(true);
	m_ui->feedsViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->feedsViewWidget->setModel(FeedsManager::getModel());
	m_ui->feedsViewWidget->expandAll();
	m_ui->feedsViewWidget->viewport()->installEventFilter(this);
	m_ui->feedsViewWidget->viewport()->setMouseTracking(true);

	connect(m_ui->feedsFilterLineEditWidget, &LineEditWidget::textChanged, m_ui->feedsViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->feedViewWidget, &ItemViewWidget::needsActionsUpdate, this, &FeedsContentsWidget::updateEntry);
	connect(m_ui->feedsViewWidget, &ItemViewWidget::customContextMenuRequested, this, &FeedsContentsWidget::showContextMenu);
}

FeedsContentsWidget::~FeedsContentsWidget()
{
	delete m_ui;
}

void FeedsContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		if (m_feedModel)
		{
			m_feedModel->setHorizontalHeaderLabels({tr("Title"), tr("From"), tr("Published")});
		}
	}
}

void FeedsContentsWidget::addFeed()
{
	FeedPropertiesDialog dialog(nullptr, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	FeedsManager::getModel()->addEntry(dialog.getFeed(), findFolder(m_ui->feedsViewWidget->currentIndex()));

	updateActions();
}

void FeedsContentsWidget::addFolder()
{
	const QString title(QInputDialog::getText(this, tr("Select Folder Name"), tr("Enter folder name:")));

	if (!title.isEmpty())
	{
		m_ui->feedsViewWidget->setCurrentIndex(FeedsManager::getModel()->addEntry(FeedsModel::FolderEntry, {{FeedsModel::TitleRole, title}}, findFolder(m_ui->feedsViewWidget->currentIndex()))->index());
	}
}

void FeedsContentsWidget::updateFeed()
{
	const FeedsModel::Entry *entry(FeedsManager::getModel()->getEntry(m_ui->feedsViewWidget->currentIndex()));

	if (entry && entry->getFeed())
	{
		entry->getFeed()->update();
	}
}

void FeedsContentsWidget::removeFeed()
{
	FeedsManager::getModel()->trashEntry(FeedsManager::getModel()->getEntry(m_ui->feedsViewWidget->currentIndex()));
}

void FeedsContentsWidget::openFeed()
{
///TODO
}

void FeedsContentsWidget::feedProperties()
{
	FeedsModel::Entry *entry(FeedsManager::getModel()->getEntry(m_ui->feedsViewWidget->currentIndex()));

	if (entry)
	{
		FeedPropertiesDialog dialog(entry->getFeed(), this);
		dialog.exec();

		updateActions();
	}
}

void FeedsContentsWidget::showContextMenu(const QPoint &position)
{
	const QModelIndex index(m_ui->feedsViewWidget->indexAt(position));
	const FeedsModel::EntryType type(static_cast<FeedsModel::EntryType>(index.data(FeedsModel::TypeRole).toInt()));
	QMenu menu(this);

	switch (type)
	{
		case FeedsModel::TrashEntry:
			{
				QAction *emptyTrashAction(menu.addAction(ThemesManager::createIcon(QLatin1String("trash-empty")), tr("Empty Trash")));
				emptyTrashAction->setEnabled(FeedsManager::getModel()->getTrashEntry()->rowCount() > 0);

				connect(emptyTrashAction, &QAction::triggered, FeedsManager::getModel(), &FeedsModel::emptyTrash);
			}

			break;
		case FeedsModel::UnknownEntry:
			connect(menu.addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…")), &QAction::triggered, this, &FeedsContentsWidget::addFolder);
			connect(menu.addAction(tr("Add Feed…")), &QAction::triggered, this, &FeedsContentsWidget::addFeed);

			break;
		default:
			{
				const bool isInTrash(index.data(FeedsModel::IsTrashedRole).toBool());

				ActionExecutor::Object executor(this, this);

				if (!isInTrash)
				{
					if (type == FeedsModel::FeedEntry)
					{
						connect(menu.addAction(ThemesManager::createIcon(QLatin1String("view-refresh")), QCoreApplication::translate("actions", "Update")), &QAction::triggered, this, &FeedsContentsWidget::updateFeed);

						menu.addSeparator();

						connect(menu.addAction(ThemesManager::createIcon(QLatin1String("document-open")), QCoreApplication::translate("actions", "Open")), &QAction::triggered, this, &FeedsContentsWidget::openFeed);
					}

					menu.addSeparator();

					QMenu *addMenu(menu.addMenu(tr("Add New")));

					connect(addMenu->addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…")), &QAction::triggered, this, &FeedsContentsWidget::addFolder);
					connect(addMenu->addAction(tr("Add Feed…")), &QAction::triggered, this, &FeedsContentsWidget::addFeed);
				}

				if (type != FeedsModel::RootEntry)
				{
					menu.addSeparator();

					if (isInTrash)
					{
						connect(menu.addAction(tr("Restore Feed")), &QAction::triggered, [&]()
						{
							FeedsManager::getModel()->restoreEntry(FeedsManager::getModel()->getEntry(m_ui->feedsViewWidget->currentIndex()));
						});
					}
					else
					{
						menu.addAction(new Action(ActionsManager::DeleteAction, {}, executor, &menu));
					}

					if (type == FeedsModel::FeedEntry)
					{
						menu.addSeparator();

						connect(menu.addAction(tr("Properties…")), &QAction::triggered, this, &FeedsContentsWidget::feedProperties);
					}
				}
			}

			break;
	}

	menu.exec(m_ui->feedsViewWidget->mapToGlobal(position));
}

void FeedsContentsWidget::updateActions()
{
///TODO
}

void FeedsContentsWidget::updateEntry()
{
	const QModelIndex index(m_ui->feedViewWidget->currentIndex().sibling(m_ui->feedViewWidget->currentIndex().row(), 0));

	m_ui->titleLabelWidget->setText(index.isValid() ? index.data(Qt::DisplayRole).toString() : QString());
	m_ui->authorLabelWidget->setText(index.isValid() ? index.data(AuthorRole).toString() : QString());
	m_ui->timeLabelWidget->setText(index.isValid() ? Utils::formatDateTime(index.data(index.data(UpdateTimeRole).isNull() ? PublicationTimeRole : UpdateTimeRole).toDateTime()) : QString());
	m_ui->textBrowser->setText(index.data(SummaryRole).toString() + QLatin1Char('\n') + index.data(ContentRole).toString());

	for (int i = (m_ui->categoriesLayout->count() - 1); i >= 0; --i)
	{
		m_ui->categoriesLayout->takeAt(i)->widget()->deleteLater();
	}

	const QStringList entryCategories(index.data(CategoriesRole).toStringList());
	const QMap<QString, QString> feedCategories(m_feed->getCategories());

	for (int i = 0; i < entryCategories.count(); ++i)
	{
		if (!entryCategories.at(i).isEmpty())
		{
			const QString label(feedCategories.value(entryCategories.at(i)));
			QToolButton *toolButton(new QToolButton(m_ui->detailsWidget));
			toolButton->setText(label.isEmpty() ? QString(entryCategories.at(i)).replace(QLatin1Char('_'), QLatin1Char(' ')) : label);
			toolButton->setObjectName(entryCategories.at(i));

			m_ui->categoriesLayout->addWidget(toolButton);
		}
	}

	m_ui->categoriesLayout->addStretch();
}

void FeedsContentsWidget::updateFeedModel()
{
	if (!m_feedModel)
	{
		return;
	}

	const QString identifier(m_ui->feedViewWidget->currentIndex().data(IdentifierRole).toString());

	m_feedModel->clear();
	m_feedModel->setHorizontalHeaderLabels({tr("Title"), tr("From"), tr("Published")});
	m_feedModel->setHeaderData(0, Qt::Horizontal, QSize(300, 0), Qt::SizeHintRole);

	if (!m_feed)
	{
		return;
	}

	const QVector<Feed::Entry> entries(m_feed->getEntries());

	for (int i = 0; i < entries.count(); ++i)
	{
		const Feed::Entry entry(entries.at(i));
		QList<QStandardItem*> items({new QStandardItem(entry.title.isEmpty() ? tr("(Untitled)") : entry.title), new QStandardItem(entry.author.isEmpty() ? tr("(Untitled)") : entry.author), new QStandardItem(Utils::formatDateTime(entry.updateTime.isNull() ? entry.publicationTime : entry.updateTime))});
		items[0]->setData(entry.url, UrlRole);
		items[0]->setData(entry.identifier, IdentifierRole);
		items[0]->setData(entry.summary, SummaryRole);
		items[0]->setData(entry.content, ContentRole);
		items[0]->setData(entry.publicationTime, PublicationTimeRole);
		items[0]->setData(entry.updateTime, UpdateTimeRole);
		items[0]->setData(entry.author, AuthorRole);
		items[0]->setData(entry.email, EmailRole);
		items[0]->setData(entry.categories, CategoriesRole);
		items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
		items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);
		items[2]->setFlags(items[2]->flags() | Qt::ItemNeverHasChildren);

		m_feedModel->appendRow(items);

		if (entry.identifier == identifier)
		{
			m_ui->feedViewWidget->setCurrentIndex(m_ui->feedViewWidget->getIndex(i));
		}
	}

	if (!m_ui->feedViewWidget->selectionModel()->hasSelection())
	{
		m_ui->feedViewWidget->setCurrentIndex(m_ui->feedViewWidget->getIndex(0));
	}
}

void FeedsContentsWidget::setUrl(const QUrl &url, bool isTyped)
{
	Q_UNUSED(isTyped)

	if (url.scheme() == QLatin1String("view-feed"))
	{
		m_feed = FeedsManager::createFeed(url.toDisplayString().mid(10));

		if (m_feedModel)
		{
			m_feedModel->clear();
		}

		if (m_feed)
		{
			if (!m_feedModel)
			{
				m_feedModel = new QStandardItemModel(this);

				m_ui->feedViewWidget->setModel(m_feedModel);
				m_ui->feedViewWidget->setViewMode(ItemViewWidget::ListViewMode);
			}

			if (m_feed->getLastSynchronizationTime().isNull())
			{
				m_feed->update();
			}

			updateFeedModel();

			m_ui->stackedWidget->setCurrentIndex(1);

			connect(m_feed, &Feed::feedModified, this, &FeedsContentsWidget::updateFeedModel);
		}
	}
}

FeedsModel::Entry* FeedsContentsWidget::findFolder(const QModelIndex &index) const
{
	FeedsModel::Entry *entry(FeedsManager::getModel()->getEntry(index));

	if (!entry || entry == FeedsManager::getModel()->getRootEntry() || entry == FeedsManager::getModel()->getTrashEntry())
	{
		return FeedsManager::getModel()->getRootEntry();
	}

	const FeedsModel::EntryType type(static_cast<FeedsModel::EntryType>(entry->data(FeedsModel::TypeRole).toInt()));

	return ((type == FeedsModel::RootEntry || type == FeedsModel::FolderEntry) ? entry : static_cast<FeedsModel::Entry*>(entry->parent()));
}

QString FeedsContentsWidget::getTitle() const
{
	if (m_feed)
	{
		const QString title(m_feed->getTitle());

		return tr("Feed: %1").arg(title.isEmpty() ? tr("(Untitled)") : title);
	}

	return tr("Feeds");
}

QLatin1String FeedsContentsWidget::getType() const
{
	return QLatin1String("feeds");
}

QUrl FeedsContentsWidget::getUrl() const
{
	return (m_feed ? QUrl(QLatin1String("view-feed:") + m_feed->getUrl().toDisplayString()) : QUrl(QLatin1String("about:feeds")));
}

QIcon FeedsContentsWidget::getIcon() const
{
	if (m_feed)
	{
		const QIcon icon(m_feed->getIcon());

		return (icon.isNull() ? ThemesManager::createIcon(QLatin1String("application-rss+xml")) : icon);
	}

	return ThemesManager::createIcon(QLatin1String("feeds"), false);
}

bool FeedsContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->feedsViewWidget->viewport() && event->type() == QEvent::ToolTip)
	{
		const QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));
		const QModelIndex index(m_ui->feedsViewWidget->indexAt(helpEvent->pos()));

		if (static_cast<FeedsModel::EntryType>(index.data(FeedsModel::TypeRole).toInt()) != FeedsModel::FeedEntry)
		{
			return false;
		}

		QString toolTip;

		if (index.isValid())
		{
			toolTip = tr("Title: %1").arg(index.data(FeedsModel::TitleRole).toString()) + QLatin1Char('\n') + tr("Address: %1").arg(index.data(FeedsModel::UrlRole).toUrl().toDisplayString());

			if (!index.data(FeedsModel::LastSynchronizationTimeRole).isNull())
			{
				toolTip.append(QLatin1Char('\n') + tr("Last update: %1").arg(Utils::formatDateTime(index.data(FeedsModel::LastSynchronizationTimeRole).toDateTime())));
			}
		}

		QToolTip::showText(helpEvent->globalPos(), QFontMetrics(QToolTip::font()).elidedText(toolTip, Qt::ElideRight, (QApplication::desktop()->screenGeometry(m_ui->feedsViewWidget).width() / 2)), m_ui->feedsViewWidget, m_ui->feedsViewWidget->visualRect(index));

		return true;
	}

	return ContentsWidget::eventFilter(object, event);
}

}
