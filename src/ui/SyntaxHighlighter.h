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

#ifndef OTTER_SYNTAXHIGHLIGHTER_H
#define OTTER_SYNTAXHIGHLIGHTER_H

#include <QtGui/QSyntaxHighlighter>

namespace Otter
{

class SyntaxHighlighter final : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	enum HighlightingSyntax
	{
		NoSyntax = 0,
		HtmlSyntax
	};

	Q_ENUM(HighlightingSyntax)

	enum HighlightingState
	{
		NoState = 0,
		DoctypeState,
		KeywordState,
		AttributeState,
		EntityState,
		ValueState,
		CharacterDataState,
		CommentState
	};

	Q_ENUM(HighlightingState)

	struct BlockData final : public QTextBlockUserData
	{
		QString context;
		HighlightingSyntax currentSyntax = HtmlSyntax;
		HighlightingSyntax previousSyntax = HtmlSyntax;
		HighlightingState state = NoState;
	};

	explicit SyntaxHighlighter(QTextDocument *parent);

protected:
	void highlightBlock(const QString &text) override;

private:
	static QMap<HighlightingSyntax, QMap<HighlightingState, QTextCharFormat> > m_formats;
};

}

#endif
