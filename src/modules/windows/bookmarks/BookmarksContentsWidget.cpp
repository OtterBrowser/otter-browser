/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../core/SettingsManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/BookmarkPropertiesDialog.h"
#include "../../../ui/ItemDelegate.h"

#include "ui_BookmarksContentsWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QMenu>

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
	m_ui->bookmarksView->viewport()->setMouseTracking(true);

	if (!window)
	{
		m_ui->detailsWidget->hide();
	}

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
	BookmarksItem *bookmark = BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, QUrl(), QString(), findFolder(m_ui->bookmarksView->currentIndex()));
	BookmarkPropertiesDialog dialog(bookmark, BookmarkPropertiesDialog::AddBookmarkMode, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		delete bookmark;
	}
}

void BookmarksContentsWidget::addFolder()
{
	BookmarksItem *bookmark = BookmarksManager::addBookmark(BookmarksModel::FolderBookmark, QUrl(), QString(), findFolder(m_ui->bookmarksView->currentIndex()));
	BookmarkPropertiesDialog dialog(bookmark, BookmarkPropertiesDialog::AddBookmarkMode, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		delete bookmark;
	}
}

void BookmarksContentsWidget::addSeparator()
{
	BookmarksManager::addBookmark(BookmarksModel::SeparatorBookmark, QUrl(), QString(), findFolder(m_ui->bookmarksView->currentIndex()));
}

void BookmarksContentsWidget::removeBookmark()
{
	BookmarksManager::getModel()->trashBookmark(BookmarksManager::getModel()->bookmarkFromIndex(m_ui->bookmarksView->currentIndex()));
}

void BookmarksContentsWidget::restoreBookmark()
{
	BookmarksManager::getModel()->restoreBookmark(BookmarksManager::getModel()->bookmarkFromIndex(m_ui->bookmarksView->currentIndex()));
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

void BookmarksContentsWidget::bookmarkProperties()
{
	BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(BookmarksManager::getModel()->itemFromIndex(m_ui->bookmarksView->currentIndex()));

	if (bookmark)
	{
		BookmarkPropertiesDialog(bookmark, (bookmark->isInTrash() ? BookmarkPropertiesDialog::ViewBookmarkMode : BookmarkPropertiesDialog::EditBookmarkMode), this).exec();

		updateActions();
	}
}

void BookmarksContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index = m_ui->bookmarksView->indexAt(point);
	const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt());
	QMenu menu(this);

	if (type == BookmarksModel::TrashBookmark)
	{
		menu.addAction(Utils::getIcon(QLatin1String("trash-empty")), tr("Empty Trash"), BookmarksManager::getModel(), SLOT(emptyTrash()))->setEnabled(BookmarksManager::getModel()->getTrashItem()->rowCount() > 0);
	}
	else if (type == BookmarksModel::UnknownBookmark)
	{
		menu.addAction(Utils::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
		menu.addAction(tr("Add Bookmark"), this, SLOT(addBookmark()));
		menu.addAction(tr("Add Separator"), this, SLOT(addSeparator()));
	}
	else
	{
		const bool isInTrash = BookmarksManager::getModel()->bookmarkFromIndex(index)->isInTrash();

		menu.addAction(Utils::getIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openBookmark()));
		menu.addAction(tr("Open in New Tab"), this, SLOT(openBookmark()))->setData(NewTabOpen);
		menu.addAction(tr("Open in New Background Tab"), this, SLOT(openBookmark()))->setData(NewBackgroundTabOpen);
		menu.addSeparator();
		menu.addAction(tr("Open in New Window"), this, SLOT(openBookmark()))->setData(NewWindowOpen);
		menu.addAction(tr("Open in New Background Window"), this, SLOT(openBookmark()))->setData(NewBackgroundWindowOpen);

		if (type == BookmarksModel::SeparatorBookmark || (type == BookmarksModel::FolderBookmark && index.child(0, 0).data(BookmarksModel::TypeRole).toInt() == 0))
		{
			for (int i = 0; i < menu.actions().count(); ++i)
			{
				menu.actions().at(i)->setEnabled(false);
			}
		}

		if (type != BookmarksModel::RootBookmark)
		{
			Action *copyLinkAction = getAction(ActionsManager::CopyLinkToClipboardAction);
			copyLinkAction->setEnabled(type == BookmarksModel::UrlBookmark);

			menu.addSeparator();
			menu.addAction(copyLinkAction);
		}

		if (!isInTrash)
		{
			menu.addSeparator();

			QMenu *addMenu = menu.addMenu(tr("Add Bookmark"));
			addMenu->addAction(Utils::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
			addMenu->addAction(tr("Add Bookmark"), this, SLOT(addBookmark()));
			addMenu->addAction(tr("Add Separator"), this, SLOT(addSeparator()));
		}

		if (type != BookmarksModel::RootBookmark)
		{
			menu.addSeparator();

			if (isInTrash)
			{
				menu.addAction(tr("Restore Bookmark"), this, SLOT(restoreBookmark()));
			}
			else
			{
				menu.addAction(getAction(ActionsManager::DeleteAction));
			}

			menu.addSeparator();
			menu.addAction(tr("Properties…"), this, SLOT(bookmarkProperties()));
		}
	}

	menu.exec(m_ui->bookmarksView->mapToGlobal(point));
}

void BookmarksContentsWidget::triggerAction(int identifier, bool checked)
{
	Q_UNUSED(checked)

	switch (identifier)
	{
		case ActionsManager::DeleteAction:
			removeBookmark();

			break;
		case ActionsManager::CopyLinkToClipboardAction:
			if (static_cast<BookmarksModel::BookmarkType>(m_ui->bookmarksView->currentIndex().data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark)
			{
				QGuiApplication::clipboard()->setText(m_ui->bookmarksView->currentIndex().data(BookmarksModel::UrlRole).toString());
			}

			break;
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ActivateAddressFieldAction:
			m_ui->filterLineEdit->setFocus();

			break;
		default:
			break;
	}
}

void BookmarksContentsWidget::updateActions()
{
	const bool hasSelecion = !m_ui->bookmarksView->selectionModel()->selectedIndexes().isEmpty();
	const QModelIndex index = (m_ui->bookmarksView->selectionModel()->hasSelection() ? m_ui->bookmarksView->selectionModel()->currentIndex() : QModelIndex());
	const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt());

	m_ui->addressLabelWidget->setText(index.data(BookmarksModel::UrlRole).toString());
	m_ui->titleLabelWidget->setText(index.data(BookmarksModel::TitleRole).toString());
	m_ui->descriptionLabelWidget->setText(index.data(BookmarksModel::DescriptionRole).toString());
	m_ui->keywordLabelWidget->setText(index.data(BookmarksModel::KeywordRole).toString());
	m_ui->propertiesButton->setEnabled((hasSelecion && (type == BookmarksModel::FolderBookmark || type == BookmarksModel::UrlBookmark)));
	m_ui->deleteButton->setEnabled(hasSelecion && type != BookmarksModel::RootBookmark && type != BookmarksModel::TrashBookmark);

	if (m_actions.contains(ActionsManager::DeleteAction))
	{
		getAction(ActionsManager::DeleteAction)->setEnabled(m_ui->deleteButton->isEnabled());
	}
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

	if (identifier != ActionsManager::CopyLinkToClipboardAction && identifier != ActionsManager::DeleteAction)
	{
		return NULL;
	}

	Action *action = new Action(identifier, this);

	if (identifier == ActionsManager::DeleteAction)
	{
		action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Remove Bookmark"));
	}

	m_actions[identifier] = action;

	connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

	return action;
}

BookmarksItem* BookmarksContentsWidget::findFolder(const QModelIndex &index)
{
	BookmarksItem *item = BookmarksManager::getModel()->bookmarkFromIndex(index);

	if (!item || item == BookmarksManager::getModel()->getRootItem() || item == BookmarksManager::getModel()->getTrashItem())
	{
		return BookmarksManager::getModel()->getRootItem();
	}

	const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(item->data(BookmarksModel::TypeRole).toInt());

	return ((type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark) ? item : dynamic_cast<BookmarksItem*>(item->parent()));
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
		const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(branch->data(BookmarksModel::TypeRole).toInt());

		if ((type == BookmarksModel::FolderBookmark || type == BookmarksModel::UrlBookmark) && (branch->data(BookmarksModel::UrlRole).toString().contains(filter, Qt::CaseInsensitive) || branch->data(BookmarksModel::TitleRole).toString().contains(filter, Qt::CaseInsensitive) || branch->data(BookmarksModel::DescriptionRole).toString().contains(filter, Qt::CaseInsensitive) || branch->data(BookmarksModel::KeywordRole).toString().contains(filter, Qt::CaseInsensitive)))
		{
			found = true;
		}
	}

	m_ui->bookmarksView->setRowHidden(branch->row(), branch->index().parent(), !found);
	m_ui->bookmarksView->setExpanded(branch->index(), (found && !filter.isEmpty()));

	return found;
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
