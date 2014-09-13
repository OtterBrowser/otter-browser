#include "MdiWidget.h"
#include "Window.h"
#include "../core/ActionsManager.h"

#include <QtGui/QKeyEvent>

namespace Otter
{

MdiWidget::MdiWidget(QWidget *parent) : QWidget(parent),
	m_activeWindow(NULL)
{
}

void MdiWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	if (m_activeWindow)
	{
		m_activeWindow->resize(size());
	}
}

void MdiWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Tab)
	{
		ActionsManager::triggerAction(QLatin1String("ActivateTabOnRight"), parentWidget());

		event->accept();
	}
	else if (event->key() == Qt::Key_Backtab)
	{
		ActionsManager::triggerAction(QLatin1String("ActivateTabOnLeft"), parentWidget());

		event->accept();
	}
	else
	{
		QWidget::keyPressEvent(event);
	}
}

void MdiWidget::addWindow(Window *window)
{
	if (window)
	{
		window->hide();
		window->setParent(this);
		window->move(0, 0);
	}
}

void MdiWidget::setActiveWindow(Window *window)
{
	if (window != m_activeWindow)
	{
		if (window)
		{
			window->resize(size());
			window->show();
		}

		if (m_activeWindow)
		{
			m_activeWindow->hide();
		}

		m_activeWindow = window;
	}
}

Window* MdiWidget::getActiveWindow()
{
	return m_activeWindow.data();
}

}
