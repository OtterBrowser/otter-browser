/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Menu.h"
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

	connect(m_sourceEditWidget, &SourceEditWidget::updateRequest, this, &MarginWidget::updateNumbers);
	connect(m_sourceEditWidget, &SourceEditWidget::blockCountChanged, this, &MarginWidget::updateWidth);
	connect(m_sourceEditWidget, &SourceEditWidget::textChanged, this, &MarginWidget::updateWidth);
}

void MarginWidget::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.fillRect(event->rect(), Qt::transparent);

	QColor textColor(palette().color(QPalette::Text));
	QTextBlock block(m_sourceEditWidget->firstVisibleBlock());
	const QTextCursor textCursor(m_sourceEditWidget->textCursor());
	const Qt::AlignmentFlag alignment(isLeftToRight() ? Qt::AlignRight : Qt::AlignLeft);
	int top(m_sourceEditWidget->blockBoundingGeometry(block).translated(m_sourceEditWidget->contentOffset()).toRect().top());
	const int numberHeight(fontMetrics().height());
	const int numberWidth(width() - 8);
	const int selectionStart(m_sourceEditWidget->document()->findBlock(textCursor.selectionStart()).blockNumber());
	const int selectionEnd(m_sourceEditWidget->document()->findBlock(textCursor.selectionEnd()).blockNumber());
	const int initialRevison(m_sourceEditWidget->getInitialRevision());
	const int savedRevison(m_sourceEditWidget->getSavedRevision());

	while (block.isValid() && top <= event->rect().bottom())
	{
		const int bottom(top + m_sourceEditWidget->blockBoundingRect(block).toRect().height());

		if (block.isVisible() && bottom >= event->rect().top())
		{
			const int blockNumber(block.blockNumber());

			textColor.setAlpha((blockNumber >= selectionStart && blockNumber <= selectionEnd) ? 250 : 150);

			painter.setPen(textColor);
			painter.drawText(4, top, numberWidth, numberHeight, alignment, QString::number(blockNumber + 1));

			if (block.revision() > initialRevison)
			{
				painter.setPen(QPen(((block.revision() <= savedRevison) ? Qt::green : Qt::red), 2));
				painter.drawLine(1, top, 1, (top + numberHeight));
				painter.drawLine((width() - 2), top, (width() - 2), (top + numberHeight));
			}
		}

		block = block.next();
		top = bottom;
	}
}

void MarginWidget::contextMenuEvent(QContextMenuEvent *event)
{
	Menu menu(this);
	QAction *showLineNumbersAction(menu.addAction(tr("Show Line Numbers"), this, [&](bool show)
	{
		SettingsManager::setOption(SettingsManager::SourceViewer_ShowLineNumbersOption, show);
	}));
	showLineNumbersAction->setCheckable(true);
	showLineNumbersAction->setChecked(SettingsManager::getOption(SettingsManager::SourceViewer_ShowLineNumbersOption).toBool());

	menu.exec(event->globalPos());
}

void MarginWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		QTextCursor textCursor(m_sourceEditWidget->cursorForPosition({1, event->pos().y()}));
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
	QTextCursor textCursor(m_sourceEditWidget->cursorForPosition({1, event->pos().y()}));
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
	if (offset == 0)
	{
		update(0, rectangle.y(), width(), rectangle.height());
	}
	else
	{
		scroll(0, offset);
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

	setFixedWidth((fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits) + 8);

	if (isRightToLeft())
	{
		m_sourceEditWidget->setViewportMargins(0, 0, width(), 0);
	}
	else
	{
		m_sourceEditWidget->setViewportMargins(width(), 0, 0, 0);
	}
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

SourceEditWidget::SourceEditWidget(QWidget *parent) : TextEditWidget(false, parent),
	m_marginWidget(nullptr),
	m_highlighter(nullptr),
	m_findFlags(WebWidget::NoFlagsFind),
	m_initialRevision(-1),
	m_savedRevision(-1),
	m_zoom(100)
{
	setZoom(SettingsManager::getOption(SettingsManager::Content_DefaultZoomOption).toInt());
	handleOptionChanged(SettingsManager::Interface_ShowScrollBarsOption, SettingsManager::getOption(SettingsManager::Interface_ShowScrollBarsOption));
	handleOptionChanged(SettingsManager::SourceViewer_ShowLineNumbersOption, SettingsManager::getOption(SettingsManager::SourceViewer_ShowLineNumbersOption));
	handleOptionChanged(SettingsManager::SourceViewer_WrapLinesOption, SettingsManager::getOption(SettingsManager::SourceViewer_WrapLinesOption));

	connect(this, &SourceEditWidget::textChanged, this, &SourceEditWidget::updateSelection);
	connect(this, &SourceEditWidget::cursorPositionChanged, this, &SourceEditWidget::updateTextCursor);
	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &SourceEditWidget::handleOptionChanged);
}

void SourceEditWidget::changeEvent(QEvent *event)
{
	TextEditWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && m_marginWidget)
	{
		m_marginWidget->updateWidth();
		m_marginWidget->setGeometry(getMarginGeometry());
	}
}

void SourceEditWidget::resizeEvent(QResizeEvent *event)
{
	TextEditWidget::resizeEvent(event);

	if (m_marginWidget)
	{
		m_marginWidget->setGeometry(getMarginGeometry());
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
		setZoom(getZoom() + (event->angleDelta().y() / 16));

		event->accept();

		return;
	}

	TextEditWidget::wheelEvent(event);
}

void SourceEditWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	if (identifier != ActionsManager::CheckSpellingAction)
	{
		TextEditWidget::triggerAction(identifier, parameters, trigger);
	}
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
				m_marginWidget->setGeometry(getMarginGeometry());
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

void SourceEditWidget::markAsLoaded()
{
	m_initialRevision = document()->revision();
	m_savedRevision = document()->revision();

	document()->setModified(false);

	if (m_marginWidget)
	{
		m_marginWidget->update();
	}
}

void SourceEditWidget::markAsSaved()
{
	m_savedRevision = document()->revision();

	document()->setModified(false);

	if (m_marginWidget)
	{
		m_marginWidget->update();
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

			if (textCursor.isNull())
			{
				break;
			}

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

	emit findTextResultsChanged(m_findText, findTextMatchesAmount, findTextActiveResult);

	setExtraSelections(extraSelections);
}

void SourceEditWidget::setSyntax(SyntaxHighlighter::HighlightingSyntax syntax)
{
	if (m_highlighter)
	{
		if (m_highlighter->getSyntax() == syntax)
		{
			return;
		}

		m_highlighter->deleteLater();
	}

	m_highlighter = SyntaxHighlighter::createHighlighter(syntax, document());
}

void SourceEditWidget::setZoom(int zoom)
{
	if (zoom == m_zoom)
	{
		return;
	}

	m_zoom = zoom;

	const QFont font(Utils::multiplyFontSize(QFontDatabase::systemFont(QFontDatabase::FixedFont), (static_cast<qreal>(zoom) / 100)));

	setFont(font);

	if (m_marginWidget)
	{
		m_marginWidget->setFont(font);
	}

	emit zoomChanged(zoom);
}

ActionsManager::ActionDefinition::State SourceEditWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	if (identifier == ActionsManager::CheckSpellingAction)
	{
		ActionsManager::ActionDefinition::State state(TextEditWidget::getActionState(identifier, parameters));
		state.isEnabled = false;

		return state;
	}

	return TextEditWidget::getActionState(identifier, parameters);
}

QRect SourceEditWidget::getMarginGeometry() const
{
	if (!m_marginWidget)
	{
		return {};
	}

	QRect geometry({contentsRect().left(), contentsRect().top(), m_marginWidget->width(), contentsRect().height()});

	if (isRightToLeft())
	{
		geometry.setX(contentsRect().right() - m_marginWidget->width());
	}

	return geometry;
}

int SourceEditWidget::getInitialRevision() const
{
	return m_initialRevision;
}

int SourceEditWidget::getSavedRevision() const
{
	return m_savedRevision;
}

int SourceEditWidget::getZoom() const
{
	return m_zoom;
}

}
