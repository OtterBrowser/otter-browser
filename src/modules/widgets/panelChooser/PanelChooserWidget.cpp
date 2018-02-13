/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PanelChooserWidget.h"
#include "../../../ui/SidebarWidget.h"

#include <QtWidgets/QMenu>

namespace Otter
{

PanelChooserWidget::PanelChooserWidget(const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : ToolButtonWidget(definition, parent)
{
	setMenu(new QMenu(this));
	setPopupMode(QToolButton::InstantPopup);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	updateText();

	connect(ToolBarsManager::getInstance(), &ToolBarsManager::toolBarModified, this, &PanelChooserWidget::handleToolBarModified);
	connect(menu(), &QMenu::aboutToShow, this, &PanelChooserWidget::populateMenu);
	connect(menu(), &QMenu::triggered, this, &PanelChooserWidget::selectPanel);
}

void PanelChooserWidget::changeEvent(QEvent *event)
{
	QToolButton::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		updateText();
	}
}

void PanelChooserWidget::populateMenu()
{
	menu()->clear();

	const int identifier(getSideBarIdentifier());

	if (identifier >= 0)
	{
		const QStringList panels(ToolBarsManager::getToolBarDefinition(identifier).panels);

		for (int i = 0; i < panels.count(); ++i)
		{
			menu()->addAction(SidebarWidget::getPanelTitle(panels.at(i)))->setData(panels.at(i));
		}
	}
}

void PanelChooserWidget::selectPanel(QAction *action)
{
	const int identifier(getSideBarIdentifier());

	if (identifier >= 0)
	{
		ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(identifier));
		definition.currentPanel = action->data().toString();

		ToolBarsManager::setToolBar(definition);
	}
}

void PanelChooserWidget::handleToolBarModified(int identifier)
{
	if (identifier == getSideBarIdentifier())
	{
		updateText();
	}
}

void PanelChooserWidget::updateText()
{
	const int identifier(getSideBarIdentifier());

	if (identifier >= 0)
	{
		setText(SidebarWidget::getPanelTitle(ToolBarsManager::getToolBarDefinition(identifier).currentPanel));
	}
}

QSize PanelChooserWidget::minimumSizeHint() const
{
	return QSize(0, 0);
}

int PanelChooserWidget::getSideBarIdentifier() const
{
	const QVariant toolBar(getParameters().value(QLatin1String("sidebar")));

	if (toolBar.isNull())
	{
		return -1;
	}

	if (toolBar.type() == QVariant::Int)
	{
		return toolBar.toInt();
	}

	return ToolBarsManager::getToolBarIdentifier(toolBar.toString());
}

}
