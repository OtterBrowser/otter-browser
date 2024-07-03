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

#include "SyntaxHighlighter.h"
#include "../core/SessionsManager.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Otter
{

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *document) : QSyntaxHighlighter(document)
{
}

SyntaxHighlighter* SyntaxHighlighter::createHighlighter(HighlightingSyntax syntax, QTextDocument *document)
{
	switch (syntax)
	{
		case AdblockPlusSyntax:
			return new AdblockPlusSyntaxHighlighter(document);
		case HtmlSyntax:
			return new HtmlSyntaxHighlighter(document);
		default:
			return nullptr;
	}
}

QJsonObject SyntaxHighlighter::loadSyntax(HighlightingSyntax syntax) const
{
	QMetaEnum enumerator(metaObject()->enumerator(metaObject()->indexOfEnumerator(QLatin1String("HighlightingSyntax").data())));
	QFile file(SessionsManager::getReadableDataPath(QLatin1String("syntaxHighlighting.json")));
	file.open(QIODevice::ReadOnly);

	return QJsonDocument::fromJson(file.readAll()).object().value(EnumeratorMapper(enumerator, QLatin1String("Syntax")).mapToName(syntax, false)).toObject();
}

QTextCharFormat SyntaxHighlighter::loadFormat(const QJsonObject &definitionObject) const
{
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

	return format;
}

QMap<AdblockPlusSyntaxHighlighter::HighlightingState, QTextCharFormat> AdblockPlusSyntaxHighlighter::m_formats;

AdblockPlusSyntaxHighlighter::AdblockPlusSyntaxHighlighter(QTextDocument *document) : SyntaxHighlighter(document)
{
	if (!m_formats.isEmpty())
	{
		return;
	}

	const QJsonObject definitionsObject(loadSyntax(AdblockPlusSyntax));
	const QMetaEnum enumerator(metaObject()->enumerator(metaObject()->indexOfEnumerator(QLatin1String("HighlightingState").data())));
	const EnumeratorMapper enumeratorMapper(enumerator, QLatin1String("State"));

	for (int j = 0; j < enumerator.keyCount(); ++j)
	{
		m_formats[static_cast<HighlightingState>(j)] = loadFormat(definitionsObject.value(enumeratorMapper.mapToName(j, false)).toObject());
	}
}

void AdblockPlusSyntaxHighlighter::highlightBlock(const QString &text)
{
	QString buffer;
	BlockData currentData;
	HighlightingState previousState(static_cast<HighlightingState>(qMax(previousBlockState(), 0)));
	HighlightingState currentState(previousState);
	int previousStateBegin(0);
	int currentStateBegin(0);
	int position(0);
	const bool isComment(text.trimmed().startsWith(QLatin1Char('!')));
	bool isOption(false);

	if (currentBlock().previous().userData())
	{
		currentData = *static_cast<BlockData*>(currentBlock().previous().userData());
	}

	while (position < text.length())
	{
		buffer.append(text.at(position));

		++position;

		const bool isEndOfLine(position == text.length());

		if (isOption)
		{
			if (text.at(position - 1) == QLatin1Char('~') || text.at(position - 1) == QLatin1Char('|'))
			{
				currentState = ExceptionState;
				currentStateBegin = (position - 1);
			}
			else if (text.at(position - 1) == QLatin1Char(',') || text.at(position - 1) == QLatin1Char('='))
			{
				currentState = NoState;
				currentStateBegin = (position - 1);
			}
			else if (currentState != OptionState)
			{
				currentState = OptionState;
				currentStateBegin = (position - 1);
			}
		}
		else if ((currentState == NoState || currentState == CommentState) && buffer.compare(QLatin1String("[AdBlock"), Qt::CaseInsensitive) == 0)
		{
			currentState = HeaderState;
			currentStateBegin = (position - 8);
		}
		else if (currentState == HeaderState && text.at(position - 1) == QLatin1Char(']'))
		{
			currentState = (isComment ? CommentState : NoState);
			currentStateBegin = position;
		}
		else if (currentState == NoState && text.at(position - 1) == QLatin1Char('!'))
		{
			currentState = CommentState;
		}
		else if (currentState == CommentState && isEndOfLine)
		{
			currentState = NoState;
			currentStateBegin = position;
		}
		else if ((currentState == NoState || currentState == ExceptionState) && text.at(position - 1) == QLatin1Char('|'))
		{
			currentState = AnchorState;
			currentStateBegin = (position - 1);
		}
		else if (currentState == AnchorState && text.at(position - 1) != QLatin1Char('|'))
		{
			currentState = NoState;
			currentStateBegin = (position - 1);
		}
		else if (currentState == NoState && position == 1 && text.at(position - 1) == QLatin1Char('@') && text.at(position) == QLatin1Char('@'))
		{
			currentState = ExceptionState;
			currentStateBegin = (position - 1);
		}
		else if (currentState == ExceptionState && text.at(position - 1) != QLatin1Char('@'))
		{
			currentState = NoState;
			currentStateBegin = (position - 1);
		}
		else if ((currentState == NoState || currentState == AnchorState || currentState == SeparatorState || currentState == WildcardState) && (text.at(position - 1) == QLatin1Char('^') || text.at(position - 1) == QLatin1Char('$')))
		{
			currentState = SeparatorState;
			currentStateBegin = (position - 1);

			if (text.at(position - 1) == QLatin1Char('$'))
			{
				isOption = true;
			}
		}
		else if (currentState == SeparatorState && (text.at(position - 1) != QLatin1Char('*') && text.at(position - 1) != QLatin1Char('$')))
		{
			currentState = NoState;
			currentStateBegin = (position - 1);
		}
		else if ((currentState == NoState || currentState == AnchorState || currentState == WildcardState) && text.at(position - 1) == QLatin1Char('*'))
		{
			currentState = WildcardState;
			currentStateBegin = (position - 1);
		}
		else if (currentState == WildcardState && text.at(position - 1) != QLatin1Char('*'))
		{
			currentState = NoState;
			currentStateBegin = (position - 1);
		}

		if (previousState != currentState || isEndOfLine)
		{
			setFormat(previousStateBegin, (position - previousStateBegin), m_formats[previousState]);

			if (isEndOfLine)
			{
				setFormat(currentStateBegin, (position - currentStateBegin), m_formats[currentState]);

				currentState = NoState;
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

SyntaxHighlighter::HighlightingSyntax AdblockPlusSyntaxHighlighter::getSyntax() const
{
	return AdblockPlusSyntax;
}

QMap<HtmlSyntaxHighlighter::HighlightingState, QTextCharFormat> HtmlSyntaxHighlighter::m_formats;

HtmlSyntaxHighlighter::HtmlSyntaxHighlighter(QTextDocument *document) : SyntaxHighlighter(document)
{
	if (!m_formats.isEmpty())
	{
		return;
	}

	const QJsonObject definitionsObject(loadSyntax(HtmlSyntax));
	const QMetaEnum enumerator(metaObject()->enumerator(metaObject()->indexOfEnumerator(QLatin1String("HighlightingState").data())));
	const EnumeratorMapper enumeratorMapper(enumerator, QLatin1String("State"));

	for (int j = 0; j < enumerator.keyCount(); ++j)
	{
		m_formats[static_cast<HighlightingState>(j)] = loadFormat(definitionsObject.value(enumeratorMapper.mapToName(j, false)).toObject());
	}
}

void HtmlSyntaxHighlighter::highlightBlock(const QString &text)
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
			currentState = static_cast<HighlightingState>(currentData.state);
			currentStateBegin = position;
			currentData.context.clear();
			currentData.state = NoState;
		}

		if (previousState != currentState || isEndOfLine)
		{
			setFormat(previousStateBegin, (position - previousStateBegin), m_formats[previousState]);

			if (isEndOfLine)
			{
				setFormat(currentStateBegin, (position - currentStateBegin), m_formats[currentState]);
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

SyntaxHighlighter::HighlightingSyntax HtmlSyntaxHighlighter::getSyntax() const
{
	return HtmlSyntax;
}

}
