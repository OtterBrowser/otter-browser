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

#ifndef OTTER_WORKSPACEWIDGET_H
#define OTTER_WORKSPACEWIDGET_H

#include "../core/ActionsManager.h"
#include "../core/SessionsManager.h"

#include <QtCore/QPointer>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>

namespace Otter
{

class MainWindow;
class Window;

class MdiWidget final : public QMdiArea
{
public:
	explicit MdiWidget(QWidget *parent);

	bool eventFilter(QObject *object, QEvent *event) override;
};

class MdiWindow final : public QMdiSubWindow
{
	Q_OBJECT

public:
	explicit MdiWindow(Window *window, MdiWidget *parent);

	void storeState();
	void restoreState();
	Window* getWindow() const;

protected:
	void changeEvent(QEvent *event) override;
	void closeEvent(QCloseEvent *event) override;
	void moveEvent(QMoveEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
	Window *m_window;
	bool m_wasMaximized;
};

class WorkspaceWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit WorkspaceWidget(MainWindow *parent);

	void addWindow(Window *window, const Session::Window::State &state = Session::Window::State(), bool isAlwaysOnTop = false);
	void setActiveWindow(Window *window, bool force = false);
	Window* getActiveWindow() const;
	int getWindowCount(Qt::WindowState state) const;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger);
	void markAsRestored();

protected:
	void timerEvent(QTimerEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void createMdi();

protected slots:
	void handleActiveSubWindowChanged(QMdiSubWindow *subWindow);
	void handleOptionChanged(int identifier, const QVariant &value);
	void notifyActionsStateChanged();

private:
	MainWindow *m_mainWindow;
	MdiWidget *m_mdi;
	QPointer<Window> m_activeWindow;
	QPointer<Window> m_peekedWindow;
	int m_restoreTimer;
	bool m_isRestored;

signals:
	void arbitraryActionsStateChanged(const QVector<int> &identifiers);
};

}

#endif
