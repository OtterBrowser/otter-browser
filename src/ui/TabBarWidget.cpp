#include "TabBarWidget.h"
#include "../core/ActionsManager.h"

#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QMenu>

namespace Otter
{

TabBarWidget::TabBarWidget(QWidget *parent) : QTabBar(parent),
	m_clickedTab(-1)
{
	setExpanding(false);
	setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
	setTabsClosable(true);
	setElideMode(Qt::ElideRight);
	tabWidthChanged();

	connect(this, SIGNAL(tabCloseRequested(int)), this, SIGNAL(requestedClose(int)));
	connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabWidthChanged()));
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

void TabBarWidget::resizeEvent(QResizeEvent *event)
{
	QTabBar::resizeEvent(event);

	tabWidthChanged();
}

void TabBarWidget::tabInserted(int index)
{
	QTabBar::tabInserted(index);

	tabWidthChanged();
}

void TabBarWidget::tabRemoved(int index)
{
	QTabBar::tabRemoved(index);

	tabWidthChanged();
}

void TabBarWidget::closeOther()
{
	if (m_clickedTab >= 0)
	{
		emit requestedCloseOther(m_clickedTab);
	}
}

void TabBarWidget::tabWidthChanged()
{
	QString style;
	const int width = qBound(40, (size().width() / ((count() == 0) ? 1 : count())), 300);
	const bool narrow = (width < 60);

	if (narrow)
	{
		style = "color:transparent;";
	}

	for (int i = 0; i < count(); ++i)
	{
		QWidget *button = tabButton(i, QTabBar::RightSide);

		if (button)
		{
			button->setVisible(!narrow || (i == currentIndex()));
		}
	}

	setStyleSheet(QString("QTabBar::tab {width:%1px;%2}").arg(width).arg(style));
}

}
