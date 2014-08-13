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

#include "BookmarksContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/BookmarksModel.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/BookmarkPropertiesDialog.h"
#include "../../../ui/ItemDelegate.h"

#include "ui_BookmarksContentsWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

namespace Otter
{

BookmarksContentsWidget::BookmarksContentsWidget(Window *window) : ContentsWidget(window),
	m_ui(new Ui::BookmarksContentsWidget)
{
	m_ui->setupUi(this);

	QMenu *addMenu = new QMenu(m_ui->addButton);
	addMenu->addAction(Utils::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
	addMenu->addAction(tr("Add Bookmark"), this, SLOT(addBookmark()));
	addMenu->addAction(tr("Add Separator"), this, SLOT(addSeparator()));

	m_ui->addButton->setMenu(addMenu);
	m_ui->bookmarksView->setModel(BookmarksManager::getModel());
	m_ui->bookmarksView->setItemDelegate(new ItemDelegate(this));

	connect(BookmarksManager::getModel(), SIGNAL(modelReset()), this, SLOT(updateActions()));
	connect(m_ui->propertiesButton, SIGNAL(clicked()), this, SLOT(bookmarkProperties()));
	connect(m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(removeBookmark()));
	connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(addBookmark()));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterBookmarks(QString)));
	connect(m_ui->bookmarksView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openBookmark(QModelIndex)));
	connect(m_ui->bookmarksView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_ui->bookmarksView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
}

BookmarksContentsWidget::~BookmarksContentsWidget()
{
	delete m_ui;
}

void BookmarksContentsWidget::changeEvent(QEvent *event)
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

void BookmarksContentsWidget::addBookmark()
{
	BookmarkInformation *bookmark = new BookmarkInformation();
	bookmark->type = UrlBookmark;

	BookmarkPropertiesDialog dialog(bookmark, findFolder(m_ui->bookmarksView->currentIndex()), this);

	if (dialog.exec() == QDialog::Rejected)
	{
		delete bookmark;
	}
}

void BookmarksContentsWidget::addFolder()
{
	BookmarkInformation *bookmark = new BookmarkInformation();
	bookmark->type = FolderBookmark;

	BookmarkPropertiesDialog dialog(bookmark, findFolder(m_ui->bookmarksView->currentIndex()), this);

	if (dialog.exec() == QDialog::Rejected)
	{
		delete bookmark;
	}
}

void BookmarksContentsWidget::addSeparator()
{
	BookmarkInformation *bookmark = new BookmarkInformation();
	bookmark->type = SeparatorBookmark;
	bookmark->parent = findFolder(m_ui->bookmarksView->currentIndex());

	BookmarksManager::addBookmark(bookmark, bookmark->parent);
}

void BookmarksContentsWidget::removeBookmark()
{
	BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(m_ui->bookmarksView->currentIndex().data(Qt::UserRole).value<void*>());

	if (bookmark && (bookmark->type == SeparatorBookmark || QMessageBox::question(this, tr("Question"), ((bookmark->type == FolderBookmark) ? tr("Do you really want to delete this folder and all of its contents?") : tr("Do you really want to delete this bookmark?")), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Yes))
	{
		BookmarksManager::deleteBookmark(bookmark);
	}
}

void BookmarksContentsWidget::openBookmark(const QModelIndex &index)
{
	WindowsManager *manager = SessionsManager::getWindowsManager();
	QAction *action = qobject_cast<QAction*>(sender());

	if (manager && action && static_cast<BookmarkType>((index.isValid() ? index : m_ui->bookmarksView->currentIndex()).data(BookmarksModel::BookmarkTypeRole).toInt()) == UrlBookmark)
	{
		manager->open((index.isValid() ? index : m_ui->bookmarksView->currentIndex()).data(BookmarksModel::BookmarkUrlRole).toUrl(), (action ? static_cast<OpenHints>(action->data().toInt()) : DefaultOpen));
	}
}

void BookmarksContentsWidget::copyBookmarkLink()
{
	if (static_cast<BookmarkType>(m_ui->bookmarksView->currentIndex().data(BookmarksModel::BookmarkTypeRole).toInt()) == UrlBookmark)
	{
		QGuiApplication::clipboard()->setText(m_ui->bookmarksView->currentIndex().data(BookmarksModel::BookmarkUrlRole).toUrl().toString());
	}
}

void BookmarksContentsWidget::bookmarkProperties()
{
	BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(m_ui->bookmarksView->currentIndex().data(Qt::UserRole).value<void*>());

	if (bookmark)
	{
		BookmarkPropertiesDialog dialog(bookmark, -1, this);
		dialog.exec();
	}
}

void BookmarksContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index = m_ui->bookmarksView->indexAt(point);
	const BookmarkType type = static_cast<BookmarkType>(index.data(BookmarksModel::BookmarkTypeRole).toInt());
	QMenu menu(this);

	if (type != UnknownBookmark)
	{
		menu.addAction(Utils::getIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openBookmark()));
		menu.addAction(tr("Open in New Tab"), this, SLOT(openBookmark()))->setData(NewTabOpen);
		menu.addAction(tr("Open in New Background Tab"), this, SLOT(openBookmark()))->setData(NewTabBackgroundOpen);
		menu.addSeparator();
		menu.addAction(tr("Open in New Window"), this, SLOT(openBookmark()))->setData(NewWindowOpen);
		menu.addAction(tr("Open in New Background Window"), this, SLOT(openBookmark()))->setData(NewWindowBackgroundOpen);

		if (type == SeparatorBookmark || (type == FolderBookmark && index.child(0, 0).data(BookmarksModel::BookmarkTypeRole).toInt() == 0))
		{
			for (int i = 0; i < menu.actions().count(); ++i)
			{
				menu.actions().at(i)->setEnabled(false);
			}
		}

		menu.addSeparator();
		menu.addAction(tr("Copy Link to Clipboard"), this, SLOT(copyBookmarkLink()))->setEnabled(type == UrlBookmark);
		menu.addSeparator();

		QMenu *addMenu = menu.addMenu(tr("Add Bookmark"));
		addMenu->addAction(Utils::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
		addMenu->addAction(tr("Add Bookmark"), this, SLOT(addBookmark()));
		addMenu->addAction(tr("Add Separator"), this, SLOT(addSeparator()));

		menu.addSeparator();
		menu.addAction(tr("Remove Bookmark"), this, SLOT(removeBookmark()));
		menu.addSeparator();
		menu.addAction(tr("Properties..."), this, SLOT(bookmarkProperties()));
	}
	else
	{
		menu.addAction(Utils::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
		menu.addAction(tr("Add Bookmark"), this, SLOT(addBookmark()));
		menu.addAction(tr("Add Separator"), this, SLOT(addSeparator()));
	}

	menu.exec(m_ui->bookmarksView->mapToGlobal(point));
}

void BookmarksContentsWidget::triggerAction(ActionIdentifier action, bool checked)
{
	Q_UNUSED(checked)

	if (action == DeleteAction)
	{
		removeBookmark();
	}
}

void BookmarksContentsWidget::triggerAction()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		triggerAction(static_cast<ActionIdentifier>(action->data().toInt()));
	}
}

void BookmarksContentsWidget::updateActions()
{
	const bool hasSelecion = !m_ui->bookmarksView->selectionModel()->selectedIndexes().isEmpty();
	const QModelIndex index = (m_ui->bookmarksView->selectionModel()->hasSelection() ? m_ui->bookmarksView->selectionModel()->currentIndex() : QModelIndex());
	const BookmarkType type = static_cast<BookmarkType>(index.data(BookmarksModel::BookmarkTypeRole).toInt());

	m_ui->addressLabelWidget->setText(index.data(BookmarksModel::BookmarkUrlRole).toUrl().toString());
	m_ui->titleLabelWidget->setText(index.data(BookmarksModel::BookmarkTitleRole).toString());
	m_ui->descriptionLabelWidget->setText(index.data(BookmarksModel::BookmarkDescriptionRole).toString());
	m_ui->keywordLabelWidget->setText(index.data(BookmarksModel::BookmarkKeywordRole).toString());
	m_ui->propertiesButton->setEnabled((hasSelecion && (type == FolderBookmark || type == UrlBookmark)));
	m_ui->deleteButton->setEnabled(hasSelecion && type != UnknownBookmark);
}

void BookmarksContentsWidget::print(QPrinter *printer)
{
	m_ui->bookmarksView->render(printer);
}

QStandardItem *BookmarksContentsWidget::findFolder(int folder, QStandardItem *item)
{
	if (folder == 0)
	{
		return BookmarksManager::getModel()->invisibleRootItem();
	}

	if (!item)
	{
		item = BookmarksManager::getModel()->invisibleRootItem();
	}

	for (int i = 0; i < item->rowCount(); ++i)
	{
		QStandardItem *child = item->child(i, 0);

		if (!child)
		{
			continue;
		}

		BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(child->data(Qt::UserRole).value<void*>());

		if (bookmark && bookmark->type == FolderBookmark && bookmark->identifier == folder)
		{
			return child;
		}

		QStandardItem *result = findFolder(folder, child);

		if (result)
		{
			return result;
		}
	}

	return NULL;
}

QAction* BookmarksContentsWidget::getAction(ActionIdentifier action)
{
	if (m_actions.contains(action))
	{
		return m_actions[action];
	}

	if (action != DeleteAction)
	{
		return NULL;
	}

	QAction *actionObject = new QAction(this);
	actionObject->setData(action);

	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Delete"), this), actionObject);

	connect(actionObject, SIGNAL(triggered()), this, SLOT(triggerAction()));

	m_actions[action] = actionObject;

	return actionObject;

}

QString BookmarksContentsWidget::getTitle() const
{
	return tr("Bookmarks Manager");
}

QLatin1String BookmarksContentsWidget::getType() const
{
	return QLatin1String("bookmarks");
}

QUrl BookmarksContentsWidget::getUrl() const
{
	return QUrl(QLatin1String("about:bookmarks"));
}

QIcon BookmarksContentsWidget::getIcon() const
{
	return Utils::getIcon(QLatin1String("bookmarks"), false);
}

int BookmarksContentsWidget::findFolder(const QModelIndex &index)
{
	if (index.isValid())
	{
		BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(index.data(Qt::UserRole).value<void*>());

		if (bookmark)
		{
			return ((bookmark->type == FolderBookmark) ? bookmark->identifier : bookmark->parent);
		}
	}

	return 0;
}

bool BookmarksContentsWidget::filterBookmarks(const QString &filter, QStandardItem *branch)
{
	if (!branch)
	{
		if (sender())
		{
			branch = BookmarksManager::getModel()->invisibleRootItem();
		}
		else
		{
			return false;
		}
	}

	bool found = filter.isEmpty();

	for (int i = 0; i < branch->rowCount(); ++i)
	{
		QStandardItem *item = branch->child(i, 0);

		if (item && filterBookmarks(filter, item))
		{
			found = true;
		}
	}

	if (!found)
	{
		BookmarkInformation *bookmark = static_cast<BookmarkInformation*>(branch->data(Qt::UserRole).value<void*>());

		if (bookmark && bookmark->type != SeparatorBookmark && (bookmark->url.contains(filter, Qt::CaseInsensitive) || bookmark->title.contains(filter, Qt::CaseInsensitive) || bookmark->description.contains(filter, Qt::CaseInsensitive) || bookmark->keyword.contains(filter, Qt::CaseInsensitive)))
		{
			found = true;
		}
	}

	m_ui->bookmarksView->setRowHidden(branch->row(), branch->index().parent(), !found);
	m_ui->bookmarksView->setExpanded(branch->index(), (found && !filter.isEmpty()));

	return found;
}

}
