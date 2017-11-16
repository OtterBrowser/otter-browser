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

#ifndef OTTER_SOURCEVIEWERWIDGET_H
#define OTTER_SOURCEVIEWERWIDGET_H

#include "WebWidget.h"

#include <QtGui/QSyntaxHighlighter>
#include <QtWidgets/QPlainTextEdit>

namespace Otter
{

class SyntaxHighlighter final : public QSyntaxHighlighter
{
	Q_OBJECT
	Q_ENUMS(HighlightingSyntax)
	Q_ENUMS(HighlightingState)

public:
	enum HighlightingSyntax
	{
		NoSyntax = 0,
		HtmlSyntax
	};

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

class SourceViewerWidget;

class MarginWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit MarginWidget(SourceViewerWidget *parent);

public slots:
	void updateNumbers(const QRect &rectangle, int offset);
	void updateWidth();

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	bool event(QEvent *event) override;

private:
	SourceViewerWidget *m_sourceViewer;
	int m_lastClickedLine;
};

class SourceViewerWidget final : public QPlainTextEdit
{
	Q_OBJECT

public:
	explicit SourceViewerWidget(QWidget *parent = nullptr);

	void setZoom(int zoom);
	int getZoom() const;
	int findText(const QString &text, WebWidget::FindFlags flags = WebWidget::NoFlagsFind);

protected:
	void resizeEvent(QResizeEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

protected slots:
	void handleOptionChanged(int identifier, const QVariant &value);
	void updateTextCursor();
	void updateSelection();

private:
	MarginWidget *m_marginWidget;
	QString m_findText;
	QTextCursor m_findTextAnchor;
	QTextCursor m_findTextSelection;
	WebWidget::FindFlags m_findFlags;
	int m_findTextResultsAmount;
	int m_zoom;

signals:
	void zoomChanged(int zoom);

friend class MarginWidget;
};

}

#endif
