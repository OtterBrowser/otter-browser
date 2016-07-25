/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ContentBlockingInformationWidget.h"
#include "../ContentsWidget.h"
#include "../ToolBarWidget.h"
#include "../Window.h"
#include "../../core/ThemesManager.h"

namespace Otter
{

ContentBlockingInformationWidget::ContentBlockingInformationWidget(Window *window, const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_window(window),
	m_amount(0)
{
	setIcon(ThemesManager::getIcon(QLatin1String("content-blocking")));
	setWindow(window);

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
	{
		connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
	}
}

void ContentBlockingInformationWidget::handleRequest(const NetworkManager::ResourceInformation &request)
{
	Q_UNUSED(request)

	++m_amount;

	update();
}

void ContentBlockingInformationWidget::setWindow(Window *window)
{
	if (m_window)
	{
		disconnect(m_window->getContentsWidget(), SIGNAL(requestBlocked(NetworkManager::ResourceInformation)), this, SLOT(handleRequest(NetworkManager::ResourceInformation)));
	}

	m_window = window;
	m_amount = 0;

	if (window)
	{
		m_amount = window->getContentsWidget()->getBlockedRequests().count();

		connect(m_window->getContentsWidget(), SIGNAL(requestBlocked(NetworkManager::ResourceInformation)), this, SLOT(handleRequest(NetworkManager::ResourceInformation)));
	}

	update();
}

}
