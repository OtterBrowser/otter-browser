/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

ToolBarDropZoneWidget::ToolBarDropZoneWidget(MainWindow *parent) : QToolBar(parent),
	m_mainWindow(parent),
	m_isDropTarget(false)
{
	setAcceptDrops(true);
}

void ToolBarDropZoneWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);

	if (!m_isDropTarget)
	{
		painter.setOpacity(0.25);
	}

	QStyleOptionRubberBand rubberBandOption;
	rubberBandOption.initFrom(this);

	style()->drawControl(QStyle::CE_RubberBand, &rubberBandOption, &painter);
}

void ToolBarDropZoneWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (canDrop(event->mimeData()))
	{
		event->accept();

		m_isDropTarget = true;

		update();
	}
	else
	{
		event->ignore();
	}
}

void ToolBarDropZoneWidget::dragMoveEvent(QDragMoveEvent *event)
{
	if (canDrop(event->mimeData()))
	{
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void ToolBarDropZoneWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	QWidget::dragLeaveEvent(event);

	m_isDropTarget = false;

	update();
}

void ToolBarDropZoneWidget::dropEvent(QDropEvent *event)
{
	if (!canDrop(event->mimeData()))
	{
		event->ignore();

		m_mainWindow->endToolBarDragging();

		return;
	}

	event->accept();

	m_isDropTarget = false;

	update();

	const QList<ToolBarWidget*> toolBars(m_mainWindow->findChildren<ToolBarWidget*>());
	const int draggedIdentifier(event->mimeData()->property("x-toolbar-identifier").toInt());

	for (int i = 0; i < toolBars.count(); ++i)
	{
		if (toolBars.at(i)->getIdentifier() == draggedIdentifier)
		{
			m_mainWindow->insertToolBar(this, toolBars.at(i));
			m_mainWindow->insertToolBarBreak(this);

			QTimer::singleShot(100, m_mainWindow, SLOT(saveToolBarPositions()));

			break;
		}
	}

	m_mainWindow->endToolBarDragging();
}

QSize ToolBarDropZoneWidget::sizeHint() const
{
	if ((orientation() == Qt::Horizontal))
	{
		return QSize(QToolBar::sizeHint().width(), 5);
	}

	return QSize(5, QToolBar::sizeHint().height());
}

bool ToolBarDropZoneWidget::canDrop(const QMimeData *mimeData)
{
	return (mimeData && !mimeData->property("x-toolbar-identifier").isNull() && m_mainWindow->toolBarArea(this) != Qt::NoToolBarArea);
}

ToolBarWidget::ToolBarWidget(int identifier, Window *window, QWidget *parent) : QToolBar(parent),
	m_mainWindow(MainWindow::findMainWindow(parent)),
	m_window(window),
	m_sidebarWidget(nullptr),
	m_bookmark(nullptr),
	m_dropBookmark(nullptr),
	m_toggleButton(nullptr),
	m_identifier(identifier),
	m_dropIndex(-1),
	m_isCollapsed(false),
	m_isInitialized(false)
{
	setAcceptDrops(true);
	setAllowedAreas(Qt::NoToolBarArea);
	setFloatable(false);

	if (identifier >= 0 && identifier != ToolBarsManager::MenuBar)
	{
		if (identifier == ToolBarsManager::TabBar)
		{
			m_isInitialized = true;

			setContentsMargins(0, 0, 0, 0);

			layout()->setMargin(0);

			reload();

			connect(ThemesManager::getInstance(), SIGNAL(widgetStyleChanged()), this, SLOT(resetGeometry()));
			connect(ToolBarsManager::getInstance(), SIGNAL(toolBarModified(int)), this, SLOT(handleToolBarModified(int)));
		}
		else
		{
			const ToolBarsManager::ToolBarDefinition definition(getDefinition());

			for (int i = 0; i < definition.entries.count(); ++i)
			{
				if (definition.entries.at(i).action == QLatin1String("AddressWidget") || definition.entries.at(i).action == QLatin1String("SearchWidget"))
				{
					m_isInitialized = true;

					reload();

					connect(ToolBarsManager::getInstance(), SIGNAL(toolBarModified(int)), this, SLOT(handleToolBarModified(int)));

					break;
				}
			}
		}

		setToolBarLocked(ToolBarsManager::areToolBarsLocked());

		connect(ToolBarsManager::getInstance(), SIGNAL(toolBarRemoved(int)), this, SLOT(handleToolBarRemoved(int)));
		connect(ToolBarsManager::getInstance(), SIGNAL(toolBarsLockedChanged(bool)), this, SLOT(setToolBarLocked(bool)));
	}

	if (m_mainWindow && m_identifier != ToolBarsManager::AddressBar && m_identifier != ToolBarsManager::ProgressBar)
	{
		connect(m_mainWindow, SIGNAL(currentWindowChanged(quint64)), this, SLOT(notifyWindowChanged(quint64)));
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
				m_toggleButton->setToolTip(tr("Toggle Visibility"));
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
		QWidget *widget(widgetForAction(actions().value(m_dropIndex)));
		const Qt::ToolBarArea area(getArea());
		const int spacing(style()->pixelMetric(QStyle::PM_ToolBarItemSpacing));
		int position(-1);

		if (widget)
		{
			switch (area)
			{
				case Qt::LeftToolBarArea:
				case Qt::RightToolBarArea:
					position = (widget->geometry().top() - spacing);

					break;
				default:
					position = (widget->geometry().left() - spacing);

					break;
			}
		}
		else if (m_dropIndex > 0)
		{
			switch (area)
			{
				case Qt::LeftToolBarArea:
				case Qt::RightToolBarArea:
					position = (childrenRect().bottom() + spacing);

					break;
				default:
					position = (childrenRect().right() + spacing);

					break;
			}
		}

		if (position >= 0)
		{
			QPainter painter(this);

			switch (area)
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

	if (m_identifier != ToolBarsManager::TabBar)
	{
		return;
	}

	const Qt::ToolBarArea area(getArea());
	QStyleOptionTab tabOption;

	switch (area)
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
	QTabBar *tabBar(findChild<TabBarWidget*>());
	QStyleOptionTabBarBase tabBarBaseOption;
	tabBarBaseOption.initFrom(this);
	tabBarBaseOption.documentMode = true;
	tabBarBaseOption.rect = contentsRect();
	tabBarBaseOption.shape = tabOption.shape;
	tabBarBaseOption.tabBarRect = contentsRect();

	if (tabBar)
	{
		tabBarBaseOption.selectedTabRect = tabBar->tabRect(tabBar->currentIndex()).translated(tabBar->pos());
		tabBarBaseOption.tabBarRect = tabBar->geometry();
	}

	if (overlap > 0)
	{
		const QSize size(contentsRect().size());

		switch (area)
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

void ToolBarWidget::showEvent(QShowEvent *event)
{
	if (!m_isInitialized)
	{
		m_isInitialized = true;

		reload();

		connect(ToolBarsManager::getInstance(), SIGNAL(toolBarModified(int)), this, SLOT(handleToolBarModified(int)));
	}

	QToolBar::showEvent(event);
}

void ToolBarWidget::resizeEvent(QResizeEvent *event)
{
	QToolBar::resizeEvent(event);

	if (m_identifier == ToolBarsManager::TabBar)
	{
		TabBarWidget *tabBar(findChild<TabBarWidget*>());

		if (tabBar)
		{
			QTimer::singleShot(200, tabBar, SLOT(updateSize()));
		}
	}

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

	if (event->button() == Qt::LeftButton)
	{
		QStyleOptionToolBar option;
		initStyleOption(&option);

		if (style()->subElementRect(QStyle::SE_ToolBarHandle, &option, this).contains(event->pos()))
		{
			m_dragStartPosition = event->pos();
		}
		else
		{
			m_dragStartPosition = QPoint();
		}
	}
}

void ToolBarWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (!m_mainWindow || !event->buttons().testFlag(Qt::LeftButton) || m_dragStartPosition.isNull() || !isMovable() || (event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
	{
		return;
	}

	m_dragStartPosition = QPoint();

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
	if (event->mimeData()->hasUrls() && (event->keyboardModifiers().testFlag(Qt::ShiftModifier) || !ToolBarsManager::areToolBarsLocked()))
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
	if (event->mimeData()->hasUrls() && (event->keyboardModifiers().testFlag(Qt::ShiftModifier) || !ToolBarsManager::areToolBarsLocked()))
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

	updateDropIndex(QPoint());
}

void ToolBarWidget::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasUrls() && (event->keyboardModifiers().testFlag(Qt::ShiftModifier) || !ToolBarsManager::areToolBarsLocked()))
	{
		event->accept();

		const QVector<QUrl> urls(Utils::extractUrls(event->mimeData()));

		for (int i = 0; i < urls.count(); ++i)
		{
			if (m_dropBookmark)
			{
				BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, urls.at(i), QString(), m_dropBookmark);
			}
			else
			{
				BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, urls.at(i), QString(), m_bookmark, (m_dropIndex + i));
			}
		}
	}
	else
	{
		event->ignore();
	}

	updateDropIndex(QPoint());
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
		const Qt::ToolBarArea area(getArea());
		const bool isHorizontal(area != Qt::LeftToolBarArea && area != Qt::RightToolBarArea);

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

		QWidget *widget(widgetForAction(action));

		dropIndex = actions().indexOf(action);

		if (dropIndex >= 0 && dropIndex < m_bookmark->rowCount())
		{
			BookmarksItem *dropBookmark(BookmarksManager::getModel()->getBookmark(m_bookmark->index().child(dropIndex, 0)));

			if (dropBookmark && static_cast<BookmarksModel::BookmarkType>(dropBookmark->data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::FolderBookmark)
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
				if (isHorizontal && position.x() >= widget->geometry().center().x())
				{
					++dropIndex;
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
					dropIndex = ((position.x() < center.x()) ? 0 : actions().count());
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

	if (m_identifier != ToolBarsManager::TabBar)
	{
		QMenu *menu(createCustomizationMenu(m_identifier));
		menu->exec(event->globalPos());
		menu->deleteLater();

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

	QMenu menu(this);
	menu.addAction(Application::createAction(ActionsManager::NewTabAction, QVariantMap(), true, this));
	menu.addAction(Application::createAction(ActionsManager::NewTabPrivateAction, QVariantMap(), true, this));
	menu.addSeparator();

	QMenu *arrangeMenu(menu.addMenu(tr("Arrange")));
	arrangeMenu->addAction(Application::createAction(ActionsManager::RestoreTabAction, QVariantMap(), true, this));
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(Application::createAction(ActionsManager::RestoreAllAction, QVariantMap(), true, this));
	arrangeMenu->addAction(Application::createAction(ActionsManager::MaximizeAllAction, QVariantMap(), true, this));
	arrangeMenu->addAction(Application::createAction(ActionsManager::MinimizeAllAction, QVariantMap(), true, this));
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(Application::createAction(ActionsManager::CascadeAllAction, QVariantMap(), true, this));
	arrangeMenu->addAction(Application::createAction(ActionsManager::TileAllAction, QVariantMap(), true, this));

	menu.addMenu(createCustomizationMenu(m_identifier, {cycleAction, thumbnailsAction}, &menu));
	menu.exec(event->globalPos());

	cycleAction->deleteLater();

	thumbnailsAction->deleteLater();
}

void ToolBarWidget::reload()
{
	const ToolBarsManager::ToolBarDefinition definition(getDefinition());

	setDefinition(definition);

	emit areaChanged(definition.location);
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

void ToolBarWidget::loadBookmarks()
{
	clear();

	if (!m_bookmark)
	{
		return;
	}

	for (int i = 0; i < m_bookmark->rowCount(); ++i)
	{
		BookmarksItem *bookmark(dynamic_cast<BookmarksItem*>(m_bookmark->child(i)));

		if (bookmark)
		{
			if (static_cast<BookmarksModel::BookmarkType>(bookmark->data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::SeparatorBookmark)
			{
				addSeparator();
			}
			else
			{
				addWidget(new BookmarkWidget(bookmark, ActionsManager::ActionEntryDefinition(), this));
			}
		}
	}
}

void ToolBarWidget::toggleVisibility()
{
	const bool isFullScreen(m_mainWindow ? m_mainWindow->windowState().testFlag(Qt::WindowFullScreen) : false);
	ToolBarsManager::ToolBarDefinition definition(getDefinition());
	ToolBarsManager::ToolBarVisibility visibility(isFullScreen ? definition.fullScreenVisibility : definition.normalVisibility);
	visibility = ((visibility == ToolBarsManager::AlwaysVisibleToolBar) ? ToolBarsManager::AlwaysHiddenToolBar : ToolBarsManager::AlwaysVisibleToolBar);

	if (isFullScreen)
	{
		definition.fullScreenVisibility = visibility;
	}
	else
	{
		definition.normalVisibility = visibility;
	}

	ToolBarsManager::setToolBar(definition);
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

void ToolBarWidget::handleBookmarkModified(BookmarksItem *bookmark)
{
	if (bookmark == m_bookmark || m_bookmark->isAncestorOf(bookmark))
	{
		loadBookmarks();
	}
}

void ToolBarWidget::handleBookmarkMoved(BookmarksItem *bookmark, BookmarksItem *previousParent)
{
	if (bookmark == m_bookmark || previousParent == m_bookmark || m_bookmark->isAncestorOf(bookmark) || m_bookmark->isAncestorOf(previousParent))
	{
		loadBookmarks();
	}
}

void ToolBarWidget::handleBookmarkRemoved(BookmarksItem *bookmark, BookmarksItem *previousParent)
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

	switch (getArea())
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

void ToolBarWidget::updateVisibility()
{
	if (m_identifier == ToolBarsManager::TabBar && ToolBarsManager::getToolBarDefinition(ToolBarsManager::TabBar).normalVisibility == ToolBarsManager::AutoVisibilityToolBar)
	{
		TabBarWidget *tabBar(findChild<TabBarWidget*>());

		if (tabBar)
		{
			setVisible(tabBar->count() > 1);
		}
	}
}

void ToolBarWidget::setDefinition(const ToolBarsManager::ToolBarDefinition &definition)
{
	QWidget *tabBar((m_identifier == ToolBarsManager::TabBar) ? findChild<TabBarWidget*>() : nullptr);
	const bool isFullScreen(m_mainWindow ? m_mainWindow->windowState().testFlag(Qt::WindowFullScreen) : false);
	const bool isHorizontal(definition.location != Qt::LeftToolBarArea && definition.location != Qt::RightToolBarArea);

	m_isCollapsed = (definition.hasToggle && ((isFullScreen ? definition.fullScreenVisibility : definition.normalVisibility) != ToolBarsManager::AlwaysVisibleToolBar));

	setVisible(m_identifier == ToolBarsManager::ProgressBar || definition.hasToggle || shouldBeVisible(isFullScreen));
	setOrientation(isHorizontal ? Qt::Horizontal : Qt::Vertical);

	if (m_identifier == ToolBarsManager::TabBar)
	{
		for (int i = (actions().count() - 1); i >= 0; --i)
		{
			if (widgetForAction(actions().at(i)) == tabBar)
			{
				tabBar->setVisible(!m_isCollapsed);
			}
			else
			{
				removeAction(actions().at(i));
			}
		}

		resetGeometry();
	}
	else if (definition.type == ToolBarsManager::SideBarType && m_sidebarWidget)
	{
		m_sidebarWidget->reload();
	}
	else
	{
		clear();
	}

	if (definition.hasToggle)
	{
		if (!m_toggleButton)
		{
			m_toggleButton = new QPushButton(this);
			m_toggleButton->setToolTip(tr("Toggle Visibility"));
			m_toggleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

			connect(m_toggleButton, SIGNAL(clicked(bool)), this, SLOT(toggleVisibility()));
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

	switch (definition.type)
	{
		case ToolBarsManager::BookmarksBarType:
			m_bookmark = (definition.bookmarksPath.startsWith(QLatin1Char('#')) ? BookmarksManager::getBookmark(definition.bookmarksPath.mid(1).toULongLong()) : BookmarksManager::getModel()->getItem(definition.bookmarksPath));

			loadBookmarks();

			connect(BookmarksManager::getModel(), SIGNAL(bookmarkAdded(BookmarksItem*)), this, SLOT(handleBookmarkModified(BookmarksItem*)));
			connect(BookmarksManager::getModel(), SIGNAL(bookmarkModified(BookmarksItem*)), this, SLOT(handleBookmarkModified(BookmarksItem*)));
			connect(BookmarksManager::getModel(), SIGNAL(bookmarkRestored(BookmarksItem*)), this, SLOT(handleBookmarkModified(BookmarksItem*)));
			connect(BookmarksManager::getModel(), SIGNAL(bookmarkMoved(BookmarksItem*,BookmarksItem*,int)), this, SLOT(handleBookmarkMoved(BookmarksItem*,BookmarksItem*)));
			connect(BookmarksManager::getModel(), SIGNAL(bookmarkTrashed(BookmarksItem*,BookmarksItem*)), this, SLOT(handleBookmarkMoved(BookmarksItem*,BookmarksItem*)));
			connect(BookmarksManager::getModel(), SIGNAL(bookmarkRemoved(BookmarksItem*,BookmarksItem*)), this, SLOT(handleBookmarkRemoved(BookmarksItem*,BookmarksItem*)));

			return;
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

			return;
		default:
			break;
	}

	for (int i = 0; i < definition.entries.count(); ++i)
	{
		if (definition.entries.at(i).action == QLatin1String("separator"))
		{
			addSeparator();
		}
		else
		{
			if (m_identifier == ToolBarsManager::TabBar && tabBar && definition.entries.at(i).action == QLatin1String("TabBarWidget"))
			{
				addWidget(tabBar);
			}
			else
			{
				const bool isTabBar(definition.entries.at(i).action == QLatin1String("TabBarWidget"));

				if (isTabBar && (m_identifier != ToolBarsManager::TabBar || findChild<TabBarWidget*>()))
				{
					continue;
				}

				QWidget *widget(WidgetFactory::createToolBarItem(definition.entries.at(i), this, m_window));

				if (widget)
				{
					addWidget(widget);

					if (isTabBar)
					{
						tabBar = widget;

						connect(widget, SIGNAL(tabsAmountChanged(int)), this, SLOT(updateVisibility()));

						updateVisibility();
					}
					else
					{
						layout()->setAlignment(widget, (isHorizontal ? Qt::AlignVCenter : Qt::AlignHCenter));
					}
				}
			}
		}
	}

	if (tabBar)
	{
		switch (definition.location)
		{
			case Qt::BottomToolBarArea:
				layout()->setAlignment(tabBar, Qt::AlignTop);

				break;
			case Qt::LeftToolBarArea:
				layout()->setAlignment(tabBar, Qt::AlignRight);

				break;
			case Qt::RightToolBarArea:
				layout()->setAlignment(tabBar, Qt::AlignLeft);

				break;
			default:
				layout()->setAlignment(tabBar, Qt::AlignBottom);

				break;
		}

		QTimer::singleShot(200, tabBar, SLOT(updateSize()));
	}
}

void ToolBarWidget::setToolBarLocked(bool locked)
{
	setMovable(!locked && m_identifier != ToolBarsManager::MenuBar && m_identifier != ToolBarsManager::ProgressBar);
}

QMenu* ToolBarWidget::createCustomizationMenu(int identifier, QVector<QAction*> actions, QWidget *parent)
{
	const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(identifier));
	QMenu *menu(new QMenu(parent));
	menu->setTitle(tr("Customize"));

	QMenu *toolBarMenu(menu->addMenu(definition.getTitle().isEmpty() ? tr("(Untitled)") : definition.getTitle()));
	toolBarMenu->addAction(tr("Configure…"), ToolBarsManager::getInstance(), SLOT(configureToolBar()))->setData(identifier);

	QAction *resetAction(toolBarMenu->addAction(tr("Reset to Defaults…"), ToolBarsManager::getInstance(), SLOT(resetToolBar())));
	resetAction->setData(identifier);
	resetAction->setEnabled(definition.canReset);

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

	QAction *removeAction(toolBarMenu->addAction(ThemesManager::createIcon(QLatin1String("list-remove")), tr("Remove…"), ToolBarsManager::getInstance(), SLOT(removeToolBar())));
	removeAction->setData(identifier);
	removeAction->setEnabled(definition.identifier >= ToolBarsManager::OtherToolBar);

	menu->addMenu(new Menu(Menu::ToolBarsMenuRole, menu))->setText(tr("Toolbars"));

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

Qt::ToolBarArea ToolBarWidget::getArea()
{
	return (m_mainWindow ? m_mainWindow->toolBarArea(this) : Qt::TopToolBarArea);
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

bool ToolBarWidget::shouldBeVisible(bool isFullScreen) const
{
	const ToolBarsManager::ToolBarDefinition definition(getDefinition());

	return ((!isFullScreen && definition.hasToggle) || ((isFullScreen ? definition.fullScreenVisibility : definition.normalVisibility) == ToolBarsManager::AlwaysVisibleToolBar));
}

bool ToolBarWidget::event(QEvent *event)
{
	if (!GesturesManager::isTracking() && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::Wheel))
	{
		QVector<GesturesManager::GesturesContext> contexts;

		if (m_identifier == ToolBarsManager::TabBar)
		{
			contexts.append(GesturesManager::NoTabHandleContext);
		}

		contexts.append(GesturesManager::ToolBarContext);
		contexts.append(GesturesManager::GenericContext);

		GesturesManager::startGesture(this, event, contexts);
	}

	if (event->type() == QEvent::MouseButtonPress)
	{
		return QWidget::event(event);
	}

	if (event->type() == QEvent::LayoutRequest && m_identifier == ToolBarsManager::TabBar)
	{
		const bool result(QToolBar::event(event));

		setContentsMargins(0, 0, 0, 0);

		layout()->setMargin(0);

		return result;
	}

	return QToolBar::event(event);
}

}
