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

		menu.addAction(tr("Clone Tab"), this, SLOT(cloneTab()))->setEnabled(getTabProperty(m_clickedTab, "isClonable", false).toBool());
		menu.addAction((isPinned ? tr("Unpin Tab") : tr("Pin Tab")), this, SLOT(pinTab()));
		menu.addSeparator();

		if (isPinned)
		{
			QAction *closeAction = menu.addAction(QString());

			ActionsManager::setupLocalAction(closeAction, "CloseTab");

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
	const int tab = tabAt(event->pos());

	if (tab < 0)
	{
		ActionsManager::triggerAction("NewTab");
	}
	else
	{
		emit requestedClose(tab);
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

void TabBarWidget::cloneTab()
{
	if (m_clickedTab >= 0)
	{
		emit requestedClone(m_clickedTab);
	}
}

void TabBarWidget::pinTab()
{
	emit requestedPin(m_clickedTab, !getTabProperty(m_clickedTab, "isPinned", false).toBool());
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
		const bool isLoading = getTabProperty(i, "isLoading", false).toBool();
		QLabel *label = qobject_cast<QLabel*>(tabButton(i, QTabBar::LeftSide));

		if (label)
		{
			if (!isLoading && (label->movie() || !label->pixmap()))
			{
				if (label->movie())
				{
					label->movie()->deleteLater();
				}

				label->setMovie(NULL);
				label->setPixmap(getTabProperty(i, "icon", QIcon(getTabProperty(i, "isPrivate", false).toBool() ? ":/icons/tab-private.png" : ":/icons/tab.png")).value<QIcon>().pixmap(16, 16));
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
				button->setVisible((!isNarrow || (i == currentIndex())) && !getTabProperty(i, "isPinned", false).toBool());
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
