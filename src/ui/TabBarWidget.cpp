/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../core/ActionsManager.h"
#include "../core/SettingsManager.h"

#include <QtCore/QtMath>
#include <QtCore/QTimer>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMovie>
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
	m_clickedTab(-1),
	m_previewTimer(0)
{
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

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(this, SIGNAL(tabCloseRequested(int)), this, SIGNAL(requestedClose(int)));
	connect(this, SIGNAL(currentChanged(int)), this, SLOT(updateButtons()));
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

	if (m_previewWidget && m_previewWidget->isVisible())
	{
		m_previewWidget->hide();
	}

	if (m_previewTimer > 0)
	{
		killTimer(m_previewTimer);

		m_previewTimer = 0;
	}

	QMenu menu(this);
	menu.addAction(ActionsManager::getAction(QLatin1String("NewTab")));
	menu.addAction(ActionsManager::getAction(QLatin1String("NewTabPrivate")));

	if (m_clickedTab >= 0)
	{
		const bool isPinned = getTabProperty(m_clickedTab, QLatin1String("isPinned"), false).toBool();
		int amount = 0;

		for (int i = 0; i < count(); ++i)
		{
			if (getTabProperty(i, QLatin1String("isPinned"), false).toBool() || i == m_clickedTab)
			{
				continue;
			}

			++amount;
		}

		menu.addAction(tr("Clone Tab"), this, SLOT(cloneTab()))->setEnabled(getTabProperty(m_clickedTab, QLatin1String("canClone"), false).toBool());
		menu.addAction((isPinned ? tr("Unpin Tab") : tr("Pin Tab")), this, SLOT(pinTab()));
		menu.addSeparator();
		menu.addAction(tr("Detach Tab"), this, SLOT(detachTab()))->setEnabled(count() > 1);
		menu.addSeparator();

		if (isPinned)
		{
			QAction *closeAction = menu.addAction(QString());

			ActionsManager::setupLocalAction(closeAction, QLatin1String("CloseTab"), true);

			closeAction->setEnabled(false);
		}
		else
		{
			menu.addAction(ActionsManager::getAction(QLatin1String("CloseTab")));
		}

		menu.addAction(QIcon(":/icons/tab-close-other.png"), tr("Close Other Tabs"), this, SLOT(closeOther()))->setEnabled(amount > 0);
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
	const int tab = tabAt(event->pos());

	if (tab < 0)
	{
		ActionsManager::triggerAction(QLatin1String("NewTab"));
	}
	else if (SettingsManager::getValue(QLatin1String("TabBar/CloseOnDoubleClick")).toBool())
	{
		emit requestedClose(tab);
	}
}

void TabBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QTabBar::mouseReleaseEvent(event);

	if (event->button() == Qt::MidButton && SettingsManager::getValue(QLatin1String("TabBar/CloseOnMiddleClick")).toBool())
	{
		const int tab = tabAt(event->pos());

		if (tab >= 0)
		{
			emit requestedClose(tab);
		}
	}
}

void TabBarWidget::mouseMoveEvent(QMouseEvent *event)
{
	QTabBar::mouseMoveEvent(event);

	if (m_previewWidget && !m_previewWidget->isVisible() && m_previewTimer == 0)
	{
		m_previewWidget->show();
	}

	if (m_previewWidget && m_previewWidget->isVisible())
	{
		showPreview(tabAt(event->pos()));
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

	if (m_previewTimer != 0)
	{
		killTimer(m_previewTimer);

		m_previewTimer = 0;
	}

	if (m_previewWidget)
	{
		m_previewWidget->hide();
	}

	m_tabSize = 0;

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

	const int index = (currentIndex() + (event->delta() > 0 ? -1 : 1));

	if (index < 0)
	{
		setCurrentIndex(count() - 1);
	}
	else if (index >= count())
	{
		setCurrentIndex(0);
	}
	else
	{
		setCurrentIndex(index);
	}
}

void TabBarWidget::resizeEvent(QResizeEvent *event)
{
	QTabBar::resizeEvent(event);

	QTimer::singleShot(100, this, SLOT(updateTabs()));
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

void TabBarWidget::showPreview(int index)
{
	if (index >= 0 && m_clickedTab < 0)
	{
		if (!m_previewWidget)
		{
			m_previewWidget = new PreviewWidget(this);
		}

		QPoint position;
		QRect rectangle = tabRect(index);
		rectangle.moveTo(mapToGlobal(rectangle.topLeft()));

		m_previewWidget->setPreview(getTabProperty(index, QLatin1String("title"), tr("(Untitled)")).toString().toHtmlEscaped(), ((index == currentIndex()) ? QPixmap() : getTabProperty(index, "thumbnail", QPixmap()).value<QPixmap>()));

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

void TabBarWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("TabBar/ShowCloseButton"))
	{
		setTabsClosable(value.toBool());
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
		const bool isLoading = getTabProperty(i, QLatin1String("isLoading"), false).toBool();
		QLabel *label = qobject_cast<QLabel*>(tabButton(i, m_iconButtonPosition));

		if (label)
		{
			if (isLoading)
			{
				if (!label->movie())
				{
					QMovie *movie = new QMovie(QLatin1String(":/icons/loading.gif"), QByteArray(), label);
					movie->start();

					label->setMovie(movie);
				}
			}
			else
			{
				if (label->movie())
				{
					label->movie()->deleteLater();
					label->setMovie(NULL);
				}

				label->setPixmap(getTabProperty(i, QLatin1String("icon"), QIcon(getTabProperty(i, QLatin1String("isPrivate"), false).toBool() ? ":/icons/tab-private.png" : ":/icons/tab.png")).value<QIcon>().pixmap(16, 16));
			}
		}
	}

	showPreview(tabAt(mapFromGlobal(QCursor::pos())));
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

	const int size = ((m_tabSize > 0) ? m_tabSize : qBound(40, qFloor((isHorizontal ? geometry().width() : geometry().height()) / qMax(1, count())), 250));

	if (isHorizontal)
	{
		return QSize(size, QTabBar::tabSizeHint(0).height());
	}

	return QSize(QTabBar::tabSizeHint(0).width(), size);
}

}
