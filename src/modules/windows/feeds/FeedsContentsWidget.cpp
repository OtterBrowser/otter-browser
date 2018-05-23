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
#include "../../../core/FeedsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Action.h"
#include "../../../ui/FeedPropertiesDialog.h"

#include "ui_FeedsContentsWidget.h"

#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>

namespace Otter
{

FeedsContentsWidget::FeedsContentsWidget(const QVariantMap &parameters, QWidget *parent) : ContentsWidget(parameters, nullptr, parent),
	m_ui(new Ui::FeedsContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->feedsViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->feedsViewWidget->setModel(FeedsManager::getModel());
	m_ui->feedsViewWidget->expandAll();
	m_ui->feedsViewWidget->viewport()->installEventFilter(this);
	m_ui->feedsViewWidget->viewport()->setMouseTracking(true);

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
	}
}

void FeedsContentsWidget::addFeed()
{
///TODO
}

void FeedsContentsWidget::addFolder()
{
///TODO
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

QString FeedsContentsWidget::getTitle() const
{
	return tr("Feeds");
}

QLatin1String FeedsContentsWidget::getType() const
{
	return QLatin1String("feeds");
}

QUrl FeedsContentsWidget::getUrl() const
{
	return QUrl(QLatin1String("about:feeds"));
}

QIcon FeedsContentsWidget::getIcon() const
{
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
