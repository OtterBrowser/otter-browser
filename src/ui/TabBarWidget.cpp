#include "TabBarWidget.h"
#include "../core/ActionsManager.h"

#include <QtCore/QTimer>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMovie>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>

namespace Otter
{

TabBarWidget::TabBarWidget(QWidget *parent) : QTabBar(parent),
	m_clickedTab(-1)
{
	setDrawBase(false);
	setExpanding(false);
	setMovable(true);
	setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
	setTabsClosable(true);
	setElideMode(Qt::ElideRight);
	updateTabs();

	connect(this, SIGNAL(tabCloseRequested(int)), this, SIGNAL(requestedClose(int)));
	connect(this, SIGNAL(currentChanged(int)), this, SLOT(updateTabs()));
}

void TabBarWidget::contextMenuEvent(QContextMenuEvent *event)
{
	m_clickedTab = tabAt(event->pos());

	QMenu menu(this);
	menu.addAction(ActionsManager::getAction("NewTab"));
	menu.addAction(ActionsManager::getAction("NewTabPrivate"));

	if (m_clickedTab >= 0)
	{
		const bool isPinned = tabData(m_clickedTab).toHash().value("pinned", false).toBool();
		int amount = 0;

		for (int i = 0; i < count(); ++i)
		{
			if (tabData(i).toHash().value("pinned", false).toBool() || i == m_clickedTab)
			{
				continue;
			}

			++amount;
		}

		menu.addAction((isPinned ? tr("Unpin Tab") : tr("Pin Tab")), this, SLOT(pinTab()));
		menu.addSeparator();

		if (isPinned)
		{
			QAction *globalCloseAction = ActionsManager::getAction("CloseTab");
			QAction *closeAction = menu.addAction(globalCloseAction->icon(), globalCloseAction->text());
			closeAction->setShortcut(globalCloseAction->shortcut());
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
}

void TabBarWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (tabAt(event->pos()) < 0)
	{
		ActionsManager::triggerAction("NewTab");
	}
}

void TabBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MidButton)
	{
		const int tab = tabAt(event->pos());

		if (tab >= 0)
		{
			emit requestedClose(tab);
		}
	}

	QTabBar::mouseReleaseEvent(event);
}

void TabBarWidget::leaveEvent(QEvent *event)
{
	QTabBar::leaveEvent(event);

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

void TabBarWidget::closeOther()
{
	if (m_clickedTab >= 0)
	{
		emit requestedCloseOther(m_clickedTab);
	}
}

void TabBarWidget::pinTab()
{
	emit requestedPin(m_clickedTab, !tabData(m_clickedTab).toHash().value("pinned", false).toBool());
}

void TabBarWidget::updateTabs(int index)
{
	const bool isHorizontal = (shape() == QTabBar::RoundedNorth || shape() == QTabBar::RoundedSouth);
	const QSize size = getTabSize(isHorizontal);
	const bool canResize = !underMouse();
	const bool isNarrow = ((isHorizontal ? size.width() : size.height()) < 60);

	if (canResize)
	{
		if (isNarrow)
		{
			setStyleSheet("QTabBar::tab {color:transparent;}");
		}
		else
		{
			setStyleSheet(QString());
		}

		m_tabSize = size;

		updateGeometry();
	}

	const int limit = ((index >= 0) ? (index + 1) : count());

	for (int i = ((index >= 0) ? index : 0); i < limit; ++i)
	{
		const bool isLoading = tabData(i).toHash().value("loading", false).toBool();
		QLabel *label = qobject_cast<QLabel*>(tabButton(i, QTabBar::LeftSide));

		if (label)
		{
			if (!isLoading && label->movie())
			{
				label->movie()->deleteLater();
				label->setMovie(NULL);
				label->setPixmap(tabData(i).toHash().value("icon", QIcon(tabData(i).toHash().value("private", false).toBool() ? ":/icons/tab-private.png" : ":/icons/tab.png")).value<QIcon>().pixmap(16, 16));
			}
			else if (isLoading && !label->movie())
			{
				QMovie *movie = new QMovie(":/icons/loading.gif", QByteArray(), label);
				movie->start();

				label->setMovie(movie);
			}
		}

		if (canResize)
		{
			QWidget *button = tabButton(i, QTabBar::RightSide);

			if (button)
			{
				button->setVisible((!isNarrow || (i == currentIndex())) && !tabData(i).toHash().value("pinned", false).toBool());
			}
		}
	}
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

QSize TabBarWidget::tabSizeHint(int index) const
{
	if (tabData(index).toHash().value("pinned", false).toBool())
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
