/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QuickResponseCodeWidget.h"

#include <QtCore/QUrl>
#include <QtGui/QPainter>

using qrcodegen::QrCode;
using qrcodegen::QrSegment;

#define BORDER_SIZE 4

namespace Otter
{

QuickResponseCodeWidget::QuickResponseCodeWidget(QWidget *parent) : QWidget(parent),
	m_code(QrCode::encodeText(QByteArrayLiteral("https://otter-browser.org"), QrCode::Ecc::MEDIUM))
{
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void QuickResponseCodeWidget::setText(const QString &text)
{
	if (text != m_text)
	{
		m_text = text;

		m_code = QrCode::encodeText(text.toUtf8(), QrCode::Ecc::MEDIUM);

		updateGeometry();
		update();
	}
}

void QuickResponseCodeWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	const int size(getSize());
	QPainter painter(this);
	painter.translate(((width() - size) / 2), ((height() - size) / 2));
	painter.fillRect(0, 0, size, size, Qt::white);

	render(&painter);
}

void QuickResponseCodeWidget::render(QPainter *painter) const
{
	const int segmentSize(getSegmentSize());
	const int borderSize(BORDER_SIZE * segmentSize);

	painter->translate(borderSize, borderSize);

	for (int i = 0; i < m_code.getSize(); ++i)
	{
		for (int j = 0; j < m_code.getSize(); ++j)
		{
			if (m_code.getModule(j, i))
			{
				painter->fillRect((j * segmentSize), (i * segmentSize), segmentSize, segmentSize, Qt::black);
			}
		}
	}
}

void QuickResponseCodeWidget::setUrl(const QUrl &url)
{
	setText(url.toString());
}

QPixmap QuickResponseCodeWidget::getPixmap() const
{
	const int size(getSize());
	QPixmap pixmap(size, size);
	pixmap.fill(Qt::white);

	QPainter painter(&pixmap);

	render(&painter);

	return pixmap;
}

QSize QuickResponseCodeWidget::minimumSizeHint() const
{
	const int size((m_code.getSize() + 8) * getSegmentSize());

	return {size, size};
}

QSize QuickResponseCodeWidget::sizeHint() const
{
	return {width(), width()};
}

int QuickResponseCodeWidget::getSize() const
{
	return ((m_code.getSize() + (BORDER_SIZE * 2)) * getSegmentSize());
}

int QuickResponseCodeWidget::getSegmentSize() const
{
	const int segmentSize(qMin(height(), width()) / (m_code.getSize() + (BORDER_SIZE * 2)));

	return (qMax(5, segmentSize) * devicePixelRatio());
}

int QuickResponseCodeWidget::heightForWidth(int width) const
{
	return width;
}

bool QuickResponseCodeWidget::hasHeightForWidth() const
{
	return true;
}

}
