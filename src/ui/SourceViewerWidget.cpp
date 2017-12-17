/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SourceViewerWidget.h"
#include "../core/SettingsManager.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaEnum>
#include <QtGui/QPainter>
#include <QtGui/QTextBlock>
#include <QtWidgets/QScrollBar>

namespace Otter
{

QMap<SyntaxHighlighter::HighlightingSyntax, QMap<SyntaxHighlighter::HighlightingState, QTextCharFormat> > SyntaxHighlighter::m_formats;

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
	if (m_formats[HtmlSyntax].isEmpty())
	{
		QFile file(SessionsManager::getReadableDataPath(QLatin1String("syntaxHighlighting.json")));
		file.open(QIODevice::ReadOnly);

		const QJsonObject syntaxesObject(QJsonDocument::fromJson(file.readAll()).object());
		const QMetaEnum highlightingSyntaxEnum(metaObject()->enumerator(metaObject()->indexOfEnumerator(QLatin1String("HighlightingSyntax").data())));
		const QMetaEnum highlightingStateEnum(metaObject()->enumerator(metaObject()->indexOfEnumerator(QLatin1String("HighlightingState").data())));

		for (int i = 1; i < highlightingSyntaxEnum.keyCount(); ++i)
		{
			QMap<HighlightingState, QTextCharFormat> formats;
			QString syntax(highlightingSyntaxEnum.valueToKey(i));
			syntax.chop(6);

			const QJsonObject definitionsObject(syntaxesObject.value(syntax).toObject());

			for (int j = 0; j < highlightingStateEnum.keyCount(); ++j)
			{
				QString state(highlightingStateEnum.valueToKey(j));
				state.chop(5);

				const QJsonObject definitionObject(definitionsObject.value(state).toObject());
				const QString foreground(definitionObject.value(QLatin1String("foreground")).toString(QLatin1String("auto")));
				const QString fontStyle(definitionObject.value(QLatin1String("fontStyle")).toString(QLatin1String("auto")));
				const QString fontWeight(definitionObject.value(QLatin1String("fontWeight")).toString(QLatin1String("auto")));
				QTextCharFormat format;

				if (foreground != QLatin1String("auto"))
				{
					format.setForeground(QColor(foreground));
				}

				if (fontStyle == QLatin1String("italic"))
				{
					format.setFontItalic(true);
				}

				if (fontWeight != QLatin1String("auto"))
				{
					format.setFontWeight((fontWeight == QLatin1String("bold")) ? QFont::Bold : QFont::Normal);
				}

				if (definitionObject.value(QLatin1String("isUnderlined")).toBool(false))
				{
					format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
				}

				formats[static_cast<HighlightingState>(j)] = format;
			}

			m_formats[static_cast<HighlightingSyntax>(i)] = formats;
		}

		file.close();
	}
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
	QString buffer;
	BlockData currentData;
	HighlightingState previousState(static_cast<HighlightingState>(qMax(previousBlockState(), 0)));
	HighlightingState currentState(previousState);
	int previousStateBegin(0);
	int currentStateBegin(0);
	int position(0);

	if (currentBlock().previous().userData())
	{
		currentData = *static_cast<BlockData*>(currentBlock().previous().userData());
	}

	while (position < text.length())
	{
		buffer.append(text.at(position));

		++position;

		const bool isEndOfLine(position == text.length());

		if (currentState == NoState && text.at(position - 1) == QLatin1Char('<'))
		{
			currentState = KeywordState;
			currentStateBegin = (position - 1);
		}
		else if ((currentState == KeywordState || currentState == DoctypeState) && text.at(position - 1) == QLatin1Char('>'))
		{
			currentState = NoState;
			currentStateBegin = position;
		}
		else if (currentState == AttributeState && text.length() > position && text.at(position) == QLatin1Char('>'))
		{
			currentState = KeywordState;
			currentStateBegin = position;
		}
		else if (currentState == KeywordState && buffer.compare(QLatin1String("!DOCTYPE"), Qt::CaseInsensitive) == 0)
		{
			currentState = DoctypeState;
		}
		else if (currentState == KeywordState && buffer == QLatin1String("![CDATA["))
		{
			currentState = CharacterDataState;
		}
		else if (currentState == CharacterDataState && buffer.endsWith(QLatin1String("]]>")))
		{
			currentState = NoState;
			currentStateBegin = position;
		}
		else if (currentState == KeywordState && buffer == QLatin1String("!--"))
		{
			currentState = CommentState;
		}
		else if (currentState == CommentState && buffer.endsWith(QLatin1String("-->")))
		{
			currentState = NoState;
			currentStateBegin = position;
		}
		else if (currentState == KeywordState && (text.at(position - 1) == QLatin1Char('-') || text.at(position - 1).isLetter() || text.at(position - 1).isNumber()) && (position == 1 || (position > 1 && text.at(position - 2).isSpace())))
		{
			currentState = AttributeState;
			currentStateBegin = (position - 1);
		}
		else if (currentState == AttributeState && !(text.at(position - 1) == QLatin1Char('-') || text.at(position - 1).isLetter() || text.at(position - 1).isNumber()))
		{
			currentState = KeywordState;
			currentStateBegin = (position - 1);
		}
		else if ((currentState == KeywordState || currentState == DoctypeState || currentState == AttributeState) && (text.at(position - 1) == QLatin1Char('\'') || text.at(position - 1) == QLatin1Char('"')))
		{
			currentData.context = text.at(position - 1);
			currentData.state = currentState;
			currentState = ValueState;
			currentStateBegin = (position - 1);
		}
		else if (currentState == ValueState && text.at(position - 1) == currentData.context)
		{
			currentState = currentData.state;
			currentStateBegin = position;
			currentData.context.clear();
			currentData.state = NoState;
		}

		if (previousState != currentState || isEndOfLine)
		{
			setFormat(previousStateBegin, (position - previousStateBegin), m_formats[HtmlSyntax][previousState]);

			if (isEndOfLine)
			{
				setFormat(currentStateBegin, (position - currentStateBegin), m_formats[HtmlSyntax][currentState]);
			}

			buffer.clear();
			previousState = currentState;
			previousStateBegin = currentStateBegin;
		}
	}

	if (!currentData.context.isEmpty())
	{
		BlockData *nextBlockData(new BlockData());
		nextBlockData->context = currentData.context;
		nextBlockData->state = currentData.state;

		setCurrentBlockUserData(nextBlockData);
	}

	setCurrentBlockState(currentState);
}

MarginWidget::MarginWidget(SourceViewerWidget *parent) : QWidget(parent),
	m_sourceViewer(parent),
	m_lastClickedLine(-1)
{
	updateWidth();
	setContextMenuPolicy(Qt::NoContextMenu);

	connect(m_sourceViewer, &SourceViewerWidget::updateRequest, this, &MarginWidget::updateNumbers);
	connect(m_sourceViewer, &SourceViewerWidget::blockCountChanged, this, &MarginWidget::updateWidth);
	connect(m_sourceViewer, &SourceViewerWidget::textChanged, this, &MarginWidget::updateWidth);
}

void MarginWidget::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.fillRect(event->rect(), Qt::transparent);

	QTextBlock block(m_sourceViewer->firstVisibleBlock());
	int top(m_sourceViewer->blockBoundingGeometry(block).translated(m_sourceViewer->contentOffset()).toRect().top());
	int bottom(top + m_sourceViewer->blockBoundingRect(block).toRect().height());
	const int right(width() - 5);
	const int selectionStart(m_sourceViewer->document()->findBlock(m_sourceViewer->textCursor().selectionStart()).blockNumber());
	const int selectionEnd(m_sourceViewer->document()->findBlock(m_sourceViewer->textCursor().selectionEnd()).blockNumber());

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
		bottom = (top + m_sourceViewer->blockBoundingRect(block).toRect().height());
	}
}

void MarginWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		QTextCursor textCursor(m_sourceViewer->cursorForPosition(QPoint(1, event->y())));
		textCursor.select(QTextCursor::LineUnderCursor);
		textCursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);

		m_lastClickedLine = textCursor.blockNumber();

		m_sourceViewer->setTextCursor(textCursor);
	}
	else
	{
		QWidget::mousePressEvent(event);
	}
}

void MarginWidget::mouseMoveEvent(QMouseEvent *event)
{
	QTextCursor textCursor(m_sourceViewer->cursorForPosition(QPoint(1, event->y())));
	const int currentLine(textCursor.blockNumber());

	if (currentLine != m_lastClickedLine)
	{
		textCursor.movePosition(((currentLine > m_lastClickedLine) ? QTextCursor::Up : QTextCursor::Down), QTextCursor::KeepAnchor, qAbs(m_lastClickedLine - currentLine));

		m_sourceViewer->setTextCursor(textCursor);
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
	int maximum(qMax(1, m_sourceViewer->blockCount()));

	while (maximum >= 10)
	{
		maximum /= 10;

		++digits;
	}

	setFixedWidth(6 + (fontMetrics().width(QLatin1Char('9')) * digits));

	m_sourceViewer->setViewportMargins(width(), 0, 0, 0);
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

SourceViewerWidget::SourceViewerWidget(QWidget *parent) : QPlainTextEdit(parent),
	m_marginWidget(nullptr),
	m_findFlags(WebWidget::NoFlagsFind),
	m_findTextResultsAmount(0),
	m_zoom(100)
{
	new SyntaxHighlighter(document());

	setZoom(SettingsManager::getOption(SettingsManager::Content_DefaultZoomOption).toInt());
	handleOptionChanged(SettingsManager::Interface_ShowScrollBarsOption, SettingsManager::getOption(SettingsManager::Interface_ShowScrollBarsOption));
	handleOptionChanged(SettingsManager::SourceViewer_ShowLineNumbersOption, SettingsManager::getOption(SettingsManager::SourceViewer_ShowLineNumbersOption));
	handleOptionChanged(SettingsManager::SourceViewer_WrapLinesOption, SettingsManager::getOption(SettingsManager::SourceViewer_WrapLinesOption));

	connect(this, &SourceViewerWidget::textChanged, this, &SourceViewerWidget::updateSelection);
	connect(this, &SourceViewerWidget::cursorPositionChanged, this, &SourceViewerWidget::updateTextCursor);
	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &SourceViewerWidget::handleOptionChanged);
}

void SourceViewerWidget::resizeEvent(QResizeEvent *event)
{
	QPlainTextEdit::resizeEvent(event);

	if (m_marginWidget)
	{
		m_marginWidget->setGeometry(QRect(contentsRect().left(), contentsRect().top(), m_marginWidget->width(), contentsRect().height()));
	}
}

void SourceViewerWidget::focusInEvent(QFocusEvent *event)
{
	QPlainTextEdit::focusInEvent(event);

	if (event->reason() != Qt::MouseFocusReason && event->reason() != Qt::PopupFocusReason && !m_findText.isEmpty())
	{
		setTextCursor(m_findTextSelection);
	}
}

void SourceViewerWidget::wheelEvent(QWheelEvent *event)
{
	if (event->modifiers().testFlag(Qt::ControlModifier))
	{
		setZoom(getZoom() + (event->delta() / 16));

		event->accept();

		return;
	}

	QPlainTextEdit::wheelEvent(event);
}

void SourceViewerWidget::handleOptionChanged(int identifier, const QVariant &value)
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

void SourceViewerWidget::updateTextCursor()
{
	m_findTextAnchor = textCursor();
}

void SourceViewerWidget::updateSelection()
{
	QList<QTextEdit::ExtraSelection> extraSelections;

	if (m_findText.isEmpty())
	{
		m_findTextResultsAmount = 0;

		setExtraSelections(extraSelections);

		return;
	}

	int findTextResultsAmount(0);
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
				if (textCursor != m_findTextSelection)
				{
					QTextEdit::ExtraSelection extraResultSelection;
					extraResultSelection.format.setBackground(QColor(255, 255, 0));
					extraResultSelection.cursor = textCursor;

					extraSelections.append(extraResultSelection);
				}

				++findTextResultsAmount;
			}
		}
	}

	m_findTextResultsAmount = findTextResultsAmount;

	setExtraSelections(extraSelections);
}

void SourceViewerWidget::setZoom(int zoom)
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

int SourceViewerWidget::getZoom() const
{
	return m_zoom;
}

int SourceViewerWidget::findText(const QString &text, WebWidget::FindFlags flags)
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

			disconnect(this, &SourceViewerWidget::cursorPositionChanged, this, &SourceViewerWidget::updateTextCursor);

			setTextCursor(m_findTextAnchor);
			ensureCursorVisible();

			const QPoint position(horizontalScrollBar()->value(), verticalScrollBar()->value());

			setTextCursor(currentTextCursor);

			horizontalScrollBar()->setValue(position.x());
			verticalScrollBar()->setValue(position.y());

			connect(this, &SourceViewerWidget::cursorPositionChanged, this, &SourceViewerWidget::updateTextCursor);
		}
	}

	m_findTextSelection = m_findTextAnchor;

	updateSelection();

	return m_findTextResultsAmount;
}

}
