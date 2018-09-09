/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "ToolBarWidget.h"
#include "Action.h"
#include "ContentsWidget.h"
#include "MainWindow.h"
#include "Menu.h"
#include "SidebarWidget.h"
#include "Style.h"
#include "TabBarWidget.h"
#include "WidgetFactory.h"
#include "Window.h"
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/GesturesManager.h"
#include "../core/ThemesManager.h"
#include "../modules/widgets/bookmark/BookmarkWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QDrag>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QLayout>
#include <QtWidgets/QStyleOption>

namespace Otter
{

ToolBarWidget::ToolBarWidget(int identifier, Window *window, QWidget *parent) : QToolBar(parent),
	m_mainWindow(MainWindow::findMainWindow(parent)),
	m_window(window),
	m_sidebarWidget(nullptr),
	m_bookmark(nullptr),
	m_dropBookmark(nullptr),
	m_toggleButton(nullptr),
	m_area(Qt::TopToolBarArea),
	m_reloadTimer(0),
	m_identifier(identifier),
	m_dropIndex(-1),
	m_isCollapsed(false),
	m_isInitialized(false)
{
	setAcceptDrops(true);
	setAllowedAreas(Qt::NoToolBarArea);
	setFloatable(false);

	const ToolBarsManager::ToolBarDefinition definition(getDefinition());

	m_state = ToolBarState(identifier, definition);
	m_area = definition.location;

	if (definition.isValid() && identifier != ToolBarsManager::MenuBar)
	{
		if (identifier == ToolBarsManager::TabBar)
		{
			m_isInitialized = true;
		}
		else
		{
			for (int i = 0; i < definition.entries.count(); ++i)
			{
				if (definition.entries.at(i).action == QLatin1String("AddressWidget") || definition.entries.at(i).action == QLatin1String("SearchWidget"))
				{
					m_isInitialized = true;

					setDefinition(definition);

					connect(ToolBarsManager::getInstance(), &ToolBarsManager::toolBarModified, this, &ToolBarWidget::handleToolBarModified);

					break;
				}
			}
		}

		setToolBarLocked(ToolBarsManager::areToolBarsLocked());

		connect(ToolBarsManager::getInstance(), &ToolBarsManager::toolBarRemoved, this, &ToolBarWidget::handleToolBarRemoved);
		connect(ToolBarsManager::getInstance(), &ToolBarsManager::toolBarsLockedChanged, this, &ToolBarWidget::setToolBarLocked);
	}

	if (m_mainWindow)
	{
		if (definition.isGlobal())
		{
			connect(m_mainWindow, &MainWindow::currentWindowChanged, this, &ToolBarWidget::notifyWindowChanged);
		}

		connect(m_mainWindow, &MainWindow::fullScreenStateChanged, this, &ToolBarWidget::handleFullScreenStateChanged);
	}
}

void ToolBarWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_reloadTimer)
	{
		killTimer(m_reloadTimer);

		m_reloadTimer = 0;

		loadBookmarks();
	}
}

void ToolBarWidget::changeEvent(QEvent *event)
{
	QToolBar::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			if (m_toggleButton)
			{
				const QKeySequence shortcut(ActionsManager::getActionShortcut(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::getToolBarName(m_identifier)}}));

				m_toggleButton->setToolTip(tr("Toggle Visibility") + (shortcut.isEmpty() ? QString() : QLatin1String(" (") + shortcut.toString(QKeySequence::NativeText) + QLatin1Char(')')));
			}

			break;
		case QEvent::StyleChange:
			{
				const int iconSize(getIconSize());

				setIconSize(QSize(iconSize, iconSize));

				emit iconSizeChanged(iconSize);
			}

			break;
		default:
			break;
	}
}

void ToolBarWidget::paintEvent(QPaintEvent *event)
{
	if (m_identifier != ToolBarsManager::StatusBar)
	{
		QToolBar::paintEvent(event);
	}

	if (getDefinition().type == ToolBarsManager::BookmarksBarType && m_dropIndex >= 0)
	{
		const QWidget *widget(widgetForAction(actions().value(m_dropIndex)));
		const int spacing(style()->pixelMetric(QStyle::PM_ToolBarItemSpacing));
		int position(-1);

		if (widget)
		{
			switch (m_area)
			{
				case Qt::LeftToolBarArea:
				case Qt::RightToolBarArea:
					position = (widget->geometry().top() - spacing);

					break;
				default:
					if (isLeftToRight())
					{
						position = (widget->geometry().left() - spacing);
					}
					else
					{
						position = (widget->geometry().right() + spacing);
					}

					break;
			}
		}
		else if (m_dropIndex > 0)
		{
			switch (m_area)
			{
				case Qt::LeftToolBarArea:
				case Qt::RightToolBarArea:
					position = (childrenRect().bottom() + spacing);

					break;
				default:
					position = (isLeftToRight() ? (childrenRect().right() + spacing) : (childrenRect().left() - spacing));

					break;
			}
		}

		if (position >= 0)
		{
			QPainter painter(this);

			switch (m_area)
			{
				case Qt::LeftToolBarArea:
				case Qt::RightToolBarArea:
					Application::getStyle()->drawDropZone(QLine(0, position, width(), position), &painter);

					break;
				default:
					Application::getStyle()->drawDropZone(QLine(position, 0, position, height()), &painter);

					break;
			}
		}
	}
}

void ToolBarWidget::showEvent(QShowEvent *event)
{
	if (!m_isInitialized)
	{
		m_isInitialized = true;

		reload();

		connect(ToolBarsManager::getInstance(), &ToolBarsManager::toolBarModified, this, &ToolBarWidget::handleToolBarModified);
	}

	QToolBar::showEvent(event);
}

void ToolBarWidget::resizeEvent(QResizeEvent *event)
{
	QToolBar::resizeEvent(event);

	if (m_toggleButton && m_toggleButton->isVisible())
	{
		updateToggleGeometry();
	}
}

void ToolBarWidget::enterEvent(QEvent *event)
{
	QToolBar::enterEvent(event);

	if (m_toggleButton && !m_isCollapsed)
	{
		updateToggleGeometry();
	}
}

void ToolBarWidget::leaveEvent(QEvent *event)
{
	QToolBar::leaveEvent(event);

	if (m_toggleButton && !m_isCollapsed && getDefinition().type != ToolBarsManager::SideBarType)
	{
		m_toggleButton->hide();
	}
}

void ToolBarWidget::mousePressEvent(QMouseEvent *event)
{
	QWidget::mousePressEvent(event);

	if (event->button() == Qt::LeftButton && isDragHandle(event->pos()))
	{
		m_dragStartPosition = event->pos();
	}
	else
	{
		m_dragStartPosition = {};
	}
}

void ToolBarWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (!m_mainWindow || !event->buttons().testFlag(Qt::LeftButton) || m_dragStartPosition.isNull() || !isMovable() || (event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
	{
		return;
	}

	m_dragStartPosition = {};

	m_mainWindow->beginToolBarDragging(getDefinition().type == ToolBarsManager::SideBarType);

	QMimeData *mimeData(new QMimeData());
	mimeData->setProperty("x-toolbar-identifier", m_identifier);

	QDrag *drag(new QDrag(this));
	drag->setMimeData(mimeData);
	drag->exec(Qt::MoveAction);

	m_mainWindow->endToolBarDragging();
}

void ToolBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QWidget::mouseReleaseEvent(event);

	if (m_mainWindow)
	{
		m_mainWindow->endToolBarDragging();
	}
}

void ToolBarWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (canDrop(event))
	{
		event->accept();

		updateDropIndex(event->pos());
	}
	else
	{
		event->ignore();
	}
}

void ToolBarWidget::dragMoveEvent(QDragMoveEvent *event)
{
	if (canDrop(event))
	{
		event->accept();

		updateDropIndex(event->pos());
	}
	else
	{
		event->ignore();
	}
}

void ToolBarWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	QWidget::dragLeaveEvent(event);

	updateDropIndex({});
}

void ToolBarWidget::dropEvent(QDropEvent *event)
{
	if (canDrop(event))
	{
		event->accept();

		const QVector<QUrl> urls(Utils::extractUrls(event->mimeData()));

		for (int i = 0; i < urls.count(); ++i)
		{
			const QString title(event->mimeData()->property("x-url-title").toString());
			QMap<int, QVariant> metaData({{BookmarksModel::UrlRole, urls.at(i)}});

			if (!title.isEmpty())
			{
				metaData[BookmarksModel::TitleRole] = title;
			}

			if (m_dropBookmark)
			{
				BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, metaData, m_dropBookmark);
			}
			else
			{
				BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, metaData, m_bookmark, (m_dropIndex + i));
			}
		}
	}
	else
	{
		event->ignore();
	}

	updateDropIndex({});
}

void ToolBarWidget::clearEntries()
{
	if (getDefinition().type == ToolBarsManager::SideBarType && m_sidebarWidget)
	{
		m_sidebarWidget->reload();
	}
	else
	{
		clear();
	}
}

void ToolBarWidget::populateEntries()
{
	const ToolBarsManager::ToolBarDefinition definition(getDefinition());
	const bool isHorizontal(m_area != Qt::LeftToolBarArea && m_area != Qt::RightToolBarArea);

	m_addressFields.clear();
	m_searchFields.clear();

	switch (definition.type)
	{
		case ToolBarsManager::BookmarksBarType:
			m_bookmark = BookmarksManager::getBookmark(definition.bookmarksPath);

			loadBookmarks();

			connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkAdded, this, &ToolBarWidget::handleBookmarkModified);
			connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkModified, this, &ToolBarWidget::handleBookmarkModified);
			connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkRestored, this, &ToolBarWidget::handleBookmarkModified);
			connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkMoved, this, &ToolBarWidget::handleBookmarkMoved);
			connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkTrashed, this, &ToolBarWidget::handleBookmarkMoved);
			connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkRemoved, this, &ToolBarWidget::handleBookmarkRemoved);

			break;
		case ToolBarsManager::SideBarType:
			if (!m_sidebarWidget)
			{
				m_sidebarWidget = new SidebarWidget(this);

				addWidget(m_sidebarWidget);
			}

			if (m_toggleButton)
			{
				updateToggleGeometry();
			}

			break;
		default:
			m_addressFields.clear();
			m_searchFields.clear();

			for (int i = 0; i < definition.entries.count(); ++i)
			{
				if (definition.entries.at(i).action == QLatin1String("separator"))
				{
					addSeparator();
				}
				else if (definition.entries.at(i).action != QLatin1String("TabBarWidget"))
				{
					QWidget *widget(WidgetFactory::createToolBarItem(definition.entries.at(i), m_window, this));

					if (widget)
					{
						addWidget(widget);

						if (definition.entries.at(i).action == QLatin1String("AddressWidget"))
						{
							m_addressFields.append(widget);
						}
						else if (definition.entries.at(i).action == QLatin1String("SearchWidget"))
						{
							m_searchFields.append(widget);
						}

						layout()->setAlignment(widget, (isHorizontal ? Qt::AlignVCenter : Qt::AlignHCenter));
					}
				}
			}

			break;
	}
}

void ToolBarWidget::updateDropIndex(const QPoint &position)
{
	if (!m_bookmark)
	{
		return;
	}

	int dropIndex(-1);

	m_dropBookmark = nullptr;

	if (!position.isNull())
	{
		QAction *action(actionAt(position));
		const bool isHorizontal(m_area != Qt::LeftToolBarArea && m_area != Qt::RightToolBarArea);

		if (!action)
		{
			const QPoint center(contentsRect().center());
			const int spacing(style()->pixelMetric(QStyle::PM_ToolBarItemSpacing) * 2);

			if (isHorizontal)
			{
				const QPoint adjustedPosition(position.x(), center.y());

				action = (actionAt(adjustedPosition + QPoint(spacing, 0)));

				if (!action)
				{
					action = (actionAt(adjustedPosition - QPoint(spacing, 0)));
				}
			}
			else
			{
				const QPoint adjustedPosition(center.x(), position.y());

				action = (actionAt(adjustedPosition + QPoint(0, spacing)));

				if (!action)
				{
					action = (actionAt(adjustedPosition - QPoint(0, spacing)));
				}
			}
		}

		const QWidget *widget(widgetForAction(action));

		dropIndex = actions().indexOf(action);

		if (dropIndex >= 0 && dropIndex < m_bookmark->rowCount())
		{
			BookmarksModel::Bookmark *dropBookmark(BookmarksManager::getModel()->getBookmark(m_bookmark->index().child(dropIndex, 0)));

			if (dropBookmark && dropBookmark->getType() == BookmarksModel::FolderBookmark)
			{
				bool canNest(false);

				if (widget)
				{
					const int margin((isHorizontal ? widget->geometry().width() : widget->geometry().height()) / 3);

					if (isHorizontal)
					{
						canNest = (position.x() >= (widget->geometry().left() + margin) && position.x() <= (widget->geometry().right() - margin));
					}
					else
					{
						canNest = (position.y() >= (widget->geometry().top() + margin) && position.y() <= (widget->geometry().bottom() - margin));
					}
				}

				if (canNest)
				{
					m_dropBookmark = dropBookmark;

					dropIndex = -1;
				}
			}
		}

		if (!m_dropBookmark)
		{
			if (widget && dropIndex >= 0)
			{
				if (isHorizontal)
				{
					if (isLeftToRight() && position.x() >= widget->geometry().center().x())
					{
						++dropIndex;
					}
					else if (!isLeftToRight() && position.x() < widget->geometry().center().x())
					{
						++dropIndex;
					}
				}
				else if (!isHorizontal && position.y() >= widget->geometry().center().y())
				{
					++dropIndex;
				}
			}

			if (dropIndex < 0)
			{
				const QPoint center(contentsRect().center());

				if (isHorizontal)
				{
					if (isLeftToRight())
					{
						dropIndex = ((position.x() < center.x()) ? 0 : actions().count());
					}
					else
					{
						dropIndex = ((position.x() < center.x()) ? actions().count() : 0);
					}
				}
				else
				{
					dropIndex = ((position.y() < center.y()) ? 0 : actions().count());
				}
			}
		}
	}

	if (dropIndex != m_dropIndex)
	{
		m_dropIndex = dropIndex;

		update();
	}
}

void ToolBarWidget::contextMenuEvent(QContextMenuEvent *event)
{
	if (m_identifier < 0)
	{
		event->ignore();

		return;
	}

	if (event->reason() == QContextMenuEvent::Mouse)
	{
		event->accept();

		return;
	}

	QMenu *menu(createCustomizationMenu(m_identifier));
	menu->exec(event->globalPos());
	menu->deleteLater();
}

void ToolBarWidget::reload()
{
	setDefinition(getDefinition());
}

void ToolBarWidget::resetGeometry()
{
	for (int i = 0; i < actions().count(); ++i)
	{
		QWidget *widget(widgetForAction(actions().at(i)));

		if (widget)
		{
			widget->setMaximumSize(0, 0);
		}
	}

	setMaximumSize(0, 0);

	for (int i = 0; i < actions().count(); ++i)
	{
		QWidget *widget(widgetForAction(actions().at(i)));

		if (widget)
		{
			widget->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
		}
	}

	setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

void ToolBarWidget::scheduleBookmarksReload()
{
	if (m_reloadTimer != 0)
	{
		killTimer(m_reloadTimer);
	}

	m_reloadTimer = startTimer(100);
}

void ToolBarWidget::loadBookmarks()
{
	clear();

	if (!m_bookmark)
	{
		return;
	}

	for (int i = 0; i < m_bookmark->rowCount(); ++i)
	{
		BookmarksModel::Bookmark *bookmark(static_cast<BookmarksModel::Bookmark*>(m_bookmark->child(i)));

		if (bookmark)
		{
			if (bookmark->getType() == BookmarksModel::SeparatorBookmark)
			{
				addSeparator();
			}
			else
			{
				addWidget(new BookmarkWidget(bookmark, ToolBarsManager::ToolBarDefinition::Entry(), this));
			}
		}
	}
}

void ToolBarWidget::toggleVisibility()
{
	const ToolBarsManager::ToolBarsMode mode((m_mainWindow ? m_mainWindow->windowState().testFlag(Qt::WindowFullScreen) : false) ? ToolBarsManager::FullScreenMode : ToolBarsManager::NormalMode);
	ToolBarState state(m_state);
	state.setVisibility(mode, (calculateShouldBeVisible(getDefinition(), m_state, mode) ? ToolBarState::AlwaysHiddenToolBar : ToolBarState::AlwaysVisibleToolBar));

	setState(state);
}

void ToolBarWidget::notifyWindowChanged(quint64 identifier)
{
	m_window = m_mainWindow->getWindowByIdentifier(identifier);

	emit windowChanged(m_window);
}

void ToolBarWidget::handleToolBarModified(int identifier)
{
	if (identifier == m_identifier)
	{
		reload();

		emit toolBarModified();
	}
}

void ToolBarWidget::handleToolBarRemoved(int identifier)
{
	if (identifier == m_identifier)
	{
		deleteLater();
	}
}

void ToolBarWidget::handleBookmarkModified(BookmarksModel::Bookmark *bookmark)
{
	if (bookmark == m_bookmark || m_bookmark->isAncestorOf(bookmark))
	{
		scheduleBookmarksReload();
	}
}

void ToolBarWidget::handleBookmarkMoved(BookmarksModel::Bookmark *bookmark, BookmarksModel::Bookmark *previousParent)
{
	if (bookmark == m_bookmark || previousParent == m_bookmark || m_bookmark->isAncestorOf(bookmark) || m_bookmark->isAncestorOf(previousParent))
	{
		scheduleBookmarksReload();
	}
}

void ToolBarWidget::handleBookmarkRemoved(BookmarksModel::Bookmark *bookmark, BookmarksModel::Bookmark *previousParent)
{
	if (bookmark == m_bookmark)
	{
		m_bookmark = nullptr;

		loadBookmarks();
	}
	else if (previousParent == m_bookmark || m_bookmark->isAncestorOf(previousParent))
	{
		loadBookmarks();
	}
}

void ToolBarWidget::handleFullScreenStateChanged(bool isFullScreen)
{
	if (getDefinition().hasToggle)
	{
		reload();
	}

	setVisible(shouldBeVisible(isFullScreen ? ToolBarsManager::FullScreenMode : ToolBarsManager::NormalMode));
}

void ToolBarWidget::updateToggleGeometry()
{
	if (!m_toggleButton)
	{
		return;
	}

	if (m_isCollapsed || underMouse())
	{
		m_toggleButton->show();
		m_toggleButton->raise();
	}

	const bool isHorizontal(orientation() == Qt::Horizontal);

	m_toggleButton->setMaximumSize((isHorizontal ? QWIDGETSIZE_MAX : 6), (isHorizontal ? 6 : QWIDGETSIZE_MAX));
	m_toggleButton->resize((isHorizontal ? width() : 6), (isHorizontal ? 6 : height()));

	if (m_isCollapsed)
	{
		setMaximumSize((isHorizontal ? QWIDGETSIZE_MAX : 8), (isHorizontal ? 8 : QWIDGETSIZE_MAX));
	}
	else
	{
		setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	}

	switch (m_area)
	{
		case Qt::BottomToolBarArea:
			m_toggleButton->move(0, (height() - 6));

			break;
		case Qt::LeftToolBarArea:
			m_toggleButton->move(0, 0);

			break;
		case Qt::RightToolBarArea:
			m_toggleButton->move((width() - 6), 0);

			break;
		default:
			m_toggleButton->move(0, 0);

			break;
	}
}

void ToolBarWidget::setAddressFields(QVector<QPointer<QWidget> > addressFields)
{
	m_addressFields = addressFields;
}

void ToolBarWidget::setSearchFields(QVector<QPointer<QWidget> > searchFields)
{
	m_searchFields = searchFields;
}

void ToolBarWidget::setArea(Qt::ToolBarArea area)
{
	if (area != m_area)
	{
		m_area = area;

		emit areaChanged(m_area);
	}
}

void ToolBarWidget::setDefinition(const ToolBarsManager::ToolBarDefinition &definition)
{
	const ToolBarsManager::ToolBarsMode mode((m_mainWindow ? m_mainWindow->windowState().testFlag(Qt::WindowFullScreen) : false) ? ToolBarsManager::FullScreenMode : ToolBarsManager::NormalMode);

	m_isCollapsed = (definition.hasToggle && !calculateShouldBeVisible(definition, m_state, mode));

	setVisible(shouldBeVisible(mode));
	setOrientation((m_area != Qt::LeftToolBarArea && m_area != Qt::RightToolBarArea) ? Qt::Horizontal : Qt::Vertical);
	clearEntries();

	if (definition.hasToggle)
	{
		if (!m_toggleButton)
		{
			const QKeySequence shortcut(ActionsManager::getActionShortcut(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::getToolBarName(m_identifier)}}));

			m_toggleButton = new QPushButton(this);
			m_toggleButton->setToolTip(tr("Toggle Visibility") + (shortcut.isEmpty() ? QString() : QLatin1String(" (") + shortcut.toString(QKeySequence::NativeText) + QLatin1Char(')')));
			m_toggleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

			connect(m_toggleButton, &QPushButton::clicked, this, &ToolBarWidget::toggleVisibility);
		}

		updateToggleGeometry();

		if (m_isCollapsed)
		{
			return;
		}
	}
	else if (m_toggleButton)
	{
		m_toggleButton->deleteLater();
		m_toggleButton = nullptr;
	}

	const int iconSize(getIconSize());

	setToolButtonStyle(definition.buttonStyle);
	setIconSize(QSize(iconSize, iconSize));

	emit buttonStyleChanged(definition.buttonStyle);
	emit iconSizeChanged(iconSize);

	populateEntries();
}

void ToolBarWidget::setState(const ToolBarState &state)
{
	m_state = state;

	handleFullScreenStateChanged(m_mainWindow ? m_mainWindow->windowState().testFlag(Qt::WindowFullScreen) : false);
}

void ToolBarWidget::setToolBarLocked(bool locked)
{
	setMovable(!locked && m_identifier != ToolBarsManager::MenuBar && m_identifier != ToolBarsManager::ProgressBar);
}

MainWindow* ToolBarWidget::getMainWindow() const
{
	return m_mainWindow;
}

Window* ToolBarWidget::getWindow() const
{
	return m_window;
}

QMenu* ToolBarWidget::createCustomizationMenu(int identifier, QVector<QAction*> actions, QWidget *parent)
{
	const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(identifier));
	QMenu *menu(new QMenu(parent));
	menu->setTitle(tr("Customize"));

	QMenu *toolBarMenu(menu->addMenu(definition.getTitle().isEmpty() ? tr("(Untitled)") : definition.getTitle()));
	toolBarMenu->addAction(tr("Configure…"), menu, [=]()
	{
		ToolBarsManager::configureToolBar(identifier);
	});
	toolBarMenu->addAction(tr("Reset to Defaults…"), menu, [=]()
	{
		ToolBarsManager::resetToolBar(identifier);
	})->setEnabled(definition.canReset);

	if (!actions.isEmpty())
	{
		toolBarMenu->addSeparator();
		toolBarMenu->addActions(actions.toList());

		for (int i = 0; i < actions.count(); ++i)
		{
			actions.at(i)->setParent(toolBarMenu);
		}
	}

	toolBarMenu->addSeparator();
	toolBarMenu->addAction(ThemesManager::createIcon(QLatin1String("list-remove")), tr("Remove…"), menu, [=]()
	{
		ToolBarsManager::removeToolBar(identifier);
	})->setEnabled(!definition.canReset);

	menu->addMenu(new Menu(Menu::ToolBarsMenu, menu));

	return menu;
}

QString ToolBarWidget::getTitle() const
{
	return getDefinition().getTitle();
}

ToolBarsManager::ToolBarDefinition ToolBarWidget::getDefinition() const
{
	return ToolBarsManager::getToolBarDefinition(m_identifier);
}

ToolBarState ToolBarWidget::getState() const
{
	return m_state;
}

QVector<QPointer<QWidget> > ToolBarWidget::getAddressFields() const
{
	return m_addressFields;
}

QVector<QPointer<QWidget> > ToolBarWidget::getSearchFields() const
{
	return m_searchFields;
}

Qt::ToolBarArea ToolBarWidget::getArea() const
{
	return m_area;
}

Qt::ToolButtonStyle ToolBarWidget::getButtonStyle() const
{
	return getDefinition().buttonStyle;
}

int ToolBarWidget::getIdentifier() const
{
	return m_identifier;
}

int ToolBarWidget::getIconSize() const
{
	const int iconSize(getDefinition().iconSize);

	if (iconSize > 0)
	{
		return iconSize;
	}

	return style()->pixelMetric(QStyle::PM_ToolBarIconSize);
}

int ToolBarWidget::getMaximumButtonSize() const
{
	return getDefinition().maximumButtonSize;
}

bool ToolBarWidget::calculateShouldBeVisible(const ToolBarsManager::ToolBarDefinition &definition, const ToolBarState &state, ToolBarsManager::ToolBarsMode mode)
{
	const ToolBarState::ToolBarVisibility visibility(state.getVisibility(mode));

	if (visibility != ToolBarState::UnspecifiedVisibilityToolBar)
	{
		return (visibility == ToolBarState::AlwaysVisibleToolBar);
	}

	return (definition.getVisibility(mode) == ToolBarsManager::AlwaysVisibleToolBar);
}

bool ToolBarWidget::canDrop(QDropEvent *event) const
{
	return (m_bookmark && event->mimeData()->hasUrls() && (event->keyboardModifiers().testFlag(Qt::ShiftModifier) || !ToolBarsManager::areToolBarsLocked()));
}

bool ToolBarWidget::isCollapsed() const
{
	return m_isCollapsed;
}

bool ToolBarWidget::isDragHandle(const QPoint &position) const
{
	if (!isMovable())
	{
		return false;
	}

	QStyleOptionToolBar option;
	initStyleOption(&option);

	return (style()->subElementRect(QStyle::SE_ToolBarHandle, &option, this).contains(position));
}

bool ToolBarWidget::shouldBeVisible(ToolBarsManager::ToolBarsMode mode) const
{
	const ToolBarsManager::ToolBarDefinition definition(getDefinition());

	return ((mode == ToolBarsManager::NormalMode && definition.hasToggle) || calculateShouldBeVisible(definition, m_state, mode));
}

bool ToolBarWidget::event(QEvent *event)
{
	if (!GesturesManager::isTracking() && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::Wheel))
	{
		QPoint position;

		if (event->type() == QEvent::Wheel)
		{
			position = static_cast<QWheelEvent*>(event)->pos();
		}
		else
		{
			position = static_cast<QMouseEvent*>(event)->pos();
		}

		if (position.isNull() || !isDragHandle(position))
		{
			QVector<GesturesManager::GesturesContext> contexts({GesturesManager::ToolBarContext, GesturesManager::GenericContext});

			if (m_identifier == ToolBarsManager::TabBar)
			{
				contexts.prepend(GesturesManager::NoTabHandleContext);
			}

			GesturesManager::startGesture(this, event, contexts);
		}
	}

	if (event->type() == QEvent::MouseButtonPress)
	{
		return QWidget::event(event);
	}

	return QToolBar::event(event);
}

TabBarToolBarWidget::TabBarToolBarWidget(int identifier, Window *window, QWidget *parent) : ToolBarWidget(identifier, window, parent),
	m_tabBar(nullptr)
{
	setContentsMargins(0, 0, 0, 0);

	layout()->setMargin(0);

	setDefinition(getDefinition());

	connect(ThemesManager::getInstance(), &ThemesManager::widgetStyleChanged, this, &TabBarToolBarWidget::resetGeometry);
	connect(ToolBarsManager::getInstance(), &ToolBarsManager::toolBarModified, this, &TabBarToolBarWidget::handleToolBarModified);
}

void TabBarToolBarWidget::paintEvent(QPaintEvent *event)
{
	ToolBarWidget::paintEvent(event);

	QStyleOptionTab tabOption;

	switch (getArea())
	{
		case Qt::BottomToolBarArea:
			tabOption.shape = QTabBar::RoundedSouth;

			break;
		case Qt::LeftToolBarArea:
			tabOption.shape = QTabBar::RoundedWest;

			break;
		case Qt::RightToolBarArea:
			tabOption.shape = QTabBar::RoundedEast;

			break;
		default:
			tabOption.shape = QTabBar::RoundedNorth;

			break;
	}

	const int overlap(style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, &tabOption));
	QPainter painter(this);
	QStyleOptionTabBarBase tabBarBaseOption;
	tabBarBaseOption.initFrom(this);
	tabBarBaseOption.documentMode = true;
	tabBarBaseOption.rect = contentsRect();
	tabBarBaseOption.shape = tabOption.shape;
	tabBarBaseOption.tabBarRect = contentsRect();

	if (m_tabBar)
	{
		tabBarBaseOption.selectedTabRect = m_tabBar->tabRect(m_tabBar->currentIndex()).translated(m_tabBar->pos());
		tabBarBaseOption.tabBarRect = m_tabBar->geometry();
	}

	if (overlap > 0)
	{
		const QSize size(contentsRect().size());

		switch (getArea())
		{
			case Qt::BottomToolBarArea:
				tabBarBaseOption.rect.setRect(0, 0, size.width(), overlap);

				break;
			case Qt::LeftToolBarArea:
				tabBarBaseOption.rect.setRect((size.width() - overlap), 0, overlap, size.height());

				break;
			case Qt::RightToolBarArea:
				tabBarBaseOption.rect.setRect(0, 0, overlap, size.height());

				break;
			default:
				tabBarBaseOption.rect.setRect(0, (size.height() - overlap), size.width(), overlap);

				break;
		}
	}

	style()->drawPrimitive(QStyle::PE_FrameTabBarBase, &tabBarBaseOption, &painter, this);
}

void TabBarToolBarWidget::resizeEvent(QResizeEvent *event)
{
	ToolBarWidget::resizeEvent(event);

	if (m_tabBar)
	{
		QTimer::singleShot(200, m_tabBar, &TabBarWidget::updateSize);
	}
}

void TabBarToolBarWidget::contextMenuEvent(QContextMenuEvent *event)
{
	if (event->reason() == QContextMenuEvent::Mouse)
	{
		event->accept();

		return;
	}

	QAction *cycleAction(new QAction(tr("Switch Tabs Using the Mouse Wheel"), this));
	cycleAction->setCheckable(true);
	cycleAction->setChecked(!SettingsManager::getOption(SettingsManager::TabBar_RequireModifierToSwitchTabOnScrollOption).toBool());

	QAction *thumbnailsAction(new QAction(tr("Show Thumbnails in Tabs"), this));
	thumbnailsAction->setCheckable(true);
	thumbnailsAction->setChecked(SettingsManager::getOption(SettingsManager::TabBar_EnableThumbnailsOption).toBool());

	connect(cycleAction, &QAction::toggled, [&](bool isEnabled)
	{
		SettingsManager::setOption(SettingsManager::TabBar_RequireModifierToSwitchTabOnScrollOption, !isEnabled);
	});
	connect(thumbnailsAction, &QAction::toggled, [&](bool areEnabled)
	{
		SettingsManager::setOption(SettingsManager::TabBar_EnableThumbnailsOption, areEnabled);
	});

	ActionExecutor::Object executor(getMainWindow(), getMainWindow());
	QMenu menu(this);
	menu.addAction(new Action(ActionsManager::NewTabAction, {}, executor, &menu));
	menu.addAction(new Action(ActionsManager::NewTabPrivateAction, {}, executor, &menu));
	menu.addSeparator();

	QMenu *arrangeMenu(menu.addMenu(tr("Arrange")));
	arrangeMenu->addAction(new Action(ActionsManager::RestoreTabAction, {}, executor, arrangeMenu));
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(new Action(ActionsManager::RestoreAllAction, {}, executor, arrangeMenu));
	arrangeMenu->addAction(new Action(ActionsManager::MaximizeAllAction, {}, executor, arrangeMenu));
	arrangeMenu->addAction(new Action(ActionsManager::MinimizeAllAction, {}, executor, arrangeMenu));
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(new Action(ActionsManager::CascadeAllAction, {}, executor, arrangeMenu));
	arrangeMenu->addAction(new Action(ActionsManager::TileAllAction, {}, executor, arrangeMenu));

	menu.addMenu(createCustomizationMenu(getIdentifier(), {cycleAction, thumbnailsAction}, &menu));
	menu.exec(event->globalPos());
}

void TabBarToolBarWidget::findTabBar()
{
	TabBarWidget *tabBar(m_tabBar ? m_tabBar : findChild<TabBarWidget*>());

	if (!tabBar && getMainWindow())
	{
		tabBar = getMainWindow()->getTabBar();

		if (tabBar)
		{
			connect(tabBar, &TabBarWidget::tabsAmountChanged, this, &TabBarToolBarWidget::updateVisibility);

			updateVisibility();
		}
	}

	if (tabBar && !m_tabBar)
	{
		tabBar->setParent(this);
		tabBar->setVisible(!isCollapsed());

		m_tabBar = tabBar;
	}
}

void TabBarToolBarWidget::clearEntries()
{
	findTabBar();

	for (int i = (actions().count() - 1); i >= 0; --i)
	{
		if (m_tabBar && widgetForAction(actions().at(i)) != m_tabBar)
		{
			removeAction(actions().at(i));
		}
	}

	resetGeometry();
}

void TabBarToolBarWidget::populateEntries()
{
	const ToolBarsManager::ToolBarDefinition definition(getDefinition());
	const bool isHorizontal(getArea() != Qt::LeftToolBarArea && getArea() != Qt::RightToolBarArea);
	QVector<QPointer<QWidget> > addressFields;
	QVector<QPointer<QWidget> > searchFields;

	findTabBar();

	for (int i = 0; i < definition.entries.count(); ++i)
	{
		if (definition.entries.at(i).action == QLatin1String("separator"))
		{
			addSeparator();
		}
		else
		{
			if (m_tabBar && definition.entries.at(i).action == QLatin1String("TabBarWidget"))
			{
				addWidget(m_tabBar);
			}
			else
			{
				const bool isTabBar(definition.entries.at(i).action == QLatin1String("TabBarWidget"));

				if (isTabBar && m_tabBar)
				{
					continue;
				}

				QWidget *widget(WidgetFactory::createToolBarItem(definition.entries.at(i), getWindow(), this));

				if (widget)
				{
					addWidget(widget);

					if (isTabBar)
					{
						m_tabBar = qobject_cast<TabBarWidget*>(widget);

						if (m_tabBar)
						{
							connect(m_tabBar, &TabBarWidget::tabsAmountChanged, this, &TabBarToolBarWidget::updateVisibility);

							updateVisibility();
						}
					}
					else
					{
						if (definition.entries.at(i).action == QLatin1String("AddressWidget"))
						{
							addressFields.append(widget);
						}
						else if (definition.entries.at(i).action == QLatin1String("SearchWidget"))
						{
							searchFields.append(widget);
						}

						layout()->setAlignment(widget, (isHorizontal ? Qt::AlignVCenter : Qt::AlignHCenter));
					}
				}
			}
		}
	}

	if (m_tabBar)
	{
		switch (getArea())
		{
			case Qt::BottomToolBarArea:
				layout()->setAlignment(m_tabBar, Qt::AlignTop);

				break;
			case Qt::LeftToolBarArea:
				layout()->setAlignment(m_tabBar, Qt::AlignRight);

				break;
			case Qt::RightToolBarArea:
				layout()->setAlignment(m_tabBar, Qt::AlignLeft);

				break;
			default:
				layout()->setAlignment(m_tabBar, Qt::AlignBottom);

				break;
		}

		QTimer::singleShot(200, m_tabBar, &TabBarWidget::updateSize);
	}

	setAddressFields(addressFields);
	setSearchFields(searchFields);
}

void TabBarToolBarWidget::updateVisibility()
{
	setVisible(shouldBeVisible((getMainWindow() ? getMainWindow()->windowState().testFlag(Qt::WindowFullScreen) : false) ? ToolBarsManager::FullScreenMode : ToolBarsManager::NormalMode));
}

bool TabBarToolBarWidget::shouldBeVisible(ToolBarsManager::ToolBarsMode mode) const
{
	const ToolBarsManager::ToolBarDefinition definition(getDefinition());

	if (definition.getVisibility(mode) == ToolBarsManager::AutoVisibilityToolBar)
	{
		const TabBarWidget *tabBar(findChild<TabBarWidget*>());

		return (tabBar && tabBar->count() > 1);
	}

	return ((mode == ToolBarsManager::NormalMode && definition.hasToggle) || calculateShouldBeVisible(definition, getState(), mode));
}

bool TabBarToolBarWidget::event(QEvent *event)
{
	if (event->type() == QEvent::LayoutRequest)
	{
		const bool result(QToolBar::event(event));

		setContentsMargins(0, 0, 0, 0);

		layout()->setMargin(0);

		return result;
	}

	return ToolBarWidget::event(event);
}

}
