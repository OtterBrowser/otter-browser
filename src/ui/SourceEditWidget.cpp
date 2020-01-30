/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SourceEditWidget.h"
#include "SyntaxHighlighter.h"
#include "../core/SettingsManager.h"

#include <QtGui/QPainter>
#include <QtGui/QTextBlock>
#include <QtWidgets/QScrollBar>

namespace Otter
{

MarginWidget::MarginWidget(SourceEditWidget *parent) : QWidget(parent),
	m_sourceEditWidget(parent),
	m_lastClickedLine(-1)
{
	updateWidth();
	setContextMenuPolicy(Qt::NoContextMenu);

	connect(m_sourceEditWidget, &SourceEditWidget::updateRequest, this, &MarginWidget::updateNumbers);
	connect(m_sourceEditWidget, &SourceEditWidget::blockCountChanged, this, &MarginWidget::updateWidth);
	connect(m_sourceEditWidget, &SourceEditWidget::textChanged, this, &MarginWidget::updateWidth);
}

void MarginWidget::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.fillRect(event->rect(), Qt::transparent);

	QTextBlock block(m_sourceEditWidget->firstVisibleBlock());
	int top(m_sourceEditWidget->blockBoundingGeometry(block).translated(m_sourceEditWidget->contentOffset()).toRect().top());
	int bottom(top + m_sourceEditWidget->blockBoundingRect(block).toRect().height());
	const int right(width() - 5);
	const int selectionStart(m_sourceEditWidget->document()->findBlock(m_sourceEditWidget->textCursor().selectionStart()).blockNumber());
	const int selectionEnd(m_sourceEditWidget->document()->findBlock(m_sourceEditWidget->textCursor().selectionEnd()).blockNumber());

	while (block.isValid() && top <= event->rect().bottom())
	{
		if (block.isVisible() && bottom >= event->rect().top())
		{
			QColor textColor(palette().color(QPalette::Text));
			textColor.setAlpha((block.blockNumber() >= selectionStart && block.blockNumber() <= selectionEnd) ? 250 : 150);

			painter.setPen(textColor);
			painter.drawText(0, top, right, fontMetrics().height(), Qt::AlignRight, QString::number(block.blockNumber() + 1));
		}

		block = block.next();
		top = bottom;
		bottom = (top + m_sourceEditWidget->blockBoundingRect(block).toRect().height());
	}
}

void MarginWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		QTextCursor textCursor(m_sourceEditWidget->cursorForPosition(QPoint(1, event->y())));
		textCursor.select(QTextCursor::LineUnderCursor);
		textCursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);

		m_lastClickedLine = textCursor.blockNumber();

		m_sourceEditWidget->setTextCursor(textCursor);
	}
	else
	{
		QWidget::mousePressEvent(event);
	}
}

void MarginWidget::mouseMoveEvent(QMouseEvent *event)
{
	QTextCursor textCursor(m_sourceEditWidget->cursorForPosition(QPoint(1, event->y())));
	const int currentLine(textCursor.blockNumber());

	if (currentLine != m_lastClickedLine)
	{
		const bool isMovingUp(currentLine < m_lastClickedLine);

		textCursor.movePosition((isMovingUp ? QTextCursor::Down : QTextCursor::Up), QTextCursor::KeepAnchor, qAbs(m_lastClickedLine - currentLine - (isMovingUp ? 0 : 1)));

		m_sourceEditWidget->setTextCursor(textCursor);
	}
}

void MarginWidget::mouseReleaseEvent(QMouseEvent *event)
{
	m_lastClickedLine = -1;

	QWidget::mouseReleaseEvent(event);
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

void MarginWidget::updateWidth()
{
	int digits(1);
	int maximum(qMax(1, m_sourceEditWidget->blockCount()));

	while (maximum >= 10)
	{
		maximum /= 10;

		++digits;
	}

	setFixedWidth(6 + (Utils::calculateCharacterWidth(QLatin1Char('9'), fontMetrics()) * digits));

	m_sourceEditWidget->setViewportMargins(width(), 0, 0, 0);
}

bool MarginWidget::event(QEvent *event)
{
	const bool result(QWidget::event(event));

	if (event->type() == QEvent::FontChange)
	{
		updateWidth();
	}

	return result;
}

SourceEditWidget::SourceEditWidget(QWidget *parent) : TextEditWidget(parent),
	m_marginWidget(nullptr),
	m_findFlags(WebWidget::NoFlagsFind),
	m_zoom(100)
{
	SyntaxHighlighter::createHighlighter(SyntaxHighlighter::HtmlSyntax, document());

	setSpellCheckingEnabled(false);
	setZoom(SettingsManager::getOption(SettingsManager::Content_DefaultZoomOption).toInt());
	handleOptionChanged(SettingsManager::Interface_ShowScrollBarsOption, SettingsManager::getOption(SettingsManager::Interface_ShowScrollBarsOption));
	handleOptionChanged(SettingsManager::SourceViewer_ShowLineNumbersOption, SettingsManager::getOption(SettingsManager::SourceViewer_ShowLineNumbersOption));
	handleOptionChanged(SettingsManager::SourceViewer_WrapLinesOption, SettingsManager::getOption(SettingsManager::SourceViewer_WrapLinesOption));

	connect(this, &SourceEditWidget::textChanged, this, &SourceEditWidget::updateSelection);
	connect(this, &SourceEditWidget::cursorPositionChanged, this, &SourceEditWidget::updateTextCursor);
	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &SourceEditWidget::handleOptionChanged);
}

void SourceEditWidget::resizeEvent(QResizeEvent *event)
{
	TextEditWidget::resizeEvent(event);

	if (m_marginWidget)
	{
		m_marginWidget->setGeometry(QRect(contentsRect().left(), contentsRect().top(), m_marginWidget->width(), contentsRect().height()));
	}
}

void SourceEditWidget::focusInEvent(QFocusEvent *event)
{
	TextEditWidget::focusInEvent(event);

	if (event->reason() != Qt::MouseFocusReason && event->reason() != Qt::PopupFocusReason && !m_findText.isEmpty())
	{
		setTextCursor(m_findTextSelection);
	}
}

void SourceEditWidget::wheelEvent(QWheelEvent *event)
{
	if (event->modifiers().testFlag(Qt::ControlModifier))
	{
		setZoom(getZoom() + (event->delta() / 16));

		event->accept();

		return;
	}

	TextEditWidget::wheelEvent(event);
}

void SourceEditWidget::findText(const QString &text, WebWidget::FindFlags flags)
{
	const bool isTheSame(text == m_findText);

	m_findText = text;
	m_findFlags = flags;

	if (!text.isEmpty())
	{
		QTextDocument::FindFlags nativeFlags;

		if (flags.testFlag(WebWidget::BackwardFind))
		{
			nativeFlags |= QTextDocument::FindBackward;
		}

		if (flags.testFlag(WebWidget::CaseSensitiveFind))
		{
			nativeFlags |= QTextDocument::FindCaseSensitively;
		}

		QTextCursor findTextCursor(m_findTextAnchor);

		if (!isTheSame)
		{
			findTextCursor = textCursor();
		}
		else if (!flags.testFlag(WebWidget::BackwardFind))
		{
			findTextCursor.setPosition(findTextCursor.selectionEnd(), QTextCursor::MoveAnchor);
		}

		m_findTextAnchor = document()->find(text, findTextCursor, nativeFlags);

		if (m_findTextAnchor.isNull())
		{
			m_findTextAnchor = textCursor();
			m_findTextAnchor.setPosition((flags.testFlag(WebWidget::BackwardFind) ? (document()->characterCount() - 1) : 0), QTextCursor::MoveAnchor);
			m_findTextAnchor = document()->find(text, m_findTextAnchor, nativeFlags);
		}

		if (!m_findTextAnchor.isNull())
		{
			const QTextCursor currentTextCursor(textCursor());

			disconnect(this, &SourceEditWidget::cursorPositionChanged, this, &SourceEditWidget::updateTextCursor);

			setTextCursor(m_findTextAnchor);
			ensureCursorVisible();

			const QPoint position(horizontalScrollBar()->value(), verticalScrollBar()->value());

			setTextCursor(currentTextCursor);

			horizontalScrollBar()->setValue(position.x());
			verticalScrollBar()->setValue(position.y());

			connect(this, &SourceEditWidget::cursorPositionChanged, this, &SourceEditWidget::updateTextCursor);
		}
	}

	m_findTextSelection = m_findTextAnchor;

	updateSelection();
}

void SourceEditWidget::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::Interface_ShowScrollBarsOption:
			setHorizontalScrollBarPolicy(value.toBool() ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
			setVerticalScrollBarPolicy(value.toBool() ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);

			break;
		case SettingsManager::SourceViewer_ShowLineNumbersOption:
			if (value.toBool() && !m_marginWidget)
			{
				m_marginWidget = new MarginWidget(this);
				m_marginWidget->show();
				m_marginWidget->setGeometry(QRect(contentsRect().left(), contentsRect().top(), m_marginWidget->width(), contentsRect().height()));
			}
			else if (!value.toBool() && m_marginWidget)
			{
				m_marginWidget->deleteLater();
				m_marginWidget = nullptr;

				setViewportMargins(0, 0, 0, 0);
			}

			break;
		case SettingsManager::SourceViewer_WrapLinesOption:
			setLineWrapMode(value.toBool() ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);

			break;
		default:
			break;
	}
}

void SourceEditWidget::updateTextCursor()
{
	m_findTextAnchor = textCursor();
}

void SourceEditWidget::updateSelection()
{
	QList<QTextEdit::ExtraSelection> extraSelections;

	if (m_findText.isEmpty())
	{
		setExtraSelections(extraSelections);

		return;
	}

	int findTextMatchesAmount(0);
	int findTextActiveResult(0);
	QTextEdit::ExtraSelection currentResultSelection;
	currentResultSelection.format.setBackground(QColor(255, 150, 50));
	currentResultSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
	currentResultSelection.cursor = m_findTextSelection;

	extraSelections.append(currentResultSelection);

	QTextCursor textCursor(this->textCursor());
	textCursor.setPosition(0);

	if (m_findFlags.testFlag(WebWidget::HighlightAllFind))
	{
		QTextDocument::FindFlags nativeFlags;

		if (m_findFlags.testFlag(WebWidget::CaseSensitiveFind))
		{
			nativeFlags |= QTextDocument::FindCaseSensitively;
		}

		while (!textCursor.isNull())
		{
			textCursor = document()->find(m_findText, textCursor, nativeFlags);

			if (!textCursor.isNull())
			{
				if (textCursor == m_findTextSelection)
				{
					findTextActiveResult = (findTextMatchesAmount + 1);
				}
				else
				{
					QTextEdit::ExtraSelection extraResultSelection;
					extraResultSelection.format.setBackground(QColor(255, 255, 0));
					extraResultSelection.cursor = textCursor;

					extraSelections.append(extraResultSelection);
				}

				++findTextMatchesAmount;
			}
		}
	}

	emit findTextResultsChanged(m_findText, findTextMatchesAmount, findTextActiveResult);

	setExtraSelections(extraSelections);
}

void SourceEditWidget::setZoom(int zoom)
{
	if (zoom != m_zoom)
	{
		m_zoom = zoom;

		QFont font(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		font.setPointSize(qRound(font.pointSize() * (static_cast<qreal>(zoom) / 100)));

		setFont(font);

		if (m_marginWidget)
		{
			m_marginWidget->setFont(font);
		}

		emit zoomChanged(zoom);
	}
}

int SourceEditWidget::getZoom() const
{
	return m_zoom;
}

}
