#include "ContentsWidget.h"

namespace Otter
{

ContentsWidget::ContentsWidget(Window *window) : QWidget(window)
{
	connect(window, SIGNAL(aboutToClose()), this, SLOT(close()));
}

void ContentsWidget::showEvent(QShowEvent *event)
{
	if (isVisible())
	{
		for (int i = 0; i < m_dialogs.count(); ++i)
		{
			if (m_dialogs.at(i))
			{
				m_dialogs.at(i)->setEnabled(true);
				m_dialogs.at(i)->show();
			}
		}
	}

	QWidget::showEvent(event);
}

void ContentsWidget::hideEvent(QHideEvent *event)
{
	for (int i = 0; i < m_dialogs.count(); ++i)
	{
		if (m_dialogs.at(i))
		{
			m_dialogs.at(i)->hide();
		}
	}

	QWidget::hideEvent(event);
}

void ContentsWidget::close()
{
	for (int i = 0; i < m_dialogs.count(); ++i)
	{
		if (m_dialogs.at(i))
		{
			m_dialogs.at(i)->close();
		}
	}
}

void ContentsWidget::showDialog(QWidget *dialog)
{
	if (!dialog)
	{
		return;
	}
//TODO semi transparent layer or paint event? (gray out)
	setEnabled(false);

	if (isVisible())
	{
		dialog->setEnabled(true);
		dialog->show();
	}

	m_dialogs.append(dialog);
}

void ContentsWidget::hideDialog(QWidget *dialog)
{
	m_dialogs.removeAll(dialog);

	setEnabled(m_dialogs.isEmpty());
}

}
