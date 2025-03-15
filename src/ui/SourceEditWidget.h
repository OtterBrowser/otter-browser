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

#ifndef OTTER_SOURCEEDITWIDGET_H
#define OTTER_SOURCEEDITWIDGET_H

#include "SyntaxHighlighter.h"
#include "TextEditWidget.h"
#include "WebWidget.h"

namespace Otter
{

class SourceEditWidget;

class MarginWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit MarginWidget(SourceEditWidget *parent);

public slots:
	void updateNumbers(const QRect &rectangle, int offset);
	void updateWidth();

protected:
	void paintEvent(QPaintEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	bool event(QEvent *event) override;

private:
	SourceEditWidget *m_sourceEditWidget;
	int m_lastClickedLine;
};

class SourceEditWidget final : public TextEditWidget
{
	Q_OBJECT

public:
	explicit SourceEditWidget(QWidget *parent = nullptr);

	void findText(const QString &text, WebWidget::FindFlags flags = WebWidget::NoFlagsFind);
	void markAsLoaded();
	void markAsSaved();
	void setSyntax(SyntaxHighlighter::HighlightingSyntax syntax);
	void setZoom(int zoom);
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters) const override;
	int getZoom() const;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;

protected:
	void changeEvent(QEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	QRect getMarginGeometry() const;
	int getInitialRevision() const;
	int getSavedRevision() const;

protected slots:
	void handleOptionChanged(int identifier, const QVariant &value);
	void updateTextCursor();
	void updateSelection();

private:
	MarginWidget *m_marginWidget;
	SyntaxHighlighter *m_highlighter;
	QString m_findText;
	QTextCursor m_findTextAnchor;
	QTextCursor m_findTextSelection;
	WebWidget::FindFlags m_findFlags;
	int m_initialRevision;
	int m_savedRevision;
	int m_zoom;

signals:
	void findTextResultsChanged(const QString &text, int matchesAmount, int activeResult);
	void zoomChanged(int zoom);

friend class MarginWidget;
};

}

#endif
