/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2021 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../core/Application.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/HandlersManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Action.h"
#include "../../../ui/Animation.h"
#include "../../../ui/FeedPropertiesDialog.h"
#include "../../../ui/MainWindow.h"

#include "ui_FeedsContentsWidget.h"

#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QToolTip>

namespace Otter
{

EntryDelegate::EntryDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void EntryDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	ItemDelegate::initStyleOption(option, index);

	if (index.sibling(index.row(), 0).data(FeedsContentsWidget::LastReadTimeRole).isNull())
	{
		option->font.setBold(true);
	}
}

FeedDelegate::FeedDelegate(QObject *parent) : ItemDelegate({{ItemDelegate::ProgressHasErrorRole, FeedsModel::HasErrorRole}, {ItemDelegate::ProgressHasIndicatorRole, FeedsModel::IsShowingProgressIndicatorRole}, {ItemDelegate::ProgressValueRole, FeedsModel::UpdateProgressValueRole}}, parent)
{
}

void FeedDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	ItemDelegate::initStyleOption(option, index);

	if (index.data(FeedsModel::IsUpdatingRole).toBool())
	{
		const Animation *animation(FeedsContentsWidget::getUpdateAnimation());

		if (animation)
		{
			option->icon = QIcon(animation->getCurrentPixmap());
		}
	}
}

Animation* FeedsContentsWidget::m_updateAnimation = nullptr;

FeedsContentsWidget::FeedsContentsWidget(const QVariantMap &parameters, QWidget *parent) : ContentsWidget(parameters, nullptr, parent),
	m_feed(nullptr),
	m_feedModel(nullptr),
	m_ui(new Ui::FeedsContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->subscribeFeedWidget->hide();
	m_ui->feedsHorizontalSplitterWidget->setSizes({300, qMax(500, (width() - 300))});
	m_ui->entriesFilterLineEditWidget->setClearOnEscape(true);
	m_ui->entriesViewWidget->setItemDelegate(new EntryDelegate(this));
	m_ui->entriesViewWidget->installEventFilter(this);
	m_ui->entriesViewWidget->viewport()->installEventFilter(this);
	m_ui->entriesViewWidget->viewport()->setMouseTracking(true);
	m_ui->feedsFilterLineEditWidget->setClearOnEscape(true);
	m_ui->feedsViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->feedsViewWidget->setModel(FeedsManager::getModel());
	m_ui->feedsViewWidget->setItemDelegate(new FeedDelegate(this));
	m_ui->feedsViewWidget->expandAll();
	m_ui->feedsViewWidget->installEventFilter(this);
	m_ui->feedsViewWidget->viewport()->installEventFilter(this);
	m_ui->feedsViewWidget->viewport()->setMouseTracking(true);
	m_ui->feedsViewWidget->getViewportWidget()->setUpdateDataRole(FeedsModel::IsShowingProgressIndicatorRole);
	m_ui->emailButton->setIcon(ThemesManager::createIcon(QLatin1String("mail-send")));
	m_ui->urlButton->setIcon(ThemesManager::createIcon(QLatin1String("text-html")));
	m_ui->textBrowserWidget->setOpenExternalLinks(true);

	if (isSidebarPanel())
	{
		m_ui->entriesWidget->hide();
		m_ui->entryWidget->hide();
	}

	connect(FeedsManager::getInstance(), &FeedsManager::feedModified, this, &FeedsContentsWidget::handleFeedModified);
	connect(m_ui->entriesFilterLineEditWidget, &LineEditWidget::textChanged, m_ui->entriesViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->entriesViewWidget, &ItemViewWidget::doubleClicked, this, &FeedsContentsWidget::openEntry);
	connect(m_ui->entriesViewWidget, &ItemViewWidget::customContextMenuRequested, this, &FeedsContentsWidget::showEntriesContextMenu);
	connect(m_ui->entriesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &FeedsContentsWidget::updateEntry);
	connect(m_ui->feedsFilterLineEditWidget, &LineEditWidget::textChanged, m_ui->feedsViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->feedsViewWidget, &ItemViewWidget::doubleClicked, this, &FeedsContentsWidget::openFeed);
	connect(m_ui->feedsViewWidget, &ItemViewWidget::customContextMenuRequested, this, &FeedsContentsWidget::showFeedsContextMenu);
	connect(m_ui->feedsViewWidget, &ItemViewWidget::needsActionsUpdate, this, &FeedsContentsWidget::updateActions);
	connect(m_ui->okButton, &QToolButton::clicked, this, &FeedsContentsWidget::subscribeFeed);
	connect(m_ui->cancelButton, &QToolButton::clicked, m_ui->subscribeFeedWidget, &QWidget::hide);
	connect(m_ui->emailButton, &QToolButton::clicked, [&]()
	{
		const QModelIndex index(m_ui->entriesViewWidget->currentIndex());

		HandlersManager::handleUrl(QLatin1String("mailto:") + index.sibling(index.row(), 0).data(EmailRole).toString());
	});
	connect(m_ui->urlButton, &QToolButton::clicked, [&]()
	{
		const QModelIndex index(m_ui->entriesViewWidget->currentIndex());

		Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), index.sibling(index.row(), 0).data(UrlRole).toString()}}, this);
	});
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

void FeedsContentsWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	switch (identifier)
	{
		case ActionsManager::ReloadAction:
			if (m_feed && !m_feed->isUpdating())
			{
				m_feed->update();
			}

			break;
		case ActionsManager::DeleteAction:
			if (m_ui->feedsViewWidget->hasFocus())
			{
				removeFeed();
			}
			else if (m_ui->entriesViewWidget->hasFocus())
			{
				removeEntry();
			}

			break;
		default:
			ContentsWidget::triggerAction(identifier, parameters, trigger);

			break;
	}
}

void FeedsContentsWidget::addFeed()
{
	FeedPropertiesDialog dialog(nullptr, findFolder(m_ui->feedsViewWidget->currentIndex()), this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	if (BookmarksManager::getModel()->hasFeed(dialog.getFeed()->getUrl()) || FeedsManager::getModel()->hasFeed(dialog.getFeed()->getUrl()))
	{
		QMessageBox messageBox;
		messageBox.setWindowTitle(tr("Question"));
		messageBox.setText(tr("You already subscribed this feed."));
		messageBox.setInformativeText(tr("Do you want to continue?"));
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		messageBox.setDefaultButton(QMessageBox::Yes);

		if (messageBox.exec() == QMessageBox::Cancel)
		{
			return;
		}
	}

	FeedsManager::getModel()->addEntry(dialog.getFeed(), dialog.getFolder());

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

void FeedsContentsWidget::openFeed()
{
	const FeedsModel::Entry *entry(FeedsManager::getModel()->getEntry(m_ui->feedsViewWidget->currentIndex()));
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (mainWindow && entry && entry->getFeed())
	{
		mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), FeedsManager::createFeedReaderUrl(entry->getFeed()->getUrl())}});
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

void FeedsContentsWidget::subscribeFeed()
{
	if (!m_feed)
	{
		return;
	}

	if (m_ui->applicationComboBox->currentIndex() == 0)
	{
		FeedPropertiesDialog dialog(m_feed, findFolder(m_ui->feedsViewWidget->currentIndex()), this);

		if (dialog.exec() == QDialog::Accepted)
		{
			FeedsManager::getModel()->addEntry(m_feed, dialog.getFolder());
		}
	}
	else
	{
		Utils::runApplication(m_ui->applicationComboBox->currentData(Qt::UserRole).toString(), m_feed->getUrl());
	}

	m_ui->subscribeFeedWidget->hide();
}

void FeedsContentsWidget::feedProperties()
{
	FeedsModel::Entry *entry(FeedsManager::getModel()->getEntry(m_ui->feedsViewWidget->currentIndex()));

	if (entry)
	{
		FeedsModel::Entry *folder(findFolder(m_ui->feedsViewWidget->currentIndex()));
		FeedPropertiesDialog dialog(entry->getFeed(), folder, this);

		if (dialog.exec() == QDialog::Accepted && dialog.getFolder() != folder)
		{
			FeedsManager::getModel()->moveEntry(entry, dialog.getFolder());
		}

		updateActions();
	}
}

void FeedsContentsWidget::openEntry()
{
	const QModelIndex index(m_ui->entriesViewWidget->currentIndex().sibling(m_ui->entriesViewWidget->currentIndex().row(), 0));
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (mainWindow && index.isValid() && !index.data(UrlRole).isNull())
	{
		mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), index.data(UrlRole)}});
	}
}

void FeedsContentsWidget::removeEntry()
{
	if (m_feed)
	{
		const QModelIndex index(m_ui->entriesViewWidget->currentIndex().sibling(m_ui->entriesViewWidget->currentIndex().row(), 0));

		if (index.isValid() && !index.data(IdentifierRole).isNull())
		{
			m_feed->markEntryAsRemoved(index.data(IdentifierRole).toString());

			m_ui->entriesViewWidget->removeRow();
		}
	}
}

void FeedsContentsWidget::selectCategory()
{
	const QToolButton *toolButton(qobject_cast<QToolButton*>(sender()));

	if (toolButton)
	{
		m_categories = QStringList({toolButton->objectName()});

		updateFeedModel();
	}
}

void FeedsContentsWidget::handleFeedModified(const QUrl &url)
{
	const Feed *feed(FeedsManager::getFeed(url));

	if (!m_updateAnimation && feed && feed->isUpdating() && FeedsManager::getModel()->hasFeed(url))
	{
		const QString path(ThemesManager::getAnimationPath(QLatin1String("spinner")));

		if (path.isEmpty())
		{
			m_updateAnimation = new SpinnerAnimation(QCoreApplication::instance());
		}
		else
		{
			m_updateAnimation = new GenericAnimation(path, QCoreApplication::instance());
		}

		m_updateAnimation->start();
	}

	m_ui->feedsViewWidget->getViewportWidget()->updateDirtyIndexesList();
}

void FeedsContentsWidget::showEntriesContextMenu(const QPoint &position)
{
	const QModelIndex index(m_ui->entriesViewWidget->indexAt(position).sibling(m_ui->entriesViewWidget->indexAt(position).row(), 0));
	ActionExecutor::Object executor(this, this);
	QMenu menu(this);
	Action *openAction(new Action(ActionsManager::OpenUrlAction, {{QLatin1String("url"), index.data(UrlRole)}}, executor, &menu));
	openAction->setTextOverride(QT_TRANSLATE_NOOP("actions", "Open"));
	openAction->setEnabled(!index.data(UrlRole).isNull());

	menu.addAction(openAction);
	menu.addSeparator();
	menu.addAction(new Action(ActionsManager::DeleteAction, {}, executor, &menu));
	menu.exec(m_ui->entriesViewWidget->mapToGlobal(position));
}

void FeedsContentsWidget::showFeedsContextMenu(const QPoint &position)
{
	const QModelIndex index(m_ui->feedsViewWidget->indexAt(position));
	const FeedsModel::EntryType type(static_cast<FeedsModel::EntryType>(index.data(FeedsModel::TypeRole).toInt()));
	QMenu menu(this);

	switch (type)
	{
		case FeedsModel::TrashEntry:
			menu.addAction(ThemesManager::createIcon(QLatin1String("trash-empty")), tr("Empty Trash"), FeedsManager::getModel(), &FeedsModel::emptyTrash)->setEnabled(FeedsManager::getModel()->getTrashEntry()->rowCount() > 0);

			break;
		case FeedsModel::UnknownEntry:
			menu.addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…"), this, &FeedsContentsWidget::addFolder);
			menu.addAction(tr("Add Feed…"), this, &FeedsContentsWidget::addFeed);

			break;
		default:
			{
				const bool isInTrash(index.data(FeedsModel::IsTrashedRole).toBool());

				ActionExecutor::Object executor(this, this);

				if (!isInTrash)
				{
					if (type == FeedsModel::FeedEntry)
					{
						menu.addAction(ThemesManager::createIcon(QLatin1String("view-refresh")), QCoreApplication::translate("actions", "Update"), this, &FeedsContentsWidget::updateFeed);
						menu.addSeparator();
						menu.addAction(ThemesManager::createIcon(QLatin1String("document-open")), QCoreApplication::translate("actions", "Open"), this, &FeedsContentsWidget::openFeed);
						menu.addSeparator();
						menu.addAction(tr("Mark All as Read"), this, [&]()
						{
							if (m_feed)
							{
								m_feed->markAllEntriesAsRead();

								updateFeedModel();
							}
						});
					}

					menu.addSeparator();

					QMenu *addMenu(menu.addMenu(tr("Add New")));
					addMenu->addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…"), this, &FeedsContentsWidget::addFolder);
					addMenu->addAction(tr("Add Feed…"), this, &FeedsContentsWidget::addFeed);
				}

				if (type != FeedsModel::RootEntry)
				{
					menu.addSeparator();

					if (isInTrash)
					{
						menu.addAction(tr("Restore Feed"), &menu, [&]()
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
						menu.addAction(tr("Properties…"), this, &FeedsContentsWidget::feedProperties);
					}
				}
			}

			break;
	}

	menu.exec(m_ui->feedsViewWidget->mapToGlobal(position));
}

void FeedsContentsWidget::updateActions()
{
	if (!isSidebarPanel())
	{
		const FeedsModel::Entry *entry(FeedsManager::getModel()->getEntry(m_ui->feedsViewWidget->currentIndex()));

		setFeed((entry && entry->getFeed()) ? entry->getFeed() : nullptr);
	}
}

void FeedsContentsWidget::updateEntry()
{
	const QModelIndex index(m_ui->entriesViewWidget->currentIndex().sibling(m_ui->entriesViewWidget->currentIndex().row(), 0));
	QString content(index.data(ContentRole).toString());

	if (!index.data(SummaryRole).isNull())
	{
		QString summary(index.data(SummaryRole).toString());

		if (!summary.contains(QLatin1Char('<')))
		{
			summary = QLatin1String("<p>") + summary + QLatin1String("</p>");
		}

		summary.append(QLatin1Char('\n'));

		content.prepend(summary);
	}

	const QString enableImages(SettingsManager::getOption(SettingsManager::Permissions_EnableImagesOption, Utils::extractHost(m_feed->getUrl())).toString());
	TextBrowserWidget::ImagesPolicy imagesPolicy(TextBrowserWidget::AllImages);

	if (enableImages == QLatin1String("onlyCached"))
	{
		imagesPolicy = TextBrowserWidget::OnlyCachedImages;
	}
	else if (enableImages == QLatin1String("disabled"))
	{
		imagesPolicy = TextBrowserWidget::NoImages;
	}

	m_ui->titleLabelWidget->setText(index.isValid() ? index.data(Qt::DisplayRole).toString() : QString());
	m_ui->emailButton->setVisible(!index.data(EmailRole).isNull());
	m_ui->emailButton->setToolTip(tr("Send email to %1").arg(index.data(EmailRole).toString()));
	m_ui->urlButton->setVisible(!index.data(UrlRole).isNull());
	m_ui->urlButton->setToolTip(tr("Go to %1").arg(index.data(UrlRole).toUrl().toDisplayString()));
	m_ui->authorLabelWidget->setText(index.isValid() ? index.data(AuthorRole).toString() : QString());
	m_ui->timeLabelWidget->setText(index.isValid() ? Utils::formatDateTime(index.data(index.data(UpdateTimeRole).isNull() ? PublicationTimeRole : UpdateTimeRole).toDateTime()) : QString());
	m_ui->textBrowserWidget->setImagesPolicy(imagesPolicy);
	m_ui->textBrowserWidget->setText(content);

	for (int i = (m_ui->categoriesLayout->count() - 1); i >= 0; --i)
	{
		m_ui->categoriesLayout->takeAt(i)->widget()->deleteLater();
	}

	const QStringList entryCategories(index.data(CategoriesRole).toStringList());
	const QMap<QString, QString> feedCategories(m_feed ? m_feed->getCategories() : QMap<QString, QString>());

	for (int i = 0; i < entryCategories.count(); ++i)
	{
		if (!entryCategories.at(i).isEmpty())
		{
			const QString label(feedCategories.value(entryCategories.at(i)));
			QToolButton *toolButton(new QToolButton(m_ui->entryWidget));
			toolButton->setText(label.isEmpty() ? QString(entryCategories.at(i)).replace(QLatin1Char('_'), QLatin1Char(' ')) : label);
			toolButton->setObjectName(entryCategories.at(i));

			m_ui->categoriesLayout->addWidget(toolButton);

			connect(toolButton, &QToolButton::clicked, this, &FeedsContentsWidget::selectCategory);
		}
	}

	m_ui->categoriesLayout->addStretch();

	if (index.isValid() && m_feed && m_feedModel)
	{
		disconnect(m_ui->entriesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &FeedsContentsWidget::updateEntry);

		m_feed->markEntryAsRead(index.data(IdentifierRole).toString());

		m_feedModel->setData(index, QDateTime::currentDateTimeUtc(), LastReadTimeRole);

		m_ui->entriesViewWidget->update();

		connect(m_ui->entriesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &FeedsContentsWidget::updateEntry);
	}

	emit arbitraryActionsStateChanged({ActionsManager::DeleteAction});
}

void FeedsContentsWidget::updateFeedModel()
{
	if (!m_feedModel)
	{
		return;
	}

	const QString identifier(m_ui->entriesViewWidget->currentIndex().data(IdentifierRole).toString());

	m_feedModel->clear();
	m_feedModel->setHorizontalHeaderLabels({tr("Title"), tr("From"), tr("Published")});
	m_feedModel->setHeaderData(0, Qt::Horizontal, 300, HeaderViewWidget::WidthRole);

	m_ui->applicationComboBox->clear();

	if (m_feed && !FeedsManager::getModel()->hasFeed(m_feed->getUrl()))
	{
		m_ui->applicationComboBox->setItemDelegate(new ItemDelegate(m_ui->applicationComboBox));
		m_ui->applicationComboBox->addItem(Application::windowIcon(), Application::applicationDisplayName());

		const QVector<ApplicationInformation> applications(Utils::getApplicationsForMimeType(m_feed->getMimeType()));

		if (applications.count() > 0)
		{
			m_ui->applicationComboBox->addItem({});
			m_ui->applicationComboBox->setItemData(1, QLatin1String("separator"), Qt::AccessibleDescriptionRole);

			for (int i = 0; i < applications.count(); ++i)
			{
				m_ui->applicationComboBox->addItem(applications.at(i).icon, applications.at(i).name);
				m_ui->applicationComboBox->setItemData((i + 2), applications.at(i).command, Qt::UserRole);

				if (applications.at(i).icon.isNull())
				{
					m_ui->applicationComboBox->setItemData((i + 2), QColor(Qt::transparent), Qt::DecorationRole);
				}
			}
		}
	}

	if (m_ui->categoriesButton->menu())
	{
		m_ui->categoriesButton->menu()->deleteLater();
	}

	QMenu *menu(new QMenu(m_ui->categoriesButton));

	m_ui->categoriesButton->setMenu(menu);

	connect(menu, &QMenu::triggered, menu, [=](QAction *action)
	{
		if (action->data().isNull() && action->isChecked())
		{
			m_categories.clear();
		}
		else if (menu->actions().count() > 0)
		{
			QStringList categories;
			bool hasAllCategories = true;

			m_categories.clear();

			for (int i = 2; i < menu->actions().count(); ++i)
			{
				if (menu->actions().at(i)->isChecked())
				{
					categories.append(menu->actions().at(i)->data().toString());
				}
				else
				{
					hasAllCategories = false;
				}
			}

			menu->actions().value(0)->setChecked(hasAllCategories);

			if (!hasAllCategories)
			{
				m_categories = categories;
			}
		}

		updateFeedModel();
	});

	if (!m_feed)
	{
		return;
	}

	QAction *allAction(menu->addAction(tr("All (%1)").arg(m_feed->getEntries().count())));
	allAction->setCheckable(true);
	allAction->setChecked(m_categories.isEmpty());

	menu->addSeparator();

	const QMap<QString, QString> categories(m_feed->getCategories());
	QMap<QString, QString>::const_iterator iterator;

	for (iterator = categories.begin(); iterator != categories.end(); ++iterator)
	{
		const int amount(m_feed->getEntries({iterator.key()}).count());

		if (amount > 0)
		{
			QAction *action(menu->addAction((iterator.value().isEmpty() ? QString(iterator.key()).replace(QLatin1Char('_'), QLatin1Char(' ')) : iterator.value()) + QLatin1String(" (") + QString::number(amount) + QLatin1Char(')')));
			action->setCheckable(true);
			action->setChecked(m_categories.isEmpty() || m_categories.contains(iterator.key()));
			action->setData(iterator.key());
		}
	}

	m_ui->categoriesButton->setEnabled(!categories.isEmpty());

	const QVector<Feed::Entry> entries(m_feed->getEntries(m_categories));

	for (int i = 0; i < entries.count(); ++i)
	{
		const Feed::Entry &entry(entries.at(i));

		if (!m_categories.isEmpty())
		{
			bool hasFound(false);

			for (int j = 0; j < m_categories.count(); ++j)
			{
				if (entry.categories.contains(m_categories.at(j)))
				{
					hasFound = true;

					break;
				}
			}

			if (!hasFound)
			{
				continue;
			}
		}

		QList<QStandardItem*> items({new QStandardItem(entry.title.isEmpty() ? tr("(Untitled)") : entry.title), new QStandardItem(entry.author.isEmpty() ? tr("(Untitled)") : entry.author), new QStandardItem(Utils::formatDateTime(entry.updateTime.isNull() ? entry.publicationTime : entry.updateTime))});
		items[0]->setData(entry.url, UrlRole);
		items[0]->setData(entry.identifier, IdentifierRole);
		items[0]->setData(entry.summary, SummaryRole);
		items[0]->setData(entry.content, ContentRole);
		items[0]->setData(entry.publicationTime, PublicationTimeRole);
		items[0]->setData(entry.updateTime, UpdateTimeRole);
		items[0]->setData(entry.author, AuthorRole);
		items[0]->setData(entry.email, EmailRole);
		items[0]->setData(entry.lastReadTime, LastReadTimeRole);
		items[0]->setData(entry.categories, CategoriesRole);
		items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
		items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);
		items[2]->setFlags(items[2]->flags() | Qt::ItemNeverHasChildren);

		m_feedModel->appendRow(items);

		if (entry.identifier == identifier)
		{
			m_ui->entriesViewWidget->setCurrentIndex(m_ui->entriesViewWidget->getIndex(i));
		}
	}

	if (!m_ui->entriesViewWidget->selectionModel()->hasSelection())
	{
		m_ui->entriesViewWidget->setCurrentIndex(m_ui->entriesViewWidget->getIndex(0));
	}
}

void FeedsContentsWidget::setFeed(Feed *feed)
{
	if (feed == m_feed)
	{
		return;
	}

	if (m_feed)
	{
		disconnect(m_feed, &Feed::entriesModified, this, &FeedsContentsWidget::updateFeedModel);
	}

	m_feed = feed;

	if (m_feedModel)
	{
		m_feedModel->clear();
	}

	if (m_feed)
	{
		m_categories.clear();

		if (!m_feedModel)
		{
			m_feedModel = new QStandardItemModel(this);

			m_ui->entriesViewWidget->setModel(m_feedModel);
			m_ui->entriesViewWidget->setViewMode(ItemViewWidget::ListView);
		}

		if (m_feed->getLastSynchronizationTime().isNull())
		{
			m_feed->update();
		}

		if (!FeedsManager::getModel()->hasFeed(m_feed->getUrl()))
		{
			m_ui->iconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("application-rss+xml"), false).pixmap(m_ui->iconLabel->size()));
			m_ui->messageLabel->setText(tr("Subscribe to this feed using:"));

			m_ui->subscribeFeedWidget->show();
		}
		else
		{
			m_ui->subscribeFeedWidget->hide();
		}

		connect(m_feed, &Feed::entriesModified, this, &FeedsContentsWidget::updateFeedModel);
	}

	updateFeedModel();

	emit arbitraryActionsStateChanged({ActionsManager::ReloadAction});
	emit titleChanged(getTitle());
	emit iconChanged(getIcon());
	emit urlChanged(getUrl());
}

void FeedsContentsWidget::setUrl(const QUrl &url, bool isTypedIn)
{
	Q_UNUSED(isTypedIn)

	if (url.scheme() == QLatin1String("view-feed"))
	{
		setFeed(FeedsManager::createFeed(url.toDisplayString().mid(10)));
	}
}

Animation* FeedsContentsWidget::getUpdateAnimation()
{
	return m_updateAnimation;
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
	return (m_feed ? FeedsManager::createFeedReaderUrl(m_feed->getUrl()) : QUrl(QLatin1String("about:feeds")));
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

ActionsManager::ActionDefinition::State FeedsContentsWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());

	switch (identifier)
	{
		case ActionsManager::ReloadAction:
			state.isEnabled = (m_feed != nullptr);

			return state;
		case ActionsManager::DeleteAction:
			state.isEnabled = false;

			if (m_ui->feedsViewWidget->hasFocus())
			{
				const FeedsModel::EntryType type(static_cast<FeedsModel::EntryType>(m_ui->feedsViewWidget->currentIndex().data(FeedsModel::TypeRole).toInt()));

				state.isEnabled = (type == FeedsModel::FeedEntry || type == FeedsModel::FolderEntry);
			}
			else if (m_ui->entriesViewWidget->hasFocus())
			{
				state.isEnabled = (m_feed && m_ui->entriesViewWidget->selectionModel()->hasSelection());
			}

			return state;
		default:
			break;
	}

	return ContentsWidget::getActionState(identifier, parameters);
}

bool FeedsContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if ((object == m_ui->entriesViewWidget || object == m_ui->feedsViewWidget ) && event->type() == QEvent::KeyPress)
	{
		switch (static_cast<QKeyEvent*>(event)->key())
		{
			case Qt::Key_Delete:
				if (m_ui->feedsViewWidget->hasFocus())
				{
					removeFeed();
				}
				else if (m_ui->entriesViewWidget->hasFocus())
				{
					removeEntry();
				}

				return true;
			case Qt::Key_Enter:
			case Qt::Key_Return:
				if (m_ui->feedsViewWidget->hasFocus())
				{
					openFeed();
				}
				else if (m_ui->entriesViewWidget->hasFocus())
				{
					openEntry();
				}

				return true;
			default:
				break;
		}
	}
	else if ((object == m_ui->entriesViewWidget->viewport() || object == m_ui->feedsViewWidget->viewport()) && event->type() == QEvent::ToolTip)
	{
		const QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));
		ItemViewWidget *viewWidget(object == m_ui->feedsViewWidget->viewport() ? m_ui->feedsViewWidget : m_ui->entriesViewWidget);
		const QModelIndex index(viewWidget->indexAt(helpEvent->pos()));

		if (static_cast<FeedsModel::EntryType>(index.data(FeedsModel::TypeRole).toInt()) != FeedsModel::FeedEntry)
		{
			return false;
		}

		QString toolTip;

		if (index.isValid())
		{
			if (object == m_ui->feedsViewWidget)
			{
				toolTip = tr("Title: %1").arg(index.data(FeedsModel::TitleRole).toString()) + QLatin1Char('\n') + tr("Address: %1").arg(index.data(FeedsModel::UrlRole).toUrl().toDisplayString());

				if (!index.data(FeedsModel::LastSynchronizationTimeRole).isNull())
				{
					toolTip.append(QLatin1Char('\n') + tr("Last update: %1").arg(Utils::formatDateTime(index.data(FeedsModel::LastSynchronizationTimeRole).toDateTime())));
				}
			}
			else
			{
				toolTip = tr("Title: %1").arg(index.data(TitleRole).toString());

				if (!index.data(UrlRole).isNull())
				{
					toolTip.append(QLatin1Char('\n') + tr("Address: %1").arg(index.data(UrlRole).toUrl().toDisplayString()));
				}
			}
		}

		QToolTip::showText(helpEvent->globalPos(), QFontMetrics(QToolTip::font()).elidedText(toolTip, Qt::ElideRight, (QApplication::desktop()->screenGeometry(viewWidget).width() / 2)), viewWidget, viewWidget->visualRect(index));

		return true;
	}

	return ContentsWidget::eventFilter(object, event);
}

}
