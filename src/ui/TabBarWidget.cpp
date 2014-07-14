/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "PreviewWidget.h"
#include "Window.h"
#include "../core/ActionsManager.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"

#include <QtCore/QtMath>
#include <QtCore/QTimer>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMovie>
#include <QtGui/QStatusTipEvent>
#include <QtWidgets/QStyle>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>

namespace Otter
{

TabBarWidget::TabBarWidget(QWidget *parent) : QTabBar(parent),
	m_previewWidget(NULL),
	m_tabSize(0),
	m_pinnedTabsAmount(0),
	m_clickedTab(-1),
	m_hoveredTab(-1),
	m_previewTimer(0),
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

	m_closeButtonPosition = static_cast<QTabBar::ButtonPosition>(QApplication::style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition));
	m_iconButtonPosition = ((m_closeButtonPosition == QTabBar::RightSide) ? QTabBar::LeftSide : QTabBar::RightSide);

	optionChanged(QLatin1String("TabBar/ShowCloseButton"), SettingsManager::getValue(QLatin1String("TabBar/ShowCloseButton")));
	optionChanged(QLatin1String("TabBar/EnablePreviews"), SettingsManager::getValue(QLatin1String("TabBar/EnablePreviews")));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(this, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
	connect(this, SIGNAL(tabCloseRequested(int)), this, SIGNAL(requestedClose(int)));
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

void TabBarWidget::contextMenuEvent(QContextMenuEvent *event)
{
	m_clickedTab = tabAt(event->pos());

	hidePreview();

	QMenu menu(this);
	menu.addAction(ActionsManager::getAction(QLatin1String("NewTab"), this));
	menu.addAction(ActionsManager::getAction(QLatin1String("NewTabPrivate"), this));

	if (m_clickedTab >= 0)
	{
		const bool isPinned = getTabProperty(m_clickedTab, QLatin1String("isPinned"), false).toBool();

		menu.addAction(tr("Clone Tab"), this, SLOT(cloneTab()))->setEnabled(getTabProperty(m_clickedTab, QLatin1String("canClone"), false).toBool());
		menu.addAction((isPinned ? tr("Unpin Tab") : tr("Pin Tab")), this, SLOT(pinTab()));
		menu.addSeparator();
		menu.addAction(tr("Detach Tab"), this, SLOT(detachTab()))->setEnabled(count() > 1);
		menu.addSeparator();

		if (isPinned)
		{
			QAction *closeAction = menu.addAction(Utils::getIcon(QLatin1String("tab-close")), tr("Close Tab"));

			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("CloseTab"), this), closeAction, true);

			closeAction->setEnabled(false);
		}
		else
		{
			menu.addAction(ActionsManager::getAction(QLatin1String("CloseTab"), this));
		}

		const int amount = (count() - getPinnedTabsAmount());

		menu.addAction(Utils::getIcon(QLatin1String("tab-close-other")), tr("Close Other Tabs"), this, SLOT(closeOther()))->setEnabled(amount > 0 && !(amount == 1 && !isPinned));
	}

	menu.exec(event->globalPos());

	m_clickedTab = -1;

	if (underMouse())
	{
		m_previewTimer = startTimer(250);
	}
}

void TabBarWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
	{
		return;
	}

	const int tab = tabAt(event->pos());

	if (tab < 0)
	{
		ActionsManager::triggerAction(QLatin1String("NewTab"), this);
	}
	else if (SettingsManager::getValue(QLatin1String("TabBar/CloseOnDoubleClick")).toBool())
	{
		emit requestedClose(tab);
	}
}

void TabBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QTabBar::mouseReleaseEvent(event);

	if (event->button() == Qt::MidButton)
	{
		const int tab = tabAt(event->pos());

		if (tab < 0)
		{
			ActionsManager::triggerAction(QLatin1String("NewTab"), this);
		}
		else if (SettingsManager::getValue(QLatin1String("TabBar/CloseOnMiddleClick")).toBool())
		{
			emit requestedClose(tab);
		}
	}

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

void TabBarWidget::wheelEvent(QWheelEvent *event)
{
	QWidget::wheelEvent(event);

	if (!(event->modifiers() & Qt::ControlModifier))
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

void TabBarWidget::resizeEvent(QResizeEvent *event)
{
	QTabBar::resizeEvent(event);

	QTimer::singleShot(100, this, SLOT(updateTabs()));
}

void TabBarWidget::tabLayoutChange()
{
	QTabBar::tabLayoutChange();

	int offset = 0;
	const bool isHorizontal = (shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth);

	for (int i = 0; i < count(); ++i)
	{
		if (isHorizontal)
		{
			offset += tabSizeHint(i).width();
		}
		else
		{
			offset += tabSizeHint(i).height();
		}
	}

	emit moveNewTabButton(offset);

	updateButtons();
}

void TabBarWidget::tabInserted(int index)
{
	QTabBar::tabInserted(index);

	QLabel *label = new QLabel();
	label->setFixedSize(QSize(16, 16));

	setTabButton(index, m_iconButtonPosition, label);
	updateTabs();
}

void TabBarWidget::tabRemoved(int index)
{
	QTabBar::tabRemoved(index);

	QTimer::singleShot(100, this, SLOT(updateTabs()));
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
	setTabData(index, QVariant::fromValue(window));

	connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(updateTabs()));
	connect(window, SIGNAL(loadingStateChanged(WindowLoadingState)), this, SLOT(updateTabs()));
	connect(window, SIGNAL(isPinnedChanged(bool)), this, SLOT(updatePinnedTabsAmount()));
	connect(tabButton(index, QTabBar::LeftSide), SIGNAL(destroyed()), window, SLOT(deleteLater()));

	if (window->isPinned())
	{
		updatePinnedTabsAmount();
	}

	updateTabs(index);
}

void TabBarWidget::removeTab(int index)
{
	if (underMouse())
	{
		const bool isHorizontal = (shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth);
		const QSize size = tabSizeHint(count() - 1);

		m_tabSize = (isHorizontal ? size.width() : size.height());
	}

	QTabBar::removeTab(index);

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
	if (!m_enablePreviews || (parentWidget() && !parentWidget()->parentWidget()->parentWidget()->underMouse()))
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
		QRect rectangle = tabRect(index);
		rectangle.moveTo(mapToGlobal(rectangle.topLeft()));

		m_previewWidget->setPreview(getTabProperty(index, QLatin1String("title"), tr("(Untitled)")).toString(), ((index == currentIndex()) ? QPixmap() : getTabProperty(index, "thumbnail", QPixmap()).value<QPixmap>()));

		switch (shape())
		{
			case QTabBar::RoundedEast:
				position = QPoint((rectangle.left() - m_previewWidget->width()), qMax(0, ((rectangle.bottom() - (rectangle.height() / 2)) - (m_previewWidget->height() / 2))));

				break;
			case QTabBar::RoundedWest:
				position = QPoint(rectangle.right(), qMax(0, ((rectangle.bottom() - (rectangle.height() / 2)) - (m_previewWidget->height() / 2))));

				break;
			case QTabBar::RoundedSouth:
				position = QPoint(qMax(0, ((rectangle.right() - (rectangle.width() / 2)) - (m_previewWidget->width() / 2))), (rectangle.top() - m_previewWidget->height()));

				break;
			default:
				position = QPoint(qMax(0, ((rectangle.right() - (rectangle.width() / 2)) - (m_previewWidget->width() / 2))), rectangle.bottom());

				break;
		}

		const QRect screen = QApplication::desktop()->screenGeometry(this);

		if ((position.x() + m_previewWidget->width()) > screen.bottomRight().x())
		{
			position.setX(screen.width() - m_previewWidget->width());
		}

		if ((position.y() + m_previewWidget->height()) > screen.bottomRight().y())
		{
			position.setY(screen.height() - m_previewWidget->height());
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
		setTabsClosable(value.toBool());
	}
	else if (option == QLatin1String("TabBar/EnablePreviews"))
	{
		m_enablePreviews = value.toBool();
	}
}

void TabBarWidget::currentTabChanged(int index)
{
	Q_UNUSED(index)

	if (m_previewWidget && m_previewWidget->isVisible())
	{
		showPreview(tabAt(mapFromGlobal(QCursor::pos())));
	}

	if (tabsClosable())
	{
		updateButtons();
	}
}

void TabBarWidget::closeOther()
{
	if (m_clickedTab >= 0)
	{
		emit requestedCloseOther(m_clickedTab);
	}
}

void TabBarWidget::cloneTab()
{
	if (m_clickedTab >= 0)
	{
		emit requestedClone(m_clickedTab);
	}
}

void TabBarWidget::detachTab()
{
	if (m_clickedTab >= 0)
	{
		emit requestedDetach(m_clickedTab);
	}
}

void TabBarWidget::pinTab()
{
	if (m_clickedTab >= 0)
	{
		emit requestedPin(m_clickedTab, !getTabProperty(m_clickedTab, QLatin1String("isPinned"), false).toBool());
	}
}

void TabBarWidget::updatePinnedTabsAmount()
{
	int amount = 0;

	for (int i = 0; i < count(); ++i)
	{
		if (!getTabProperty(i, QLatin1String("isPinned"), false).toBool())
		{
			break;
		}

		++amount;
	}

	m_pinnedTabsAmount = amount;

	updateTabs();
}

void TabBarWidget::updateButtons()
{
	const QSize size = tabSizeHint(count() - 1);
	const bool isNarrow = (((shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth) ? size.width() : size.height()) < 60);

	for (int i = 0; i < count(); ++i)
	{
		QWidget *button = tabButton(i, m_closeButtonPosition);

		if (button)
		{
			button->setVisible((!isNarrow || (i == currentIndex())) && !getTabProperty(i, QLatin1String("isPinned"), false).toBool());
		}
	}
}

void TabBarWidget::updateTabs(int index)
{
	if (index < 0 && sender() && sender()->inherits(QStringLiteral("Otter::Window").toLatin1()))
	{
		for (int i = 0; i < count(); ++i)
		{
			if (sender() == qvariant_cast<QObject*>(tabData(i)))
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

void TabBarWidget::setOrientation(Qt::DockWidgetArea orientation)
{
	switch (orientation)
	{
		case Qt::LeftDockWidgetArea:
			setShape(QTabBar::RoundedWest);

			break;
		case Qt::RightDockWidgetArea:
			setShape(QTabBar::RoundedEast);

			break;
		case Qt::BottomDockWidgetArea:
			setShape(QTabBar::RoundedSouth);

			break;
		default:
			setShape(QTabBar::RoundedNorth);

			break;
	}

	QDockWidget *widget = qobject_cast<QDockWidget*>(parentWidget()->parentWidget());

	if (widget)
	{
		if (orientation == Qt::LeftDockWidgetArea || orientation == Qt::RightDockWidgetArea)
		{
			widget->setFeatures(widget->features() & ~QDockWidget::DockWidgetVerticalTitleBar);
		}
		else
		{
			widget->setFeatures(widget->features() | QDockWidget::DockWidgetVerticalTitleBar);
		}
	}

	QBoxLayout *layout = qobject_cast<QBoxLayout*>(parentWidget()->layout());

	if (layout)
	{
		if (orientation == Qt::LeftDockWidgetArea || orientation == Qt::RightDockWidgetArea)
		{
			layout->setDirection(QBoxLayout::TopToBottom);
		}
		else
		{
			layout->setDirection(QBoxLayout::LeftToRight);
		}
	}
}

void TabBarWidget::setShape(QTabBar::Shape shape)
{
	if (shape == QTabBar::RoundedNorth || shape == QTabBar::RoundedSouth)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

		parentWidget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	}
	else
	{
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

		parentWidget()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	}

	QTabBar::setShape(shape);

	QTimer::singleShot(100, this, SLOT(updateTabs()));
}

void TabBarWidget::setTabProperty(int index, const QString &key, const QVariant &value)
{
	if (index < 0 || index >= count())
	{
		return;
	}

	QObject *window = qvariant_cast<QObject*>(tabData(index));

	if (window)
	{
		window->setProperty(key.toLatin1(), value);
	}
}

QVariant TabBarWidget::getTabProperty(int index, const QString &key, const QVariant &defaultValue) const
{
	if (index < 0 || index >= count())
	{
		return defaultValue;
	}

	QObject *window = qvariant_cast<QObject*>(tabData(index));

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

	if (getTabProperty(index, QLatin1String("isPinned"), false).toBool())
	{
		if (isHorizontal)
		{
			return QSize(40, QTabBar::tabSizeHint(0).height());
		}

		return QSize(QTabBar::tabSizeHint(0).width(), 40);
	}

	const int amount = getPinnedTabsAmount();
	const int size = ((m_tabSize > 0) ? m_tabSize : qBound(40, qFloor(((isHorizontal ? geometry().width() : geometry().height()) - (amount * 40)) / qMax(1, (count() - amount))), 250));

	if (isHorizontal)
	{
		return QSize(size, QTabBar::tabSizeHint(0).height());
	}

	return QSize(QTabBar::tabSizeHint(0).width(), size);
}

int TabBarWidget::getPinnedTabsAmount() const
{
	return m_pinnedTabsAmount;
}

}
