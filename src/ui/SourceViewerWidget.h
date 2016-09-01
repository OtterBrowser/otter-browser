/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_SOURCEVIEWERWIDGET_H
#define OTTER_SOURCEVIEWERWIDGET_H

#include "WebWidget.h"

#include <QtGui/QSyntaxHighlighter>
#include <QtWidgets/QPlainTextEdit>

namespace Otter
{

class SyntaxHighlighter : public  QSyntaxHighlighter
{
	Q_OBJECT
	Q_ENUMS(HighlightingSyntax)
	Q_ENUMS(HighlightingState)

public:
	enum HighlightingSyntax
	{
		NoSyntax = 0,
		HtmlSyntax = 1
	};

	enum HighlightingState
	{
		NoState = 0,
		DoctypeState = 1,
		KeywordState = 2,
		AttributeState = 3,
		EntityState = 4,
		ValueState = 5,
		CommentState = 6
	};

	struct BlockData : public QTextBlockUserData
	{
		QString context;
		HighlightingSyntax currentSyntax;
		HighlightingSyntax previousSyntax;
		HighlightingState state;

		BlockData() : currentSyntax(HtmlSyntax), previousSyntax(HtmlSyntax), state(NoState) {}
	};

	explicit SyntaxHighlighter(QTextDocument *parent);

protected:
	void highlightBlock(const QString &text);

private:
	static QMap<HighlightingSyntax, QMap<HighlightingState, QTextCharFormat> > m_formats;
};

class SourceViewerWidget;

class MarginWidget : public QWidget
{
	Q_OBJECT

public:
	explicit MarginWidget(SourceViewerWidget *parent = NULL);

public slots:
	void updateNumbers(const QRect &rectangle, int offset);
	void setAmount(int amount = -1);

protected:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	bool event(QEvent *event);

private:
	SourceViewerWidget *m_sourceViewer;
	int m_lastClickedLine;
};

class SourceViewerWidget : public QPlainTextEdit
{
	Q_OBJECT

public:
	explicit SourceViewerWidget(QWidget *parent = NULL);

	void setZoom(int zoom);
	int getZoom() const;
	bool findText(const QString &text, WebWidget::FindFlags flags = WebWidget::NoFlagsFind);

protected:
	void resizeEvent(QResizeEvent *event);
	void focusInEvent(QFocusEvent *event);
	void wheelEvent(QWheelEvent *event);

protected slots:
	void optionChanged(int identifier, const QVariant &value);
	void updateTextCursor();
	void updateSelection();

private:
	MarginWidget *m_marginWidget;
	QString m_findText;
	QTextCursor m_findTextAnchor;
	QTextCursor m_findTextSelection;
	WebWidget::FindFlags m_findFlags;
	int m_zoom;

signals:
	void zoomChanged(int zoom);

friend class MarginWidget;
};

}

#endif
