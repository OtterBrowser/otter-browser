#include "TabBarWidget.h"
#include "PreviewWidget.h"
#include "../core/ActionsManager.h"
#include "../core/SettingsManager.h"

#include <QtCore/QTimer>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMovie>
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
	m_newTabButton(new QToolButton(this)),
	m_clickedTab(-1),
	m_previewTimer(0)
{
	setDrawBase(false);
	setExpanding(false);
	setMovable(true);
	setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
	setTabsClosable(true);
	setElideMode(Qt::ElideRight);
	setMouseTracking(true);
	updateTabs();

	m_newTabButton->setAutoRaise(true);
	m_newTabButton->setDefaultAction(ActionsManager::getAction("NewTab"));
	m_newTabButton->raise();

	connect(this, SIGNAL(tabCloseRequested(int)), this, SIGNAL(requestedClose(int)));
	connect(this, SIGNAL(currentChanged(int)), this, SLOT(updateTabs()));
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
	menu.addAction(ActionsManager::getAction("NewTab"));
	menu.addAction(ActionsManager::getAction("NewTabPrivate"));

	if (m_clickedTab >= 0)
	{
		const bool isPinned = getTabProperty(m_clickedTab, "isPinned", false).toBool();
		int amount = 0;

		for (int i = 0; i < count(); ++i)
		{
			if (getTabProperty(i, "isPinned", false).toBool() || i == m_clickedTab)
			{
				continue;
			}

			++amount;
		}

		menu.addAction(tr("Clone Tab"), this, SLOT(cloneTab()))->setEnabled(getTabProperty(m_clickedTab, "canClone", false).toBool());
		menu.addAction((isPinned ? tr("Unpin Tab") : tr("Pin Tab")), this, SLOT(pinTab()));
		menu.addSeparator();
		menu.addAction(tr("Detach Tab"), this, SLOT(detachTab()))->setEnabled(count() > 1);
		menu.addSeparator();

		if (isPinned)
		{
			QAction *closeAction = menu.addAction(QString());

			ActionsManager::setupLocalAction(closeAction, "CloseTab", true);

			closeAction->setEnabled(false);
		}
		else
		{
			menu.addAction(ActionsManager::getAction("CloseTab"));
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
		ActionsManager::triggerAction("NewTab");
	}
	else if (SettingsManager::getValue("Tabs/CloseOnDoubleClick").toBool())
	{
		emit requestedClose(tab);
	}
}

void TabBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MidButton && SettingsManager::getValue("Tabs/CloseOnMiddleClick").toBool())
	{
		const int tab = tabAt(event->pos());

		if (tab >= 0)
		{
			emit requestedClose(tab);
		}
	}

	QTabBar::mouseReleaseEvent(event);
}

void TabBarWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (m_previewWidget && !m_previewWidget->isVisible() && m_previewTimer == 0)
	{
		m_previewWidget->show();
	}

	if (m_previewWidget && m_previewWidget->isVisible())
	{
		showPreview(tabAt(event->pos()));
	}

	QTabBar::mouseMoveEvent(event);
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

	updateTabs();
}

void TabBarWidget::resizeEvent(QResizeEvent *event)
{
	QTabBar::resizeEvent(event);

	updateTabs();
}

void TabBarWidget::tabInserted(int index)
{
	QTabBar::tabInserted(index);
	QLabel *label = new QLabel();
	label->setFixedSize(QSize(16, 16));

	setTabButton(index, QTabBar::LeftSide, label);
	updateTabs();
}

void TabBarWidget::tabRemoved(int index)
{
	QTabBar::tabRemoved(index);

	updateTabs();
}

void TabBarWidget::tabLayoutChange()
{
	setUpdatesEnabled(false);

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

	if ((offset + (isHorizontal ? m_newTabButton->width() : m_newTabButton->height())) >= (isHorizontal ? width() : height()))
	{
		if (m_newTabButton->isVisible())
		{
			emit showNewTabButton(true);

			m_newTabButton->hide();
		}
	}
	else
	{
		if (isHorizontal)
		{
			m_newTabButton->move(offset, ((height() - m_newTabButton->height()) / 2));
		}
		else
		{
			m_newTabButton->move(((width() - m_newTabButton->width()) / 2), offset);
		}

		if (!m_newTabButton->isVisible())
		{
			emit showNewTabButton(false);

			m_newTabButton->show();
		}
	}

	setUpdatesEnabled(true);

	QTabBar::tabLayoutChange();
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

		m_previewWidget->setPreview(getTabProperty(index, "title", tr("(Untitled)")).toString().toHtmlEscaped(), ((index == currentIndex()) ? QPixmap() : getTabProperty(index, "thumbnail", QPixmap()).value<QPixmap>()));

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

		if ((position.x() + m_previewWidget->width()) > screen.width())
		{
			position.setX(screen.width() - m_previewWidget->width());
		}

		if ((position.y() + m_previewWidget->height()) > screen.height())
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
		emit requestedPin(m_clickedTab, !getTabProperty(m_clickedTab, "isPinned", false).toBool());
	}
}

void TabBarWidget::updateTabs(int index)
{
	if (index < 0 && sender() && sender()->inherits("Otter::Window"))
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

	const bool isHorizontal = (shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth);
	const QSize size = getTabSize(isHorizontal);
	const bool canResize = !underMouse();
	const bool isNarrow = ((isHorizontal ? size.width() : size.height()) < 60);

	if (canResize)
	{
		m_tabSize = size;

		updateGeometry();
	}

	const int limit = ((index >= 0) ? (index + 1) : count());

	for (int i = ((index >= 0) ? index : 0); i < limit; ++i)
	{
		const bool isLoading = getTabProperty(i, "isLoading", false).toBool();
		QLabel *label = qobject_cast<QLabel*>(tabButton(i, QTabBar::LeftSide));

		if (label)
		{
			if (isLoading)
			{
				if (!label->movie())
				{
					QMovie *movie = new QMovie(":/icons/loading.gif", QByteArray(), label);
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

				label->setPixmap(getTabProperty(i, "icon", QIcon(getTabProperty(i, "isPrivate", false).toBool() ? ":/icons/tab-private.png" : ":/icons/tab.png")).value<QIcon>().pixmap(16, 16));
			}
		}

		if (canResize)
		{
			QWidget *button = tabButton(i, QTabBar::RightSide);

			if (button)
			{
				button->setVisible((!isNarrow || (i == currentIndex())) && !getTabProperty(i, "isPinned", false).toBool());
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

	m_tabSize = getTabSize(shape == QTabBar::RoundedNorth || shape == QTabBar::RoundedSouth);

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
	if (getTabProperty(index, "isPinned", false).toBool())
	{
		QSize size = m_tabSize;

		if (shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth)
		{
			size.setWidth(40);
		}
		else
		{
			size.setHeight(40);
		}

		return size;
	}

	return m_tabSize;
}

QSize TabBarWidget::getTabSize(bool isHorizontal) const
{
	const int size = qBound(40, ((isHorizontal ? geometry().width() : geometry().height()) / ((count() == 0) ? 1 : count())), 250);

	if (isHorizontal)
	{
		return QSize(size, QTabBar::tabSizeHint(0).height());
	}

	return QSize(QTabBar::tabSizeHint(0).width(), size);
}

}
