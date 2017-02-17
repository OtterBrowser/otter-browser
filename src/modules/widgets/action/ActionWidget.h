/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../ui/ToolButtonWidget.h"

#include <QtCore/QPointer>

namespace Otter
{

class Window;

class ActionWidget : public ToolButtonWidget
{
	Q_OBJECT

public:
	explicit ActionWidget(int identifier, Window *window, const ActionsManager::ActionEntryDefinition &definition, QWidget *parent = nullptr);

	Window* getWindow() const;
	int getIdentifier() const;

protected:
	void mouseReleaseEvent(QMouseEvent *event) override;
	bool event(QEvent *event) override;

protected slots:
	void resetAction();
	void setWindow(Window *window);

private:
	QPointer<Window> m_window;
	int m_identifier;
};

}

#endif
