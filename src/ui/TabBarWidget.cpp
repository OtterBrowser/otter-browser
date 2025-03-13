/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "TabBarWidget.h"
#include "Action.h"
#include "Animation.h"
#include "ContentsWidget.h"
#include "MainWindow.h"
#include "PreviewWidget.h"
#include "Style.h"
#include "ToolBarWidget.h"
#include "Window.h"
#include "../core/Application.h"
#include "../core/InputInterpreter.h"
#include "../core/SettingsManager.h"
#include "../core/ThemesManager.h"

#include <QtCore/QMimeData>
#include <QtCore/QtMath>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QDrag>
#include <QtGui/QScreen>
#include <QtGui/QStatusTipEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QStylePainter>
#include <QtWidgets/QToolTip>

namespace Otter
{

QIcon TabHandleWidget::m_lockedIcon;
Animation* TabHandleWidget::m_spinnerAnimation(nullptr);
bool TabBarWidget::m_areThumbnailsEnabled(true);
bool TabBarWidget::m_isLayoutReversed(false);
bool TabBarWidget::m_isCloseButtonEnabled(true);
bool TabBarWidget::m_isUrlIconEnabled(true);

TabHandleWidget::TabHandleWidget(Window *window, TabBarWidget *parent) : QWidget(parent),
	m_window(window),
	m_tabBarWidget(parent),
	m_dragTimer(0),
	m_isActiveWindow(false),
	m_isCloseButtonUnderMouse(false),
	m_wasCloseButtonPressed(false)
{
	handleLoadingStateChanged(window->getLoadingState());
	setAcceptDrops(true);
	setMouseTracking(true);

	connect(window, &Window::needsAttention, this, [&]()
	{
		if (!m_isActiveWindow)
		{
			QFont font(parentWidget()->font());
			font.setBold(true);

			setFont(font);
			updateTitle();
		}
	});
	connect(window, &Window::titleChanged, this, &TabHandleWidget::updateTitle);
	connect(window, &Window::iconChanged, this, static_cast<void(TabHandleWidget::*)()>(&TabHandleWidget::update));
	connect(window, &Window::loadingStateChanged, this, &TabHandleWidget::handleLoadingStateChanged);
	connect(parent, &TabBarWidget::currentChanged, this, &TabHandleWidget::updateGeometries);
	connect(parent, &TabBarWidget::tabsAmountChanged, this, &TabHandleWidget::updateGeometries);
	connect(parent, &TabBarWidget::needsGeometriesUpdate, this, &TabHandleWidget::updateGeometries);
}

void TabHandleWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_dragTimer)
	{
		killTimer(m_dragTimer);

		m_dragTimer = 0;

		if (rect().contains(mapFromGlobal(QCursor::pos())))
		{
			MainWindow *mainWindow(MainWindow::findMainWindow(this));

			if (mainWindow)
			{
				mainWindow->setActiveWindowByIdentifier(m_window->getIdentifier());
			}
		}
	}
}

void TabHandleWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	if (!m_window)
	{
		return;
	}

	QPainter painter(this);
	const WebWidget::LoadingState loadingState(m_window->getLoadingState());

	if (m_closeButtonRectangle.isValid())
	{
		if (m_window->isPinned())
		{
			if (m_lockedIcon.isNull())
			{
				m_lockedIcon = ThemesManager::createIcon(QLatin1String("object-locked"));
			}

			m_lockedIcon.paint(&painter, m_closeButtonRectangle);
		}
		else
		{
			QStyleOption option;
			option.initFrom(this);
			option.rect = m_closeButtonRectangle;
			option.state = (QStyle::State_Enabled | QStyle::State_AutoRaise);

			if (m_isCloseButtonUnderMouse)
			{
				option.state |= (QGuiApplication::mouseButtons().testFlag(Qt::LeftButton) ? QStyle::State_Sunken : QStyle::State_Raised);
			}

			if (m_tabBarWidget->getWindow(m_tabBarWidget->currentIndex()) == m_window)
			{
				option.state |= QStyle::State_Selected;
			}

			style()->drawPrimitive(QStyle::PE_IndicatorTabClose, &option, &painter, this);
		}
	}

	if (m_urlIconRectangle.isValid())
	{
		if (loadingState == WebWidget::OngoingLoadingState && m_spinnerAnimation)
		{
			m_spinnerAnimation->paint(&painter, m_urlIconRectangle);
		}
		else
		{
			m_window->getIcon().paint(&painter, m_urlIconRectangle);

			if (loadingState == WebWidget::CrashedLoadingState)
			{
				ThemesManager::getStyle()->drawIconOverlay(m_urlIconRectangle, ThemesManager::createIcon(QLatin1String("emblem-crashed")), &painter);
			}
		}
	}

	if (m_thumbnailRectangle.isValid())
	{
		const QPixmap thumbnail(m_window->createThumbnail());

		if (thumbnail.isNull())
		{
			painter.fillRect(m_thumbnailRectangle, Qt::white);

			if (m_thumbnailRectangle.height() >= 16 && m_thumbnailRectangle.width() >= 16)
			{
				if (loadingState == WebWidget::OngoingLoadingState && m_spinnerAnimation)
				{
					m_spinnerAnimation->paint(&painter, {(m_thumbnailRectangle.left() + ((m_thumbnailRectangle.width() - 16) / 2)), (m_thumbnailRectangle.top() + ((m_thumbnailRectangle.height() - 16) / 2)), 16, 16});
				}
				else
				{
					m_window->getIcon().paint(&painter, m_thumbnailRectangle);
				}
			}
		}
		else
		{
			QRect sourceRectangle(m_thumbnailRectangle);
			sourceRectangle.moveTo(0, 0);

			painter.drawPixmap(m_thumbnailRectangle, thumbnail, sourceRectangle);
		}
	}

	if (!m_title.isEmpty())
	{
		QStyleOptionTab option;
		option.initFrom(this);
		option.documentMode = true;
		option.rect = m_titleRectangle;
		option.text = m_title;

		if (m_isActiveWindow)
		{
			option.state |= QStyle::State_Selected;
		}

		painter.save();

		if (loadingState == WebWidget::DeferredLoadingState)
		{
			painter.setOpacity(0.75);
		}

		style()->drawControl(QStyle::CE_TabBarTabLabel, &option, &painter);

		painter.restore();
	}
}

void TabHandleWidget::moveEvent(QMoveEvent *event)
{
	QWidget::moveEvent(event);

	if (underMouse())
	{
		m_isCloseButtonUnderMouse = m_closeButtonRectangle.contains(mapFromGlobal(QCursor::pos()));
	}
}

void TabHandleWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	updateGeometries();
}

void TabHandleWidget::leaveEvent(QEvent *event)
{
	QWidget::leaveEvent(event);

	m_isCloseButtonUnderMouse = false;

	update();
}

void TabHandleWidget::mousePressEvent(QMouseEvent *event)
{
	m_wasCloseButtonPressed = m_closeButtonRectangle.contains(event->pos());

	QWidget::mousePressEvent(event);

	update();
}

void TabHandleWidget::mouseMoveEvent(QMouseEvent *event)
{
	const bool wasCloseButtonUnderMouse(m_isCloseButtonUnderMouse);

	m_isCloseButtonUnderMouse = m_closeButtonRectangle.contains(event->pos());

	if (m_window && !m_window->isPinned())
	{
		if (wasCloseButtonUnderMouse && !m_isCloseButtonUnderMouse)
		{
			m_tabBarWidget->showPreview(-1, SettingsManager::getOption(SettingsManager::TabBar_PreviewsAnimationDurationOption).toInt());

			QToolTip::hideText();

			setToolTip(QString());
		}
		else if (!wasCloseButtonUnderMouse && m_isCloseButtonUnderMouse)
		{
			m_tabBarWidget->hidePreview();

			const QKeySequence shortcut(ActionsManager::getActionShortcut(ActionsManager::CloseTabAction));

			setToolTip(Utils::appendShortcut(tr("Close Tab"), shortcut));
		}
	}

	QWidget::mouseMoveEvent(event);

	update();
}

void TabHandleWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_window && !m_window->isPinned() && event->button() == Qt::LeftButton && m_wasCloseButtonPressed && m_closeButtonRectangle.contains(event->pos()))
	{
		m_window->requestClose();

		event->accept();
	}

	QWidget::mouseReleaseEvent(event);
}

void TabHandleWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (m_dragTimer == 0 && event->mimeData()->property("x-window-identifier").isNull() && m_tabBarWidget->getWindow(m_tabBarWidget->currentIndex()) != m_window)
	{
		m_dragTimer = startTimer(500);
	}
}

void TabHandleWidget::handleLoadingStateChanged(WebWidget::LoadingState state)
{
	if (state == WebWidget::OngoingLoadingState)
	{
		if (!m_spinnerAnimation)
		{
			m_spinnerAnimation = ThemesManager::createAnimation();
			m_spinnerAnimation->start();
		}

		connect(m_spinnerAnimation, &Animation::frameChanged, this, static_cast<void(TabHandleWidget::*)()>(&TabHandleWidget::update));
	}
	else if (m_spinnerAnimation)
	{
		for (int i = 0; i < m_tabBarWidget->count(); ++i)
		{
			const Window *window(m_tabBarWidget->getWindow(i));

			if (window && window->getLoadingState() == WebWidget::OngoingLoadingState)
			{
				return;
			}
		}

		m_spinnerAnimation->deleteLater();
		m_spinnerAnimation = nullptr;

		update();
	}
}

void TabHandleWidget::updateGeometries()
{
	if (!m_window)
	{
		return;
	}

	QStyleOption option;
	option.initFrom(this);

	QRect controlsRectangle(style()->subElementRect(QStyle::SE_TabBarTabLeftButton, &option, m_tabBarWidget));

	m_closeButtonRectangle = {};
	m_urlIconRectangle = {};
	m_thumbnailRectangle = {};
	m_labelRectangle = {};
	m_titleRectangle = {};

	if (TabBarWidget::areThumbnailsEnabled())
	{
		const int controlsHeight(qRound(qMax(16.0, QFontMetrics(font()).height() * 1.5)));

		if (controlsRectangle.height() > (controlsHeight * 2))
		{
			m_thumbnailRectangle = controlsRectangle;
			m_thumbnailRectangle.setHeight(controlsRectangle.height() - controlsHeight);
			m_thumbnailRectangle.setTop(style()->pixelMetric(QStyle::PM_TabBarTabVSpace) / 2);

			controlsRectangle.setTop(m_thumbnailRectangle.bottom());
		}
	}

	const int controlsWidth(controlsRectangle.width());
	const bool isActive(m_tabBarWidget->getWindow(m_tabBarWidget->currentIndex()) == m_window);
	const bool isCloseButtonEnabled(TabBarWidget::isCloseButtonEnabled());
	const bool isUrlIconEnabled(TabBarWidget::isUrlIconEnabled());

	if (controlsWidth <= 18 && (isCloseButtonEnabled || isUrlIconEnabled))
	{
		if (isUrlIconEnabled)
		{
			if (isActive && isCloseButtonEnabled && !m_window->isPinned())
			{
				const int buttonWidth((controlsRectangle.width() / 2) - 2);

				m_closeButtonRectangle = controlsRectangle;
				m_urlIconRectangle = controlsRectangle;

				if (TabBarWidget::isLayoutReversed())
				{
					m_closeButtonRectangle.setWidth(buttonWidth);

					m_urlIconRectangle.setLeft(m_urlIconRectangle.right() - buttonWidth);
				}
				else
				{
					m_urlIconRectangle.setWidth(buttonWidth);

					m_closeButtonRectangle.setLeft(m_closeButtonRectangle.right() - buttonWidth);
				}
			}
			else
			{
				m_urlIconRectangle = controlsRectangle;
			}
		}
		else
		{
			m_closeButtonRectangle = controlsRectangle;
		}
	}
	else if (controlsWidth <= 34 && isActive && (isCloseButtonEnabled && !m_window->isPinned()) && isUrlIconEnabled)
	{
		const int buttonWidth((controlsRectangle.width() / 2) - 2);

		m_closeButtonRectangle = controlsRectangle;
		m_urlIconRectangle = controlsRectangle;

		if (TabBarWidget::isLayoutReversed())
		{
			m_closeButtonRectangle.setWidth(buttonWidth);

			m_urlIconRectangle.setLeft(m_urlIconRectangle.right() - buttonWidth);
		}
		else
		{
			m_urlIconRectangle.setWidth(buttonWidth);

			m_closeButtonRectangle.setLeft(m_closeButtonRectangle.right() - buttonWidth);
		}
	}
	else
	{
		m_labelRectangle = controlsRectangle;

		if (isUrlIconEnabled)
		{
			m_urlIconRectangle = controlsRectangle;

			if (TabBarWidget::isLayoutReversed())
			{
				m_urlIconRectangle.setLeft(controlsRectangle.right() - 16);

				m_labelRectangle.setRight(controlsRectangle.right() - 20);
			}
			else
			{
				m_urlIconRectangle.setWidth(16);

				m_labelRectangle.setLeft(m_urlIconRectangle.right() + 4);
			}
		}

		if (isCloseButtonEnabled && (isActive || controlsWidth >= 70))
		{
			m_closeButtonRectangle = m_labelRectangle;

			if (TabBarWidget::isLayoutReversed())
			{
				m_closeButtonRectangle.setWidth(16);
			}
			else
			{
				m_closeButtonRectangle.setLeft(m_labelRectangle.right() - 16);
			}

			if (controlsWidth <= 40)
			{
				m_labelRectangle = {};
			}
			else
			{
				if (TabBarWidget::isLayoutReversed())
				{
					m_labelRectangle.setLeft(m_labelRectangle.left() + 20);
				}
				else
				{
					m_labelRectangle.setRight(m_closeButtonRectangle.left() - 4);
				}
			}
		}
	}

	if (m_closeButtonRectangle.isValid() && m_closeButtonRectangle.height() > m_closeButtonRectangle.width())
	{
		m_closeButtonRectangle.setTop(controlsRectangle.top() + ((m_closeButtonRectangle.height() - m_closeButtonRectangle.width()) / 2));
		m_closeButtonRectangle.setHeight(m_closeButtonRectangle.width());
	}

	if (m_urlIconRectangle.isValid() && m_urlIconRectangle.height() > m_urlIconRectangle.width())
	{
		m_urlIconRectangle.setTop(controlsRectangle.top() + ((m_urlIconRectangle.height() - m_urlIconRectangle.width()) / 2));
		m_urlIconRectangle.setHeight(m_urlIconRectangle.width());
	}

	m_isCloseButtonUnderMouse = (underMouse() && m_closeButtonRectangle.contains(mapFromGlobal(QCursor::pos())));

	updateTitle();
}

void TabHandleWidget::updateTitle()
{
	QString title(m_window->getTitle());

	if (!m_labelRectangle.isValid() || m_labelRectangle.width() < 5)
	{
		title.clear();
	}
	else
	{
		int length(fontMetrics().horizontalAdvance(title));

		if (length > m_labelRectangle.width())
		{
			title = fontMetrics().elidedText(title, Qt::ElideRight, m_labelRectangle.width());
			length = fontMetrics().horizontalAdvance(title);
		}

		m_titleRectangle = m_labelRectangle;

		if (length < m_labelRectangle.width() && ThemesManager::getStyle()->getExtraStyleHint(Style::CanAlignTabBarLabelHint) > 0)
		{
			if (isLeftToRight())
			{
				m_titleRectangle.setWidth(length);
			}
			else
			{
				m_titleRectangle.setLeft(m_titleRectangle.right() - length);
			}
		}

		title.replace(QLatin1Char('&'), QLatin1String("&&"));
	}

	if (title != m_title)
	{
		m_title = title;

		update();
	}
}

void TabHandleWidget::setIsActiveWindow(bool isActive)
{
	if (isActive != m_isActiveWindow)
	{
		m_isActiveWindow = isActive;

		if (isActive)
		{
			setFont(parentWidget()->font());
			updateTitle();
		}
		else
		{
			update();
		}
	}
}

Window* TabHandleWidget::getWindow() const
{
	return m_window;
}

TabBarWidget::TabBarWidget(QWidget *parent) : QTabBar(parent),
	m_previewWidget(nullptr),
	m_activeTabHandleWidget(nullptr),
	m_movableTabWidget(nullptr),
	m_tabWidth(0),
	m_clickedTab(-1),
	m_hoveredTab(-1),
	m_pinnedTabsAmount(0),
	m_previewTimer(0),
	m_arePreviewsEnabled(SettingsManager::getOption(SettingsManager::TabBar_EnablePreviewsOption).toBool()),
	m_isDraggingTab(false),
	m_isDetachingTab(false),
	m_isIgnoringTabDrag(false),
	m_needsUpdateOnLeave(false)
{
	m_areThumbnailsEnabled = SettingsManager::getOption(SettingsManager::TabBar_EnableThumbnailsOption).toBool();
	m_isCloseButtonEnabled = SettingsManager::getOption(SettingsManager::TabBar_ShowCloseButtonOption).toBool();
	m_isUrlIconEnabled = SettingsManager::getOption(SettingsManager::TabBar_ShowUrlIconOption).toBool();

	installGesturesFilter(this, this);
	setAcceptDrops(true);
	setExpanding(false);
	setMovable(true);
	setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
	setElideMode(Qt::ElideRight);
	setMouseTracking(true);
	setDocumentMode(true);
	setMaximumSize(0, 0);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	updateStyle();
	handleOptionChanged(SettingsManager::TabBar_MaximumTabHeightOption, SettingsManager::getOption(SettingsManager::TabBar_MaximumTabHeightOption));
	handleOptionChanged(SettingsManager::TabBar_MinimumTabHeightOption, SettingsManager::getOption(SettingsManager::TabBar_MinimumTabHeightOption));
	handleOptionChanged(SettingsManager::TabBar_MaximumTabWidthOption, SettingsManager::getOption(SettingsManager::TabBar_MaximumTabWidthOption));
	handleOptionChanged(SettingsManager::TabBar_MinimumTabWidthOption, SettingsManager::getOption(SettingsManager::TabBar_MinimumTabWidthOption));

	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar)
	{
		setArea(toolBar->getArea());

		connect(toolBar, &ToolBarWidget::areaChanged, this, &TabBarWidget::setArea);
	}

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &TabBarWidget::handleOptionChanged);
	connect(ThemesManager::getInstance(), &ThemesManager::widgetStyleChanged, this, &TabBarWidget::updateStyle);
	connect(this, &TabBarWidget::currentChanged, this, &TabBarWidget::handleCurrentChanged);
}

void TabBarWidget::changeEvent(QEvent *event)
{
	QTabBar::changeEvent(event);

	switch (event->type())
	{
		case QEvent::ApplicationLayoutDirectionChange:
		case QEvent::LayoutDirectionChange:
			updateStyle();

			break;
		case QEvent::FontChange:
			handleOptionChanged(SettingsManager::TabBar_MinimumTabHeightOption, SettingsManager::getOption(SettingsManager::TabBar_MinimumTabHeightOption));

			break;
		default:
			break;
	}
}

void TabBarWidget::childEvent(QChildEvent *event)
{
	QTabBar::childEvent(event);

	if (m_isDraggingTab && !m_isIgnoringTabDrag && !m_movableTabWidget && event->added())
	{
		QWidget *widget(qobject_cast<QWidget*>(event->child()));

		if (widget)
		{
			m_movableTabWidget = widget;
		}
	}
}

void TabBarWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_previewTimer)
	{
		killTimer(m_previewTimer);

		m_previewTimer = 0;

		showPreview(tabAt(mapFromGlobal(QCursor::pos())));
	}
}

void TabBarWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QStylePainter painter(this);
	const int selectedIndex(currentIndex());

	for (int i = 0; i < count(); ++i)
	{
		if (i == selectedIndex)
		{
			continue;
		}

		const QStyleOptionTab tabOption(createStyleOptionTab(i));

		if (rect().intersects(tabOption.rect))
		{
			painter.drawControl(QStyle::CE_TabBarTab, tabOption);
		}
	}

	if (selectedIndex >= 0)
	{
		const QStyleOptionTab tabOption(createStyleOptionTab(selectedIndex));

		if (m_isDraggingTab && !m_isIgnoringTabDrag && m_movableTabWidget)
		{
			const int tabOverlap(style()->pixelMetric(QStyle::PM_TabBarTabOverlap, nullptr, this));

			m_movableTabWidget->setGeometry(tabOption.rect.adjusted(-tabOverlap, 0, tabOverlap, 0));
		}
		else
		{
			painter.drawControl(QStyle::CE_TabBarTab, tabOption);
		}
	}

	if (m_dragMovePosition.isNull())
	{
		return;
	}

	const int dropIndex(getDropIndex());

	if (dropIndex < 0)
	{
		return;
	}

	int lineOffset(0);

	if (count() == 0)
	{
		lineOffset = 0;
	}
	else if (dropIndex >= count())
	{
		lineOffset = tabRect(count() - 1).right();
	}
	else
	{
		lineOffset = tabRect(dropIndex).left();
	}

	ThemesManager::getStyle()->drawDropZone((isHorizontal() ? QLine(lineOffset, 0, lineOffset, height()) : QLine(0, lineOffset, width(), lineOffset)), &painter);
}

#if QT_VERSION >= 0x060000
void TabBarWidget::enterEvent(QEnterEvent *event)
#else
void TabBarWidget::enterEvent(QEvent *event)
#endif
{
	QTabBar::enterEvent(event);

	showPreview(-1, SettingsManager::getOption(SettingsManager::TabBar_PreviewsAnimationDurationOption).toInt());
}

void TabBarWidget::leaveEvent(QEvent *event)
{
	QTabBar::leaveEvent(event);

	hidePreview();

	m_tabWidth = 0;
	m_hoveredTab = -1;

	if (m_needsUpdateOnLeave)
	{
		updateSize();

		m_needsUpdateOnLeave = false;
	}

	QStatusTipEvent statusTipEvent((QString()));

	QApplication::sendEvent(this, &statusTipEvent);
}

void TabBarWidget::contextMenuEvent(QContextMenuEvent *event)
{
	if (event->reason() == QContextMenuEvent::Mouse)
	{
		event->accept();

		return;
	}

	m_clickedTab = tabAt(event->pos());

	hidePreview();

	MainWindow *mainWindow(MainWindow::findMainWindow(this));
	ActionExecutor::Object mainWindowExecutor(mainWindow, mainWindow);
	ActionExecutor::Object windowExecutor;
	QMenu menu(this);
	menu.addAction(new Action(ActionsManager::NewTabAction, {}, mainWindowExecutor, &menu));
	menu.addAction(new Action(ActionsManager::NewTabPrivateAction, {}, mainWindowExecutor, &menu));

	if (m_clickedTab >= 0)
	{
		Window *window(getWindow(m_clickedTab));

		if (window)
		{
			windowExecutor = ActionExecutor::Object(window, window);

			menu.addAction(new Action(ActionsManager::CloneTabAction, {}, windowExecutor, &menu));
			menu.addAction(new Action(ActionsManager::PinTabAction, {}, windowExecutor, &menu));
			menu.addAction(new Action(ActionsManager::MuteTabMediaAction, {}, windowExecutor, &menu));
			menu.addSeparator();
			menu.addAction(new Action(ActionsManager::DetachTabAction, {}, windowExecutor, &menu));
			menu.addSeparator();
			menu.addAction(new Action(ActionsManager::CloseTabAction, {}, windowExecutor, &menu));
			menu.addAction(new Action(ActionsManager::CloseOtherTabsAction, {{QLatin1String("tab"), window->getIdentifier()}}, mainWindowExecutor, &menu));
			menu.addAction(new Action(ActionsManager::ClosePrivateTabsAction, {}, mainWindowExecutor, &menu));
		}
	}

	menu.addSeparator();

	QMenu *arrangeMenu(menu.addMenu(tr("Arrange")));
	arrangeMenu->addAction(new Action(ActionsManager::RestoreTabAction, {}, windowExecutor, arrangeMenu));
	arrangeMenu->addAction(new Action(ActionsManager::MinimizeTabAction, {}, windowExecutor, arrangeMenu));
	arrangeMenu->addAction(new Action(ActionsManager::MaximizeTabAction, {}, windowExecutor, arrangeMenu));
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(new Action(ActionsManager::RestoreAllAction, {}, mainWindowExecutor, arrangeMenu));
	arrangeMenu->addAction(new Action(ActionsManager::MaximizeAllAction, {}, mainWindowExecutor, arrangeMenu));
	arrangeMenu->addAction(new Action(ActionsManager::MinimizeAllAction, {}, mainWindowExecutor, arrangeMenu));
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(new Action(ActionsManager::CascadeAllAction, {}, mainWindowExecutor, arrangeMenu));
	arrangeMenu->addAction(new Action(ActionsManager::TileAllAction, {}, mainWindowExecutor, arrangeMenu));

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

	if (qobject_cast<ToolBarWidget*>(parentWidget()))
	{
		menu.addMenu(ToolBarWidget::createCustomizationMenu(ToolBarsManager::TabBar, {cycleAction, thumbnailsAction}, &menu));
	}
	else
	{
		QMenu *customizationMenu(menu.addMenu(tr("Customize")));
		customizationMenu->addAction(cycleAction);
		customizationMenu->addAction(thumbnailsAction);
		customizationMenu->addSeparator();
		customizationMenu->addAction(new Action(ActionsManager::LockToolBarsAction, {}, mainWindowExecutor, &menu));
	}

	menu.exec(event->globalPos());

	m_clickedTab = -1;

	if (underMouse())
	{
		m_previewTimer = startTimer(SettingsManager::getOption(SettingsManager::TabBar_PreviewsAnimationDurationOption).toInt());
	}
}

void TabBarWidget::mousePressEvent(QMouseEvent *event)
{
	QTabBar::mousePressEvent(event);

	if (event->button() == Qt::LeftButton)
	{
		const Window *window(getWindow(tabAt(event->pos())));

		m_isIgnoringTabDrag = (count() == 1);

		if (window)
		{
			m_dragStartPosition = event->pos();
			m_draggedWindow = window->getIdentifier();
		}
	}

	hidePreview();
}

void TabBarWidget::mouseMoveEvent(QMouseEvent *event)
{
	tabHovered(tabAt(event->pos()));

	if (!m_isDraggingTab && !m_dragStartPosition.isNull())
	{
		m_isDraggingTab = ((event->pos() - m_dragStartPosition).manhattanLength() > QApplication::startDragDistance());
	}

	if (m_isDraggingTab && !rect().adjusted(-10, -10, 10, 10).contains(event->pos()))
	{
		m_isDraggingTab = false;

		QMouseEvent mouseEvent(QEvent::MouseButtonRelease, event->pos(), event->globalPos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());

		QApplication::sendEvent(this, &mouseEvent);

		m_isDetachingTab = true;

		updateSize();

		const MainWindow *mainWindow(MainWindow::findMainWindow(this));

		if (mainWindow)
		{
			const Window *window(mainWindow->getWindowByIdentifier(m_draggedWindow));

			if (window)
			{
				QMimeData *mimeData(new QMimeData());
				mimeData->setText(window->getUrl().toString());
				mimeData->setUrls({window->getUrl()});
				mimeData->setProperty("x-url-title", window->getTitle());
				mimeData->setProperty("x-window-identifier", window->getIdentifier());

				const QPixmap thumbnail(window->createThumbnail());
				QDrag *drag(new QDrag(this));
				drag->setMimeData(mimeData);
				drag->setPixmap(thumbnail.isNull() ? window->getIcon().pixmap(16, 16) : thumbnail);
				drag->exec(Qt::CopyAction | Qt::MoveAction);

				m_isDetachingTab = false;

				if (!drag->target())
				{
					Application::triggerAction(ActionsManager::DetachTabAction, {{QLatin1String("tab"), window->getIdentifier()}}, parentWidget());
				}
			}
		}

		return;
	}

	if (!m_isIgnoringTabDrag && !m_isDetachingTab)
	{
		QTabBar::mouseMoveEvent(event);
	}
}

void TabBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QTabBar::mouseReleaseEvent(event);

	if (event->button() == Qt::LeftButton)
	{
		if (m_isDetachingTab)
		{
			Application::triggerAction(ActionsManager::DetachTabAction, {{QLatin1String("tab"), m_draggedWindow}}, parentWidget());

			m_isDetachingTab = false;
		}

		m_dragStartPosition = {};
		m_isDraggingTab = false;
	}
}

void TabBarWidget::wheelEvent(QWheelEvent *event)
{
	QWidget::wheelEvent(event);

	if (event->modifiers().testFlag(Qt::ControlModifier) || !SettingsManager::getOption(SettingsManager::TabBar_RequireModifierToSwitchTabOnScrollOption).toBool())
	{
		Application::triggerAction(((event->angleDelta().y() > 0) ? ActionsManager::ActivateTabOnLeftAction : ActionsManager::ActivateTabOnRightAction), {}, parentWidget());
	}
}

void TabBarWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasText() || event->mimeData()->hasUrls() || (event->source() && !event->mimeData()->property("x-window-identifier").isNull()))
	{
		event->accept();

		m_dragMovePosition = event->pos();

		update();
	}
}

void TabBarWidget::dragMoveEvent(QDragMoveEvent *event)
{
	m_dragMovePosition = event->pos();

	update();
}

void TabBarWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	Q_UNUSED(event)

	m_dragMovePosition = {};

	update();
}

void TabBarWidget::dropEvent(QDropEvent *event)
{
	const int dropIndex(getDropIndex());

	if (event->source() && !event->mimeData()->property("x-window-identifier").isNull())
	{
		event->setDropAction(Qt::MoveAction);
		event->accept();

		int previousIndex(-1);
		const quint64 windowIdentifier(event->mimeData()->property("x-window-identifier").toULongLong());

		if (event->source() == this)
		{
			for (int i = 0; i < count(); ++i)
			{
				const Window *window(getWindow(i));

				if (window && window->getIdentifier() == windowIdentifier)
				{
					previousIndex = i;

					break;
				}
			}
		}

		if (previousIndex < 0)
		{
			MainWindow *mainWindow(MainWindow::findMainWindow(this));

			if (mainWindow)
			{
				const QVector<MainWindow*> mainWindows(Application::getWindows());

				for (int i = 0; i < mainWindows.count(); ++i)
				{
					if (mainWindows.at(i))
					{
						Window *window(mainWindows.at(i)->getWindowByIdentifier(windowIdentifier));

						if (window)
						{
							mainWindows.at(i)->moveWindow(window, mainWindow, {{QLatin1String("index"), dropIndex}});

							break;
						}
					}
				}
			}
		}
		else if (previousIndex != dropIndex && (previousIndex + 1) != dropIndex)
		{
			moveTab(previousIndex, (dropIndex - ((dropIndex > previousIndex) ? 1 : 0)));
		}
	}
	else if (event->mimeData()->hasText() || event->mimeData()->hasUrls())
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(this));
		bool canOpen(mainWindow != nullptr);

		if (canOpen)
		{
			const QVector<QUrl> urls(Utils::extractUrls(event->mimeData()));

			if (urls.isEmpty())
			{
				const InputInterpreter::InterpreterResult result(InputInterpreter::interpret(event->mimeData()->text(), (InputInterpreter::NoBookmarkKeywordsFlag | InputInterpreter::NoSearchKeywordsFlag)));

				if (result.isValid())
				{
					switch (result.type)
					{
						case InputInterpreter::InterpreterResult::UrlType:
							mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), result.url}, {QLatin1String("index"), dropIndex}});

							break;
						case InputInterpreter::InterpreterResult::SearchType:
							mainWindow->search(result.searchQuery, result.searchEngine, SessionsManager::NewTabOpen);

							break;
						default:
							break;
					}
				}
			}
			else
			{
				if (urls.count() > 1 && SettingsManager::getOption(SettingsManager::Choices_WarnOpenMultipleDroppedUrlsOption).toBool())
				{
					QMessageBox messageBox;
					messageBox.setWindowTitle(tr("Question"));
					messageBox.setText(tr("You are about to open %n URL(s).", "", urls.count()));
					messageBox.setInformativeText(tr("Do you want to continue?"));
					messageBox.setIcon(QMessageBox::Question);
					messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
					messageBox.setDefaultButton(QMessageBox::Yes);
					messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

					if (messageBox.exec() == QMessageBox::Cancel)
					{
						canOpen = false;
					}

					SettingsManager::setOption(SettingsManager::Choices_WarnOpenMultipleDroppedUrlsOption, !messageBox.checkBox()->isChecked());
				}

				if (canOpen)
				{
					for (int i = 0; i < urls.count(); ++i)
					{
						mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), urls.at(i)}, {QLatin1String("index"), (dropIndex + i)}});
					}
				}
			}
		}

		if (canOpen)
		{
			event->setDropAction(Qt::CopyAction);
			event->accept();
		}
		else
		{
			event->ignore();
		}
	}
	else
	{
		event->ignore();
	}

	m_dragMovePosition = {};

	update();
}

void TabBarWidget::tabLayoutChange()
{
	QTabBar::tabLayoutChange();

	for (int i = 0; i < count(); ++i)
	{
		QWidget *tabHandleWidget(tabButton(i, QTabBar::LeftSide));

		if (tabHandleWidget)
		{
			QStyleOptionTab tabOption;

			initStyleOption(&tabOption, i);

			tabHandleWidget->resize(style()->subElementRect(QStyle::SE_TabBarTabLeftButton, &tabOption, this).size());
		}
	}

	tabHovered(tabAt(mapFromGlobal(QCursor::pos())));
}

void TabBarWidget::tabInserted(int index)
{
	setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

	QTabBar::tabInserted(index);

	emit tabsAmountChanged(count());
}

void TabBarWidget::tabRemoved(int index)
{
	QTabBar::tabRemoved(index);

	if (count() == 0)
	{
		setMaximumSize(0, 0);
	}
	else if (underMouse())
	{
		m_needsUpdateOnLeave = true;
	}

	emit tabsAmountChanged(count());
}

void TabBarWidget::tabHovered(int index)
{
	if (index == m_hoveredTab)
	{
		return;
	}

	m_hoveredTab = index;

	if (m_previewWidget && !m_previewWidget->isVisible() && m_previewTimer == 0)
	{
		m_previewWidget->show();
	}

	if (m_previewWidget && m_previewWidget->isVisible())
	{
		showPreview(index);
	}

	if (!m_isDraggingTab)
	{
		const Window *window(getWindow(index));

		if (window)
		{
			QStatusTipEvent statusTipEvent(window->getUrl().toDisplayString());

			QApplication::sendEvent(this, &statusTipEvent);
		}
	}
}

void TabBarWidget::addTab(int index, Window *window)
{
	const int selectedIndex(currentIndex());

	blockSignals(true);
	insertTab(index, {});
	blockSignals(false);
	setTabButton(index, QTabBar::LeftSide, new TabHandleWidget(window, this));
	setTabButton(index, QTabBar::RightSide, nullptr);

	if (selectedIndex != currentIndex() || count() == 1)
	{
		emit currentChanged(currentIndex());
	}

	connect(window, &Window::isPinnedChanged, this, &TabBarWidget::updatePinnedTabsAmount);

	if (window->isPinned())
	{
		updatePinnedTabsAmount();
	}
}

void TabBarWidget::removeTab(int index)
{
	if (underMouse())
	{
		m_tabWidth = tabSizeHint(count() - 1).width();
	}

	Window *window(getWindow(index));

	if (window)
	{
		window->deleteLater();
	}

	QTabBar::removeTab(index);

	if (window && window->isPinned())
	{
		updatePinnedTabsAmount();
		updateSize();
	}

	if (underMouse() && tabAt(mapFromGlobal(QCursor::pos())) < 0)
	{
		m_tabWidth = 0;

		updateSize();
	}
}

void TabBarWidget::showPreview(int index, int delay)
{
	if (delay > 0)
	{
		if (m_previewTimer == 0)
		{
			m_previewTimer = startTimer(delay);
		}

		return;
	}

	if (!m_arePreviewsEnabled || !isActiveWindow())
	{
		hidePreview();

		return;
	}

	Window *window(getWindow(index));

	if (window && m_clickedTab < 0)
	{
		if (!m_previewWidget)
		{
			m_previewWidget = new PreviewWidget(this);
		}

		QPoint position;
		// Note that screen rectangle, tab rectangle and preview rectangle could have
		// negative values on multiple monitors systems. All calculations must be done in context
		// of a current screen rectangle. Because top left point of current screen could
		// have coordinates (-1366, 250) instead of (0, 0).
		///TODO: Calculate screen rectangle based on current mouse pointer position
		const QRect screenGeometry(screen()->geometry());
		QRect rectangle(tabRect(index));
		rectangle.moveTo(mapToGlobal(rectangle.topLeft()));

		const bool isActive(index == currentIndex());

		m_previewWidget->setPreview(window->getTitle(), ((isActive || m_areThumbnailsEnabled) ? QPixmap() : window->createThumbnail()), isActive);

		switch (shape())
		{
			case QTabBar::RoundedEast:
				position = {(rectangle.left() - m_previewWidget->width()), qMax(screenGeometry.top(), ((rectangle.bottom() - (rectangle.height() / 2)) - (m_previewWidget->height() / 2)))};

				break;
			case QTabBar::RoundedWest:
				position = {rectangle.right(), qMax(screenGeometry.top(), ((rectangle.bottom() - (rectangle.height() / 2)) - (m_previewWidget->height() / 2)))};

				break;
			case QTabBar::RoundedSouth:
				position = {qMax(screenGeometry.left(), ((rectangle.right() - (rectangle.width() / 2)) - (m_previewWidget->width() / 2))), (rectangle.top() - m_previewWidget->height())};

				break;
			default:
				position = {qMax(screenGeometry.left(), ((rectangle.right() - (rectangle.width() / 2)) - (m_previewWidget->width() / 2))), rectangle.bottom()};

				break;
		}

		if ((position.x() + m_previewWidget->width()) > screenGeometry.right())
		{
			position.setX(screenGeometry.right() - m_previewWidget->width());
		}

		if ((position.y() + m_previewWidget->height()) > screenGeometry.bottom())
		{
			position.setY(screenGeometry.bottom() - m_previewWidget->height());
		}

		if (m_previewWidget->isVisible())
		{
			m_previewWidget->setPosition(position);
		}
		else
		{
			m_previewWidget->move(position);
			m_previewWidget->show();
		}
	}
	else if (m_previewWidget)
	{
		m_previewWidget->hide();
	}
}

void TabBarWidget::hidePreview()
{
	if (m_previewWidget)
	{
		m_previewWidget->hide();
	}

	if (m_previewTimer != 0)
	{
		killTimer(m_previewTimer);

		m_previewTimer = 0;
	}
}

void TabBarWidget::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::Interface_WidgetStyleOption:
			updateStyle();

			break;
		case SettingsManager::TabBar_EnablePreviewsOption:
			m_arePreviewsEnabled = value.toBool();

			emit needsGeometriesUpdate();

			break;
		case SettingsManager::TabBar_EnableThumbnailsOption:
			if (value.toBool() != m_areThumbnailsEnabled)
			{
				m_areThumbnailsEnabled = value.toBool();

				if (!m_areThumbnailsEnabled)
				{
					ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

					if (toolBar)
					{
						toolBar->resetGeometry();
					}
				}

				updateSize();

				emit needsGeometriesUpdate();
			}

			break;
		case SettingsManager::TabBar_MaximumTabHeightOption:
			{
				const int oldValue(m_maximumTabSize.height());

				m_maximumTabSize.setHeight(value.toInt());

				if (m_maximumTabSize.height() < 0)
				{
					m_maximumTabSize.setHeight(QWIDGETSIZE_MAX);
				}

				if (m_maximumTabSize.height() != oldValue)
				{
					updateSize();
				}
			}

			break;
		case SettingsManager::TabBar_MaximumTabWidthOption:
			{
				const int oldValue(m_maximumTabSize.width());

				m_maximumTabSize.setWidth(value.toInt());

				if (m_maximumTabSize.width() < 0)
				{
					m_maximumTabSize.setWidth(250);
				}

				if (m_maximumTabSize.width() != oldValue)
				{
					updateSize();
				}
			}

			break;
		case SettingsManager::TabBar_MinimumTabHeightOption:
			{
				const int oldValue(m_minimumTabSize.height());

				m_minimumTabSize.setHeight(value.toInt());

				if (m_minimumTabSize.height() < 0)
				{
					m_minimumTabSize.setHeight(qRound(QFontMetrics(font()).lineSpacing() * 1.25) + style()->pixelMetric(QStyle::PM_TabBarTabVSpace));
				}

				if (m_minimumTabSize.height() != oldValue)
				{
					updateSize();
				}
			}

			break;
		case SettingsManager::TabBar_MinimumTabWidthOption:
			{
				const int oldValue(m_minimumTabSize.width());

				m_minimumTabSize.setWidth(value.toInt());

				if (m_minimumTabSize.width() < 0)
				{
					m_minimumTabSize.setWidth(16 + style()->pixelMetric(QStyle::PM_TabBarTabHSpace));
				}

				if (m_minimumTabSize.width() != oldValue)
				{
					updateSize();
				}
			}

			break;
		case SettingsManager::TabBar_ShowCloseButtonOption:
			if (value.toBool() != m_isCloseButtonEnabled)
			{
				m_isCloseButtonEnabled = value.toBool();

				emit needsGeometriesUpdate();
			}

			break;
		case SettingsManager::TabBar_ShowUrlIconOption:
			if (value.toBool() != m_isUrlIconEnabled)
			{
				m_isUrlIconEnabled = value.toBool();

				emit needsGeometriesUpdate();
			}

			break;
		default:
			break;
	}
}

void TabBarWidget::handleCurrentChanged(int index)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (mainWindow)
	{
		mainWindow->setActiveWindowByIndex(index);
	}

	if (m_previewWidget && m_previewWidget->isVisible())
	{
		showPreview(tabAt(mapFromGlobal(QCursor::pos())));
	}

	TabHandleWidget *tabHandleWidget(qobject_cast<TabHandleWidget*>(tabButton(index, QTabBar::LeftSide)));

	if (tabHandleWidget)
	{
		tabHandleWidget->setIsActiveWindow(true);
	}

	if (m_activeTabHandleWidget && tabHandleWidget != m_activeTabHandleWidget)
	{
		m_activeTabHandleWidget->setIsActiveWindow(false);
	}

	m_activeTabHandleWidget = tabHandleWidget;
}

void TabBarWidget::updatePinnedTabsAmount()
{
	int amount(0);

	for (int i = 0; i < count(); ++i)
	{
		const Window *window(getWindow(i));

		if (window && window->isPinned())
		{
			++amount;
		}
	}

	if (amount != m_pinnedTabsAmount)
	{
		m_pinnedTabsAmount = amount;

		updateSize();
	}
}

void TabBarWidget::updateSize()
{
	updateGeometry();
	adjustSize();
}

void TabBarWidget::updateStyle()
{
	m_isLayoutReversed = (static_cast<QTabBar::ButtonPosition>(style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition)) == QTabBar::LeftSide);

	if (isRightToLeft())
	{
		m_isLayoutReversed = !m_isLayoutReversed;
	}

	handleOptionChanged(SettingsManager::TabBar_MinimumTabHeightOption, SettingsManager::getOption(SettingsManager::TabBar_MinimumTabHeightOption));
	handleOptionChanged(SettingsManager::TabBar_MinimumTabWidthOption, SettingsManager::getOption(SettingsManager::TabBar_MinimumTabWidthOption));

	emit needsGeometriesUpdate();
}

void TabBarWidget::setArea(Qt::ToolBarArea area)
{
	switch (area)
	{
		case Qt::LeftToolBarArea:
			setShape(QTabBar::RoundedWest);

			break;
		case Qt::RightToolBarArea:
			setShape(QTabBar::RoundedEast);

			break;
		case Qt::BottomToolBarArea:
			setShape(QTabBar::RoundedSouth);

			break;
		default:
			setShape(QTabBar::RoundedNorth);

			break;
	}

	setSizePolicy(QSizePolicy::Preferred, ((area != Qt::LeftToolBarArea && area != Qt::RightToolBarArea) ? QSizePolicy::Maximum : QSizePolicy::Preferred));
}

Window* TabBarWidget::getWindow(int index) const
{
	if (index >= 0 && index < count())
	{
		const TabHandleWidget *widget(qobject_cast<TabHandleWidget*>(tabButton(index, QTabBar::LeftSide)));

		if (widget)
		{
			return widget->getWindow();
		}
	}

	return nullptr;
}

GesturesController::GestureContext TabBarWidget::getGestureContext(const QPoint &position) const
{
	GestureContext context;
	const int tab(tabAt(position));

	if (tab >= 0)
	{
		const Window *window(getWindow(tab));

		if (window)
		{
			context.parameters[QLatin1String("tab")] = window->getIdentifier();
		}
	}

	if (tab < 0)
	{
		context.contexts.append(GesturesManager::NoTabHandleContext);
	}
	else if (tab == currentIndex())
	{
		context.contexts.append(GesturesManager::ActiveTabHandleContext);
		context.contexts.append(GesturesManager::TabHandleContext);
	}
	else
	{
		context.contexts.append(GesturesManager::TabHandleContext);
	}

	if (qobject_cast<ToolBarWidget*>(parentWidget()))
	{
		context.contexts.append(GesturesManager::ToolBarContext);
	}

	context.contexts.append(GesturesManager::GenericContext);

	return context;
}

QStyleOptionTab TabBarWidget::createStyleOptionTab(int index) const
{
	QStyleOptionTab tabOption;

	initStyleOption(&tabOption, index);

	const QWidget *widget(tabButton(index, QTabBar::LeftSide));

	if (widget)
	{
		const QPoint position(widget->mapToParent(widget->rect().topLeft()));

		if (isHorizontal())
		{
			tabOption.rect.moveTo(position.x(), tabOption.rect.y());
		}
		else
		{
			tabOption.rect.moveTo(tabOption.rect.x(), position.y());
		}
	}

	return tabOption;
}

QSize TabBarWidget::tabSizeHint(int index) const
{
	if (isHorizontal())
	{
		const Window *window(getWindow(index));
		const int tabHeight(qBound(m_minimumTabSize.height(), qMax((m_areThumbnailsEnabled ? 200 : 0), (parentWidget() ? parentWidget()->height() : height())), m_maximumTabSize.height()));

		if (window && window->isPinned())
		{
			return {m_minimumTabSize.width(), tabHeight};
		}

		if (m_tabWidth > 0)
		{
			return {m_tabWidth, tabHeight};
		}

		return {qBound(m_minimumTabSize.width(), qFloor((rect().width() - (m_pinnedTabsAmount * m_minimumTabSize.width())) / qMax(1, (count() - m_pinnedTabsAmount))), m_maximumTabSize.width()), tabHeight};
	}

	return {m_maximumTabSize.width(), (m_areThumbnailsEnabled ? 200 : m_minimumTabSize.height())};
}

QSize TabBarWidget::minimumSizeHint() const
{
	return {0, 0};
}

QSize TabBarWidget::sizeHint() const
{
	if (isHorizontal())
	{
		int size(0);

		for (int i = 0; i < count(); ++i)
		{
			const Window *window(getWindow(i));

			size += ((window && window->isPinned()) ? m_minimumTabSize.width() : m_maximumTabSize.width());
		}

		if (parentWidget() && size > parentWidget()->width())
		{
			size = parentWidget()->width();
		}

		return {size, tabSizeHint(0).height()};
	}

	return {QTabBar::sizeHint().width(), (tabSizeHint(0).height() * count())};
}

int TabBarWidget::getDropIndex() const
{
	if (m_dragMovePosition.isNull())
	{
		return ((count() > 0) ? (count() + 1) : 0);
	}

	int index(tabAt(m_dragMovePosition));
	const bool isHorizontal(this->isHorizontal());

	if (index >= 0)
	{
		const QPoint tabCenter(tabRect(index).center());

		if ((isHorizontal && m_dragMovePosition.x() > tabCenter.x()) || (!isHorizontal && m_dragMovePosition.y() > tabCenter.y()))
		{
			++index;
		}
	}
	else
	{
		index = (((isHorizontal && m_dragMovePosition.x() < rect().left()) || (!isHorizontal && m_dragMovePosition.y() < rect().top())) ? count() : 0);
	}

	return index;
}

bool TabBarWidget::areThumbnailsEnabled()
{
	return m_areThumbnailsEnabled;
}

bool TabBarWidget::isLayoutReversed()
{
	return m_isLayoutReversed;
}

bool TabBarWidget::isCloseButtonEnabled()
{
	return m_isCloseButtonEnabled;
}

bool TabBarWidget::isUrlIconEnabled()
{
	return m_isUrlIconEnabled;
}

bool TabBarWidget::isHorizontal() const
{
	return (shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth);
}

bool TabBarWidget::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::LayoutDirectionChange:
			emit needsGeometriesUpdate();

			break;
		case QEvent::ParentChange:
			{
				const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent()));

				if (toolBar)
				{
					setArea(toolBar->getArea());

					connect(toolBar, &ToolBarWidget::areaChanged, this, &TabBarWidget::setArea);
				}
			}

			break;
		default:
			break;
	}

	return QTabBar::event(event);
}

}
