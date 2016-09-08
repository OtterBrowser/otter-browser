/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PanelChooserWidget.h"
#include "../../../core/SettingsManager.h"
#include "../../../ui/SidebarWidget.h"

#include <QtWidgets/QMenu>

namespace Otter
{

PanelChooserWidget::PanelChooserWidget(const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : ToolButtonWidget(definition, parent)
{
	setMenu(new QMenu(this));
	setPopupMode(QToolButton::InstantPopup);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	optionChanged(SettingsManager::Sidebar_CurrentPanelOption, SettingsManager::getValue(SettingsManager::Sidebar_CurrentPanelOption));

	connect(menu(), SIGNAL(aboutToShow()), this, SLOT(menuAboutToShow()));
	connect(menu(), SIGNAL(triggered(QAction*)), this, SLOT(selectPanel(QAction*)));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int,QVariant)));
}

void PanelChooserWidget::changeEvent(QEvent *event)
{
	QToolButton::changeEvent(event);

	if (event->type() ==  QEvent::LanguageChange)
	{
		setText(SidebarWidget::getPanelTitle(SettingsManager::getValue(SettingsManager::Sidebar_CurrentPanelOption).toString()));
	}
}

void PanelChooserWidget::optionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::Sidebar_CurrentPanelOption)
	{
		setText(SidebarWidget::getPanelTitle(value.toString()));
	}
}

void PanelChooserWidget::menuAboutToShow()
{
	menu()->clear();

	const QStringList panels(SettingsManager::getValue(SettingsManager::Sidebar_PanelsOption).toStringList());

	for (int i = 0; i < panels.count(); ++i)
	{
		menu()->addAction(SidebarWidget::getPanelTitle(panels[i]))->setData(panels[i]);
	}
}

void PanelChooserWidget::selectPanel(QAction *action)
{
	SettingsManager::setValue(SettingsManager::Sidebar_CurrentPanelOption, action->data());
}

QSize PanelChooserWidget::minimumSizeHint() const
{
	return QSize(0, 0);
}

}
