/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TABBARWIDGET_H
#define OTTER_TABBARWIDGET_H

#include <QtWidgets/QTabBar>

namespace Otter
{

class PreviewWidget;

class TabBarWidget : public QTabBar
{
	Q_OBJECT

public:
	explicit TabBarWidget(QWidget *parent = NULL);

	void removeTab(int index);
	QVariant getTabProperty(int index, const QString &key, const QVariant &defaultValue) const;

public slots:
	void updateTabs(int index = -1);
	void setOrientation(Qt::DockWidgetArea orientation);
	void setShape(QTabBar::Shape shape);
	void setTabProperty(int index, const QString &key, const QVariant &value);

protected:
	void timerEvent(QTimerEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void enterEvent(QEvent *event);
	void leaveEvent(QEvent *event);
	void wheelEvent(QWheelEvent *event);
	void resizeEvent(QResizeEvent *event);
	void tabLayoutChange();
	void tabInserted(int index);
	void tabRemoved(int index);
	void tabHovered(int index);
	void showPreview(int index);
	QSize tabSizeHint(int index) const;

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void currentTabChanged(int index);
	void closeOther();
	void cloneTab();
	void detachTab();
	void pinTab();
	void updateButtons();

private:
	PreviewWidget *m_previewWidget;
	QTabBar::ButtonPosition m_closeButtonPosition;
	QTabBar::ButtonPosition m_iconButtonPosition;
	int m_tabSize;
	int m_clickedTab;
	int m_hoveredTab;
	int m_previewTimer;

signals:
	void requestedClone(int index);
	void requestedDetach(int index);
	void requestedPin(int index, bool pin);
	void requestedClose(int index);
	void requestedCloseOther(int index);
	void moveNewTabButton(int position);
};

}

#endif
