/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../core/Utils.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>

namespace Otter
{

ProgressBarWidget::ProgressBarWidget(QWidget *parent) : QProgressBar(parent),
	m_mode(NormalMode),
	m_hasError(false)
{
}

void ProgressBarWidget::setMode(ProgressBarWidget::StyleMode mode)
{
	m_mode = mode;

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

void ProgressBarWidget::paintEvent(QPaintEvent *event)
{
	if (m_mode == NormalMode)
	{
		QProgressBar::paintEvent(event);

		return;
	}

	QPainter painter(this);
	QColor background(palette().highlight().color());
	background.setAlpha(50);

	painter.setBrush(background);
	painter.setPen(Qt::transparent);
	painter.drawRoundedRect(rect(), 2, 2);
	painter.setBrush(palette().highlight());

	if (value() > 0)
	{
		painter.drawRoundedRect(0, 0, static_cast<int>(Utils::calculatePercent(value(), maximum(), 1) * width()), height(), 2, 2);
	}
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
