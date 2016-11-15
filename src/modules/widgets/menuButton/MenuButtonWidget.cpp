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

#include "MenuButtonWidget.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Menu.h"
#include "../../../ui/ToolBarWidget.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>

namespace Otter
{

MenuButtonWidget::MenuButtonWidget(const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_menu(new Menu(Menu::NoMenuRole, this)),
	m_isHidden(false)
{
	setIcon(ThemesManager::getIcon(QLatin1String("otter-browser"), false));
	setText(definition.options.value(QLatin1String("text"), tr("Menu")).toString());
	setMenu(m_menu);
	setPopupMode(QToolButton::InstantPopup);
	setButtonStyle(Qt::ToolButtonTextBesideIcon);
	toolBarModified(ToolBarsManager::MenuBar);

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar)
	{
		disconnect(toolBar, SIGNAL(buttonStyleChanged(Qt::ToolButtonStyle)), this, SLOT(setButtonStyle(Qt::ToolButtonStyle)));
	}

	connect(ToolBarsManager::getInstance(), SIGNAL(toolBarModified(int)), this, SLOT(toolBarModified(int)));
	connect(m_menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
}

void MenuButtonWidget::toolBarModified(int identifier)
{
	if (identifier == ToolBarsManager::MenuBar)
	{
		m_isHidden = (ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar).normalVisibility == ToolBarsManager::AlwaysVisibleToolBar);

		updateGeometry();
	}
}

void MenuButtonWidget::updateMenu()
{
	disconnect(m_menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));

	m_menu->load(QLatin1String("menu/menuButton.json"));

	connect(m_menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
}

QSize MenuButtonWidget::minimumSizeHint() const
{
	if (m_isHidden)
	{
		return QSize(0, 0);
	}

	return ToolButtonWidget::minimumSizeHint();
}

QSize MenuButtonWidget::sizeHint() const
{
	if (m_isHidden)
	{
		return QSize(0, 0);
	}

	return ToolButtonWidget::sizeHint();
}

}
