/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "NotesContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/NotesManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/ItemDelegate.h"

#include "ui_NotesContentsWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>

namespace Otter
{

NotesContentsWidget::NotesContentsWidget(Window *window) : ContentsWidget(window),
	m_ui(new Ui::NotesContentsWidget)
{
	m_ui->setupUi(this);

	QMenu *addMenu = new QMenu(m_ui->addButton);
	addMenu->addAction(Utils::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
	addMenu->addAction(tr("Add Note"), this, SLOT(addNote()));
	addMenu->addAction(tr("Add Separator"), this, SLOT(addSeparator()));

	m_ui->addButton->setMenu(addMenu);
	m_ui->notesView->setModel(NotesManager::getModel());
	m_ui->notesView->setItemDelegate(new ItemDelegate(this));
	m_ui->notesView->setExpanded(NotesManager::getModel()->getRootItem()->index(), true);
	m_ui->notesView->viewport()->installEventFilter(this);
	m_ui->notesView->viewport()->setMouseTracking(true);

#if QT_VERSION >= 0x050300
	m_ui->textEdit->setPlaceholderText(tr("Add note..."));
#endif

	connect(NotesManager::getModel(), SIGNAL(modelReset()), this, SLOT(updateActions()));
	connect(m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(removeNote()));
	connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(addNote()));
	connect(m_ui->textEdit, SIGNAL(textChanged()), this, SLOT(updateText()));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterNotes(QString)));
	connect(m_ui->notesView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openUrl(QModelIndex)));
	connect(m_ui->notesView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_ui->notesView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
}

NotesContentsWidget::~NotesContentsWidget()
{
	delete m_ui;
}

void NotesContentsWidget::changeEvent(QEvent *event)
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

void NotesContentsWidget::addNote()
{
	NotesManager::addNote(BookmarksItem::UrlBookmark, QUrl(), QString(), findFolder(m_ui->notesView->currentIndex()));
}

void NotesContentsWidget::addFolder()
{
	const QString title = QInputDialog::getText(this, tr("Select Folder Name"), tr("Enter folder name:"));

	if (!title.isEmpty())
	{
		NotesManager::addNote(BookmarksItem::FolderBookmark, QUrl(), title, findFolder(m_ui->notesView->currentIndex()));
	}
}

void NotesContentsWidget::addSeparator()
{
	NotesManager::addNote(BookmarksItem::SeparatorBookmark, QUrl(), QString(), findFolder(m_ui->notesView->currentIndex()));
}

void NotesContentsWidget::removeNote()
{
	NotesManager::getModel()->trashBookmark(NotesManager::getModel()->bookmarkFromIndex(m_ui->notesView->currentIndex()));
}

void NotesContentsWidget::restoreNote()
{
	NotesManager::getModel()->restoreBookmark(NotesManager::getModel()->bookmarkFromIndex(m_ui->notesView->currentIndex()));
}

void NotesContentsWidget::openUrl(const QModelIndex &index)
{
	BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(NotesManager::getModel()->itemFromIndex(index.isValid() ? index : m_ui->notesView->currentIndex()));
	WindowsManager *manager = SessionsManager::getWindowsManager();

	if (bookmark && bookmark->data(BookmarksModel::UrlRole).toUrl().isValid() && manager)
	{
		manager->open(bookmark);
	}
}

void NotesContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index = m_ui->notesView->indexAt(point);
	const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt());
	QMenu menu(this);

	if (type != BookmarksItem::UrlBookmark && type != BookmarksItem::TrashBookmark)
	{
		menu.addAction(getAction(Action::PasteAction));
		menu.addSeparator();
	}

	if (type == BookmarksItem::TrashBookmark)
	{
		menu.addAction(Utils::getIcon(QLatin1String("trash-empty")), tr("Empty Trash"), NotesManager::getModel(), SLOT(emptyTrash()))->setEnabled(NotesManager::getModel()->getTrashItem()->rowCount() > 0);
	}
	else if (type == BookmarksItem::UnknownBookmark)
	{
		menu.addAction(Utils::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
		menu.addAction(tr("Add Bookmark"), this, SLOT(addNote()));
		menu.addAction(tr("Add Separator"), this, SLOT(addSeparator()));
	}
	else
	{
		const bool isInTrash = NotesManager::getModel()->bookmarkFromIndex(index)->isInTrash();

		if (type == BookmarksItem::UrlBookmark)
		{
			menu.addAction(getAction(Action::CutAction));
			menu.addAction(getAction(Action::CopyAction));
			menu.addAction(getAction(Action::PasteAction));
			menu.addSeparator();
		}

		menu.addAction(Utils::getIcon(QLatin1String("document-open")), tr("Open source page"), this, SLOT(openUrl()))->setEnabled(type == BookmarksItem::UrlBookmark && index.data(BookmarksModel::UrlRole).toUrl().isValid());

		if (type != BookmarksItem::RootBookmark)
		{
			Action *copyLinkAction = getAction(Action::CopyLinkToClipboardAction);
			copyLinkAction->setEnabled(type == BookmarksItem::UrlBookmark && index.data(BookmarksModel::UrlRole).toUrl().isValid());

			menu.addAction(copyLinkAction);
		}

		if (!isInTrash)
		{
			menu.addSeparator();

			QMenu *addMenu = menu.addMenu(tr("Add Note"));
			addMenu->addAction(Utils::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
			addMenu->addAction(tr("Add Note"), this, SLOT(addNote()));
			addMenu->addAction(tr("Add Separator"), this, SLOT(addSeparator()));
		}

		if (type != BookmarksItem::RootBookmark)
		{
			menu.addSeparator();

			if (isInTrash)
			{
				menu.addAction(tr("Restore Note"), this, SLOT(restoreNote()));
			}
			else
			{
				menu.addAction(getAction(Action::DeleteAction));
			}
		}
	}

	menu.exec(m_ui->notesView->mapToGlobal(point));
}

void NotesContentsWidget::triggerAction(int identifier, bool checked)
{
	Q_UNUSED(checked)

	switch (identifier)
	{
		case Action::CopyLinkToClipboardAction:
			if (static_cast<BookmarksItem::BookmarkType>(m_ui->notesView->currentIndex().data(BookmarksModel::TypeRole).toInt()) == BookmarksItem::UrlBookmark)
			{
				QGuiApplication::clipboard()->setText(m_ui->notesView->currentIndex().data(BookmarksModel::UrlRole).toString());
			}

			break;
		case Action::CutAction:
			triggerAction(Action::CopyAction);
			triggerAction(Action::DeleteAction);

			break;
		case Action::CopyAction:
			if (static_cast<BookmarksItem::BookmarkType>(m_ui->notesView->currentIndex().data(BookmarksModel::TypeRole).toInt()) == BookmarksItem::UrlBookmark)
			{
				QGuiApplication::clipboard()->setText(m_ui->notesView->currentIndex().data(BookmarksModel::DescriptionRole).toString());
			}

			break;
		case Action::PasteAction:
			{
				BookmarksItem *bookmark = NotesManager::addNote(BookmarksItem::UrlBookmark, QUrl(), QString(), findFolder(m_ui->notesView->currentIndex()));;
				bookmark->setData(QGuiApplication::clipboard()->text(), BookmarksModel::DescriptionRole);
			}

			break;
		case Action::DeleteAction:
			removeNote();

			break;
		default:
			break;
	}
}

void NotesContentsWidget::triggerAction()
{
	Action *action = qobject_cast<Action*>(sender());

	if (action)
	{
		triggerAction(action->getIdentifier());
	}
}

void NotesContentsWidget::updateActions()
{
	const bool hasSelecion = !m_ui->notesView->selectionModel()->selectedIndexes().isEmpty();
	const QModelIndex index = (m_ui->notesView->selectionModel()->hasSelection() ? m_ui->notesView->selectionModel()->currentIndex() : QModelIndex());
	const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt());

	m_ui->deleteButton->setEnabled(hasSelecion && type != BookmarksItem::RootBookmark && type != BookmarksItem::TrashBookmark);

	disconnect(m_ui->textEdit, SIGNAL(textChanged()), this, SLOT(updateText()));

	m_ui->textEdit->setPlainText(index.data(BookmarksModel::DescriptionRole).toString());

	connect(m_ui->textEdit, SIGNAL(textChanged()), this, SLOT(updateText()));

	if (m_actions.contains(Action::DeleteAction))
	{
		getAction(Action::DeleteAction)->setEnabled(m_ui->deleteButton->isEnabled());
	}
}

void NotesContentsWidget::updateText()
{
	const QModelIndex index = (m_ui->notesView->selectionModel()->hasSelection() ? m_ui->notesView->selectionModel()->currentIndex() : QModelIndex());

	if (index.isValid() && static_cast<BookmarksItem::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()) == BookmarksItem::UrlBookmark)
	{
		m_ui->notesView->model()->setData(index, m_ui->textEdit->toPlainText(), BookmarksModel::DescriptionRole);
	}
	else
	{
		BookmarksItem *bookmark = NotesManager::addNote(BookmarksItem::UrlBookmark, QUrl(), QString(), findFolder(index));
		bookmark->setData(m_ui->textEdit->toPlainText(), BookmarksModel::DescriptionRole);

		m_ui->notesView->setCurrentIndex(bookmark->index());
	}
}

void NotesContentsWidget::print(QPrinter *printer)
{
	m_ui->notesView->render(printer);
}

Action* NotesContentsWidget::getAction(int identifier)
{
	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	switch (identifier)
	{
		case Action::CopyLinkToClipboardAction:
		case Action::CutAction:
		case Action::CopyAction:
		case Action::PasteAction:
		case Action::DeleteAction:
			{
				Action *action = new Action(identifier, qobject_cast<Window*>(parentWidget()), this);

				m_actions[identifier] = action;

				if (identifier == Action::CopyLinkToClipboardAction)
				{
					action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Copy address of source page"));
				}

				connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

				return action;
			}
	}

	return NULL;
}

BookmarksItem* NotesContentsWidget::findFolder(const QModelIndex &index)
{
	BookmarksItem *item = NotesManager::getModel()->bookmarkFromIndex(index);

	if (!item || item == NotesManager::getModel()->getRootItem() || item == NotesManager::getModel()->getTrashItem())
	{
		return NotesManager::getModel()->getRootItem();
	}

	const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(item->data(BookmarksModel::TypeRole).toInt());

	return ((type == BookmarksItem::RootBookmark || type == BookmarksItem::FolderBookmark) ? item : dynamic_cast<BookmarksItem*>(item->parent()));
}

QString NotesContentsWidget::getTitle() const
{
	return tr("Notes Manager");
}

QLatin1String NotesContentsWidget::getType() const
{
	return QLatin1String("notes");
}

QUrl NotesContentsWidget::getUrl() const
{
	return QUrl(QLatin1String("about:notes"));
}

QIcon NotesContentsWidget::getIcon() const
{
	return Utils::getIcon(QLatin1String("notes"), false);
}

bool NotesContentsWidget::filterNotes(const QString &filter, QStandardItem *branch)
{
	if (!branch)
	{
		if (sender())
		{
			branch = NotesManager::getModel()->invisibleRootItem();
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

		if (item && filterNotes(filter, item))
		{
			found = true;
		}
	}

	if (!found)
	{
		const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(branch->data(BookmarksModel::TypeRole).toInt());

		if ((type == BookmarksItem::FolderBookmark || type == BookmarksItem::UrlBookmark) && (branch->data(BookmarksModel::UrlRole).toString().contains(filter, Qt::CaseInsensitive) || branch->data(BookmarksModel::DescriptionRole).toString().contains(filter, Qt::CaseInsensitive) || branch->data(BookmarksModel::KeywordRole).toString().contains(filter, Qt::CaseInsensitive)))
		{
			found = true;
		}
	}

	m_ui->notesView->setRowHidden(branch->row(), branch->index().parent(), !found);
	m_ui->notesView->setExpanded(branch->index(), (found && !filter.isEmpty()));

	return found;
}

bool NotesContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonRelease && object == m_ui->notesView->viewport())
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
		WindowsManager *manager = SessionsManager::getWindowsManager();

		if (mouseEvent && ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton))
		{
			BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(NotesManager::getModel()->itemFromIndex(m_ui->notesView->indexAt(mouseEvent->pos())));

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
