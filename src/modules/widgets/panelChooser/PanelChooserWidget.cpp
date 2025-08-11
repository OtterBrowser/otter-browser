/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

PanelChooserWidget::PanelChooserWidget(const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_toolBarIdentifier(-1)
{
	const QVariant toolBar(definition.parameters.value(QLatin1String("sidebar")));

	if (!toolBar.isNull())
	{
		m_toolBarIdentifier = ((toolBar.type() == QVariant::Int) ? toolBar.toInt() : ToolBarsManager::getToolBarIdentifier(toolBar.toString()));
	}

	QMenu *menu(new QMenu(this));

	setMenu(menu);
	setPopupMode(QToolButton::InstantPopup);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	updateText();

	connect(ToolBarsManager::getInstance(), &ToolBarsManager::toolBarModified, this, [=](int identifier)
	{
		if (identifier == m_toolBarIdentifier)
		{
			updateText();
		}
	});
	connect(menu, &QMenu::aboutToShow, this, [=]()
	{
		menu->clear();

		if (m_toolBarIdentifier >= 0)
		{
			const QStringList panels(ToolBarsManager::getToolBarDefinition(m_toolBarIdentifier).panels);

			for (int i = 0; i < panels.count(); ++i)
			{
				const QString panel(panels.at(i));

				menu->addAction(SidebarWidget::getPanelTitle(panel))->setData(panel);
			}
		}
	});
	connect(menu, &QMenu::triggered, this, [=](QAction *action)
	{
		if (m_toolBarIdentifier >= 0)
		{
			ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(m_toolBarIdentifier));
			definition.currentPanel = action->data().toString();

			ToolBarsManager::setToolBar(definition);
		}
	});
}

void PanelChooserWidget::changeEvent(QEvent *event)
{
	QToolButton::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		updateText();
	}
}

void PanelChooserWidget::updateText()
{
	if (m_toolBarIdentifier >= 0)
	{
		setText(SidebarWidget::getPanelTitle(ToolBarsManager::getToolBarDefinition(m_toolBarIdentifier).currentPanel));
	}
}

QSize PanelChooserWidget::minimumSizeHint() const
{
	return {0, 0};
}

}
