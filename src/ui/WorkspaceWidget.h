/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_WORKSPACEWIDGET_H
#define OTTER_WORKSPACEWIDGET_H

#include "../core/SessionsManager.h"

#include <QtCore/QPointer>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>

namespace Otter
{

class MainWindow;
class Window;

class MdiWidget : public QMdiArea
{
public:
	explicit MdiWidget(QWidget *parent);

	bool eventFilter(QObject *object, QEvent *event);

protected:
	void contextMenuEvent(QContextMenuEvent *event);
};

class MdiWindow : public QMdiSubWindow
{
	Q_OBJECT

public:
	explicit MdiWindow(Window *window, MdiWidget *parent);

	void storeState();
	void restoreState();

protected:
	void changeEvent(QEvent *event);
	void closeEvent(QCloseEvent *event);
	void moveEvent(QMoveEvent *event);
	void resizeEvent(QResizeEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);

private:
	bool m_wasMaximized;
};

class WorkspaceWidget : public QWidget
{
	Q_OBJECT

public:
	explicit WorkspaceWidget(MainWindow *parent);

	void addWindow(Window *window, const QRect &geometry = QRect(), WindowState state = MaximizedWindowState, bool isAlwaysOnTop = false);
	void setActiveWindow(Window *window, bool force = false);
	Window* getActiveWindow();

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap());
	void markRestored();
	void updateActions();

protected:
	void timerEvent(QTimerEvent *event);
	void resizeEvent(QResizeEvent *event);
	void createMdi();

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void activeSubWindowChanged(QMdiSubWindow *subWindow);

private:
	MainWindow *m_mainWindow;
	MdiWidget *m_mdi;
	QPointer<Window> m_activeWindow;
	int m_restoreTimer;
	bool m_isRestored;
};

}

#endif
