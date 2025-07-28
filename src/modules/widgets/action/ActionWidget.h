/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_ACTIONWIDGET_H
#define OTTER_ACTIONWIDGET_H

#include "../../../core/ActionsManager.h"
#include "../../../core/GesturesController.h"
#include "../../../core/SessionsManager.h"
#include "../../../ui/ToolButtonWidget.h"

#include <QtCore/QPointer>

namespace Otter
{

class Action;
class Window;

class ActionWidget : public ToolButtonWidget, public GesturesController
{
	Q_OBJECT

public:
	explicit ActionWidget(int identifier, Window *window, const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent = nullptr);

	int getIdentifier() const;

protected:
	void mouseReleaseEvent(QMouseEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	bool event(QEvent *event) override;

protected slots:
	void setWindow(Window *window);

private:
	Action *m_action;
};

class NavigationActionWidget final : public ActionWidget
{
	Q_OBJECT

public:
	explicit NavigationActionWidget(Window *window, const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent = nullptr);

	bool eventFilter(QObject *object, QEvent *event) override;

protected:
	void addMenuEntry(int index, const Session::Window::History::Entry &entry);
	bool event(QEvent *event) override;

protected slots:
	void updateMenu();

private:
	QPointer<Window> m_window;
};

}

#endif
