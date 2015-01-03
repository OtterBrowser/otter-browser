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
#include <QtGui/QMouseEvent>
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
	m_ui->bookmarksView->setExpanded(BookmarksManager::getModel()->getRootItem()->index(), true);
	m_ui->bookmarksView->viewport()->installEventFilter(this);

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
	BookmarksItem *bookmark = new BookmarksItem(BookmarksItem::UrlBookmark);
	BookmarkPropertiesDialog dialog(bookmark, findFolder(m_ui->bookmarksView->currentIndex()), this);

	if (dialog.exec() == QDialog::Rejected)
	{
		delete bookmark;
	}
}

void BookmarksContentsWidget::addFolder()
{
	BookmarksItem *bookmark = new BookmarksItem(BookmarksItem::FolderBookmark);
	BookmarkPropertiesDialog dialog(bookmark, findFolder(m_ui->bookmarksView->currentIndex()), this);

	if (dialog.exec() == QDialog::Rejected)
	{
		delete bookmark;
	}
}

void BookmarksContentsWidget::addSeparator()
{
	findFolder(m_ui->bookmarksView->currentIndex())->appendRow(new BookmarksItem(BookmarksItem::SeparatorBookmark));
}

void BookmarksContentsWidget::removeBookmark()
{
	QStandardItem *bookmark = BookmarksManager::getModel()->itemFromIndex(m_ui->bookmarksView->currentIndex());
	const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(m_ui->bookmarksView->currentIndex().data(BookmarksModel::TypeRole).toInt());

	if (bookmark && type != BookmarksItem::RootBookmark && type != BookmarksItem::TrashBookmark)
	{
		if (type == BookmarksItem::SeparatorBookmark || isInTrash(bookmark->index()))
		{
			bookmark->parent()->removeRow(bookmark->row());
		}
		else
		{
			m_trash[bookmark] = qMakePair(bookmark->parent()->index(), bookmark->row());

			BookmarksManager::getModel()->getTrashItem()->appendRow(bookmark->parent()->takeRow(bookmark->row()));
			BookmarksManager::getModel()->getTrashItem()->setEnabled(true);
		}
	}
}

void BookmarksContentsWidget::restoreBookmark()
{
	QStandardItem *bookmark = BookmarksManager::getModel()->itemFromIndex(m_ui->bookmarksView->currentIndex());
	QStandardItem *formerParent = (m_trash.contains(bookmark) ? BookmarksManager::getModel()->itemFromIndex(m_trash[bookmark].first) : BookmarksManager::getModel()->getRootItem());

	if (!formerParent || static_cast<BookmarksItem::BookmarkType>(formerParent->data(BookmarksModel::TypeRole).toInt()) != BookmarksItem::FolderBookmark)
	{
		formerParent = BookmarksManager::getModel()->getRootItem();
	}

	if (bookmark)
	{
		if (m_trash.contains(bookmark))
		{
			formerParent->insertRow(m_trash[bookmark].second, bookmark->parent()->takeRow(bookmark->row()));

			m_trash.remove(bookmark);
		}
		else
		{
			formerParent->appendRow(bookmark->parent()->takeRow(bookmark->row()));
		}

		BookmarksManager::getModel()->getTrashItem()->setEnabled(BookmarksManager::getModel()->getTrashItem()->rowCount() > 0);
	}
}

void BookmarksContentsWidget::openBookmark(const QModelIndex &index)
{
	BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(BookmarksManager::getModel()->itemFromIndex(index.isValid() ? index : m_ui->bookmarksView->currentIndex()));
	WindowsManager *manager = SessionsManager::getWindowsManager();

	if (bookmark && manager)
	{
		QAction *action = qobject_cast<QAction*>(sender());

		manager->open(bookmark, (action ? static_cast<OpenHints>(action->data().toInt()) : DefaultOpen));
	}
}

void BookmarksContentsWidget::copyBookmarkLink()
{
	if (static_cast<BookmarksItem::BookmarkType>(m_ui->bookmarksView->currentIndex().data(BookmarksModel::TypeRole).toInt()) == BookmarksItem::UrlBookmark)
	{
		QGuiApplication::clipboard()->setText(m_ui->bookmarksView->currentIndex().data(BookmarksModel::UrlRole).toString());
	}
}

void BookmarksContentsWidget::bookmarkProperties()
{
	BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(BookmarksManager::getModel()->itemFromIndex(m_ui->bookmarksView->currentIndex()));

	if (bookmark)
	{
		BookmarkPropertiesDialog dialog(bookmark, NULL, this);
		dialog.setReadOnly(isInTrash(bookmark->index()));
		dialog.exec();

		updateActions();
	}
}

void BookmarksContentsWidget::emptyTrash()
{
	BookmarksManager::getModel()->getTrashItem()->removeRows(0, BookmarksManager::getModel()->getTrashItem()->rowCount());
	BookmarksManager::getModel()->getTrashItem()->setEnabled(false);
}

void BookmarksContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index = m_ui->bookmarksView->indexAt(point);
	const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt());
	QMenu menu(this);

	if (type == BookmarksItem::TrashBookmark)
	{
		menu.addAction(Utils::getIcon(QLatin1String("trash-empty")), tr("Empty Trash"), this, SLOT(emptyTrash()))->setEnabled(BookmarksManager::getModel()->getTrashItem()->rowCount() > 0);
	}
	else if (type == BookmarksItem::UnknownBookmark)
	{
		menu.addAction(Utils::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
		menu.addAction(tr("Add Bookmark"), this, SLOT(addBookmark()));
		menu.addAction(tr("Add Separator"), this, SLOT(addSeparator()));
	}
	else
	{
		const bool inTrash = isInTrash(index);

		menu.addAction(Utils::getIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openBookmark()));
		menu.addAction(tr("Open in New Tab"), this, SLOT(openBookmark()))->setData(NewTabOpen);
		menu.addAction(tr("Open in New Background Tab"), this, SLOT(openBookmark()))->setData(NewTabBackgroundOpen);
		menu.addSeparator();
		menu.addAction(tr("Open in New Window"), this, SLOT(openBookmark()))->setData(NewWindowOpen);
		menu.addAction(tr("Open in New Background Window"), this, SLOT(openBookmark()))->setData(NewWindowBackgroundOpen);

		if (type == BookmarksItem::SeparatorBookmark || (type == BookmarksItem::FolderBookmark && index.child(0, 0).data(BookmarksModel::TypeRole).toInt() == 0))
		{
			for (int i = 0; i < menu.actions().count(); ++i)
			{
				menu.actions().at(i)->setEnabled(false);
			}
		}

		if (type != BookmarksItem::RootBookmark)
		{
			menu.addSeparator();
			menu.addAction(tr("Copy Link to Clipboard"), this, SLOT(copyBookmarkLink()))->setEnabled(type == BookmarksItem::UrlBookmark);
		}

		if (!inTrash)
		{
			menu.addSeparator();

			QMenu *addMenu = menu.addMenu(tr("Add Bookmark"));
			addMenu->addAction(Utils::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
			addMenu->addAction(tr("Add Bookmark"), this, SLOT(addBookmark()));
			addMenu->addAction(tr("Add Separator"), this, SLOT(addSeparator()));
		}

		if (type != BookmarksItem::RootBookmark)
		{
			menu.addSeparator();

			if (inTrash)
			{
				menu.addAction(tr("Restore Bookmark"), this, SLOT(restoreBookmark()));
			}
			else
			{
				menu.addAction(tr("Remove Bookmark"), this, SLOT(removeBookmark()));
			}

			menu.addSeparator();
			menu.addAction(tr("Properties..."), this, SLOT(bookmarkProperties()));
		}
	}

	menu.exec(m_ui->bookmarksView->mapToGlobal(point));
}

void BookmarksContentsWidget::triggerAction(int identifier, bool checked)
{
	Q_UNUSED(checked)

	if (identifier == Action::DeleteAction)
	{
		removeBookmark();
	}
}

void BookmarksContentsWidget::triggerAction()
{
	Action *action = qobject_cast<Action*>(sender());

	if (action)
	{
		triggerAction(action->getIdentifier());
	}
}

void BookmarksContentsWidget::updateActions()
{
	const bool hasSelecion = !m_ui->bookmarksView->selectionModel()->selectedIndexes().isEmpty();
	const QModelIndex index = (m_ui->bookmarksView->selectionModel()->hasSelection() ? m_ui->bookmarksView->selectionModel()->currentIndex() : QModelIndex());
	const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt());

	m_ui->addressLabelWidget->setText(index.data(BookmarksModel::UrlRole).toString());
	m_ui->titleLabelWidget->setText(index.data(BookmarksModel::TitleRole).toString());
	m_ui->descriptionLabelWidget->setText(index.data(BookmarksModel::DescriptionRole).toString());
	m_ui->keywordLabelWidget->setText(index.data(BookmarksModel::KeywordRole).toString());
	m_ui->propertiesButton->setEnabled((hasSelecion && (type == BookmarksItem::FolderBookmark || type == BookmarksItem::UrlBookmark)));
	m_ui->deleteButton->setEnabled(hasSelecion && type != BookmarksItem::RootBookmark && type != BookmarksItem::TrashBookmark);

	getAction(Action::DeleteAction)->setEnabled(m_ui->deleteButton->isEnabled());
}

void BookmarksContentsWidget::print(QPrinter *printer)
{
	m_ui->bookmarksView->render(printer);
}

Action* BookmarksContentsWidget::getAction(int identifier)
{
	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	if (identifier != Action::DeleteAction)
	{
		return NULL;
	}

	Action *action = new Action(identifier, this);

	m_actions[identifier] = action;

	connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

	return action;
}

QStandardItem* BookmarksContentsWidget::findFolder(const QModelIndex &index)
{
	QStandardItem *item = BookmarksManager::getModel()->itemFromIndex(index);

	if (!item || item == BookmarksManager::getModel()->getRootItem() || item == BookmarksManager::getModel()->getTrashItem())
	{
		return BookmarksManager::getModel()->getRootItem();
	}

	const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(item->data(BookmarksModel::TypeRole).toInt());

	return ((type == BookmarksItem::RootBookmark || type == BookmarksItem::FolderBookmark) ? item : item->parent());
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
		const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(branch->data(BookmarksModel::TypeRole).toInt());

		if ((type == BookmarksItem::FolderBookmark || type == BookmarksItem::UrlBookmark) && (branch->data(BookmarksModel::UrlRole).toString().contains(filter, Qt::CaseInsensitive) || branch->data(BookmarksModel::TitleRole).toString().contains(filter, Qt::CaseInsensitive) || branch->data(BookmarksModel::DescriptionRole).toString().contains(filter, Qt::CaseInsensitive) || branch->data(BookmarksModel::KeywordRole).toString().contains(filter, Qt::CaseInsensitive)))
		{
			found = true;
		}
	}

	m_ui->bookmarksView->setRowHidden(branch->row(), branch->index().parent(), !found);
	m_ui->bookmarksView->setExpanded(branch->index(), (found && !filter.isEmpty()));

	return found;
}

bool BookmarksContentsWidget::isInTrash(const QModelIndex &index) const
{
	QModelIndex parent = index;

	while (parent.isValid())
	{
		const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(parent.data(BookmarksModel::TypeRole).toInt());

		if (type == BookmarksItem::TrashBookmark)
		{
			return true;
		}

		if (type == BookmarksItem::RootBookmark)
		{
			break;
		}

		parent = parent.parent();
	}

	return false;
}

bool BookmarksContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonRelease && object == m_ui->bookmarksView->viewport())
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
		WindowsManager *manager = SessionsManager::getWindowsManager();

		if (mouseEvent && ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton))
		{
			BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(BookmarksManager::getModel()->itemFromIndex(m_ui->bookmarksView->indexAt(mouseEvent->pos())));

			if (bookmark)
			{
				manager->open(bookmark, WindowsManager::calculateOpenHints(mouseEvent->modifiers(), mouseEvent->button(), NewTabOpen));

				return true;
			}
		}
	}

	return ContentsWidget::eventFilter(object, event);
}

}
