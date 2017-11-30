/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "NotesContentsWidget.h"
#include "../../../core/Application.h"
#include "../../../core/NotesManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/Action.h"

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

NotesContentsWidget::NotesContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_ui(new Ui::NotesContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);

	QMenu *addMenu(new QMenu(m_ui->addButton));
	addMenu->addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…"), this, SLOT(addFolder()));
	addMenu->addAction(tr("Add Note"), this, SLOT(addNote()));
	addMenu->addAction(tr("Add Separator"), this, SLOT(addSeparator()));

	m_ui->addButton->setMenu(addMenu);
	m_ui->notesViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->notesViewWidget->setModel(NotesManager::getModel());
	m_ui->notesViewWidget->setExpanded(NotesManager::getModel()->getRootItem()->index(), true);
	m_ui->notesViewWidget->setFilterRoles({BookmarksModel::UrlRole, BookmarksModel::TitleRole, BookmarksModel::DescriptionRole, BookmarksModel::KeywordRole});
	m_ui->notesViewWidget->installEventFilter(this);
	m_ui->notesViewWidget->viewport()->installEventFilter(this);
	m_ui->notesViewWidget->viewport()->setMouseTracking(true);
	m_ui->textEditWidget->setPlaceholderText(tr("Add note…"));

	if (isSidebarPanel())
	{
		m_ui->actionsWidget->hide();
	}

	connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &NotesContentsWidget::notifyPasteActionStateChanged);
	connect(NotesManager::getModel(), &BookmarksModel::modelReset, this, &NotesContentsWidget::updateActions);
	connect(m_ui->deleteButton, &QPushButton::clicked, this, &NotesContentsWidget::removeNote);
	connect(m_ui->addButton, &QPushButton::clicked, this, &NotesContentsWidget::addNote);
	connect(m_ui->textEditWidget, &TextEditWidget::textChanged, this, &NotesContentsWidget::updateText);
	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->notesViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->notesViewWidget, &ItemViewWidget::doubleClicked, this, &NotesContentsWidget::openUrl);
	connect(m_ui->notesViewWidget, &ItemViewWidget::customContextMenuRequested, this, &NotesContentsWidget::showContextMenu);
	connect(m_ui->notesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &NotesContentsWidget::updateActions);
}

NotesContentsWidget::~NotesContentsWidget()
{
	delete m_ui;
}

void NotesContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void NotesContentsWidget::print(QPrinter *printer)
{
	m_ui->notesViewWidget->render(printer);
}

void NotesContentsWidget::addNote()
{
	m_ui->notesViewWidget->setCurrentIndex(NotesManager::addNote(BookmarksModel::UrlBookmark, {}, findFolder(m_ui->notesViewWidget->currentIndex()))->index());
	m_ui->textEditWidget->setFocus();
}

void NotesContentsWidget::addFolder()
{
	const QString title(QInputDialog::getText(this, tr("Select Folder Name"), tr("Enter folder name:")));

	if (!title.isEmpty())
	{
		m_ui->notesViewWidget->setCurrentIndex(NotesManager::addNote(BookmarksModel::FolderBookmark, {{BookmarksModel::TitleRole, title}}, findFolder(m_ui->notesViewWidget->currentIndex()))->index());
	}
}

void NotesContentsWidget::addSeparator()
{
	m_ui->notesViewWidget->setCurrentIndex(NotesManager::addNote(BookmarksModel::SeparatorBookmark, {}, findFolder(m_ui->notesViewWidget->currentIndex()))->index());
}

void NotesContentsWidget::removeNote()
{
	NotesManager::getModel()->trashBookmark(NotesManager::getModel()->getBookmark(m_ui->notesViewWidget->currentIndex()));
}

void NotesContentsWidget::restoreNote()
{
	NotesManager::getModel()->restoreBookmark(NotesManager::getModel()->getBookmark(m_ui->notesViewWidget->currentIndex()));
}

void NotesContentsWidget::openUrl(const QModelIndex &index)
{
	const BookmarksItem *bookmark(NotesManager::getModel()->getBookmark(index.isValid() ? index : m_ui->notesViewWidget->currentIndex()));

	if (bookmark && bookmark->getUrl().isValid())
	{
		Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), bookmark->getUrl()}}, parentWidget());
	}
}

void NotesContentsWidget::notifyPasteActionStateChanged()
{
	emit arbitraryActionsStateChanged({ActionsManager::PasteAction});
}

void NotesContentsWidget::showContextMenu(const QPoint &position)
{
	const QModelIndex index(m_ui->notesViewWidget->indexAt(position));
	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()));
	ActionExecutor::Object executor(this, this);
	QMenu menu(this);

	if (type != BookmarksModel::UrlBookmark && type != BookmarksModel::TrashBookmark)
	{
		menu.addAction(new Action(ActionsManager::PasteAction, {}, executor, &menu));
		menu.addSeparator();
	}

	if (type == BookmarksModel::TrashBookmark)
	{
		menu.addAction(ThemesManager::createIcon(QLatin1String("trash-empty")), tr("Empty Trash"), NotesManager::getModel(), SLOT(emptyTrash()))->setEnabled(NotesManager::getModel()->getTrashItem()->rowCount() > 0);
	}
	else if (type == BookmarksModel::UnknownBookmark)
	{
		menu.addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder"), this, SLOT(addFolder()));
		menu.addAction(tr("Add Note"), this, SLOT(addNote()));
		menu.addAction(tr("Add Separator"), this, SLOT(addSeparator()));
	}
	else
	{
		const bool isInTrash(index.data(BookmarksModel::IsTrashedRole).toBool());

		if (type == BookmarksModel::UrlBookmark)
		{
			menu.addAction(new Action(ActionsManager::CutAction, {}, executor, &menu));
			menu.addAction(new Action(ActionsManager::CopyAction, {}, executor, &menu));
			menu.addAction(new Action(ActionsManager::PasteAction, {}, executor, &menu));
			menu.addSeparator();
		}

		menu.addAction(ThemesManager::createIcon(QLatin1String("document-open")), tr("Open source page"), this, SLOT(openUrl()))->setEnabled(type == BookmarksModel::UrlBookmark && index.data(BookmarksModel::UrlRole).toUrl().isValid());

		if (type != BookmarksModel::RootBookmark)
		{
			menu.addAction(new Action(ActionsManager::CopyLinkToClipboardAction, {}, executor, &menu));
		}

		if (!isInTrash)
		{
			menu.addSeparator();

			QMenu *addMenu(menu.addMenu(tr("Add Note")));
			addMenu->addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…"), this, SLOT(addFolder()));
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
				menu.addAction(new Action(ActionsManager::DeleteAction, {}, executor, &menu));
			}
		}
	}

	menu.exec(m_ui->notesViewWidget->mapToGlobal(position));
}

void NotesContentsWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
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
				NotesManager::addNote(BookmarksModel::UrlBookmark, {{BookmarksModel::DescriptionRole, QGuiApplication::clipboard()->text()}}, findFolder(m_ui->notesViewWidget->currentIndex()));
			}

			break;
		case ActionsManager::DeleteAction:
			removeNote();

			break;
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
			m_ui->filterLineEditWidget->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->notesViewWidget->setFocus();

			break;
		default:
			ContentsWidget::triggerAction(identifier, parameters);

			break;
	}
}

void NotesContentsWidget::updateActions()
{
	const QModelIndex index(m_ui->notesViewWidget->getCurrentIndex());
	const QString text(index.data(BookmarksModel::DescriptionRole).toString());
	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()));
	const bool isUrlBookmark(type == BookmarksModel::UrlBookmark);

	m_ui->addressLabelWidget->setText(isUrlBookmark ? index.data(BookmarksModel::UrlRole).toString() : QString());
	m_ui->addressLabelWidget->setUrl(isUrlBookmark ? index.data(BookmarksModel::UrlRole).toUrl() : QUrl());
	m_ui->dateLabelWidget->setText(isUrlBookmark ? Utils::formatDateTime(index.data(BookmarksModel::TimeAddedRole).toDateTime()) : QString());
	m_ui->deleteButton->setEnabled(!m_ui->notesViewWidget->selectionModel()->selectedIndexes().isEmpty() && type != BookmarksModel::RootBookmark && type != BookmarksModel::TrashBookmark);

	if (!m_ui->textEditWidget->hasFocus() || m_ui->textEditWidget->toPlainText() != text)
	{
		m_ui->textEditWidget->blockSignals(true);
		m_ui->textEditWidget->setPlainText(text);
		m_ui->textEditWidget->blockSignals(false);
	}

	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::EditingCategory});
}

void NotesContentsWidget::updateText()
{
	disconnect(m_ui->notesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &NotesContentsWidget::updateActions);

	const QModelIndex index(m_ui->notesViewWidget->getCurrentIndex());

	if (index.isValid() && static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark)
	{
		m_ui->notesViewWidget->model()->setData(index, m_ui->textEditWidget->toPlainText(), BookmarksModel::DescriptionRole);
		m_ui->notesViewWidget->model()->setData(index, QDateTime::currentDateTime(), BookmarksModel::TimeModifiedRole);
	}
	else
	{
		const QString text(m_ui->textEditWidget->toPlainText());

		if (!text.isEmpty())
		{
			m_ui->notesViewWidget->setCurrentIndex(NotesManager::addNote(BookmarksModel::UrlBookmark, {{BookmarksModel::DescriptionRole, text}}, findFolder(index))->index());

			updateActions();
		}
	}

	connect(m_ui->notesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &NotesContentsWidget::updateActions);
}

BookmarksItem* NotesContentsWidget::findFolder(const QModelIndex &index)
{
	BookmarksItem *item(NotesManager::getModel()->getBookmark(index));

	if (!item || item == NotesManager::getModel()->getRootItem() || item == NotesManager::getModel()->getTrashItem())
	{
		return NotesManager::getModel()->getRootItem();
	}

	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(item->getType()));

	return ((type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark) ? item : static_cast<BookmarksItem*>(item->parent()));
}

QString NotesContentsWidget::getTitle() const
{
	return tr("Notes");
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
	return ThemesManager::createIcon(QLatin1String("notes"), false);
}

ActionsManager::ActionDefinition::State NotesContentsWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());

	switch (identifier)
	{
		case ActionsManager::CopyLinkToClipboardAction:
			state.text = QCoreApplication::translate("actions", "Copy address of source page");
			state.isEnabled = (static_cast<BookmarksModel::BookmarkType>(m_ui->notesViewWidget->currentIndex().data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark && !m_ui->notesViewWidget->currentIndex().data(BookmarksModel::UrlRole).toString().isEmpty());

			return state;
		case ActionsManager::CutAction:
		case ActionsManager::CopyAction:
			state.isEnabled = m_ui->textEditWidget->textCursor().hasSelection();

			return state;
		case ActionsManager::PasteAction:
			state.isEnabled = m_ui->textEditWidget->canPaste();

			return state;
		case ActionsManager::DeleteAction:
			state.isEnabled = m_ui->deleteButton->isEnabled();

			return state;
		default:
			break;
	}

	return ContentsWidget::getActionState(identifier, parameters);
}

bool NotesContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->notesViewWidget && event->type() == QEvent::KeyPress)
	{
		const QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent)
		{
			switch (keyEvent->key())
			{
				case Qt::Key_Enter:
				case Qt::Key_Return:
					openUrl();

					return true;
				case Qt::Key_Delete:
					removeNote();

					return true;
				default:
					break;
			}
		}
	}
	else if (object == m_ui->notesViewWidget->viewport() && event->type() == QEvent::MouseButtonRelease)
	{
		const QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent && ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton))
		{
			const BookmarksItem *bookmark(NotesManager::getModel()->getBookmark(m_ui->notesViewWidget->indexAt(mouseEvent->pos())));

			if (bookmark)
			{
				Application::triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark->getIdentifier()}, {QLatin1String("hints"), QVariant(SessionsManager::calculateOpenHints(SessionsManager::NewTabOpen, mouseEvent->button(), mouseEvent->modifiers()))}}, parentWidget());

				return true;
			}
		}
	}
	else if (object == m_ui->notesViewWidget->viewport() && event->type() == QEvent::ToolTip)
	{
		const QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));

		if (helpEvent)
		{
			const QModelIndex index(m_ui->notesViewWidget->indexAt(helpEvent->pos()));
			const BookmarksItem *bookmark(NotesManager::getModel()->getBookmark(index));

			if (bookmark)
			{
				QToolTip::showText(helpEvent->globalPos(), QFontMetrics(QToolTip::font()).elidedText(bookmark->toolTip(), Qt::ElideRight, (QApplication::desktop()->screenGeometry(m_ui->notesViewWidget).width() / 2)), m_ui->notesViewWidget, m_ui->notesViewWidget->visualRect(index));
			}

			return true;
		}
	}

	return ContentsWidget::eventFilter(object, event);
}

}
