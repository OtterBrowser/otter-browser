/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "MainWindow.h"
#include "PreviewWidget.h"
#include "TabBarStyle.h"
#include "ToolBarWidget.h"
#include "Window.h"
#include "../core/ActionsManager.h"
#include "../core/GesturesManager.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"

#include <QtCore/QtMath>
#include <QtCore/QTimer>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMovie>
#include <QtGui/QPainter>
#include <QtGui/QStatusTipEvent>
#include <QtWidgets/QStyle>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QToolTip>

namespace Otter
{

TabBarWidget::TabBarWidget(QWidget *parent) : QTabBar(parent),
	m_previewWidget(NULL),
	m_tabSize(0),
	m_maximumTabSize(40),
	m_minimumTabSize(250),
	m_pinnedTabsAmount(0),
	m_clickedTab(-1),
	m_hoveredTab(-1),
	m_previewTimer(0),
	m_showCloseButton(true),
	m_showUrlIcon(true),
	m_enablePreviews(true)
{
	qRegisterMetaType<WindowLoadingState>("WindowLoadingState");
	setDrawBase(false);
	setExpanding(false);
	setMovable(true);
	setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
	setElideMode(Qt::ElideRight);
	setMouseTracking(true);
	setDocumentMode(true);
	setMaximumSize(0, 0);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setStyle(new TabBarStyle());

	m_closeButtonPosition = static_cast<QTabBar::ButtonPosition>(QApplication::style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition));
	m_iconButtonPosition = ((m_closeButtonPosition == QTabBar::RightSide) ? QTabBar::LeftSide : QTabBar::RightSide);

	optionChanged(QLatin1String("TabBar/ShowCloseButton"), SettingsManager::getValue(QLatin1String("TabBar/ShowCloseButton")));
	optionChanged(QLatin1String("TabBar/ShowUrlIcon"), SettingsManager::getValue(QLatin1String("TabBar/ShowUrlIcon")));
	optionChanged(QLatin1String("TabBar/EnablePreviews"), SettingsManager::getValue(QLatin1String("TabBar/EnablePreviews")));
	optionChanged(QLatin1String("TabBar/MaximumTabSize"), SettingsManager::getValue(QLatin1String("TabBar/MaximumTabSize")));
	optionChanged(QLatin1String("TabBar/MinimumTabSize"), SettingsManager::getValue(QLatin1String("TabBar/MinimumTabSize")));

	ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parent);

	if (toolBar)
	{
		setArea(toolBar->getArea());

		connect(toolBar, SIGNAL(areaChanged(Qt::ToolBarArea)), this, SLOT(setArea(Qt::ToolBarArea)));
	}

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(this, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
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

void TabBarWidget::resizeEvent(QResizeEvent *event)
{
	QTabBar::resizeEvent(event);

	QTimer::singleShot(100, this, SLOT(updateTabs()));
}

void TabBarWidget::enterEvent(QEvent *event)
{
	QTabBar::enterEvent(event);

	m_previewTimer = startTimer(250);
}

void TabBarWidget::leaveEvent(QEvent *event)
{
	QTabBar::leaveEvent(event);

	hidePreview();

	m_tabSize = 0;
	m_hoveredTab = -1;

	QStatusTipEvent statusTipEvent((QString()));

	QApplication::sendEvent(this, &statusTipEvent);

	updateGeometry();
	adjustSize();
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

	MainWindow *mainWindow = MainWindow::findMainWindow(this);
	QVariantMap parameters;
	QMenu menu(this);
	menu.addAction(ActionsManager::getAction(ActionsManager::NewTabAction, this));
	menu.addAction(ActionsManager::getAction(ActionsManager::NewTabPrivateAction, this));

	if (m_clickedTab >= 0)
	{
		Window *window = getWindow(m_clickedTab);

		if (window)
		{
			parameters[QLatin1String("window")] = window->getIdentifier();
		}

		const int amount = (count() - getPinnedTabsAmount());
		const bool isPinned = getTabProperty(m_clickedTab, QLatin1String("isPinned"), false).toBool();
		Action *cloneTabAction = new Action(ActionsManager::CloneTabAction, &menu);
		cloneTabAction->setEnabled(getTabProperty(m_clickedTab, QLatin1String("canClone"), false).toBool());
		cloneTabAction->setData(parameters);

		Action *pinTabAction = new Action(ActionsManager::PinTabAction, &menu);
		pinTabAction->setOverrideText(isPinned ? QT_TRANSLATE_NOOP("actions", "Unpin Tab") : QT_TRANSLATE_NOOP("actions", "Pin Tab"));
		pinTabAction->setData(parameters);

		Action *detachTabAction = new Action(ActionsManager::DetachTabAction, &menu);
		detachTabAction->setEnabled(count() > 1);
		detachTabAction->setData(parameters);

		Action *closeTabAction = new Action(ActionsManager::CloseTabAction, &menu);
		closeTabAction->setEnabled(!isPinned);
		closeTabAction->setData(parameters);

		Action *closeOtherTabsAction = new Action(ActionsManager::CloseOtherTabsAction, &menu);
		closeOtherTabsAction->setEnabled(amount > 0 && !(amount == 1 && !isPinned));
		closeOtherTabsAction->setData(parameters);

		menu.addAction(cloneTabAction);
		menu.addAction(pinTabAction);
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

	menu.addSeparator();

	QMenu *arrangeMenu = menu.addMenu(tr("Arrange"));
	Action *restoreTabAction = new Action(ActionsManager::RestoreTabAction, &menu);
	restoreTabAction->setEnabled(m_clickedTab >= 0);
	restoreTabAction->setData(parameters);

	Action *minimizeTabAction = new Action(ActionsManager::MinimizeTabAction, &menu);
	minimizeTabAction->setEnabled(m_clickedTab >= 0);
	minimizeTabAction->setData(parameters);

	Action *maximizeTabAction = new Action(ActionsManager::MaximizeTabAction, &menu);
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

	QAction *cycleAction = new QAction(tr("Switch tabs using the mouse wheel"), this);
	cycleAction->setCheckable(true);
	cycleAction->setChecked(!SettingsManager::getValue(QLatin1String("TabBar/RequireModifierToSwitchTabOnScroll")).toBool());

	connect(cycleAction, SIGNAL(toggled(bool)), this, SLOT(setCycle(bool)));
	connect(restoreTabAction, SIGNAL(triggered()), mainWindow, SLOT(triggerAction()));
	connect(minimizeTabAction, SIGNAL(triggered()), mainWindow, SLOT(triggerAction()));
	connect(maximizeTabAction, SIGNAL(triggered()), mainWindow, SLOT(triggerAction()));

	ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parentWidget());

	if (toolBar)
	{
		QList<QAction*> actions;
		actions.append(cycleAction);

		menu.addMenu(ToolBarWidget::createCustomizationMenu(ToolBarsManager::TabBar, actions, &menu));
	}
	else
	{
		QMenu *customizationMenu = menu.addMenu(tr("Customize"));
		customizationMenu->addAction(cycleAction);
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

	hidePreview();
}

void TabBarWidget::mouseMoveEvent(QMouseEvent *event)
{
	QTabBar::mouseMoveEvent(event);

	if (event->buttons() == Qt::NoButton)
	{
		tabHovered(tabAt(event->pos()));
	}
}

void TabBarWidget::wheelEvent(QWheelEvent *event)
{
	QWidget::wheelEvent(event);

	if (!(event->modifiers().testFlag(Qt::ControlModifier)) && SettingsManager::getValue(QLatin1String("TabBar/RequireModifierToSwitchTabOnScroll")).toBool())
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

void TabBarWidget::tabLayoutChange()
{
	QTabBar::tabLayoutChange();

	updateButtons();

	emit layoutChanged();
}

void TabBarWidget::tabInserted(int index)
{
	setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

	QTabBar::tabInserted(index);

	if (m_showUrlIcon)
	{
		QLabel *label = new QLabel();
		label->setFixedSize(QSize(16, 16));

		setTabButton(index, m_iconButtonPosition, label);
	}
	else
	{
		setTabButton(index, m_iconButtonPosition, NULL);
	}

	if (m_showCloseButton || getTabProperty(index, QLatin1String("isPinned"), false).toBool())
	{
		QLabel *label = new QLabel();
		label->setFixedSize(QSize(16, 16));
		label->installEventFilter(this);

		setTabButton(index, m_closeButtonPosition, label);
	}

	updateTabs();

	emit tabsAmountChanged(count());
}

void TabBarWidget::tabRemoved(int index)
{
	QTabBar::tabRemoved(index);

	if (count() == 0)
	{
		setMaximumSize(0, 0);
	}
	else
	{
		QTimer::singleShot(100, this, SLOT(updateTabs()));
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

	QStatusTipEvent statusTipEvent((index >= 0) ? getTabProperty(index, QLatin1String("url"), QUrl()).toString() : QString());

	QApplication::sendEvent(this, &statusTipEvent);

	if (m_previewWidget && m_previewWidget->isVisible())
	{
		showPreview(index);
	}
}

void TabBarWidget::addTab(int index, Window *window)
{
	insertTab(index, window->getTitle());
	setTabData(index, window->getIdentifier());

	connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(updateTabs()));
	connect(window, SIGNAL(loadingStateChanged(WindowLoadingState)), this, SLOT(updateTabs()));
	connect(window, SIGNAL(isPinnedChanged(bool)), this, SLOT(isPinnedChanged()));

	if (window->isPinned())
	{
		isPinnedChanged(window);
	}

	updateTabs(index);
}

void TabBarWidget::removeTab(int index)
{
	if (underMouse())
	{
		const QSize size = tabSizeHint(count() - 1);

		m_tabSize = size.width();
	}

	Window *window = getWindow(index);

	if (window)
	{
		window->deleteLater();
	}

	QTabBar::removeTab(index);

	if (window && window->isPinned())
	{
		isPinnedChanged();
		updateGeometry();
		adjustSize();
	}

	if (underMouse() && tabAt(mapFromGlobal(QCursor::pos())) < 0)
	{
		m_tabSize = 0;

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

void TabBarWidget::showPreview(int index)
{
	if (!m_enablePreviews || (window() && !window()->underMouse()))
	{
		hidePreview();

		return;
	}

	if (index >= 0 && m_clickedTab < 0)
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
		const QRect screen = QApplication::desktop()->screenGeometry(this);
		QRect rectangle = tabRect(index);
		rectangle.moveTo(mapToGlobal(rectangle.topLeft()));

		m_previewWidget->setPreview(getTabProperty(index, QLatin1String("title"), tr("(Untitled)")).toString(), ((index == currentIndex()) ? QPixmap() : getTabProperty(index, QLatin1String("thumbnail"), QPixmap()).value<QPixmap>()));

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

void TabBarWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("TabBar/ShowCloseButton"))
	{
		const bool showCloseButton = value.toBool();

		if (showCloseButton != m_showCloseButton)
		{
			for (int i = 0; i < count(); ++i)
			{
				if (showCloseButton)
				{
					QLabel *label = new QLabel();
					label->setFixedSize(QSize(16, 16));

					setTabButton(i, m_closeButtonPosition, label);
				}
				else
				{
					setTabButton(i, m_closeButtonPosition, NULL);
				}
			}

			updateTabs();
		}

		m_showCloseButton = showCloseButton;
	}
	else if (option == QLatin1String("TabBar/ShowUrlIcon"))
	{
		const bool showUrlIcon = value.toBool();

		if (showUrlIcon != m_showUrlIcon)
		{
			for (int i = 0; i < count(); ++i)
			{
				if (showUrlIcon)
				{
					QLabel *label = new QLabel();
					label->setFixedSize(QSize(16, 16));

					setTabButton(i, m_iconButtonPosition, label);
				}
				else
				{
					setTabButton(i, m_iconButtonPosition, NULL);
				}
			}

			updateTabs();
		}

		m_showUrlIcon = showUrlIcon;
	}
	else if (option == QLatin1String("TabBar/EnablePreviews"))
	{
		m_enablePreviews = value.toBool();
	}
	else if (option == QLatin1String("TabBar/MaximumTabSize"))
	{
		const int oldValue = m_maximumTabSize;

		m_maximumTabSize = value.toInt();

		if (m_maximumTabSize < 0)
		{
			m_maximumTabSize = 250;
		}

		if (m_maximumTabSize != oldValue)
		{
			updateGeometry();
			updateTabs();
		}
	}
	else if (option == QLatin1String("TabBar/MinimumTabSize") && value.toInt() != m_minimumTabSize)
	{
		const int oldValue = m_minimumTabSize;

		m_minimumTabSize = value.toInt();

		if (m_minimumTabSize < 0)
		{
			m_minimumTabSize = 40;
		}

		if (m_minimumTabSize != oldValue)
		{
			updateGeometry();
			updateTabs();
		}
	}
}

void TabBarWidget::currentTabChanged(int index)
{
	Q_UNUSED(index)

	if (m_previewWidget && m_previewWidget->isVisible())
	{
		showPreview(tabAt(mapFromGlobal(QCursor::pos())));
	}

	if (m_showCloseButton)
	{
		updateButtons();
	}
}

void TabBarWidget::isPinnedChanged(Window *window)
{
	int amount = 0;

	for (int i = 0; i < count(); ++i)
	{
		if (getTabProperty(i, QLatin1String("isPinned"), false).toBool())
		{
			++amount;
		}
	}

	m_pinnedTabsAmount = amount;

	if (!window)
	{
		window = qobject_cast<Window*>(sender());
	}

	if (window)
	{
		int index = -1;

		for (int i = 0; i < count(); ++i)
		{
			if (tabData(i).toULongLong() == window->getIdentifier())
			{
				index = i;

				break;
			}
		}

		if (index >= 0)
		{
			moveTab(index, (window->isPinned() ? qMax(0, (m_pinnedTabsAmount - 1)) : m_pinnedTabsAmount));
			updateButtons();
			updateGeometry();
			adjustSize();
		}
	}

	updateTabs();
}

void TabBarWidget::updateButtons()
{
	const QSize size = tabSizeHint(count() - 1);
	const bool isVertical = (shape() == QTabBar::RoundedWest || shape() == QTabBar::RoundedEast);
	const bool isNarrow = (size.width() < 60);

	for (int i = 0; i < count(); ++i)
	{
		const bool isCurrent = (i == currentIndex());

		QLabel *closeLabel = qobject_cast<QLabel*>(tabButton(i, m_closeButtonPosition));
		QLabel *iconLabel = qobject_cast<QLabel*>(tabButton(i, m_iconButtonPosition));

		if (iconLabel)
		{
			iconLabel->setVisible(!isNarrow || !(isCurrent && closeLabel));
		}

		if (!closeLabel)
		{
			continue;
		}

		const bool isPinned = getTabProperty(i, QLatin1String("isPinned"), false).toBool();
		const bool wasPinned = closeLabel->property("isPinned").toBool();

		if (!closeLabel->buddy())
		{
			Window *window = getWindow(i);

			if (window)
			{
				closeLabel->setBuddy(window);
			}
		}

		if (isPinned != wasPinned || !closeLabel->pixmap())
		{
			closeLabel->setProperty("isPinned", isPinned);

			if (isPinned)
			{
				closeLabel->setPixmap(Utils::getIcon(QLatin1String("object-locked")).pixmap(16, 16));
			}
			else
			{
				QStyleOption option;
				option.rect = QRect(0, 0, 16, 16);

				QPixmap pixmap(QSize(16, 16) * devicePixelRatio());
				pixmap.setDevicePixelRatio(devicePixelRatio());
				pixmap.fill(Qt::transparent);

				QPainter painter(&pixmap);

				style()->drawPrimitive(QStyle::PE_IndicatorTabClose, &option, &painter, this);

				closeLabel->setPixmap(pixmap);
			}
		}

		closeLabel->setVisible((!isNarrow || isCurrent) && (isVertical || !isPinned));
	}
}

void TabBarWidget::updateTabs(int index)
{
	if (index < 0 && sender() && sender()->inherits(QStringLiteral("Otter::Window").toLatin1()))
	{
		for (int i = 0; i < count(); ++i)
		{
			if (sender() == getWindow(i))
			{
				index = i;

				break;
			}
		}
	}

	const int limit = ((index >= 0) ? (index + 1) : count());

	for (int i = ((index >= 0) ? index : 0); i < limit; ++i)
	{
		const WindowLoadingState loadingState = static_cast<WindowLoadingState>(getTabProperty(i, QLatin1String("loadingState"), LoadedState).toInt());
		QLabel *label = qobject_cast<QLabel*>(tabButton(i, m_iconButtonPosition));

		if (label)
		{
			if (loadingState != LoadedState)
			{
				if (!label->movie())
				{
					QMovie *movie = new QMovie(QLatin1String(":/icons/loading.gif"), QByteArray(), label);
					movie->start();

					label->setMovie(movie);
				}

				label->movie()->setSpeed((loadingState == LoadingState) ? 100 : 10);
			}
			else
			{
				if (label->movie())
				{
					label->movie()->deleteLater();
					label->setMovie(NULL);
				}

				label->setPixmap(getTabProperty(i, QLatin1String("icon"), Utils::getIcon(getTabProperty(i, QLatin1String("isPrivate"), false).toBool() ? QLatin1String("tab-private") : QLatin1String("tab"))).value<QIcon>().pixmap(16, 16));
			}
		}
	}

	m_hoveredTab = -1;

	tabHovered(tabAt(mapFromGlobal(QCursor::pos())));
}

void TabBarWidget::setCycle(bool enable)
{
	SettingsManager::setValue(QLatin1String("TabBar/RequireModifierToSwitchTabOnScroll"), !enable);
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
}

void TabBarWidget::setShape(QTabBar::Shape shape)
{
	QTabBar::setShape(shape);

	QTimer::singleShot(100, this, SLOT(updateTabs()));
}

Window* TabBarWidget::getWindow(int index) const
{
	if (index < 0 || index >= count())
	{
		return NULL;
	}

	MainWindow *mainWindow = MainWindow::findMainWindow(parentWidget());

	if (mainWindow)
	{
		return mainWindow->getWindowsManager()->getWindowByIdentifier(tabData(index).toULongLong());
	}

	return NULL;
}

QVariant TabBarWidget::getTabProperty(int index, const QString &key, const QVariant &defaultValue) const
{
	if (index < 0 || index >= count())
	{
		return defaultValue;
	}

	Window *window = getWindow(index);

	if (window)
	{
		const QVariant value = window->property(key.toLatin1());

		if (!value.isNull())
		{
			return value;
		}
	}

	return defaultValue;
}

QSize TabBarWidget::tabSizeHint(int index) const
{
	const bool isHorizontal = (shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth);

	if (isHorizontal && getTabProperty(index, QLatin1String("isPinned"), false).toBool())
	{
		return QSize(m_minimumTabSize, QTabBar::tabSizeHint(0).height());
	}

	if (isHorizontal)
	{
		const int amount = getPinnedTabsAmount();

		return QSize(((m_tabSize > 0) ? m_tabSize : qBound(m_minimumTabSize, qFloor(((isHorizontal ? geometry().width() : geometry().height()) - (amount * m_minimumTabSize)) / qMax(1, (count() - amount))), m_maximumTabSize)), QTabBar::tabSizeHint(0).height());
	}

	return QSize(m_maximumTabSize, QTabBar::tabSizeHint(0).height());
}

QSize TabBarWidget::minimumSizeHint() const
{
	return QSize(0, 0);
}

QSize TabBarWidget::sizeHint() const
{
	if (shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth)
	{
		int size = 0;

		for (int i = 0; i < count(); ++i)
		{
			size += (getTabProperty(i, QLatin1String("isPinned"), false).toBool() ? m_minimumTabSize : m_maximumTabSize);
		}

		return QSize(size, QTabBar::sizeHint().height());
	}

	return QSize(QTabBar::sizeHint().width(), (tabSizeHint(0).height() * count()));
}

int TabBarWidget::getPinnedTabsAmount() const
{
	return m_pinnedTabsAmount;
}

bool TabBarWidget::event(QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::Wheel)
	{
		QVariantMap parameters;
		int tab = -1;

		if (event->type() == QEvent::Wheel)
		{
			QWheelEvent *wheelEvent = dynamic_cast<QWheelEvent*>(event);

			if (wheelEvent)
			{
				tab = tabAt(wheelEvent->pos());
			}
		}
		else
		{
			QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(event);

			if (mouseEvent)
			{
				tab = tabAt(mouseEvent->pos());
			}
		}

		if (tab >= 0)
		{
			Window *window = getWindow(tab);

			if (window)
			{
				parameters[QLatin1String("window")] = window->getIdentifier();
			}
		}

		QList<GesturesManager::GesturesContext> contexts;

		if (tab < 0)
		{
			contexts << GesturesManager::NoTabHandleGesturesContext;
		}
		else if (tab == currentIndex())
		{
			contexts << GesturesManager::ActiveTabHandleGesturesContext << GesturesManager::TabHandleGesturesContext;
		}
		else
		{
			contexts << GesturesManager::TabHandleGesturesContext;
		}

		GesturesManager::startGesture(this, event, contexts, parameters);
	}

	return QTabBar::event(event);
}

bool TabBarWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::Enter && !object->property("isPinned").toBool())
	{
		hidePreview();
	}
	else if (event->type() == QEvent::Leave)
	{
		m_previewTimer = startTimer(250);
	}
	else if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = dynamic_cast<QHelpEvent*>(event);

		if (helpEvent && !object->property("isPinned").toBool())
		{
			const QVector<QKeySequence> shortcuts = ActionsManager::getActionDefinition(ActionsManager::CloseTabAction).shortcuts;

			QToolTip::showText(helpEvent->globalPos(), tr("Close Tab") + (shortcuts.isEmpty() ? QString() : QLatin1String(" (") + shortcuts.at(0).toString(QKeySequence::NativeText) + QLatin1Char(')')));
		}

		return true;
	}
	else if (event->type() == QEvent::MouseButtonPress && !object->property("isPinned").toBool())
	{
		return true;
	}
	else if (event->type() == QEvent::MouseButtonRelease && !object->property("isPinned").toBool())
	{
		QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(event);
		QLabel *label = qobject_cast<QLabel*>(object);

		if (label && mouseEvent && mouseEvent->button() == Qt::LeftButton)
		{
			Window *window = qobject_cast<Window*>(label->buddy());

			if (window)
			{
				window->close();

				return true;
			}
		}
	}

	return QTabBar::eventFilter(object, event);
}

}
