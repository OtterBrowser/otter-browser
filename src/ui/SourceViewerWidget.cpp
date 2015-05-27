/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SourceViewerWidget.h"

#include <QtGui/QPainter>
#include <QtGui/QTextBlock>

namespace Otter
{

MarginWidget::MarginWidget(SourceViewerWidget *parent) : QWidget(parent),
	m_sourceViewer(parent),
	m_lastClickedLine(-1)
{
	setAmount(1);

	connect(m_sourceViewer, SIGNAL(blockCountChanged(int)), this, SLOT(setAmount(int)));
	connect(m_sourceViewer, SIGNAL(textChanged()), this, SLOT(setAmount()));
	connect(m_sourceViewer, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateNumbers(QRect,int)));
}

void MarginWidget::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.fillRect(event->rect(), Qt::transparent);

	QTextBlock block = m_sourceViewer->firstVisibleBlock();
	int top = (int) m_sourceViewer->blockBoundingGeometry(block).translated(m_sourceViewer->contentOffset()).top();
	int bottom = (top + (int) m_sourceViewer->blockBoundingRect(block).height());
	const int right = (width() - 5);
	const int left = (width() - 2);
	const int selectionStart = m_sourceViewer->document()->findBlock(m_sourceViewer->textCursor().selectionStart()).blockNumber();
	const int selectionEnd = m_sourceViewer->document()->findBlock(m_sourceViewer->textCursor().selectionEnd()).blockNumber();

	while (block.isValid() && top <= event->rect().bottom())
	{
		if (block.isVisible() && bottom >= event->rect().top())
		{
			painter.setPen(palette().color(QPalette::Window).darker((block.blockNumber() >= selectionStart && block.blockNumber() <= selectionEnd) ? 250 : 150));
			painter.drawText(0, top, right, fontMetrics().height(), Qt::AlignRight, QString::number(block.blockNumber() + 1));
		}

		block = block.next();
		top = bottom;
		bottom = (top + (int) m_sourceViewer->blockBoundingRect(block).height());
	}
}

void MarginWidget::mouseMoveEvent(QMouseEvent *event)
{
	QTextCursor textCursor = m_sourceViewer->cursorForPosition(QPoint(1, event->y()));
	int currentLine = textCursor.blockNumber();

	if (currentLine == m_lastClickedLine)
	{
		return;
	}

	textCursor.movePosition(((currentLine > m_lastClickedLine)?QTextCursor::Up:QTextCursor::Down), QTextCursor::KeepAnchor, qAbs(m_lastClickedLine - currentLine));

	m_sourceViewer->setTextCursor(textCursor);
}

void MarginWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		QTextCursor textCursor = m_sourceViewer->cursorForPosition(QPoint(1, event->y()));
		textCursor.select(QTextCursor::LineUnderCursor);

		m_lastClickedLine = textCursor.blockNumber();

		m_sourceViewer->setTextCursor(textCursor);
	}
}

void MarginWidget::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event)

	m_lastClickedLine = -1;
}

void MarginWidget::updateNumbers(const QRect &rectangle, int offset)
{
	if (offset)
	{
		 scroll(0, offset);
	}
	else
	{
		update(0, rectangle.y(), width(), rectangle.height());
	}
}

void MarginWidget::setAmount(int amount)
{
	if (amount < 0 && m_sourceViewer)
	{
		amount = m_sourceViewer->blockCount();
	}

	int digits = 1;
	int max = qMax(1, amount);

	while (max >= 10)
	{
		max /= 10;

		++digits;
	}

	setFixedWidth(6 + (fontMetrics().width(QLatin1Char('9')) * digits));

	m_sourceViewer->setViewportMargins(width(), 0, 0, 0);
}

bool MarginWidget::event(QEvent *event)
{
	const bool value = QWidget::event(event);

	if (event->type() == QEvent::FontChange)
	{
		setAmount();
	}

	return value;
}

SourceViewerWidget::SourceViewerWidget(QWidget *parent) : QPlainTextEdit(parent),
	m_marginWidget(new MarginWidget(this)),
	m_zoom(100)
{
}

void SourceViewerWidget::resizeEvent(QResizeEvent *event)
{
	QPlainTextEdit::resizeEvent(event);

	m_marginWidget->setGeometry(QRect(contentsRect().left(), contentsRect().top(), m_marginWidget->width(), contentsRect().height()));
}

void SourceViewerWidget::setZoom(int zoom)
{
///TODO
	m_zoom = zoom;
}

int SourceViewerWidget::getZoom() const
{
	return m_zoom;
}

}
