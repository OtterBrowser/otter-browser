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
				button->setVisible(!isNarrow || (i == currentIndex()));
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
	Q_UNUSED(index)

	return m_tabSize;
}

QSize TabBarWidget::getTabSize(bool isHorizontal) const
{
	const int size = qBound(40, ((isHorizontal ? geometry().width() : geometry().height()) / ((count() == 0) ? 1 : count())), 300);

	if (isHorizontal)
	{
		return QSize(size, QTabBar::tabSizeHint(0).height());
	}

	return QSize(QTabBar::tabSizeHint(0).width(), size);
}

}
