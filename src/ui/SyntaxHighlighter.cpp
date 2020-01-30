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

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaEnum>

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

}
