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
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>

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

	QSet<int> filterRoles;
	filterRoles << BookmarksModel::UrlRole << BookmarksModel::TitleRole << BookmarksModel::DescriptionRole << BookmarksModel::KeywordRole;

	m_ui->addButton->setMenu(addMenu);
	m_ui->notesViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->notesViewWidget->setModel(NotesManager::getModel());
	m_ui->notesViewWidget->setItemDelegate(new ItemDelegate(this));
	m_ui->notesViewWidget->setExpanded(NotesManager::getModel()->getRootItem()->index(), true);
	m_ui->notesViewWidget->setFilterRoles(filterRoles);
	m_ui->notesViewWidget->viewport()->installEventFilter(this);
	m_ui->notesViewWidget->viewport()->setMouseTracking(true);
	m_ui->filterLineEdit->installEventFilter(this);

#if QT_VERSION >= 0x050300
	m_ui->textEdit->setPlaceholderText(tr("Add note…"));
#endif

	if (!window)
	{
		m_ui->actionsWidget->hide();
	}

	connect(NotesManager::getModel(), SIGNAL(modelReset()), this, SLOT(updateActions()));
	connect(m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(removeNote()));
	connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(addNote()));
	connect(m_ui->textEdit, SIGNAL(textChanged()), this, SLOT(updateText()));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), m_ui->notesViewWidget, SLOT(setFilterString(QString)));
	connect(m_ui->notesViewWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openUrl(QModelIndex)));
	connect(m_ui->notesViewWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_ui->notesViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateActions()));
}

NotesContentsWidget::~NotesContentsWidget()
{
	delete m_ui;
}

void NotesContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void NotesContentsWidget::addNote()
{
	NotesManager::addNote(BookmarksModel::UrlBookmark, QUrl(), QString(), findFolder(m_ui->notesViewWidget->currentIndex()));
}

void NotesContentsWidget::addFolder()
{
	const QString title = QInputDialog::getText(this, tr("Select Folder Name"), tr("Enter folder name:"));

	if (!title.isEmpty())
	{
		NotesManager::addNote(BookmarksModel::FolderBookmark, QUrl(), title, findFolder(m_ui->notesViewWidget->currentIndex()));
	}
}

void NotesContentsWidget::addSeparator()
{
	NotesManager::addNote(BookmarksModel::SeparatorBookmark, QUrl(), QString(), findFolder(m_ui->notesViewWidget->currentIndex()));
}

void NotesContentsWidget::removeNote()
{
	NotesManager::getModel()->trashBookmark(NotesManager::getModel()->bookmarkFromIndex(m_ui->notesViewWidget->currentIndex()));
}

void NotesContentsWidget::restoreNote()
{
	NotesManager::getModel()->restoreBookmark(NotesManager::getModel()->bookmarkFromIndex(m_ui->notesViewWidget->currentIndex()));
}

void NotesContentsWidget::openUrl(const QModelIndex &index)
{
	BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(NotesManager::getModel()->itemFromIndex(index.isValid() ? index : m_ui->notesViewWidget->currentIndex()));
	WindowsManager *manager = SessionsManager::getWindowsManager();

	if (bookmark && bookmark->data(BookmarksModel::UrlRole).toUrl().isValid() && manager)
	{
		manager->open(bookmark);
	}
}

void NotesContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index = m_ui->notesViewWidget->indexAt(point);
	const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt());
	QMenu menu(this);

	if (type != BookmarksModel::UrlBookmark && type != BookmarksModel::TrashBookmark)
	{
		menu.addAction(getAction(ActionsManager::PasteAction));
		menu.addSeparator();
	}

	if (type == BookmarksModel::TrashBookmark)
	{
		menu.addAction(Utils::getIcon(QLatin1String("trash-empty")), tr("Empty Trash"), NotesManager::getModel(), SLOT(emptyTrash()))->setEnabled(NotesManager::getModel()->getTrashItem()->rowCount() > 0);
	}
	else if (type == BookmarksModel::UnknownBookmark)
	{
		menu.addAction(Utils::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
		menu.addAction(tr("Add Bookmark"), this, SLOT(addNote()));
		menu.addAction(tr("Add Separator"), this, SLOT(addSeparator()));
	}
	else
	{
		const bool isInTrash = NotesManager::getModel()->bookmarkFromIndex(index)->isInTrash();

		if (type == BookmarksModel::UrlBookmark)
		{
			menu.addAction(getAction(ActionsManager::CutAction));
			menu.addAction(getAction(ActionsManager::CopyAction));
			menu.addAction(getAction(ActionsManager::PasteAction));
			menu.addSeparator();
		}

		menu.addAction(Utils::getIcon(QLatin1String("document-open")), tr("Open source page"), this, SLOT(openUrl()))->setEnabled(type == BookmarksModel::UrlBookmark && index.data(BookmarksModel::UrlRole).toUrl().isValid());

		if (type != BookmarksModel::RootBookmark)
		{
			Action *copyLinkAction = getAction(ActionsManager::CopyLinkToClipboardAction);
			copyLinkAction->setEnabled(type == BookmarksModel::UrlBookmark && index.data(BookmarksModel::UrlRole).toUrl().isValid());

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

		if (type != BookmarksModel::RootBookmark)
		{
			menu.addSeparator();

			if (isInTrash)
			{
				menu.addAction(tr("Restore Note"), this, SLOT(restoreNote()));
			}
			else
			{
				menu.addAction(getAction(ActionsManager::DeleteAction));
			}
		}
	}

	menu.exec(m_ui->notesViewWidget->mapToGlobal(point));
}

void NotesContentsWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	Q_UNUSED(parameters)

	switch (identifier)
	{
		case ActionsManager::CopyLinkToClipboardAction:
			if (static_cast<BookmarksModel::BookmarkType>(m_ui->notesViewWidget->currentIndex().data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark)
			{
				QGuiApplication::clipboard()->setText(m_ui->notesViewWidget->currentIndex().data(BookmarksModel::UrlRole).toString());
			}

			break;
		case ActionsManager::CutAction:
			triggerAction(ActionsManager::CopyAction);
			triggerAction(ActionsManager::DeleteAction);

			break;
		case ActionsManager::CopyAction:
			if (static_cast<BookmarksModel::BookmarkType>(m_ui->notesViewWidget->currentIndex().data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark)
			{
				QGuiApplication::clipboard()->setText(m_ui->notesViewWidget->currentIndex().data(BookmarksModel::DescriptionRole).toString());
			}

			break;
		case ActionsManager::PasteAction:
			if (!QGuiApplication::clipboard()->text().isEmpty())
			{
				BookmarksItem *bookmark = NotesManager::addNote(BookmarksModel::UrlBookmark, QUrl(), QString(), findFolder(m_ui->notesViewWidget->currentIndex()));
				bookmark->setData(QGuiApplication::clipboard()->text(), BookmarksModel::DescriptionRole);
			}

			break;
		case ActionsManager::DeleteAction:
			removeNote();

			break;
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ActivateAddressFieldAction:
			m_ui->filterLineEdit->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->notesViewWidget->setFocus();

			break;
		default:
			break;
	}
}

void NotesContentsWidget::updateActions(bool updateText)
{
	const bool hasSelecion = !m_ui->notesViewWidget->selectionModel()->selectedIndexes().isEmpty();
	const QModelIndex index = (m_ui->notesViewWidget->selectionModel()->hasSelection() ? m_ui->notesViewWidget->selectionModel()->currentIndex() : QModelIndex());
	const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt());

	m_ui->deleteButton->setEnabled(hasSelecion && type != BookmarksModel::RootBookmark && type != BookmarksModel::TrashBookmark);

	if (updateText)
	{
		disconnect(m_ui->textEdit, SIGNAL(textChanged()), this, SLOT(updateText()));

		m_ui->textEdit->setPlainText(index.data(BookmarksModel::DescriptionRole).toString());

		connect(m_ui->textEdit, SIGNAL(textChanged()), this, SLOT(updateText()));
	}

	if (m_actions.contains(ActionsManager::PasteAction))
	{
		m_actions[ActionsManager::PasteAction]->setEnabled(!QGuiApplication::clipboard()->text().isEmpty());
	}

	if (m_actions.contains(ActionsManager::DeleteAction))
	{
		m_actions[ActionsManager::DeleteAction]->setEnabled(m_ui->deleteButton->isEnabled());
	}
}

void NotesContentsWidget::updateText()
{
	const QModelIndex index = (m_ui->notesViewWidget->selectionModel()->hasSelection() ? m_ui->notesViewWidget->selectionModel()->currentIndex() : QModelIndex());

	disconnect(m_ui->notesViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateActions()));

	if (index.isValid() && static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark)
	{
		m_ui->notesViewWidget->model()->setData(index, m_ui->textEdit->toPlainText(), BookmarksModel::DescriptionRole);
	}
	else
	{
		BookmarksItem *bookmark = NotesManager::addNote(BookmarksModel::UrlBookmark, QUrl(), QString(), findFolder(index));
		bookmark->setData(m_ui->textEdit->toPlainText(), BookmarksModel::DescriptionRole);

		m_ui->notesViewWidget->setCurrentIndex(bookmark->index());

		updateActions(false);
	}

	connect(m_ui->notesViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateActions()));
}

void NotesContentsWidget::print(QPrinter *printer)
{
	m_ui->notesViewWidget->render(printer);
}

Action* NotesContentsWidget::getAction(int identifier)
{
	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	switch (identifier)
	{
		case ActionsManager::CopyLinkToClipboardAction:
		case ActionsManager::CutAction:
		case ActionsManager::CopyAction:
		case ActionsManager::PasteAction:
		case ActionsManager::DeleteAction:
			{
				Action *action = new Action(identifier, this);

				m_actions[identifier] = action;

				if (identifier == ActionsManager::CopyLinkToClipboardAction)
				{
					action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Copy address of source page"));
				}
				else if (identifier == ActionsManager::PasteAction)
				{
					action->setEnabled(!QGuiApplication::clipboard()->text().isEmpty());

					connect(QGuiApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(updateActions()));
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

	const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(item->data(BookmarksModel::TypeRole).toInt());

	return ((type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark) ? item : dynamic_cast<BookmarksItem*>(item->parent()));
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

bool NotesContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->notesViewWidget->viewport() && event->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
		WindowsManager *manager = SessionsManager::getWindowsManager();

		if (mouseEvent && ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton))
		{
			BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(NotesManager::getModel()->itemFromIndex(m_ui->notesViewWidget->indexAt(mouseEvent->pos())));

			if (bookmark)
			{
				manager->open(bookmark, WindowsManager::calculateOpenHints(mouseEvent->modifiers(), mouseEvent->button(), NewTabOpen));

				return true;
			}
		}
	}
	else if (object == m_ui->notesViewWidget->viewport() && event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);

		if (helpEvent)
		{
			const QModelIndex index = m_ui->notesViewWidget->indexAt(helpEvent->pos());
			BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(NotesManager::getModel()->itemFromIndex(index));

			if (bookmark)
			{
				QToolTip::showText(helpEvent->globalPos(), QFontMetrics(QToolTip::font()).elidedText(bookmark->toolTip(), Qt::ElideRight, (QApplication::desktop()->screenGeometry(m_ui->notesViewWidget).width() / 2)), m_ui->notesViewWidget, m_ui->notesViewWidget->visualRect(index));
			}

			return true;
		}
	}
	else if (object == m_ui->filterLineEdit && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent->key() == Qt::Key_Escape)
		{
			m_ui->filterLineEdit->clear();
		}
	}

	return ContentsWidget::eventFilter(object, event);
}

}
