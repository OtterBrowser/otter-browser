#include "MdiWidget.h"
#include "Window.h"

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
