/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "ProgressBarWidget.h"
#include "Style.h"
#include "../core/Application.h"

#include <QtGui/QPainter>

namespace Otter
{

ProgressBarWidget::ProgressBarWidget(QWidget *parent) : QProgressBar(parent),
	m_mode(NormalMode),
	m_updateTimer(0),
	m_hasError(false)
{
	connect(this, &ProgressBarWidget::valueChanged, this, [&](int value)
	{
		if (value < 0 && m_mode == ThinMode && m_updateTimer == 0)
		{
			m_updateTimer = startTimer(15);
		}
		else if (value >= 0 && m_updateTimer != 0)
		{
			killTimer(m_updateTimer);

			m_updateTimer = 0;
		}
	});
}

void ProgressBarWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_updateTimer)
	{
		update();
	}
}

void ProgressBarWidget::paintEvent(QPaintEvent *event)
{
	if (m_mode == NormalMode)
	{
		QProgressBar::paintEvent(event);

		return;
	}

	QStyleOptionProgressBar option;
	QPainter painter(this);

	initStyleOption(&option);

	Application::getStyle()->drawThinProgressBar(&option, &painter);
}

void ProgressBarWidget::setMode(StyleMode mode)
{
	m_mode = mode;

	if (value() < 0 && mode == ThinMode && m_updateTimer == 0)
	{
		m_updateTimer = startTimer(100);
	}

	update();
}

void ProgressBarWidget::setHasError(bool hasError)
{
	QPalette palette(this->palette());

	if (hasError)
	{
		palette.setColor(QPalette::Highlight, QColor(Qt::red));
	}
	else
	{
		palette = QGuiApplication::palette();
	}

	setPalette(palette);

	m_hasError = hasError;
}

QSize ProgressBarWidget::minimumSizeHint() const
{
	if (m_mode == ThinMode)
	{
		return sizeHint();
	}

	return QProgressBar::minimumSizeHint();
}

QSize ProgressBarWidget::sizeHint() const
{
	if (m_mode == ThinMode)
	{
		QSize size(QProgressBar::sizeHint());
		size.setHeight(5);

		return size;
	}

	return QProgressBar::sizeHint();
}

bool ProgressBarWidget::hasError() const
{
	return m_hasError;
}

}
