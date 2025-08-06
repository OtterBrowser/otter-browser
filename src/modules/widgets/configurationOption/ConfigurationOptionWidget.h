/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 - 2017 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_CONFIGURATIONOPTIONWIDGET_H
#define OTTER_CONFIGURATIONOPTIONWIDGET_H

#include "../../../ui/Window.h"

#include <QtWidgets/QWidget>

namespace Otter
{

class OptionWidget;

class ConfigurationOptionWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit ConfigurationOptionWidget(Window *window, const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent = nullptr);

protected:
	enum OptionScope
	{
		WindowScope = 0,
		GlobalScope
	};

protected slots:
	void handleOptionChanged(int option, const QVariant &value);
	void setWindow(Window *window);
	void save();

private:
	OptionWidget *m_optionWidget;
	QPointer<Window> m_window;
	OptionScope m_scope;
	int m_identifier;
};

}

#endif
