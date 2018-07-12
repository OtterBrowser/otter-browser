/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "MainWindow.h"
#include "TabBarWidget.h"
#include "ToolBarWidget.h"
#include "Window.h"
#include "../core/BookmarksManager.h"
#include "../modules/widgets/action/ActionWidget.h"
#include "../modules/widgets/address/AddressWidget.h"
#include "../modules/widgets/bookmark/BookmarkWidget.h"
#include "../modules/widgets/configurationOption/ConfigurationOptionWidget.h"
#include "../modules/widgets/contentBlockingInformation/ContentBlockingInformationWidget.h"
#include "../modules/widgets/errorConsole/ErrorConsoleWidget.h"
#include "../modules/widgets/menuButton/MenuButtonWidget.h"
#include "../modules/widgets/panelChooser/PanelChooserWidget.h"
#include "../modules/widgets/progressInformation/ProgressInformationWidget.h"
#include "../modules/widgets/privateWindowIndicator/PrivateWindowIndicatorWidget.h"
#include "../modules/widgets/search/SearchWidget.h"
#include "../modules/widgets/statusMessage/StatusMessageWidget.h"
#include "../modules/widgets/transfers/TransfersWidget.h"
#include "../modules/widgets/zoom/ZoomWidget.h"
#include "../modules/windows/addons/AddonsContentsWidget.h"
#include "../modules/windows/bookmarks/BookmarksContentsWidget.h"
#include "../modules/windows/cache/CacheContentsWidget.h"
#include "../modules/windows/configuration/ConfigurationContentsWidget.h"
#include "../modules/windows/cookies/CookiesContentsWidget.h"
#include "../modules/windows/feeds/FeedsContentsWidget.h"
#include "../modules/windows/history/HistoryContentsWidget.h"
#include "../modules/windows/links/LinksContentsWidget.h"
#include "../modules/windows/notes/NotesContentsWidget.h"
#include "../modules/windows/pageInformation/PageInformationContentsWidget.h"
#include "../modules/windows/passwords/PasswordsContentsWidget.h"
#include "../modules/windows/preferences/PreferencesContentsWidget.h"
#include "../modules/windows/tabHistory/TabHistoryContentsWidget.h"
#include "../modules/windows/transfers/TransfersContentsWidget.h"
#include "../modules/windows/web/WebContentsWidget.h"
#include "../modules/windows/windows/WindowsContentsWidget.h"

#include <QtWidgets/QSizeGrip>

namespace Otter
{

namespace WidgetFactory
{

ToolBarWidget* createToolBar(int identifier, Window *window, QWidget *parent)
{
	if (identifier == ToolBarsManager::TabBar)
	{
		return new TabBarToolBarWidget(identifier, window, parent);
	}

	return new ToolBarWidget(identifier, window, parent);
}

QWidget* createToolBarItem(const ToolBarsManager::ToolBarDefinition::Entry &definition, Window *window, QWidget *parent)
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

	if (definition.action == QLatin1String("ConfigurationOptionWidget"))
	{
		return new ConfigurationOptionWidget(window, definition, parent);
	}

	if (definition.action == QLatin1String("ErrorConsoleWidget"))
	{
		return new ErrorConsoleWidget(parent);
	}

	if (definition.action == QLatin1String("ContentBlockingInformationWidget"))
	{
		return new ContentBlockingInformationWidget(window, definition, parent);
	}

	if (definition.action == QLatin1String("MenuButtonWidget"))
	{
		return new MenuButtonWidget(definition, parent);
	}

	if (definition.action == QLatin1String("PanelChooserWidget"))
	{
		return new PanelChooserWidget(definition, parent);
	}

	if (definition.action == QLatin1String("PrivateWindowIndicatorWidget"))
	{
		return new PrivateWindowIndicatorWidget(definition, parent);
	}

	if (definition.action == QLatin1String("ProgressInformationDocumentProgressWidget") || definition.action == QLatin1String("ProgressInformationTotalProgressWidget") || definition.action == QLatin1String("ProgressInformationTotalSizeWidget") || definition.action == QLatin1String("ProgressInformationElementsWidget") || definition.action == QLatin1String("ProgressInformationSpeedWidget") || definition.action == QLatin1String("ProgressInformationElapsedTimeWidget") || definition.action == QLatin1String("ProgressInformationMessageWidget"))
	{
		return new ProgressInformationWidget(window, definition, parent);
	}

	if (definition.action == QLatin1String("SearchWidget"))
	{
		SearchWidget *searchWidget(new SearchWidget(window, parent));
		searchWidget->setOptions(definition.options);

		return searchWidget;
	}

	if (definition.action == QLatin1String("SizeGripWidget"))
	{
		return new QSizeGrip(parent);
	}

	if (definition.action == QLatin1String("StatusMessageWidget"))
	{
		return new StatusMessageWidget(parent);
	}

	if (definition.action == QLatin1String("TransfersWidget"))
	{
		return new TransfersWidget(definition, parent);
	}

	if (definition.action == QLatin1String("TabBarWidget"))
	{
		return new TabBarWidget(parent);
	}

	if (definition.action == QLatin1String("ZoomWidget"))
	{
		return new ZoomWidget(window, parent);
	}

	if (definition.action.startsWith(QLatin1String("bookmarks:")))
	{
		BookmarksModel::Bookmark *bookmark(definition.action.startsWith(QLatin1String("bookmarks:/")) ? BookmarksManager::getModel()->getBookmarkByPath(definition.action.mid(11)) : BookmarksManager::getBookmark(definition.action.mid(10).toULongLong()));

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
			ActionWidget *actionWidget(nullptr);

			if (identifier == ActionsManager::GoBackAction || identifier == ActionsManager::GoForwardAction)
			{
				actionWidget = new NavigationActionWidget(window, definition, parent);
			}
			else
			{
				actionWidget = new ActionWidget(identifier, window, definition, parent);
			}

			actionWidget->setOptions(definition.options);

			return actionWidget;
		}
	}

	if (definition.action.endsWith(QLatin1String("Menu")))
	{
		return new ToolButtonWidget(definition, parent);
	}

	return nullptr;
}

ContentsWidget* createContentsWidget(const QString &identifier, const QVariantMap &parameters, Window *window, QWidget *parent)
{
	if (identifier == QLatin1String("addons"))
	{
		return new AddonsContentsWidget(parameters, window, parent);
	}

	if (identifier == QLatin1String("bookmarks"))
	{
		return new BookmarksContentsWidget(parameters, window, parent);
	}

	if (identifier == QLatin1String("cache"))
	{
		return new CacheContentsWidget(parameters, window, parent);
	}

	if (identifier == QLatin1String("config"))
	{
		return new ConfigurationContentsWidget(parameters, window, parent);
	}

	if (identifier == QLatin1String("cookies"))
	{
		return new CookiesContentsWidget(parameters, window, parent);
	}

	if (identifier == QLatin1String("feeds"))
	{
		return new FeedsContentsWidget(parameters, parent);
	}

	if (identifier == QLatin1String("history"))
	{
		return new HistoryContentsWidget(parameters, window, parent);
	}

	if (identifier == QLatin1String("notes"))
	{
		return new NotesContentsWidget(parameters, window, parent);
	}

	if (identifier == QLatin1String("passwords"))
	{
		return new PasswordsContentsWidget(parameters, window, parent);
	}

	if (identifier == QLatin1String("preferences"))
	{
		return new PreferencesContentsWidget(parameters, window, parent);
	}

	if (identifier == QLatin1String("transfers"))
	{
		return new TransfersContentsWidget(parameters, window, parent);
	}

	if (identifier == QLatin1String("windows"))
	{
		return new WindowsContentsWidget(parameters, window, parent);
	}

	return nullptr;
}

ContentsWidget* createSidebarPanel(const QString &identifier, int sidebar, MainWindow *mainWindow, QWidget *parent)
{
	QVariantMap parameters({{QLatin1String("sidebar"), sidebar}});

	if (identifier == QLatin1String("links"))
	{
		return new LinksContentsWidget(parameters, parent);
	}

	if (identifier == QLatin1String("pageInformation"))
	{
		return new PageInformationContentsWidget(parameters, parent);
	}

	if (identifier == QLatin1String("tabHistory"))
	{
		return new TabHistoryContentsWidget(parameters, parent);
	}

	if (identifier.startsWith(QLatin1String("web:")))
	{
		if (!mainWindow || mainWindow->isPrivate())
		{
			parameters[QLatin1String("hints")] = SessionsManager::PrivateOpen;
		}

		WebContentsWidget *webWidget(new WebContentsWidget(parameters, {}, nullptr, nullptr, nullptr));
		webWidget->setUrl(identifier.section(QLatin1Char(':'), 1, -1), false);

		return webWidget;
	}

	if (!AddonsManager::getSpecialPage(identifier).types.testFlag(AddonsManager::SpecialPageInformation::SidebarPanelType))
	{
		return nullptr;
	}

	return createContentsWidget(identifier, parameters, nullptr, parent);
}

}

}
