/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Martin Rejda <rejdi@otter.ksp.sk>
* Copyright (C) 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "PluginWidget.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

namespace Otter
{

PluginWidget::PluginWidget(const QString &mimeType, const QUrl &url, QWidget *parent) : QWidget(parent),
	m_isHovered(false)
{
	setToolTip(tr("Click to load content (%1) handled by plugin from: %2").arg(mimeType).arg(url.toDisplayString()));
}

void PluginWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	int size = 0;

	if (width() > height())
	{
		size = height();
	}
	else
	{
		size = width();
	}

	const QRect rectangle(((width() - size) / 2), ((height() - size) / 2), size, size);

	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing);
	painter.fillRect(rect(), Qt::transparent);

	if (m_isHovered)
	{
		painter.setPen(QColor(215, 215, 215));
		painter.drawRect(rect());
	}

	painter.setBrush(QColor(215, 215, 215));
	painter.setPen(Qt::transparent);
	painter.drawEllipse(rectangle);

	QPainterPath path;
	path.moveTo(rectangle.topLeft() + QPoint((size / 3), (size / 4)));
	path.lineTo(rectangle.topLeft() + QPoint(((size / 4) * 3), (size / 2)));
	path.lineTo(rectangle.topLeft() + QPoint((size / 3), ((size / 4) * 3)));
	path.lineTo(rectangle.topLeft() + QPoint((size / 3), (size / 4)));

	painter.fillPath(path, QColor(255, 255, 255));
}

void PluginWidget::enterEvent(QEvent *event)
{
	QWidget::enterEvent(event);

	m_isHovered = true;

	update();
}

void PluginWidget::leaveEvent(QEvent *event)
{
	QWidget::leaveEvent(event);

	m_isHovered = false;

	update();
}

}
