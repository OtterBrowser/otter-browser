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

class SyntaxHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	enum HighlightingSyntax
	{
		NoSyntax = 0,
		AdblockPlusSyntax,
		HtmlSyntax
	};

	Q_ENUM(HighlightingSyntax)

	struct BlockData final : public QTextBlockUserData
	{
		QString context;
		HighlightingSyntax currentSyntax = NoSyntax;
		HighlightingSyntax previousSyntax = NoSyntax;
		int state = 0;
	};

	explicit SyntaxHighlighter(QTextDocument *document);

	static SyntaxHighlighter* createHighlighter(HighlightingSyntax syntax, QTextDocument *document);
	virtual HighlightingSyntax getSyntax() const = 0;

protected:
	QJsonObject loadSyntax(HighlightingSyntax syntax) const;
	QTextCharFormat loadFormat(const QJsonObject &definitionObject) const;
};

class AdblockPlusSyntaxHighlighter final : public SyntaxHighlighter
{
	Q_OBJECT

public:
	enum HighlightingState
	{
		NoState = 0,
		HeaderState,
		AnchorState,
		ExceptionState,
		OptionState,
		SeparatorState,
		WildcardState,
		CommentState
	};

	Q_ENUM(HighlightingState)

	explicit AdblockPlusSyntaxHighlighter(QTextDocument *document);

	HighlightingSyntax getSyntax() const override;

protected:
	void highlightBlock(const QString &text) override;

private:
	static QMap<HighlightingState, QTextCharFormat> m_formats;
};

class HtmlSyntaxHighlighter final : public SyntaxHighlighter
{
	Q_OBJECT

public:
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

	explicit HtmlSyntaxHighlighter(QTextDocument *document);

	HighlightingSyntax getSyntax() const override;

protected:
	void highlightBlock(const QString &text) override;

private:
	static QMap<HighlightingState, QTextCharFormat> m_formats;
};

}

#endif
