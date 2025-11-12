/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TOOLBARDROPZONEWIDGET_H
#define OTTER_TOOLBARDROPZONEWIDGET_H

#include <QtWidgets/QToolBar>

namespace Otter
{

class MainWindow;

class ToolBarDropZoneWidget final : public QToolBar
{
	Q_OBJECT

public:
	explicit ToolBarDropZoneWidget(MainWindow *parent);

	QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	bool canDrop(QDropEvent *event);

private:
	MainWindow *m_mainWindow;
	bool m_isDropTarget;

signals:
	void toolBarDraggedChanged(bool isDragging);
};

}

#endif
