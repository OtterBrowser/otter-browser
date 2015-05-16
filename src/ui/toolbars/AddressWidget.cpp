/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "AddressWidget.h"
#include "../BookmarkPropertiesDialog.h"
#include "../ContentsWidget.h"
#include "../ToolBarWidget.h"
#include "../Window.h"
#include "../../core/AddressCompletionModel.h"
#include "../../core/BookmarksManager.h"
#include "../../core/InputInterpreter.h"
#include "../../core/NotesManager.h"
#include "../../core/SearchesManager.h"
#include "../../core/Utils.h"

#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyleOptionFrame>

namespace Otter
{

AddressWidget::AddressWidget(Window *window, QWidget *parent) : QLineEdit(parent),
	m_window(NULL),
	m_completer(new QCompleter(AddressCompletionModel::getInstance(), this)),
	m_bookmarkLabel(NULL),
	m_feedsLabel(NULL),
	m_loadPluginsLabel(NULL),
	m_urlIconLabel(NULL),
	m_simpleMode(false)
{
	m_completer->setCaseSensitivity(Qt::CaseInsensitive);
	m_completer->setCompletionMode(QCompleter::InlineCompletion);
	m_completer->setCompletionRole(Qt::DisplayRole);
	m_completer->setFilterMode(Qt::MatchStartsWith);

	setWindow(window);
	setCompleter(m_completer);
	setMinimumWidth(100);

	ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parent);

	if (toolBar)
	{
		optionChanged(QLatin1String("AddressField/ShowBookmarkIcon"), SettingsManager::getValue(QLatin1String("AddressField/ShowBookmarkIcon")));
		optionChanged(QLatin1String("AddressField/ShowUrlIcon"), SettingsManager::getValue(QLatin1String("AddressField/ShowUrlIcon")));
		setPlaceholderText(tr("Enter address or search…"));
		setMouseTracking(true);

		connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
		connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
	}
	else
	{
		m_simpleMode = true;
	}

	connect(this, SIGNAL(textChanged(QString)), this, SLOT(setCompletion(QString)));
	connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(updateBookmark()));
}

void AddressWidget::paintEvent(QPaintEvent *event)
{
	QLineEdit::paintEvent(event);

	if (m_simpleMode)
	{
		return;
	}

	QColor badgeColor = QColor(245, 245, 245);
	QStyleOptionFrame panel;

	initStyleOption(&panel);

	panel.palette = palette();
	panel.palette.setColor(QPalette::Base, badgeColor);
	panel.state = QStyle::State_Active;

	QRect rectangle = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
	rectangle.setWidth(30);
	rectangle.moveTo(panel.lineWidth, panel.lineWidth);

	m_securityBadgeRectangle = rectangle;

	QPainter painter(this);
	painter.fillRect(rectangle, badgeColor);
	painter.setClipRect(rectangle);

	style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &painter, this);

	QPalette linePalette = palette();
	linePalette.setCurrentColorGroup(QPalette::Disabled);

	painter.setPen(QPen(linePalette.mid().color(), 1));
	painter.drawLine(rectangle.right(), rectangle.top(), rectangle.right(), rectangle.bottom());
}

void AddressWidget::resizeEvent(QResizeEvent *event)
{
	QLineEdit::resizeEvent(event);

	updateIcons();
}

void AddressWidget::focusInEvent(QFocusEvent *event)
{
	if (event->reason() == Qt::MouseFocusReason && childAt(mapFromGlobal(QCursor::pos())))
	{
		return;
	}

	QLineEdit::focusInEvent(event);

	if (!text().trimmed().isEmpty() && (event->reason() == Qt::MouseFocusReason || event->reason() == Qt::ShortcutFocusReason || (!m_simpleMode && (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason))) && SettingsManager::getValue(QLatin1String("AddressField/SelectAllOnFocus")).toBool())
	{
		QTimer::singleShot(0, this, SLOT(selectAll()));
	}
	else if (event->reason() != Qt::PopupFocusReason)
	{
		deselect();
	}
}

void AddressWidget::keyPressEvent(QKeyEvent *event)
{
	QLineEdit::keyPressEvent(event);

	if (m_window && event->key() == Qt::Key_Escape)
	{
		const QUrl url = m_window->getUrl();

		if (text().trimmed().isEmpty() || text().trimmed() != url.toString())
		{
			setText(Utils::isUrlEmpty(url) ? QString() : url.toString());

			if (!text().trimmed().isEmpty() && SettingsManager::getValue(QLatin1String("AddressField/SelectAllOnFocus")).toBool())
			{
				QTimer::singleShot(0, this, SLOT(selectAll()));
			}
		}
		else
		{
			m_window->setFocus();
		}
	}

	if (!m_simpleMode && (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return))
	{
		handleUserInput(text().trimmed(), WindowsManager::calculateOpenHints(event->modifiers(), Qt::LeftButton, CurrentTabOpen));
	}
}

void AddressWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	menu.addAction(tr("Undo"), this, SLOT(undo()), QKeySequence(QKeySequence::Undo))->setEnabled(isUndoAvailable());
	menu.addAction(tr("Redo"), this, SLOT(redo()), QKeySequence(QKeySequence::Redo))->setEnabled(isRedoAvailable());
	menu.addSeparator();
	menu.addAction(tr("Cut"), this, SLOT(cut()), QKeySequence(QKeySequence::Cut))->setEnabled(hasSelectedText());
	menu.addAction(tr("Copy"), this, SLOT(copy()), QKeySequence(QKeySequence::Copy))->setEnabled(hasSelectedText());
	menu.addAction(tr("Paste"), this, SLOT(paste()), QKeySequence(QKeySequence::Paste))->setEnabled(!QApplication::clipboard()->text().isEmpty());

	if (!m_simpleMode)
	{
		menu.addAction(ActionsManager::getAction(ActionsManager::PasteAndGoAction, this));
	}

	menu.addAction(tr("Delete"), this, SLOT(deleteText()), QKeySequence(QKeySequence::Delete))->setEnabled(hasSelectedText());
	menu.addSeparator();
	menu.addAction(tr("Copy to Note"), this, SLOT(copyToNote()))->setEnabled(!text().isEmpty());
	menu.addSeparator();
	menu.addAction(tr("Clear All"), this, SLOT(clear()))->setEnabled(!text().isEmpty());
	menu.addAction(tr("Select All"), this, SLOT(selectAll()))->setEnabled(!text().isEmpty());

	ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parentWidget());

	if (toolBar)
	{
		menu.addSeparator();
		menu.addMenu(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), QList<QAction*>(), &menu));
	}

	menu.exec(event->globalPos());
}

void AddressWidget::mouseMoveEvent(QMouseEvent *event)
{
	QLineEdit::mouseMoveEvent(event);

	if (!m_simpleMode)
	{
		if (m_securityBadgeRectangle.contains(event->pos()))
		{
			setCursor(Qt::ArrowCursor);
		}
		else
		{
			setCursor(Qt::IBeamCursor);
		}
	}
}

void AddressWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (text().isEmpty() && event->button() == Qt::MiddleButton && !QApplication::clipboard()->text().isEmpty() && SettingsManager::getValue(QLatin1String("AddressField/PasteAndGoOnMiddleClick")).toBool())
	{
		handleUserInput(QApplication::clipboard()->text().trimmed());

		event->accept();
	}
	else
	{
		QLineEdit::mouseReleaseEvent(event);
	}
}

void AddressWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		selectAll();

		event->accept();
	}
	else
	{
		QLineEdit::mouseDoubleClickEvent(event);
	}
}

void AddressWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("AddressField/ShowBookmarkIcon"))
	{
		if (value.toBool() && !m_bookmarkLabel)
		{
			m_bookmarkLabel = new QLabel(this);
			m_bookmarkLabel->setObjectName(QLatin1String("Bookmark"));
			m_bookmarkLabel->setAutoFillBackground(false);
			m_bookmarkLabel->setFixedSize(16, 16);
			m_bookmarkLabel->setPixmap(Utils::getIcon(QLatin1String("bookmarks")).pixmap(m_bookmarkLabel->size(), QIcon::Disabled));
			m_bookmarkLabel->setCursor(Qt::ArrowCursor);
			m_bookmarkLabel->setFocusPolicy(Qt::NoFocus);
			m_bookmarkLabel->installEventFilter(this);

			updateIcons();
		}
		else if (!value.toBool() && m_bookmarkLabel)
		{
			m_bookmarkLabel->deleteLater();
			m_bookmarkLabel = NULL;

			updateIcons();
		}
	}
	else if (option == QLatin1String("AddressField/ShowUrlIcon"))
	{
		if (value.toBool() && !m_urlIconLabel)
		{
			m_urlIconLabel = new QLabel(this);
			m_urlIconLabel->setObjectName(QLatin1String("Url"));
			m_urlIconLabel->setAutoFillBackground(false);
			m_urlIconLabel->setFixedSize(16, 16);
			m_urlIconLabel->setPixmap((m_window ? m_window->getIcon() : Utils::getIcon(QLatin1String("tab"))).pixmap(m_urlIconLabel->size()));
			m_urlIconLabel->setFocusPolicy(Qt::NoFocus);
			m_urlIconLabel->installEventFilter(this);

			if (m_window)
			{
				connect(m_window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
			}
		}
		else
		{
			if (!value.toBool() && m_urlIconLabel)
			{
				m_urlIconLabel->deleteLater();
				m_urlIconLabel = NULL;

				updateIcons();
			}

			if (m_window)
			{
				disconnect(m_window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
			}
		}

		updateIcons();
	}
	else if (option == QLatin1String("AddressField/ShowLoadPluginsIcon") && m_window)
	{
		if (value.toBool())
		{
			connect(m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction), SIGNAL(changed()), this, SLOT(updateLoadPlugins()));
		}
		else
		{
			disconnect(m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction), SIGNAL(changed()), this, SLOT(updateLoadPlugins()));
		}

		updateLoadPlugins();
	}
}

void AddressWidget::openFeed(QAction *action)
{
	if (action && m_window)
	{
		m_window->setUrl(action->data().toUrl());
	}
}

void AddressWidget::copyToNote()
{
	const QString note(hasSelectedText() ? selectedText() : text());

	if (!note.isEmpty())
	{
		NotesManager::addNote(BookmarksModel::UrlBookmark, getUrl(), note);
	}
}

void AddressWidget::deleteText()
{
	del();
}

void AddressWidget::removeIcon()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		SettingsManager::setValue(QStringLiteral("AddressField/Show%1Icon").arg(action->data().toString()), false);
	}
}

void AddressWidget::handleUserInput(const QString &text, OpenHints hints)
{
	if (!text.isEmpty())
	{
		InputInterpreter *interpreter = new InputInterpreter(this);

		connect(interpreter, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)), this, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)));
		connect(interpreter, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SIGNAL(requestedOpenUrl(QUrl,OpenHints)));
		connect(interpreter, SIGNAL(requestedSearch(QString,QString,OpenHints)), this, SIGNAL(requestedSearch(QString,QString,OpenHints)));

		interpreter->interpret(text, hints);
	}
}

void AddressWidget::updateBookmark()
{
	if (!m_bookmarkLabel)
	{
		return;
	}

	const QUrl url = getUrl();

	if (Utils::isUrlEmpty(url) || url.scheme() == QLatin1String("about"))
	{
		m_bookmarkLabel->setEnabled(false);
		m_bookmarkLabel->setPixmap(Utils::getIcon(QLatin1String("bookmarks")).pixmap(m_bookmarkLabel->size(), QIcon::Disabled));
		m_bookmarkLabel->setToolTip(QString());

		return;
	}

	const bool hasBookmark = BookmarksManager::hasBookmark(url);

	m_bookmarkLabel->setEnabled(true);
	m_bookmarkLabel->setPixmap(Utils::getIcon(QLatin1String("bookmarks")).pixmap(m_bookmarkLabel->size(), (hasBookmark ? QIcon::Active : QIcon::Disabled)));
	m_bookmarkLabel->setToolTip(hasBookmark ? tr("Remove Bookmark") : tr("Add Bookmark"));
}

void AddressWidget::updateFeeds()
{
	const QList<FeedUrl> feeds = ((m_window && m_window->getLoadingState() == LoadedState) ? m_window->getContentsWidget()->getFeeds() : QList<FeedUrl>());

	if (!feeds.isEmpty() && !m_feedsLabel)
	{
		m_feedsLabel = new QLabel(this);
		m_feedsLabel->show();
		m_feedsLabel->setObjectName(QLatin1String("Feeds"));
		m_feedsLabel->setAutoFillBackground(false);
		m_feedsLabel->setFixedSize(16, 16);
		m_feedsLabel->setPixmap(Utils::getIcon(QLatin1String("application-rss+xml")).pixmap(m_feedsLabel->size()));
		m_feedsLabel->setCursor(Qt::ArrowCursor);
		m_feedsLabel->setToolTip(tr("Feed List"));
		m_feedsLabel->setFocusPolicy(Qt::NoFocus);
		m_feedsLabel->installEventFilter(this);

		updateIcons();
	}
	else if (feeds.isEmpty() && m_feedsLabel)
	{
		m_feedsLabel->deleteLater();
		m_feedsLabel = NULL;

		updateIcons();
	}
}

void AddressWidget::updateLoadPlugins()
{
	const bool canLoadPlugins = (SettingsManager::getValue(QLatin1String("AddressField/ShowLoadPluginsIcon")).toBool() && m_window && m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction) && m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction)->isEnabled());

	if (canLoadPlugins && !m_loadPluginsLabel)
	{
		m_loadPluginsLabel = new QLabel(this);
		m_loadPluginsLabel->show();
		m_loadPluginsLabel->setObjectName(QLatin1String("LoadPlugins"));
		m_loadPluginsLabel->setAutoFillBackground(false);
		m_loadPluginsLabel->setFixedSize(16, 16);
		m_loadPluginsLabel->setPixmap(Utils::getIcon(QLatin1String("preferences-plugin")).pixmap(m_loadPluginsLabel->size()));
		m_loadPluginsLabel->setCursor(Qt::ArrowCursor);
		m_loadPluginsLabel->setToolTip(tr("Click to load all plugins on the page"));
		m_loadPluginsLabel->setFocusPolicy(Qt::NoFocus);
		m_loadPluginsLabel->installEventFilter(this);

		updateIcons();
	}
	else if (!canLoadPlugins && m_loadPluginsLabel)
	{
		m_loadPluginsLabel->deleteLater();
		m_loadPluginsLabel = NULL;

		updateIcons();
	}
}

void AddressWidget::updateIcons()
{
	QMargins margins;

	if (!m_simpleMode)
	{
		margins.setLeft(30);
	}

	if (m_urlIconLabel)
	{
		m_urlIconLabel->move(36, ((height() - m_urlIconLabel->height()) / 2));

		margins.setLeft(margins.left() + 22);
	}

	if (m_bookmarkLabel)
	{
		margins.setRight(margins.right() + 22);

		m_bookmarkLabel->move((width() - margins.right()), ((height() - m_bookmarkLabel->height()) / 2));
	}

	if (m_feedsLabel)
	{
		margins.setRight(margins.right() + 22);

		m_feedsLabel->move((width() - margins.right()), ((height() - m_feedsLabel->height()) / 2));
	}

	if (m_loadPluginsLabel)
	{
		margins.setRight(margins.right() + 22);

		m_loadPluginsLabel->move((width() - margins.right()), ((height() - m_loadPluginsLabel->height()) / 2));
	}

	setTextMargins(margins);
}

void AddressWidget::setCompletion(const QString &text)
{
	m_completer->setCompletionPrefix(text);
}

void AddressWidget::setIcon(const QIcon &icon)
{
	if (m_urlIconLabel)
	{
		m_urlIconLabel->setPixmap((icon.isNull() ? Utils::getIcon(QLatin1String("tab")) : icon).pixmap(m_urlIconLabel->size()));
	}
}

void AddressWidget::setUrl(const QUrl &url)
{
	updateBookmark();
	updateFeeds();

	if (!hasFocus() && url.scheme() != QLatin1String("javascript"))
	{
		const QString text(Utils::isUrlEmpty(url) ? QString() : url.toString());

		setText(text);
		setToolTip(text);
		setCursorPosition(0);
	}
}

void AddressWidget::setWindow(Window *window)
{
	if (m_window)
	{
		m_window->detachAddressWidget(this);

		disconnect(this, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), m_window, SLOT(handleOpenUrlRequest(QUrl,OpenHints)));
		disconnect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)), m_window, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)));
		disconnect(this, SIGNAL(requestedSearch(QString,QString,OpenHints)), m_window, SLOT(handleSearchRequest(QString,QString,OpenHints)));
		disconnect(m_window, SIGNAL(loadingStateChanged(WindowLoadingState)), this, SLOT(updateFeeds()));
		disconnect(m_window, SIGNAL(urlChanged(QUrl)), this, SLOT(setUrl(QUrl)));
		disconnect(m_window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
		disconnect(m_window, SIGNAL(aboutToClose()), this, SLOT(setWindow()));

		if (m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction))
		{
			disconnect(m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction), SIGNAL(changed()), this, SLOT(updateLoadPlugins()));
		}
	}

	m_window = window;

	if (window)
	{
		window->attachAddressWidget(this);

		if (m_urlIconLabel)
		{
			connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
		}

		connect(this, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), window, SLOT(handleOpenUrlRequest(QUrl,OpenHints)));
		connect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)), window, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)));
		connect(this, SIGNAL(requestedSearch(QString,QString,OpenHints)), window, SLOT(handleSearchRequest(QString,QString,OpenHints)));
		connect(window, SIGNAL(loadingStateChanged(WindowLoadingState)), this, SLOT(updateFeeds()));
		connect(window, SIGNAL(urlChanged(QUrl)), this, SLOT(setUrl(QUrl)));

		ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parentWidget());

		if (!toolBar || toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
		{
			connect(window, SIGNAL(aboutToClose()), this, SLOT(setWindow()));
		}

		if (window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction))
		{
			connect(window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction), SIGNAL(changed()), this, SLOT(updateLoadPlugins()));
		}
	}

	setIcon(window ? window->getIcon() : QIcon());
	setUrl(window ? window->getUrl() : QUrl());
	updateLoadPlugins();
}

QUrl AddressWidget::getUrl() const
{
	return (m_window ? m_window->getUrl() : QUrl(QLatin1String("about:blank")));
}

bool AddressWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_bookmarkLabel && m_bookmarkLabel && m_window && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
		{
			if (m_bookmarkLabel->isEnabled())
			{
				const QUrl url = getUrl();

				if (BookmarksManager::hasBookmark(url))
				{
					BookmarksManager::removeBookmark(url);
				}
				else
				{
					BookmarksItem *bookmark = BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, url.adjusted(QUrl::RemovePassword), m_window->getTitle());
					BookmarkPropertiesDialog dialog(bookmark, BookmarkPropertiesDialog::AddBookmarkMode, this);

					if (dialog.exec() == QDialog::Rejected)
					{
						delete bookmark;
					}
				}

				updateBookmark();
			}

			event->accept();

			return true;
		}
	}

	if (object == m_feedsLabel && m_feedsLabel && m_window && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
		{
			const QList<FeedUrl> feeds = ((m_window && m_window->getLoadingState() == LoadedState) ? m_window->getContentsWidget()->getFeeds() : QList<FeedUrl>());

			if (feeds.count() == 1 && m_window)
			{
				m_window->setUrl(feeds.at(0).url);
			}
			else if (feeds.count() > 1)
			{
				QMenu menu;

				for (int i = 0; i < feeds.count(); ++i)
				{
					menu.addAction(feeds.at(i).title.isEmpty() ? tr("(Untitled)") : feeds.at(i).title)->setData(feeds.at(i).url);
				}

				connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(openFeed(QAction*)));

				menu.exec(mouseEvent->globalPos());
			}

			event->accept();

			return true;
		}
	}

	if (object == m_loadPluginsLabel && m_loadPluginsLabel && m_window && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
		{
			m_window->getContentsWidget()->triggerAction(ActionsManager::LoadPluginsAction);

			event->accept();

			return true;
		}
	}

	if (event->type() == QEvent::ContextMenu)
	{
		QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent*>(event);

		if (contextMenuEvent)
		{
			QMenu menu(this);
			QAction *action = menu.addAction(tr("Remove This Icon"), this, SLOT(removeIcon()));
			action->setData(object->objectName());

			menu.exec(contextMenuEvent->globalPos());

			event->accept();

			return true;
		}
	}

	return QObject::eventFilter(object, event);
}

}
