/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2026 Water Phoenix Browser contributors
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

#ifndef OTTER_WINDOWCONTROLSWIDGET_H
#define OTTER_WINDOWCONTROLSWIDGET_H

#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

namespace Otter
{

class MainWindow;

class WindowControlsWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit WindowControlsWidget(MainWindow *mainWindow, QWidget *parent = nullptr);

	bool eventFilter(QObject *object, QEvent *event) override;

protected slots:
	void updateMaximizeButton();
	void handleMinimize();
	void handleMaximizeRestore();
	void handleClose();

private:
	MainWindow *m_mainWindow;
	QToolButton *m_minimizeButton;
	QToolButton *m_maximizeButton;
	QToolButton *m_closeButton;
};

}

#endif
