#include "TabBarWidget.h"
#include "../core/ActionsManager.h"

#include <QtGui/QContextMenuEvent>
#include <QtGui/QMovie>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>

namespace Otter
{

TabBarWidget::TabBarWidget(QWidget *parent) : QTabBar(parent),
	m_clickedTab(-1)
{
	setExpanding(false);
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
		menu.addAction(ActionsManager::getAction("CloseTab"));

		if (count() > 1)
		{
			menu.addAction(QIcon(":/icons/tab-close-other.png"), tr("Close Other Tabs"), this, SLOT(closeOther()));
		}
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

void TabBarWidget::updateTabs(int index)
{
	const bool canResize = !underMouse();
	QString style;
	const int width = qBound(40, (size().width() / ((count() == 0) ? 1 : count())), 300);
	const bool narrow = (width < 60);

	if (canResize)
	{
		if (narrow)
		{
			style = "color:transparent;";
		}

		setStyleSheet(QString("QTabBar::tab {width:%1px;%2}").arg(width).arg(style));
	}

	const int limit = ((index >= 0) ? (index + 1) : count());

	for (int i = ((index >= 0) ? index : 0); i < limit; ++i)
	{
		const QVariantHash data = tabData(i).toHash();
		const bool isLoading = data.value("loading", false).toBool();
		QLabel *label = qobject_cast<QLabel*>(tabButton(i, QTabBar::LeftSide));

		if (label)
		{
			if (!isLoading && label->movie())
			{
				label->movie()->deleteLater();
				label->setMovie(NULL);
				label->setPixmap(data.value("icon", QIcon(data.value("private", false).toBool() ? ":/icons/tab-private.png" : ":/icons/tab.png")).value<QIcon>().pixmap(16, 16));
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
				button->setVisible(!narrow || (i == currentIndex()));
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

}
