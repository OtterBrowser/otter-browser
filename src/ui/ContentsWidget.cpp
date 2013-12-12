#include "ContentsWidget.h"

namespace Otter
{

ContentsWidget::ContentsWidget(Window *window) : QWidget(window),
	m_layer(NULL)
{
	connect(window, SIGNAL(aboutToClose()), this, SLOT(close()));
}

void ContentsWidget::setEnabled(bool enabled)
{
	if (enabled && m_layer)
	{
		m_layer->hide();
		m_layer->deleteLater();
		m_layer = NULL;
	}
	else if (!enabled && !m_layer)
	{
		QPalette palette = this->palette();
		palette.setColor(QPalette::Window, QColor(0, 0, 0, 70));

		m_layer = new QWidget(this);
		m_layer->setAutoFillBackground(true);
		m_layer->setPalette(palette);
		m_layer->resize(size());
		m_layer->show();
		m_layer->raise();
	}

	QWidget::setEnabled(enabled);
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
