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

#include "BookmarksContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/BookmarkPropertiesDialog.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ProxyModel.h"
#include "../../../ui/Window.h"

#include "ui_BookmarksContentsWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>

namespace Otter
{

BookmarksContentsWidget::BookmarksContentsWidget(const QVariantMap &parameters, Window *window) : ContentsWidget(parameters, window),
	m_ui(new Ui::BookmarksContentsWidget)
{
	m_ui->setupUi(this);

	QMenu *addMenu(new QMenu(m_ui->addButton));
	addMenu->addAction(ThemesManager::getIcon(QLatin1String("inode-directory")), tr("Add Folder…"), this, SLOT(addFolder()));
	addMenu->addAction(tr("Add Bookmark…"), this, SLOT(addBookmark()));
	addMenu->addAction(tr("Add Separator"), this, SLOT(addSeparator()));

	ProxyModel *model(new ProxyModel(BookmarksManager::getModel(), QList<QPair<QString, int> >({qMakePair(tr("Title"), BookmarksModel::TitleRole), qMakePair(tr("Address"), BookmarksModel::UrlRole), qMakePair(tr("Description"), BookmarksModel::DescriptionRole), qMakePair(tr("Keyword"), BookmarksModel::KeywordRole), qMakePair(tr("Added"), BookmarksModel::TimeAddedRole), qMakePair(tr("Modified"), BookmarksModel::TimeModifiedRole), qMakePair(tr("Visited"), BookmarksModel::TimeVisitedRole), qMakePair(tr("Visits"), BookmarksModel::VisitsRole)}), this));

	m_ui->addButton->setMenu(addMenu);
	m_ui->bookmarksViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->bookmarksViewWidget->setModel(model);
	m_ui->bookmarksViewWidget->setExpanded(m_ui->bookmarksViewWidget->model()->index(0, 0), true);
	m_ui->bookmarksViewWidget->setFilterRoles(QSet<int>({BookmarksModel::UrlRole, BookmarksModel::TitleRole, BookmarksModel::DescriptionRole, BookmarksModel::KeywordRole}));
	m_ui->bookmarksViewWidget->installEventFilter(this);
	m_ui->bookmarksViewWidget->viewport()->installEventFilter(this);
	m_ui->bookmarksViewWidget->viewport()->setMouseTracking(true);
	m_ui->filterLineEdit->installEventFilter(this);

	if (isSidebarPanel())
	{
		m_ui->detailsWidget->hide();
	}

	connect(BookmarksManager::getModel(), SIGNAL(modelReset()), this, SLOT(updateActions()));
	connect(m_ui->propertiesButton, SIGNAL(clicked()), this, SLOT(bookmarkProperties()));
	connect(m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(removeBookmark()));
	connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(addBookmark()));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), m_ui->bookmarksViewWidget, SLOT(setFilterString(QString)));
	connect(m_ui->bookmarksViewWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openBookmark(QModelIndex)));
	connect(m_ui->bookmarksViewWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_ui->bookmarksViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateActions()));
}

BookmarksContentsWidget::~BookmarksContentsWidget()
{
	delete m_ui;
}

void BookmarksContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void BookmarksContentsWidget::addBookmark()
{
	const QModelIndex index(m_ui->bookmarksViewWidget->currentIndex());
	BookmarksItem *folder(findFolder(index));
	BookmarkPropertiesDialog dialog(QUrl(), QString(), QString(), folder, ((folder && folder->index() == index) ? -1 : (index.row() + 1)), true, this);
	dialog.exec();
}

void BookmarksContentsWidget::addFolder()
{
	const QModelIndex index(m_ui->bookmarksViewWidget->currentIndex());
	BookmarksItem *folder(findFolder(index));
	BookmarkPropertiesDialog dialog(QUrl(), QString(), QString(), folder, ((folder && folder->index() == index) ? -1 : (index.row() + 1)), false, this);
	dialog.exec();
}

void BookmarksContentsWidget::addSeparator()
{
	const QModelIndex index(m_ui->bookmarksViewWidget->currentIndex());
	BookmarksItem *folder(findFolder(index));

	BookmarksManager::addBookmark(BookmarksModel::SeparatorBookmark, QUrl(), QString(), folder, ((folder && folder->index() == index) ? -1 : (index.row() + 1)));
}

void BookmarksContentsWidget::removeBookmark()
{
	BookmarksManager::getModel()->trashBookmark(BookmarksManager::getModel()->getBookmark(m_ui->bookmarksViewWidget->currentIndex()));
}

void BookmarksContentsWidget::restoreBookmark()
{
	BookmarksManager::getModel()->restoreBookmark(BookmarksManager::getModel()->getBookmark(m_ui->bookmarksViewWidget->currentIndex()));
}

void BookmarksContentsWidget::openBookmark(const QModelIndex &index)
{
	BookmarksItem *bookmark(BookmarksManager::getModel()->getBookmark(index.isValid() ? index : m_ui->bookmarksViewWidget->currentIndex()));

	if (bookmark)
	{
		QAction *action(qobject_cast<QAction*>(sender()));

		ActionsManager::triggerAction(ActionsManager::OpenBookmarkAction, parent(), {{QLatin1String("bookmark"), bookmark->data(BookmarksModel::IdentifierRole)}, {QLatin1String("hints"), QVariant((action ? static_cast<WindowsManager::OpenHints>(action->data().toInt()) : WindowsManager::DefaultOpen))}});
	}
}

void BookmarksContentsWidget::bookmarkProperties()
{
	BookmarksItem *bookmark(BookmarksManager::getModel()->getBookmark(m_ui->bookmarksViewWidget->currentIndex()));

	if (bookmark)
	{
		BookmarkPropertiesDialog dialog(bookmark, this);
		dialog.exec();

		updateActions();
	}
}

void BookmarksContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index(m_ui->bookmarksViewWidget->indexAt(point));
	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()));
	QMenu menu(this);

	if (type == BookmarksModel::TrashBookmark)
	{
		menu.addAction(ThemesManager::getIcon(QLatin1String("trash-empty")), tr("Empty Trash"), BookmarksManager::getModel(), SLOT(emptyTrash()))->setEnabled(BookmarksManager::getModel()->getTrashItem()->rowCount() > 0);
	}
	else if (type == BookmarksModel::UnknownBookmark)
	{
		menu.addAction(ThemesManager::getIcon(QLatin1String("inode-directory")), tr("Add Folder…"), this, SLOT(addFolder()));
		menu.addAction(tr("Add Bookmark…"), this, SLOT(addBookmark()));
		menu.addAction(tr("Add Separator"), this, SLOT(addSeparator()));
	}
	else
	{
		const bool isInTrash(index.data(BookmarksModel::IsTrashedRole).toBool());

		menu.addAction(ThemesManager::getIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openBookmark()));
		menu.addAction(tr("Open in New Tab"), this, SLOT(openBookmark()))->setData(WindowsManager::NewTabOpen);
		menu.addAction(tr("Open in New Background Tab"), this, SLOT(openBookmark()))->setData(static_cast<int>(WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen));
		menu.addSeparator();
		menu.addAction(tr("Open in New Window"), this, SLOT(openBookmark()))->setData(WindowsManager::NewWindowOpen);
		menu.addAction(tr("Open in New Background Window"), this, SLOT(openBookmark()))->setData(static_cast<int>(WindowsManager::NewWindowOpen | WindowsManager::BackgroundOpen));

		if (type == BookmarksModel::SeparatorBookmark || (type == BookmarksModel::FolderBookmark && index.child(0, 0).data(BookmarksModel::TypeRole).toInt() == 0))
		{
			for (int i = 0; i < menu.actions().count(); ++i)
			{
				menu.actions().at(i)->setEnabled(false);
			}
		}

		MainWindow *mainWindow(MainWindow::findMainWindow(this));
		Action *bookmarkAllOpenPagesAction(new Action(ActionsManager::BookmarkAllOpenPagesAction, this));
		bookmarkAllOpenPagesAction->setEnabled(mainWindow && mainWindow->getWindowsManager()->getWindowCount() > 0);

		Action *copyLinkAction(new Action(ActionsManager::CopyLinkToClipboardAction, this));
		copyLinkAction->setEnabled(type == BookmarksModel::UrlBookmark);

		menu.addSeparator();
		menu.addAction(ActionsManager::getAction(ActionsManager::BookmarkPageAction, this));
		menu.addAction(bookmarkAllOpenPagesAction);
		menu.addSeparator();
		menu.addAction(copyLinkAction);

		if (!isInTrash)
		{
			menu.addSeparator();

			QMenu *addMenu(menu.addMenu(tr("Add Bookmark")));
			addMenu->addAction(ThemesManager::getIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
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

			if (type != BookmarksModel::SeparatorBookmark)
			{
				menu.addSeparator();
				menu.addAction(tr("Properties…"), this, SLOT(bookmarkProperties()));
			}
		}

		connect(bookmarkAllOpenPagesAction, SIGNAL(triggered(bool)), this, SLOT(triggerAction()));
		connect(copyLinkAction, SIGNAL(triggered(bool)), this, SLOT(triggerAction()));
	}

	menu.exec(m_ui->bookmarksViewWidget->mapToGlobal(point));
}

void BookmarksContentsWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	Q_UNUSED(parameters)

	switch (identifier)
	{
		case ActionsManager::DeleteAction:
			removeBookmark();

			break;
		case ActionsManager::CopyLinkToClipboardAction:
			if (static_cast<BookmarksModel::BookmarkType>(m_ui->bookmarksViewWidget->currentIndex().data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark)
			{
				QGuiApplication::clipboard()->setText(m_ui->bookmarksViewWidget->currentIndex().data(BookmarksModel::UrlRole).toString());
			}

			break;
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ActivateAddressFieldAction:
			m_ui->filterLineEdit->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->bookmarksViewWidget->setFocus();

			break;
		case ActionsManager::BookmarkAllOpenPagesAction:
			{
				MainWindow *mainWindow(MainWindow::findMainWindow(this));

				if (mainWindow)
				{
					for (int i = 0; i < mainWindow->getWindowsManager()->getWindowCount(); ++i)
					{
						Window *window(mainWindow->getWindowsManager()->getWindowByIndex(i));

						if (window && !Utils::isUrlEmpty(window->getUrl()))
						{
							BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, window->getUrl(), window->getTitle(), findFolder(m_ui->bookmarksViewWidget->currentIndex()));
						}
					}
				}
			}

			break;
		default:
			break;
	}
}

void BookmarksContentsWidget::updateActions()
{
	const bool hasSelecion(!m_ui->bookmarksViewWidget->selectionModel()->selectedIndexes().isEmpty());
	const QModelIndex index(hasSelecion ? m_ui->bookmarksViewWidget->selectionModel()->currentIndex().sibling(m_ui->bookmarksViewWidget->selectionModel()->currentIndex().row(), 0) : QModelIndex());
	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()));

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
	m_ui->bookmarksViewWidget->render(printer);
}

Action* BookmarksContentsWidget::getAction(int identifier)
{
	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	if (identifier != ActionsManager::DeleteAction)
	{
		return nullptr;
	}

	Action *action(new Action(identifier, this));

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
	BookmarksItem *item(BookmarksManager::getModel()->getBookmark(index));

	if (!item || item == BookmarksManager::getModel()->getRootItem() || item == BookmarksManager::getModel()->getTrashItem())
	{
		return BookmarksManager::getModel()->getRootItem();
	}

	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(item->data(BookmarksModel::TypeRole).toInt()));

	return ((type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark) ? item : dynamic_cast<BookmarksItem*>(item->parent()));
}

QString BookmarksContentsWidget::getTitle() const
{
	return tr("Bookmarks");
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
	return ThemesManager::getIcon(QLatin1String("bookmarks"), false);
}

bool BookmarksContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->bookmarksViewWidget && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return))
		{
			openBookmark();

			return true;
		}

		if (keyEvent && keyEvent->key() == Qt::Key_Delete)
		{
			removeBookmark();

			return true;
		}
	}
	else if (object == m_ui->bookmarksViewWidget->viewport() && event->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent && ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton))
		{
			BookmarksItem *bookmark(BookmarksManager::getModel()->getBookmark(m_ui->bookmarksViewWidget->indexAt(mouseEvent->pos())));

			if (bookmark)
			{
				ActionsManager::triggerAction(ActionsManager::OpenBookmarkAction, parent(), {{QLatin1String("bookmark"), bookmark->data(BookmarksModel::IdentifierRole)}, {QLatin1String("hints"), QVariant(WindowsManager::calculateOpenHints(WindowsManager::NewTabOpen, mouseEvent->button(), mouseEvent->modifiers()))}});

				return true;
			}
		}
	}
	else if (object == m_ui->bookmarksViewWidget->viewport() && event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));

		if (helpEvent)
		{
			const QModelIndex index(m_ui->bookmarksViewWidget->indexAt(helpEvent->pos()));
			BookmarksItem *bookmark(BookmarksManager::getModel()->getBookmark(index));

			if (bookmark)
			{
				QToolTip::showText(helpEvent->globalPos(), QFontMetrics(QToolTip::font()).elidedText(bookmark->toolTip(), Qt::ElideRight, (QApplication::desktop()->screenGeometry(m_ui->bookmarksViewWidget).width() / 2)), m_ui->bookmarksViewWidget, m_ui->bookmarksViewWidget->visualRect(index));
			}

			return true;
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
