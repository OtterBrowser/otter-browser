/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TEXTLABELWIDGET_H
#define OTTER_TEXTLABELWIDGET_H

#include <QtCore/QUrl>
#include <QtWidgets/QLineEdit>

namespace Otter
{

class TextLabelWidget : public QLineEdit
{
	Q_OBJECT

public:
	explicit TextLabelWidget(QWidget *parent = nullptr);

	QString getFallbackText() const;
	QSize sizeHint() const override;
	bool event(QEvent *event) override;

public slots:
	void clear();
	void setFallbackText(const QString &text);
	void setText(const QString &text);
	void setUrl(const QUrl &url);

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void updateStyle();

private:
	QString m_fallbackText;
	QUrl m_url;
	QPoint m_dragStartPosition;
	bool m_isEmpty;
};

}

#endif
