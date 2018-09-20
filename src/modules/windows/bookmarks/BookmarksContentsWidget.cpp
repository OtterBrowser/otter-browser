/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../core/Application.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Action.h"
#include "../../../ui/BookmarkPropertiesDialog.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ProxyModel.h"
#include "../../../ui/Window.h"

#include "ui_BookmarksContentsWidget.h"

#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>

namespace Otter
{

BookmarksContentsWidget::BookmarksContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_ui(new Ui::BookmarksContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);

	QMenu *addMenu(new QMenu(m_ui->addButton));
	addMenu->addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…"), this, &BookmarksContentsWidget::addFolder);
	addMenu->addAction(tr("Add Bookmark…"), this, &BookmarksContentsWidget::addBookmark);
	addMenu->addAction(tr("Add Separator"), this, &BookmarksContentsWidget::addSeparator);

	ProxyModel *model(new ProxyModel(BookmarksManager::getModel(), QVector<QPair<QString, int> >({{tr("Title"), BookmarksModel::TitleRole}, {tr("Address"), BookmarksModel::UrlRole}, {tr("Description"), BookmarksModel::DescriptionRole}, {tr("Keyword"), BookmarksModel::KeywordRole}, {tr("Added"), BookmarksModel::TimeAddedRole}, {tr("Modified"), BookmarksModel::TimeModifiedRole}, {tr("Visited"), BookmarksModel::TimeVisitedRole}, {tr("Visits"), BookmarksModel::VisitsRole}}), this));
	model->setHeaderData(0, Qt::Horizontal, 300, HeaderViewWidget::WidthRole);
	model->setHeaderData(1, Qt::Horizontal, 300, HeaderViewWidget::WidthRole);
	model->setHeaderData(3, Qt::Horizontal, 150, HeaderViewWidget::WidthRole);
	model->setHeaderData(7, Qt::Horizontal, 150, HeaderViewWidget::WidthRole);

	m_ui->addButton->setMenu(addMenu);
	m_ui->bookmarksViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->bookmarksViewWidget->setModel(model);
	m_ui->bookmarksViewWidget->setExpanded(m_ui->bookmarksViewWidget->model()->index(0, 0), true);
	m_ui->bookmarksViewWidget->installEventFilter(this);
	m_ui->bookmarksViewWidget->viewport()->installEventFilter(this);
	m_ui->bookmarksViewWidget->viewport()->setMouseTracking(true);

	if (isSidebarPanel())
	{
		m_ui->detailsWidget->hide();
	}

	connect(BookmarksManager::getModel(), &BookmarksModel::modelReset, this, &BookmarksContentsWidget::updateActions);
	connect(m_ui->propertiesButton, &QPushButton::clicked, this, &BookmarksContentsWidget::bookmarkProperties);
	connect(m_ui->deleteButton, &QPushButton::clicked, this, &BookmarksContentsWidget::removeBookmark);
	connect(m_ui->addButton, &QPushButton::clicked, this, &BookmarksContentsWidget::addBookmark);
	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->bookmarksViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->bookmarksViewWidget, &ItemViewWidget::doubleClicked, this, &BookmarksContentsWidget::openBookmark);
	connect(m_ui->bookmarksViewWidget, &ItemViewWidget::customContextMenuRequested, this, &BookmarksContentsWidget::showContextMenu);
	connect(m_ui->bookmarksViewWidget, &ItemViewWidget::needsActionsUpdate, this, &BookmarksContentsWidget::updateActions);
}

BookmarksContentsWidget::~BookmarksContentsWidget()
{
	delete m_ui;
}

void BookmarksContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void BookmarksContentsWidget::addBookmark()
{
	const BookmarkLocation location(getBookmarkCreationLocation());
	BookmarkPropertiesDialog dialog({}, {}, {}, location.folder, location.row, true, this);
	dialog.exec();
}

void BookmarksContentsWidget::addFolder()
{
	const BookmarkLocation location(getBookmarkCreationLocation());
	BookmarkPropertiesDialog dialog({}, {}, {}, location.folder, location.row, false, this);
	dialog.exec();
}

void BookmarksContentsWidget::addSeparator()
{
	const BookmarkLocation location(getBookmarkCreationLocation());

	BookmarksManager::addBookmark(BookmarksModel::SeparatorBookmark, {}, location.folder, location.row);
}

void BookmarksContentsWidget::removeBookmark()
{
	BookmarksManager::getModel()->trashBookmark(BookmarksManager::getModel()->getBookmark(m_ui->bookmarksViewWidget->currentIndex()));
}

void BookmarksContentsWidget::openBookmark()
{
	const BookmarksModel::Bookmark *bookmark(BookmarksManager::getModel()->getBookmark(m_ui->bookmarksViewWidget->currentIndex()));

	if (bookmark)
	{
		const QAction *action(qobject_cast<QAction*>(sender()));

		Application::triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark->getIdentifier()}, {QLatin1String("hints"), QVariant((action ? static_cast<SessionsManager::OpenHints>(action->data().toInt()) : SessionsManager::DefaultOpen))}}, parentWidget());
	}
}

void BookmarksContentsWidget::bookmarkProperties()
{
	BookmarksModel::Bookmark *bookmark(BookmarksManager::getModel()->getBookmark(m_ui->bookmarksViewWidget->currentIndex()));

	if (bookmark)
	{
		BookmarkPropertiesDialog dialog(bookmark, this);
		dialog.exec();

		updateActions();
	}
}

void BookmarksContentsWidget::showContextMenu(const QPoint &position)
{
	const QModelIndex index(m_ui->bookmarksViewWidget->indexAt(position));
	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()));
	QMenu menu(this);

	switch (type)
	{
		case BookmarksModel::TrashBookmark:
			 menu.addAction(ThemesManager::createIcon(QLatin1String("trash-empty")), tr("Empty Trash"), BookmarksManager::getModel(), &BookmarksModel::emptyTrash)->setEnabled(BookmarksManager::getModel()->getTrashItem()->rowCount() > 0);

			break;
		case BookmarksModel::UnknownBookmark:
			menu.addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…"), this, &BookmarksContentsWidget::addFolder);
			menu.addAction(tr("Add Bookmark…"), this, &BookmarksContentsWidget::addBookmark);
			menu.addAction(tr("Add Separator"), this, &BookmarksContentsWidget::addSeparator);

			break;
		default:
			{
				const bool isInTrash(index.data(BookmarksModel::IsTrashedRole).toBool());

				menu.addAction(ThemesManager::createIcon(QLatin1String("document-open")), QCoreApplication::translate("actions", "Open"), this, &BookmarksContentsWidget::openBookmark);
				menu.addAction(QCoreApplication::translate("actions", "Open in New Tab"), this, &BookmarksContentsWidget::openBookmark)->setData(SessionsManager::NewTabOpen);
				menu.addAction(QCoreApplication::translate("actions", "Open in New Background Tab"), this, &BookmarksContentsWidget::openBookmark)->setData(static_cast<int>(SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen));
				menu.addSeparator();
				menu.addAction(QCoreApplication::translate("actions", "Open in New Window"), this, &BookmarksContentsWidget::openBookmark)->setData(SessionsManager::NewWindowOpen);
				menu.addAction(QCoreApplication::translate("actions", "Open in New Background Window"), this, &BookmarksContentsWidget::openBookmark)->setData(static_cast<int>(SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen));

				if (type == BookmarksModel::SeparatorBookmark || (type == BookmarksModel::FolderBookmark && index.child(0, 0).data(BookmarksModel::TypeRole).toInt() == 0))
				{
					for (int i = 0; i < menu.actions().count(); ++i)
					{
						menu.actions().at(i)->setEnabled(false);
					}
				}

				MainWindow *mainWindow(MainWindow::findMainWindow(this));
				ActionExecutor::Object executor(this, this);
				QVariantMap parameters;

				if (type == BookmarksModel::FolderBookmark)
				{
					parameters[QLatin1String("folder")] = index.data(BookmarksModel::IdentifierRole);
				}

				menu.addSeparator();
				menu.addAction(new Action(ActionsManager::BookmarkAllOpenPagesAction, parameters, ActionExecutor::Object(mainWindow, mainWindow), &menu));
				menu.addSeparator();
				menu.addAction(new Action(ActionsManager::CopyLinkToClipboardAction, {}, executor, &menu));

				if (!isInTrash)
				{
					menu.addSeparator();

					QMenu *addMenu(menu.addMenu(tr("Add Bookmark")));
					addMenu->addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…"), this, &BookmarksContentsWidget::addFolder);
					addMenu->addAction(tr("Add Bookmark…"), this, &BookmarksContentsWidget::addBookmark);
					addMenu->addAction(tr("Add Separator"), this, &BookmarksContentsWidget::addSeparator);
				}

				if (type != BookmarksModel::RootBookmark)
				{
					menu.addSeparator();

					if (isInTrash)
					{
						menu.addAction(tr("Restore Bookmark"), &menu, [&]()
						{
							BookmarksManager::getModel()->restoreBookmark(BookmarksManager::getModel()->getBookmark(m_ui->bookmarksViewWidget->currentIndex()));
						});
					}
					else
					{
						menu.addAction(new Action(ActionsManager::DeleteAction, {}, executor, &menu));
					}

					if (type != BookmarksModel::SeparatorBookmark)
					{
						menu.addSeparator();
						menu.addAction(tr("Properties…"), this, &BookmarksContentsWidget::bookmarkProperties);
					}
				}
			}

			break;
	}

	menu.exec(m_ui->bookmarksViewWidget->mapToGlobal(position));
}

void BookmarksContentsWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
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
			m_ui->filterLineEditWidget->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->bookmarksViewWidget->setFocus();

			break;
		default:
			ContentsWidget::triggerAction(identifier, parameters, trigger);

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

	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::EditingCategory, ActionsManager::ActionDefinition::LinkCategory});
}

void BookmarksContentsWidget::print(QPrinter *printer)
{
	m_ui->bookmarksViewWidget->render(printer);
}

BookmarksContentsWidget::BookmarkLocation BookmarksContentsWidget::getBookmarkCreationLocation()
{
	const QModelIndex index(m_ui->bookmarksViewWidget->currentIndex());
	BookmarksModel::Bookmark *item(BookmarksManager::getModel()->getBookmark(index));
	BookmarkLocation location;

	if (!item || item == BookmarksManager::getModel()->getRootItem() || item == BookmarksManager::getModel()->getTrashItem())
	{
		location.folder = BookmarksManager::getModel()->getRootItem();

		return location;
	}

	const BookmarksModel::BookmarkType type(item->getType());

	location.folder = ((type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark) ? item : static_cast<BookmarksModel::Bookmark*>(item->parent()));
	location.row = ((location.folder && location.folder->index() == index) ? -1 : (index.row() + 1));

	return location;
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
	return ThemesManager::createIcon(QLatin1String("bookmarks"), false);
}

ActionsManager::ActionDefinition::State BookmarksContentsWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());

	switch (identifier)
	{
		case ActionsManager::CopyLinkToClipboardAction:
			state.isEnabled = (static_cast<BookmarksModel::BookmarkType>(m_ui->bookmarksViewWidget->currentIndex().data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark);

			return state;
		case ActionsManager::DeleteAction:
			state.text = QCoreApplication::translate("actions", "Remove Bookmark");
			state.isEnabled = m_ui->deleteButton->isEnabled();

			return state;
		default:
			break;
	}

	return ContentsWidget::getActionState(identifier, parameters);
}

bool BookmarksContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->bookmarksViewWidget && event->type() == QEvent::KeyPress)
	{
		switch (static_cast<QKeyEvent*>(event)->key())
		{
			case Qt::Key_Delete:
				removeBookmark();

				return true;
			case Qt::Key_Enter:
			case Qt::Key_Return:
				openBookmark();

				return true;
			default:
				break;
		}
	}
	else if (object == m_ui->bookmarksViewWidget->viewport() && event->type() == QEvent::MouseButtonRelease)
	{
		const QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton)
		{
			const BookmarksModel::Bookmark *bookmark(BookmarksManager::getModel()->getBookmark(m_ui->bookmarksViewWidget->indexAt(mouseEvent->pos())));

			if (bookmark)
			{
				Application::triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark->getIdentifier()}, {QLatin1String("hints"), QVariant(SessionsManager::calculateOpenHints(SessionsManager::NewTabOpen, mouseEvent->button(), mouseEvent->modifiers()))}}, parentWidget());

				return true;
			}
		}
	}
	else if (object == m_ui->bookmarksViewWidget->viewport() && event->type() == QEvent::ToolTip)
	{
		const QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));
		const QModelIndex index(m_ui->bookmarksViewWidget->indexAt(helpEvent->pos()));
		const BookmarksModel::Bookmark *bookmark(BookmarksManager::getModel()->getBookmark(index));

		if (bookmark)
		{
			QToolTip::showText(helpEvent->globalPos(), QFontMetrics(QToolTip::font()).elidedText(bookmark->toolTip(), Qt::ElideRight, (QApplication::desktop()->screenGeometry(m_ui->bookmarksViewWidget).width() / 2)), m_ui->bookmarksViewWidget, m_ui->bookmarksViewWidget->visualRect(index));
		}

		return true;
	}

	return ContentsWidget::eventFilter(object, event);
}

}
