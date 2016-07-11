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
* along with parent program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "WidgetFactory.h"
#include "TabBarWidget.h"
#include "MainWindow.h"
#include "Window.h"
#include "toolbars/ActionWidget.h"
#include "toolbars/AddressWidget.h"
#include "toolbars/BookmarkWidget.h"
#include "toolbars/ContentBlockingInformationWidget.h"
#include "toolbars/GoBackActionWidget.h"
#include "toolbars/GoForwardActionWidget.h"
#include "toolbars/MenuButtonWidget.h"
#include "toolbars/PanelChooserWidget.h"
#include "toolbars/ProgressInformationWidget.h"
#include "toolbars/SearchWidget.h"
#include "toolbars/StatusMessageWidget.h"
#include "toolbars/ZoomWidget.h"
#include "../core/BookmarksManager.h"
#include "../modules/windows/addons/AddonsContentsWidget.h"
#include "../modules/windows/bookmarks/BookmarksContentsWidget.h"
#include "../modules/windows/cache/CacheContentsWidget.h"
#include "../modules/windows/configuration/ConfigurationContentsWidget.h"
#include "../modules/windows/cookies/CookiesContentsWidget.h"
#include "../modules/windows/history/HistoryContentsWidget.h"
#include "../modules/windows/notes/NotesContentsWidget.h"
#include "../modules/windows/transfers/TransfersContentsWidget.h"
#include "../modules/windows/web/WebContentsWidget.h"

namespace Otter
{

namespace WidgetFactory
{

QWidget* createToolBarItem(const ActionsManager::ActionEntryDefinition &definition, QWidget *parent, Window *window)
{
	if (definition.action == QLatin1String("spacer"))
	{
		QWidget *spacer(new QWidget(parent));
		spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

		return spacer;
	}

	if (!definition.entries.isEmpty())
	{
		return new ToolButtonWidget(definition, parent);
	}

	if (definition.action == QLatin1String("AddressWidget"))
	{
		return new AddressWidget(window, parent);
	}

	if (definition.action == QLatin1String("ContentBlockingInformationWidget"))
	{
		return new ContentBlockingInformationWidget(definition, parent);
	}

	if (definition.action == QLatin1String("MenuButtonWidget"))
	{
		return new MenuButtonWidget(definition, parent);
	}

	if (definition.action == QLatin1String("PanelChooserWidget"))
	{
		return new PanelChooserWidget(definition, parent);
	}

	if (definition.action == QLatin1String("ProgressInformationDocumentProgressWidget") || definition.action == QLatin1String("ProgressInformationTotalSizeWidget") || definition.action == QLatin1String("ProgressInformationElementsWidget") || definition.action == QLatin1String("ProgressInformationSpeedWidget") || definition.action == QLatin1String("ProgressInformationElapsedTimeWidget") || definition.action == QLatin1String("ProgressInformationMessageWidget"))
	{
		return new ProgressInformationWidget(window, definition, parent);
	}

	if (definition.action == QLatin1String("SearchWidget"))
	{
		SearchWidget *searchWidget(new SearchWidget(window, parent));
		searchWidget->setOptions(definition.options);

		return searchWidget;
	}

	if (definition.action == QLatin1String("StatusMessageWidget"))
	{
		return new StatusMessageWidget(parent);
	}

	if (definition.action == QLatin1String("TabBarWidget"))
	{
		return new TabBarWidget(parent);
	}

	if (definition.action == QLatin1String("ZoomWidget"))
	{
		return new ZoomWidget(parent);
	}

	if (definition.action.startsWith(QLatin1String("bookmarks:")))
	{
		BookmarksItem *bookmark(definition.action.startsWith(QLatin1String("bookmarks:/")) ? BookmarksManager::getModel()->getItem(definition.action.mid(11)) : BookmarksManager::getBookmark(definition.action.mid(10).toULongLong()));

		if (bookmark)
		{
			return new BookmarkWidget(bookmark, definition, parent);
		}
	}

	if (definition.action.endsWith(QLatin1String("Action")))
	{
		const int identifier(ActionsManager::getActionIdentifier(definition.action.left(definition.action.length() - 6)));

		if (identifier >= 0)
		{
			ActionWidget *actionWidget(NULL);

			if (identifier == ActionsManager::GoBackAction)
			{
				actionWidget = new GoBackActionWidget(window, definition, parent);
			}
			else if (identifier == ActionsManager::GoForwardAction)
			{
				actionWidget = new GoForwardActionWidget(window, definition, parent);
			}
			else
			{
				actionWidget = new ActionWidget(identifier, window, definition, parent);;
			}

			actionWidget->setOptions(definition.options);

			return actionWidget;
		}
	}

	if (definition.action.endsWith(QLatin1String("Menu")))
	{
		return new ToolButtonWidget(definition, parent);
	}

	return NULL;

}

QWidget* createSidebarPanel(const QString &panel, MainWindow *mainWindow)
{
	if (panel == QLatin1String("addons"))
	{
		return new AddonsContentsWidget(NULL);
	}

	if (panel == QLatin1String("bookmarks"))
	{
		return new BookmarksContentsWidget(NULL);
	}

	if (panel == QLatin1String("cache"))
	{
		return new CacheContentsWidget(NULL);
	}

	if (panel == QLatin1String("config"))
	{
		return new ConfigurationContentsWidget(NULL);
	}

	if (panel == QLatin1String("cookies"))
	{
		return new CookiesContentsWidget(NULL);
	}

	if (panel == QLatin1String("history"))
	{
		return new HistoryContentsWidget(NULL);
	}

	if (panel == QLatin1String("notes"))
	{
		return new NotesContentsWidget(NULL);
	}

	if (panel == QLatin1String("transfers"))
	{
		return new TransfersContentsWidget(NULL);
	}

	if (panel.startsWith(QLatin1String("web:")))
	{
		WebContentsWidget *webWidget(new WebContentsWidget((mainWindow ? mainWindow->getWindowsManager()->isPrivate() : true), NULL, NULL));
		webWidget->setUrl(panel.section(QLatin1Char(':'), 1, -1), false);

		return webWidget;
	}

	return NULL;
}

}

}
