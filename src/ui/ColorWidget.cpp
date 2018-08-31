/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ColorWidget.h"
#include "LineEditWidget.h"
#include "../core/ThemesManager.h"

#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMenu>

namespace Otter
{

ColorWidget::ColorWidget(QWidget *parent) : QWidget(parent),
	m_lineEditWidget(new LineEditWidget(this))
{
	QHBoxLayout *layout(new QHBoxLayout(this));
	layout->addWidget(m_lineEditWidget);
	layout->setContentsMargins((m_lineEditWidget->height() + 2), 0, 0, 0);

	setLayout(layout);
	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	setColor(QColor());

	connect(m_lineEditWidget, &LineEditWidget::textChanged, this, &ColorWidget::updateColor);
}

void ColorWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && !m_color.isValid())
	{
		setToolTip(tr("Invalid"));
	}
}

void ColorWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing);

	drawThumbnail(&painter, m_color, palette(), m_buttonRectangle);
}

void ColorWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	layout()->setContentsMargins((m_lineEditWidget->height() + 2), 0, 0, 0);

	m_buttonRectangle = rect();

	if (isRightToLeft())
	{
		m_buttonRectangle.setLeft(m_buttonRectangle.right() - m_lineEditWidget->height());
	}
	else
	{
		m_buttonRectangle.setRight(m_lineEditWidget->height());
	}

	m_buttonRectangle.adjust(2, 2, -2, -2);
}

void ColorWidget::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);

	m_lineEditWidget->setFocus();
}

void ColorWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QWidget::mouseReleaseEvent(event);

	if (event->button() == Qt::LeftButton && m_buttonRectangle.contains(event->pos()))
	{
		QMenu menu(this);
		menu.addAction(tr("Select Color…"), this, &ColorWidget::selectColor);
		menu.addAction(tr("Copy Color"), this, [&]()
		{
			QApplication::clipboard()->setText(m_color.name((m_color.alpha() < 255) ? QColor::HexArgb : QColor::HexRgb).toUpper());
		});
		menu.addSeparator();
		menu.addAction(ThemesManager::createIcon(QLatin1String("edit-clear")), tr("Clear"), this, [&]()
		{
			setColor(QColor());
		});

		menu.exec(mapToGlobal(isRightToLeft() ? m_buttonRectangle.bottomRight() : m_buttonRectangle.bottomLeft()));
	}
}

void ColorWidget::selectColor()
{
	QColorDialog dialog(this);
	dialog.setOption(QColorDialog::ShowAlphaChannel);
	dialog.setCurrentColor(m_color);

	if (dialog.exec() == QDialog::Accepted)
	{
		setColor(dialog.currentColor());
	}
}

void ColorWidget::drawThumbnail(QPainter *painter, const QColor &color, const QPalette &palette, const QRect &rectangle)
{
	painter->save();

	if (color.alpha() < 255)
	{
		QPixmap pixmap(10, 10);
		pixmap.fill(Qt::white);

		QPainter pixmapPainter(&pixmap);
		pixmapPainter.setBrush(Qt::gray);
		pixmapPainter.setPen(Qt::NoPen);
		pixmapPainter.drawRect(0, 0, 5, 5);
		pixmapPainter.drawRect(5, 5, 5, 5);
		pixmapPainter.end();

		painter->setBrush(pixmap);
		painter->setPen(Qt::NoPen);
		painter->drawRoundedRect(rectangle, 2, 2);
	}

	painter->setBrush(color);
	painter->setPen(palette.color(QPalette::Button));
	painter->drawRoundedRect(rectangle, 2, 2);
	painter->restore();
}

void ColorWidget::setColor(const QString &color)
{
	setColor(color.isEmpty() ? QColor() : QColor(color));
}

void ColorWidget::setColor(const QColor &color)
{
	if (color != m_color)
	{
		const QString text(color.isValid() ? color.name((color.alpha() < 255) ? QColor::HexArgb : QColor::HexRgb).toUpper() : QString());

		if (!m_lineEditWidget->hasFocus())
		{
			m_lineEditWidget->setText(text);
		}

		setToolTip(text.isEmpty() ? tr("Invalid") : text);
		update();

		m_color = color;

		emit colorChanged(color);
	}
}

void ColorWidget::updateColor(const QString &text)
{
	if (QColor::isValidColor(text))
	{
		setColor(text);
	}
}

QColor ColorWidget::getColor() const
{
	return m_color;
}

}
