/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TabBarWidget.h"
#include "ContentsWidget.h"
#include "MainWindow.h"
#include "PreviewWidget.h"
#include "ToolBarWidget.h"
#include "Window.h"
#include "../core/ActionsManager.h"
#include "../core/GesturesManager.h"
#include "../core/SettingsManager.h"
#include "../core/ThemesManager.h"

#include <QtCore/QMimeData>
#include <QtCore/QtMath>
#include <QtCore/QTimer>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QStatusTipEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QStylePainter>
#include <QtWidgets/QToolTip>

namespace Otter
{

QIcon TabHandleWidget::m_lockedIcon;
QMovie* TabHandleWidget::m_delayedLoadingMovie(nullptr);
QMovie* TabHandleWidget::m_ongoingLoadingMovie(nullptr);
bool TabBarWidget::m_areThumbnailsEnabled(true);
bool TabBarWidget::m_isLayoutReversed(false);
bool TabBarWidget::m_isCloseButtonEnabled(true);
bool TabBarWidget::m_isUrlIconEnabled(true);

TabDrag::TabDrag(quint64 window, QObject *parent) : QDrag(parent),
	m_window(window),
	m_releaseTimer(0)
{
	m_releaseTimer = startTimer(250);
}

TabDrag::~TabDrag()
{
	if (!target())
	{
		QVariantMap parameters;
		parameters[QLatin1String("window")] = m_window;

		ActionsManager::triggerAction(ActionsManager::DetachTabAction, this, parameters);
	}
}

void TabDrag::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_releaseTimer && QGuiApplication::mouseButtons() == Qt::NoButton)
	{
		deleteLater();
	}

	QDrag::timerEvent(event);
}

TabBarStyle::TabBarStyle(QStyle *style) : QProxyStyle(style)
{
}

QRect TabBarStyle::subElementRect(QStyle::SubElement element, const QStyleOption *option, const QWidget *widget) const
{
	if (element == QStyle::SE_TabBarTabLeftButton)
	{
		if (qstyleoption_cast<const QStyleOptionTab*>(option))
		{
			return option->rect;
		}

		QStyleOptionTab tabOption;
		tabOption.cornerWidgets = QStyleOptionTab::LeftCornerWidget;
		tabOption.rect = option->rect;
		tabOption.shape = QTabBar::RoundedNorth;

		const int offset(QProxyStyle::subElementRect(QStyle::SE_TabBarTabLeftButton, &tabOption, widget).left() - option->rect.left());
		QRect rectangle(option->rect);
		rectangle.setLeft(rectangle.left() + offset);
		rectangle.setRight(rectangle.right() - offset);

		return rectangle;
	}

	return QProxyStyle::subElementRect(element, option, widget);
}

int TabBarStyle::pixelMetric(QStyle::PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
	if (metric == QStyle::PM_TabBarTabHSpace && QProxyStyle::pixelMetric(metric, option, widget) == QCommonStyle::pixelMetric(metric, option, widget))
	{
		return QProxyStyle::pixelMetric(QStyle::PM_TabBarTabVSpace, option, widget);
	}

	return QProxyStyle::pixelMetric(metric, option, widget);
}

TabHandleWidget::TabHandleWidget(Window *window, TabBarWidget *parent) : QWidget(parent),
	m_window(window),
	m_tabBarWidget(parent),
	m_loadingMovie(nullptr),
	m_isCloseButtonUnderMouse(false)
{
	handleLoadingStateChanged(window->getLoadingState());
	setMouseTracking(true);

	connect(window, SIGNAL(activated()), this, SLOT(markAsActive()));
	connect(window, SIGNAL(needsAttention()), this, SLOT(markAsNeedingAttention()));
	connect(window, SIGNAL(titleChanged(QString)), this, SLOT(update()));
	connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(update()));
	connect(window, SIGNAL(loadingStateChanged(WindowsManager::LoadingState)), this, SLOT(handleLoadingStateChanged(WindowsManager::LoadingState)));
	connect(parent, SIGNAL(currentChanged(int)), this, SLOT(updateGeometries()));
	connect(parent, SIGNAL(tabsAmountChanged(int)), this, SLOT(updateGeometries()));
	connect(parent, SIGNAL(needsGeometriesUpdate()), this, SLOT(updateGeometries()));
}

void TabHandleWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	if (!m_window)
	{
		return;
	}

	QPainter painter(this);

	if (m_closeButtonRectangle.isValid())
	{
		if (m_window->isPinned())
		{
			if (m_lockedIcon.isNull())
			{
				m_lockedIcon = ThemesManager::getIcon(QLatin1String("object-locked"));
			}

			m_lockedIcon.paint(&painter, m_closeButtonRectangle);
		}
		else
		{
			QStyleOption option;
			option.init(this);
			option.rect = m_closeButtonRectangle;
			option.state = (QStyle::State_Enabled | QStyle::State_AutoRaise);

			if (m_isCloseButtonUnderMouse && QGuiApplication::mouseButtons().testFlag(Qt::LeftButton))
			{
				option.state |= QStyle::State_Sunken;
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
		if (m_loadingMovie)
		{
			painter.drawPixmap(m_urlIconRectangle, m_loadingMovie->currentPixmap());
		}
		else
		{
			m_window->getIcon().paint(&painter, m_urlIconRectangle);
		}
	}

	if (m_thumbnailRectangle.isValid())
	{
		const QPixmap thumbnail(m_window->getThumbnail());

		if (thumbnail.isNull())
		{
			painter.fillRect(m_thumbnailRectangle, Qt::white);

			if (m_thumbnailRectangle.height() >= 16 && m_thumbnailRectangle.width() >= 16)
			{
				if (m_loadingMovie)
				{
					painter.drawPixmap(QRect((m_thumbnailRectangle.left() + ((m_thumbnailRectangle.width() - 16) / 2)), (m_thumbnailRectangle.top() + ((m_thumbnailRectangle.height() - 16) / 2)), 16, 16), m_loadingMovie->currentPixmap());
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

	if (m_titleRectangle.isValid())
	{
		painter.drawText(m_titleRectangle, ((isRightToLeft() ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter), fontMetrics().elidedText(m_window->getTitle(), Qt::ElideRight, m_titleRectangle.width()));
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

void TabHandleWidget::mouseMoveEvent(QMouseEvent *event)
{
	const bool wasCloseButtonUnderMouse(m_isCloseButtonUnderMouse);

	m_isCloseButtonUnderMouse = m_closeButtonRectangle.contains(event->pos());

	if (m_window)
	{
		if (!m_window->isPinned())
		{
			if (wasCloseButtonUnderMouse && !m_isCloseButtonUnderMouse)
			{
				m_tabBarWidget->showPreview(-1, 250);

				QToolTip::hideText();

				setToolTip(QString());
			}
			else if (!wasCloseButtonUnderMouse && m_isCloseButtonUnderMouse)
			{
				m_tabBarWidget->hidePreview();

				const QVector<QKeySequence> shortcuts(ActionsManager::getActionDefinition(ActionsManager::CloseTabAction).shortcuts);

				setToolTip(tr("Close Tab") + (shortcuts.isEmpty() ? QString() : QLatin1String(" (") + shortcuts.at(0).toString(QKeySequence::NativeText) + QLatin1Char(')')));
			}
		}
	}

	QWidget::mouseMoveEvent(event);

	update();
}

void TabHandleWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QWidget::mouseReleaseEvent(event);

	if (m_window && !m_window->isPinned() && event->button() == Qt::LeftButton && m_isCloseButtonUnderMouse)
	{
		m_window->close();
	}
}

void TabHandleWidget::markAsActive()
{
	setFont(parentWidget()->font());
}

void TabHandleWidget::markAsNeedingAttention()
{
	if (m_tabBarWidget->getWindow(m_tabBarWidget->currentIndex()) != m_window)
	{
		QFont font(parentWidget()->font());
		font.setBold(true);

		setFont(font);
	}
}

void TabHandleWidget::handleLoadingStateChanged(WindowsManager::LoadingState state)
{
	if (m_loadingMovie)
	{
		disconnect(m_loadingMovie, SIGNAL(frameChanged(int)), this, SLOT(update()));

		m_loadingMovie = nullptr;

		update();
	}

	if (state == WindowsManager::DelayedLoadingState || state == WindowsManager::OngoingLoadingState)
	{
		if (state == WindowsManager::OngoingLoadingState)
		{
			if (!m_ongoingLoadingMovie)
			{
				m_ongoingLoadingMovie = new QMovie(QLatin1String(":/icons/loading.gif"), QByteArray(), QCoreApplication::instance());
				m_ongoingLoadingMovie->setSpeed(100);
				m_ongoingLoadingMovie->start();
			}

			m_loadingMovie = m_ongoingLoadingMovie;
		}
		else
		{
			if (!m_delayedLoadingMovie)
			{
				m_delayedLoadingMovie = new QMovie(QLatin1String(":/icons/loading.gif"), QByteArray(), QCoreApplication::instance());
				m_delayedLoadingMovie->setSpeed(10);
				m_delayedLoadingMovie->start();
			}

			m_loadingMovie = m_delayedLoadingMovie;
		}

		connect(m_loadingMovie, SIGNAL(frameChanged(int)), this, SLOT(update()));
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

	m_closeButtonRectangle = QRect();
	m_urlIconRectangle = QRect();
	m_thumbnailRectangle = QRect();
	m_titleRectangle = QRect();

	if (TabBarWidget::areThumbnailsEnabled())
	{
		const int controlsHeight(qMax(16.0, (QFontMetrics(font()).height() * 1.5)));

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
	const bool isLayoutReversed(TabBarWidget::isLayoutReversed() && isLeftToRight());
	const bool isCloseButtonEnabled(TabBarWidget::isCloseButtonEnabled());
	const bool isUrlIconEnabled(TabBarWidget::isUrlIconEnabled());

	if (controlsWidth <= 16 && (isCloseButtonEnabled || isUrlIconEnabled))
	{
		if (isUrlIconEnabled)
		{
			if (isActive && isCloseButtonEnabled && !m_window->isPinned())
			{
				m_closeButtonRectangle = controlsRectangle;
				m_urlIconRectangle = controlsRectangle;

				if (isLayoutReversed)
				{
					m_closeButtonRectangle.setWidth(controlsRectangle.width() / 2);

					m_urlIconRectangle.setLeft(m_closeButtonRectangle.right());
				}
				else
				{
					m_urlIconRectangle.setWidth(controlsRectangle.width() / 2);

					m_closeButtonRectangle.setLeft(m_urlIconRectangle.right());
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
	else if (controlsWidth <= 32 && isActive && (isCloseButtonEnabled && !m_window->isPinned()) && isUrlIconEnabled)
	{
		if (isUrlIconEnabled)
		{
			m_closeButtonRectangle = controlsRectangle;
			m_urlIconRectangle = controlsRectangle;

			if (isLayoutReversed)
			{
				m_closeButtonRectangle.setWidth(controlsRectangle.width() / 2);

				m_urlIconRectangle.setLeft(m_closeButtonRectangle.right());
			}
			else
			{
				m_urlIconRectangle.setWidth(controlsRectangle.width() / 2);

				m_closeButtonRectangle.setLeft(m_urlIconRectangle.right());
			}
		}
	}
	else
	{
		m_titleRectangle = controlsRectangle;

		if (isUrlIconEnabled)
		{
			m_urlIconRectangle = controlsRectangle;

			if (isLayoutReversed)
			{
				m_urlIconRectangle.setLeft(controlsRectangle.width() - 16);

				m_titleRectangle.setWidth(controlsRectangle.width() - 20);
			}
			else
			{
				m_urlIconRectangle.setWidth(16);

				m_titleRectangle.setLeft(m_urlIconRectangle.right() + 4);
			}
		}

		if (isCloseButtonEnabled && (isActive || controlsWidth >= 70))
		{
			m_closeButtonRectangle = m_titleRectangle;

			if (isLayoutReversed)
			{
				m_closeButtonRectangle.setWidth(16);
			}
			else
			{
				m_closeButtonRectangle.setLeft(m_titleRectangle.right() - 16);
			}

			if (controlsWidth <= 40)
			{
				m_titleRectangle = QRect();
			}
			else
			{
				if (isLayoutReversed)
				{
					m_titleRectangle.setLeft(20);
				}
				else
				{
					m_titleRectangle.setRight(m_closeButtonRectangle.left() - 4);
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

	update();
}

Window* TabHandleWidget::getWindow() const
{
	return m_window;
}

TabBarWidget::TabBarWidget(QWidget *parent) : QTabBar(parent),
	m_previewWidget(nullptr),
	m_movableTabWidget(nullptr),
	m_tabWidth(0),
	m_clickedTab(-1),
	m_hoveredTab(-1),
	m_pinnedTabsAmount(0),
	m_previewTimer(0),
	m_arePreviewsEnabled(SettingsManager::getValue(SettingsManager::TabBar_EnablePreviewsOption).toBool()),
	m_isDraggingTab(false),
	m_isDetachingTab(false),
	m_isIgnoringTabDrag(false)
{
	m_areThumbnailsEnabled = SettingsManager::getValue(SettingsManager::TabBar_EnableThumbnailsOption).toBool();
	m_isLayoutReversed = (static_cast<QTabBar::ButtonPosition>(style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition)) == QTabBar::LeftSide);
	m_isCloseButtonEnabled = SettingsManager::getValue(SettingsManager::TabBar_ShowCloseButtonOption).toBool();
	m_isUrlIconEnabled = SettingsManager::getValue(SettingsManager::TabBar_ShowUrlIconOption).toBool();

	setAcceptDrops(true);
	setDrawBase(false);
	setExpanding(false);
	setTabsClosable(false);
	setMovable(true);
	setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
	setElideMode(Qt::ElideRight);
	setMouseTracking(true);
	setDocumentMode(true);
	setMaximumSize(0, 0);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	setStyle(new TabBarStyle());
	optionChanged(SettingsManager::TabBar_MaximumTabHeightOption, SettingsManager::getValue(SettingsManager::TabBar_MaximumTabHeightOption));
	optionChanged(SettingsManager::TabBar_MinimumTabHeightOption, SettingsManager::getValue(SettingsManager::TabBar_MinimumTabHeightOption));
	optionChanged(SettingsManager::TabBar_MaximumTabWidthOption, SettingsManager::getValue(SettingsManager::TabBar_MaximumTabWidthOption));
	optionChanged(SettingsManager::TabBar_MinimumTabWidthOption, SettingsManager::getValue(SettingsManager::TabBar_MinimumTabWidthOption));

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar)
	{
		setArea(toolBar->getArea());

		connect(toolBar, SIGNAL(areaChanged(Qt::ToolBarArea)), this, SLOT(setArea(Qt::ToolBarArea)));
	}

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int,QVariant)));
	connect(this, SIGNAL(currentChanged(int)), this, SLOT(updatePreviewPosition()));
}

void TabBarWidget::changeEvent(QEvent *event)
{
	QTabBar::changeEvent(event);

	switch (event->type())
	{
		case QEvent::FontChange:
			optionChanged(SettingsManager::TabBar_MinimumTabHeightOption, SettingsManager::getValue(SettingsManager::TabBar_MinimumTabHeightOption));

			break;
		case QEvent::LayoutDirectionChange:
			emit needsGeometriesUpdate();

			break;
		case QEvent::StyleChange:
			m_isLayoutReversed = (static_cast<QTabBar::ButtonPosition>(style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition)) == QTabBar::LeftSide);

			optionChanged(SettingsManager::TabBar_MinimumTabHeightOption, SettingsManager::getValue(SettingsManager::TabBar_MinimumTabHeightOption));
			optionChanged(SettingsManager::TabBar_MinimumTabWidthOption, SettingsManager::getValue(SettingsManager::TabBar_MinimumTabWidthOption));

			emit needsGeometriesUpdate();

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
			const int taboverlap(style()->pixelMetric(QStyle::PM_TabBarTabOverlap, 0, this));

			m_movableTabWidget->setGeometry(tabOption.rect.adjusted(-taboverlap, 0, taboverlap, 0));
		}
		else
		{
			painter.drawControl(QStyle::CE_TabBarTab, tabOption);
		}
	}

	if (!m_dragMovePosition.isNull())
	{
		const int dropIndex(getDropIndex());

		if (dropIndex >= 0)
		{
			const bool isHorizontal(shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth);
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

			painter.setPen(QPen(palette().text(), 3, Qt::DotLine));

			if (isHorizontal)
			{
				painter.drawLine(lineOffset, 0, lineOffset, height());
			}
			else
			{
				painter.drawLine(0, lineOffset, width(), lineOffset);
			}

			painter.setPen(QPen(palette().text(), 9, Qt::SolidLine, Qt::RoundCap));

			if (isHorizontal)
			{
				painter.drawPoint(lineOffset, 0);
				painter.drawPoint(lineOffset, height());
			}
			else
			{
				painter.drawPoint(0, lineOffset);
				painter.drawPoint(width(), lineOffset);
			}
		}
	}
}

void TabBarWidget::enterEvent(QEvent *event)
{
	QTabBar::enterEvent(event);

	showPreview(-1, 250);
}

void TabBarWidget::leaveEvent(QEvent *event)
{
	QTabBar::leaveEvent(event);

	hidePreview();

	m_tabWidth = 0;
	m_hoveredTab = -1;

	updateGeometry();
	adjustSize();

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
	QVariantMap parameters;
	QMenu menu(this);
	menu.addAction(ActionsManager::getAction(ActionsManager::NewTabAction, this));
	menu.addAction(ActionsManager::getAction(ActionsManager::NewTabPrivateAction, this));

	if (m_clickedTab >= 0)
	{
		Window *window(getWindow(m_clickedTab));

		if (window)
		{
			parameters[QLatin1String("window")] = window->getIdentifier();

			const int amount(count() - getPinnedTabsAmount());
			const bool isPinned(window->isPinned());
			Action *cloneTabAction(new Action(ActionsManager::CloneTabAction, &menu));
			cloneTabAction->setEnabled(window->canClone());
			cloneTabAction->setData(parameters);

			Action *pinTabAction(new Action(ActionsManager::PinTabAction, &menu));
			pinTabAction->setOverrideText(isPinned ? QT_TRANSLATE_NOOP("actions", "Unpin Tab") : QT_TRANSLATE_NOOP("actions", "Pin Tab"));
			pinTabAction->setData(parameters);

			Action *detachTabAction(new Action(ActionsManager::DetachTabAction, &menu));
			detachTabAction->setEnabled(count() > 1);
			detachTabAction->setData(parameters);

			Action *closeTabAction(new Action(ActionsManager::CloseTabAction, &menu));
			closeTabAction->setEnabled(!isPinned);
			closeTabAction->setData(parameters);

			Action *closeOtherTabsAction(new Action(ActionsManager::CloseOtherTabsAction, &menu));
			closeOtherTabsAction->setEnabled(amount > 0 && !(amount == 1 && !isPinned));
			closeOtherTabsAction->setData(parameters);

			menu.addAction(cloneTabAction);
			menu.addAction(pinTabAction);
			menu.addAction(window ? window->getContentsWidget()->getAction(ActionsManager::MuteTabMediaAction) : new Action(ActionsManager::MuteTabMediaAction, &menu));
			menu.addSeparator();
			menu.addAction(detachTabAction);
			menu.addSeparator();
			menu.addAction(closeTabAction);
			menu.addAction(closeOtherTabsAction);
			menu.addAction(ActionsManager::getAction(ActionsManager::ClosePrivateTabsAction, this));

			connect(cloneTabAction, SIGNAL(triggered()), mainWindow, SLOT(triggerAction()));
			connect(pinTabAction, SIGNAL(triggered()), mainWindow, SLOT(triggerAction()));
			connect(detachTabAction, SIGNAL(triggered()), mainWindow, SLOT(triggerAction()));
			connect(closeTabAction, SIGNAL(triggered()), mainWindow, SLOT(triggerAction()));
			connect(closeOtherTabsAction, SIGNAL(triggered()), mainWindow, SLOT(triggerAction()));
		}
	}

	menu.addSeparator();

	QMenu *arrangeMenu(menu.addMenu(tr("Arrange")));
	Action *restoreTabAction(new Action(ActionsManager::RestoreTabAction, &menu));
	restoreTabAction->setEnabled(m_clickedTab >= 0);
	restoreTabAction->setData(parameters);

	Action *minimizeTabAction(new Action(ActionsManager::MinimizeTabAction, &menu));
	minimizeTabAction->setEnabled(m_clickedTab >= 0);
	minimizeTabAction->setData(parameters);

	Action *maximizeTabAction(new Action(ActionsManager::MaximizeTabAction, &menu));
	maximizeTabAction->setEnabled(m_clickedTab >= 0);
	maximizeTabAction->setData(parameters);

	arrangeMenu->addAction(restoreTabAction);
	arrangeMenu->addAction(minimizeTabAction);
	arrangeMenu->addAction(maximizeTabAction);
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::RestoreAllAction, this));
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::MaximizeAllAction, this));
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::MinimizeAllAction, this));
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::CascadeAllAction, this));
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::TileAllAction, this));

	QAction *cycleAction(new QAction(tr("Switch Tabs Using the Mouse Wheel"), this));
	cycleAction->setCheckable(true);
	cycleAction->setChecked(!SettingsManager::getValue(SettingsManager::TabBar_RequireModifierToSwitchTabOnScrollOption).toBool());

	QAction *thumbnailsAction(new QAction(tr("Show Thumbnails in Tabs"), this));
	thumbnailsAction->setCheckable(true);
	thumbnailsAction->setChecked(SettingsManager::getValue(SettingsManager::TabBar_EnableThumbnailsOption).toBool());

	connect(cycleAction, &QAction::toggled, [&](bool isEnabled)
	{
		SettingsManager::setValue(SettingsManager::TabBar_RequireModifierToSwitchTabOnScrollOption, !isEnabled);
	});
	connect(thumbnailsAction, &QAction::toggled, [&](bool areEnabled)
	{
		SettingsManager::setValue(SettingsManager::TabBar_EnableThumbnailsOption, areEnabled);
	});
	connect(restoreTabAction, SIGNAL(triggered()), mainWindow, SLOT(triggerAction()));
	connect(minimizeTabAction, SIGNAL(triggered()), mainWindow, SLOT(triggerAction()));
	connect(maximizeTabAction, SIGNAL(triggered()), mainWindow, SLOT(triggerAction()));

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

	if (toolBar)
	{
		QList<QAction*> actions;
		actions.append(cycleAction);
		actions.append(thumbnailsAction);

		menu.addMenu(ToolBarWidget::createCustomizationMenu(ToolBarsManager::TabBar, actions, &menu));
	}
	else
	{
		QMenu *customizationMenu(menu.addMenu(tr("Customize")));
		customizationMenu->addAction(cycleAction);
		customizationMenu->addAction(thumbnailsAction);
		customizationMenu->addSeparator();
		customizationMenu->addAction(ActionsManager::getAction(ActionsManager::LockToolBarsAction, this));
	}

	menu.exec(event->globalPos());

	cycleAction->deleteLater();

	m_clickedTab = -1;

	if (underMouse())
	{
		m_previewTimer = startTimer(250);
	}
}

void TabBarWidget::mousePressEvent(QMouseEvent *event)
{
	QTabBar::mousePressEvent(event);

	if (event->button() == Qt::LeftButton)
	{
		Window *window(getWindow(tabAt(event->pos())));

		m_isIgnoringTabDrag = (count() == 1 || (window && window->isPinned() && m_pinnedTabsAmount == 1));

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

		QMouseEvent mouseEvent(QEvent::MouseButtonRelease, event->pos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());

		QApplication::sendEvent(this, &mouseEvent);

		m_isDetachingTab = true;

		updateGeometry();
		adjustSize();

		MainWindow *mainWindow(MainWindow::findMainWindow(this));

		if (mainWindow)
		{
			Window *window(mainWindow->getWindowsManager()->getWindowByIdentifier(m_draggedWindow));

			if (window)
			{
				QDrag *drag(new TabDrag(window->getIdentifier(), this));

				connect(drag, &QDrag::destroyed, [&]()
				{
					m_isDetachingTab = false;
				});

				QMimeData *mimeData(new QMimeData());
				mimeData->setText(window->getUrl().toString());
				mimeData->setUrls(QList<QUrl>({window->getUrl()}));
				mimeData->setProperty("x-url-title", window->getTitle());
				mimeData->setProperty("x-window-identifier", window->getIdentifier());

				const QPixmap thumbnail(window->getThumbnail());

				drag->setMimeData(mimeData);
				drag->setPixmap(thumbnail.isNull() ? window->getIcon().pixmap(16, 16) : thumbnail);
				drag->exec(Qt::CopyAction | Qt::MoveAction);
			}
		}

		return;
	}

	if (m_isIgnoringTabDrag || m_isDetachingTab)
	{
		return;
	}

	QTabBar::mouseMoveEvent(event);
}

void TabBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QTabBar::mouseReleaseEvent(event);

	if (event->button() == Qt::LeftButton)
	{
		if (m_isDetachingTab)
		{
			QVariantMap parameters;
			parameters[QLatin1String("window")] = m_draggedWindow;

			ActionsManager::triggerAction(ActionsManager::DetachTabAction, this, parameters);

			m_isDetachingTab = false;
		}

		m_dragStartPosition = QPoint();
		m_isDraggingTab = false;
	}
}

void TabBarWidget::wheelEvent(QWheelEvent *event)
{
	QWidget::wheelEvent(event);

	if (!(event->modifiers().testFlag(Qt::ControlModifier)) && SettingsManager::getValue(SettingsManager::TabBar_RequireModifierToSwitchTabOnScrollOption).toBool())
	{
		return;
	}

	if (event->delta() > 0)
	{
		activateTabOnLeft();
	}
	else
	{
		activateTabOnRight();
	}
}

void TabBarWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls() || (event->source() && !event->mimeData()->property("x-window-identifier").isNull()))
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

	m_dragMovePosition = QPoint();

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
				Window *window(getWindow(i));

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
				const QList<MainWindow*> mainWindows(SessionsManager::getWindows());

				for (int i = 0; i < mainWindows.count(); ++i)
				{
					if (mainWindows.at(i))
					{
						Window *window(mainWindows.at(i)->getWindowsManager()->getWindowByIdentifier(windowIdentifier));

						if (window)
						{
							mainWindows.at(i)->getWindowsManager()->moveWindow(window, mainWindow, dropIndex);

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
	else if (event->mimeData()->hasUrls())
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(this));
		bool canOpen(mainWindow != nullptr);

		if (canOpen)
		{
			const QList<QUrl> urls(event->mimeData()->urls());

			if (urls.count() > 1 && SettingsManager::getValue(SettingsManager::Choices_WarnOpenMultipleDroppedUrlsOption).toBool())
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

				SettingsManager::setValue(SettingsManager::Choices_WarnOpenMultipleDroppedUrlsOption, !messageBox.checkBox()->isChecked());
			}

			if (canOpen)
			{
				for (int i = 0; i < urls.count(); ++i)
				{
					mainWindow->getWindowsManager()->open(urls.at(i), WindowsManager::DefaultOpen, (dropIndex + i));
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

	m_dragMovePosition = QPoint();

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
		Window *window(getWindow(index));

		if (window)
		{
			QStatusTipEvent statusTipEvent(window->getUrl().toDisplayString());

			QApplication::sendEvent(this, &statusTipEvent);
		}
	}
}

void TabBarWidget::addTab(int index, Window *window)
{
	insertTab(index, QString());
	setTabButton(index, QTabBar::LeftSide, new TabHandleWidget(window, this));
	setTabButton(index, QTabBar::RightSide, nullptr);

	connect(window, SIGNAL(isPinnedChanged(bool)), this, SLOT(updatePinnedTabsAmount()));

	if (window->isPinned())
	{
		updatePinnedTabsAmount(window);
	}
}

void TabBarWidget::removeTab(int index)
{
	if (underMouse())
	{
		const QSize size(tabSizeHint(count() - 1));

		m_tabWidth = size.width();
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
		updateGeometry();
		adjustSize();
	}

	if (underMouse() && tabAt(mapFromGlobal(QCursor::pos())) < 0)
	{
		m_tabWidth = 0;

		updateGeometry();
		adjustSize();
	}
}

void TabBarWidget::activateTabOnLeft()
{
	setCurrentIndex((currentIndex() > 0) ? (currentIndex() - 1) : (count() - 1));
}

void TabBarWidget::activateTabOnRight()
{
	setCurrentIndex((currentIndex() + 1 < count()) ? (currentIndex() + 1) : 0);
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
		const QRect screen(QApplication::desktop()->screenGeometry(this));
		QRect rectangle(tabRect(index));
		rectangle.moveTo(mapToGlobal(rectangle.topLeft()));

		m_previewWidget->setPreview(window->getTitle(), ((index == currentIndex()) ? QPixmap() : window->getThumbnail()));

		switch (shape())
		{
			case QTabBar::RoundedEast:
				position = QPoint((rectangle.left() - m_previewWidget->width()), qMax(screen.top(), ((rectangle.bottom() - (rectangle.height() / 2)) - (m_previewWidget->height() / 2))));

				break;
			case QTabBar::RoundedWest:
				position = QPoint(rectangle.right(), qMax(screen.top(), ((rectangle.bottom() - (rectangle.height() / 2)) - (m_previewWidget->height() / 2))));

				break;
			case QTabBar::RoundedSouth:
				position = QPoint(qMax(screen.left(), ((rectangle.right() - (rectangle.width() / 2)) - (m_previewWidget->width() / 2))), (rectangle.top() - m_previewWidget->height()));

				break;
			default:
				position = QPoint(qMax(screen.left(), ((rectangle.right() - (rectangle.width() / 2)) - (m_previewWidget->width() / 2))), rectangle.bottom());

				break;
		}

		if ((position.x() + m_previewWidget->width()) > screen.right())
		{
			position.setX(screen.right() - m_previewWidget->width());
		}

		if ((position.y() + m_previewWidget->height()) > screen.bottom())
		{
			position.setY(screen.bottom() - m_previewWidget->height());
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

	if (m_previewTimer > 0)
	{
		killTimer(m_previewTimer);

		m_previewTimer = 0;
	}
}

void TabBarWidget::optionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
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

				updateGeometry();
				adjustSize();

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
					updateGeometry();
					adjustSize();
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
					updateGeometry();
					adjustSize();
				}
			}

			break;
		case SettingsManager::TabBar_MinimumTabHeightOption:
			{
				const int oldValue(m_minimumTabSize.height());

				m_minimumTabSize.setHeight(value.toInt());

				if (m_minimumTabSize.height() < 0)
				{
					m_minimumTabSize.setHeight((QFontMetrics(font()).height() * 1.25) + style()->pixelMetric(QStyle::PM_TabBarTabVSpace));
				}

				if (m_minimumTabSize.height() != oldValue)
				{
					updateGeometry();
					adjustSize();
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
					updateGeometry();
					adjustSize();
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

void TabBarWidget::updatePreviewPosition()
{
	if (m_previewWidget && m_previewWidget->isVisible())
	{
		showPreview(tabAt(mapFromGlobal(QCursor::pos())));
	}
}

void TabBarWidget::updatePinnedTabsAmount(Window *modifiedWindow)
{
	int amount(0);

	for (int i = 0; i < count(); ++i)
	{
		Window *window(getWindow(i));

		if (window && window->isPinned())
		{
			++amount;
		}
	}

	m_pinnedTabsAmount = amount;

	if (!modifiedWindow)
	{
		modifiedWindow = qobject_cast<Window*>(sender());
	}

	if (modifiedWindow)
	{
		int index(-1);

		for (int i = 0; i < count(); ++i)
		{
			if (getWindow(i) == modifiedWindow)
			{
				index = i;

				break;
			}
		}

		if (index >= 0)
		{
			moveTab(index, (modifiedWindow->isPinned() ? qMax(0, (m_pinnedTabsAmount - 1)) : m_pinnedTabsAmount));
			updateGeometry();
			adjustSize();
		}
	}
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
	if (index < 0 || index >= count())
	{
		return nullptr;
	}

	TabHandleWidget *widget(qobject_cast<TabHandleWidget*>(tabButton(index, QTabBar::LeftSide)));

	if (widget)
	{
		return widget->getWindow();
	}

	return nullptr;
}

QStyleOptionTab TabBarWidget::createStyleOptionTab(int index) const
{
	QStyleOptionTab tabOption;

	initStyleOption(&tabOption, index);

	QWidget *widget(tabButton(index, QTabBar::LeftSide));

	if (widget)
	{
		const QPoint position(widget->mapToParent(widget->rect().topLeft()));

		if (shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth)
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
	if (shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth)
	{
		Window *window(getWindow(index));
		const int tabHeight(qBound(m_minimumTabSize.height(), qMax((m_areThumbnailsEnabled ? 200 : 0), (parentWidget() ? parentWidget()->contentsRect().height() : height())), m_maximumTabSize.height()));

		if (window && window->isPinned())
		{
			return QSize(m_minimumTabSize.width(), tabHeight);
		}

		if (m_tabWidth > 0)
		{
			return QSize(m_tabWidth, tabHeight);
		}

		return QSize(qBound(m_minimumTabSize.width(), qFloor((rect().width() - (m_pinnedTabsAmount * m_minimumTabSize.width())) / qMax(1, (count() - m_pinnedTabsAmount))), m_maximumTabSize.width()), tabHeight);
	}

	return QSize(m_maximumTabSize.width(), (m_areThumbnailsEnabled ? 200 : m_minimumTabSize.height()));
}

QSize TabBarWidget::minimumSizeHint() const
{
	return QSize(0, 0);
}

QSize TabBarWidget::sizeHint() const
{
	if (shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth)
	{
		int size(0);

		for (int i = 0; i < count(); ++i)
		{
			Window *window(getWindow(i));

			size += ((window && window->isPinned()) ? m_minimumTabSize.width() : m_maximumTabSize.width());
		}

		if (parentWidget() && size > parentWidget()->width())
		{
			size = parentWidget()->width();
		}

		return QSize(size, tabSizeHint(0).height());
	}

	return QSize(QTabBar::sizeHint().width(), (tabSizeHint(0).height() * count()));
}

int TabBarWidget::getDropIndex() const
{
	if (m_dragMovePosition.isNull())
	{
		return ((count() > 0) ? (count() + 1) : 0);
	}

	int index(tabAt(m_dragMovePosition));
	const bool isHorizontal((shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth));

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

int TabBarWidget::getPinnedTabsAmount() const
{
	return m_pinnedTabsAmount;
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

bool TabBarWidget::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonDblClick:
		case QEvent::Wheel:
			{
				QVariantMap parameters;
				int tab(-1);

				if (event->type() == QEvent::Wheel)
				{
					QWheelEvent *wheelEvent(dynamic_cast<QWheelEvent*>(event));

					if (wheelEvent)
					{
						tab = tabAt(wheelEvent->pos());
					}
				}
				else
				{
					QMouseEvent *mouseEvent(dynamic_cast<QMouseEvent*>(event));

					if (mouseEvent)
					{
						tab = tabAt(mouseEvent->pos());
					}
				}

				if (tab >= 0)
				{
					Window *window(getWindow(tab));

					if (window)
					{
						parameters[QLatin1String("window")] = window->getIdentifier();
					}
				}

				QList<GesturesManager::GesturesContext> contexts;

				if (tab < 0)
				{
					contexts.append(GesturesManager::NoTabHandleGesturesContext);
				}
				else if (tab == currentIndex())
				{
					contexts.append(GesturesManager::ActiveTabHandleGesturesContext);
					contexts.append(GesturesManager::TabHandleGesturesContext);
				}
				else
				{
					contexts.append(GesturesManager::TabHandleGesturesContext);
				}

				if (qobject_cast<ToolBarWidget*>(parentWidget()))
				{
					contexts.append(GesturesManager::ToolBarGesturesContext);
				}

				contexts.append(GesturesManager::GenericGesturesContext);

				GesturesManager::startGesture(this, event, contexts, parameters);
			}

			break;
		default:
			break;
	}

	return QTabBar::event(event);
}

}
