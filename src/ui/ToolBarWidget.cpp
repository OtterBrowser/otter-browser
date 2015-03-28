/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ToolBarWidget.h"
#include "ContentsWidget.h"
#include "MainWindow.h"
#include "Menu.h"
#include "TabBarWidget.h"
#include "Window.h"
#include "toolbars/ActionWidget.h"
#include "toolbars/AddressWidget.h"
#include "toolbars/BookmarkWidget.h"
#include "toolbars/GoBackActionWidget.h"
#include "toolbars/GoForwardActionWidget.h"
#include "toolbars/MenuButtonWidget.h"
#include "toolbars/PanelChooserWidget.h"
#include "toolbars/SearchWidget.h"
#include "toolbars/StatusMessageWidget.h"
#include "toolbars/ZoomWidget.h"
#include "../core/BookmarksManager.h"
#include "../core/Utils.h"
#include "../core/WindowsManager.h"

#include <QtGui/QMouseEvent>

namespace Otter
{

ToolBarWidget::ToolBarWidget(const QString &identifier, Window *window, QWidget *parent) : QToolBar(parent),
	m_mainWindow(MainWindow::findMainWindow(parent)),
	m_window(window),
	m_identifier(identifier)
{
	setStyleSheet(QLatin1String("QToolBar {padding:0 3px;spacing:3px;}"));
	setAllowedAreas(Qt::AllToolBarAreas);
	setFloatable(false);

	if (identifier == QLatin1String("StatusBar"))
	{
		setFixedHeight(ActionsManager::getToolBarDefinition(m_identifier).iconSize);
	}

	if (!identifier.isEmpty())
	{
		setObjectName(identifier);
		setup();

		if (m_mainWindow)
		{
			connect(m_mainWindow->getActionsManager(), SIGNAL(toolBarModified(QString)), this, SLOT(toolBarModified(QString)));
			connect(m_mainWindow->getActionsManager(), SIGNAL(toolBarRemoved(QString)), this, SLOT(toolBarRemoved(QString)));
		}

		connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(notifyAreaChanged()));
	}

	if (m_mainWindow && (parent == m_mainWindow || m_identifier.isEmpty()))
	{
		connect(m_mainWindow->getWindowsManager(), SIGNAL(currentWindowChanged(qint64)), this, SLOT(notifyWindowChanged(qint64)));
	}
}

void ToolBarWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);

	if (objectName() != QLatin1String("TabBar"))
	{
		menu.addAction(ActionsManager::getAction(Action::LockToolBarsAction, this));
		menu.exec(event->globalPos());

		return;
	}

	menu.addAction(ActionsManager::getAction(Action::NewTabAction, this));
	menu.addAction(ActionsManager::getAction(Action::NewTabPrivateAction, this));
	menu.addSeparator();

	QMenu *customizeMenu = menu.addMenu(tr("Customize"));
	QAction *cycleAction = customizeMenu->addAction(tr("Switch tabs using the mouse wheel"));
	cycleAction->setCheckable(true);
	cycleAction->setChecked(!SettingsManager::getValue(QLatin1String("TabBar/RequireModifierToSwitchTabOnScroll")).toBool());
	cycleAction->setEnabled(m_mainWindow->getTabBar());

	customizeMenu->addSeparator();
	customizeMenu->addAction(ActionsManager::getAction(Action::LockToolBarsAction, this));

	if (m_mainWindow->getTabBar())
	{
		connect(cycleAction, SIGNAL(toggled(bool)), m_mainWindow->getTabBar(), SLOT(setCycle(bool)));
	}

	menu.exec(event->globalPos());
}

void ToolBarWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && objectName() == QLatin1String("TabBar"))
	{
		ActionsManager::triggerAction((event->modifiers().testFlag(Qt::ShiftModifier) ? Action::NewTabPrivateAction : Action::NewTabAction), this);
	}
}

void ToolBarWidget::setup()
{
	const ToolBarDefinition definition = ActionsManager::getToolBarDefinition(m_identifier);

	clear();

	setToolButtonStyle(definition.buttonStyle);

	if (definition.iconSize > 0)
	{
		setIconSize(QSize(definition.iconSize, definition.iconSize));
	}

	if (!definition.bookmarksPath.isEmpty())
	{
		updateBookmarks();

		connect(BookmarksManager::getInstance(), SIGNAL(modelModified()), this, SLOT(updateBookmarks()));

		return;
	}

	for (int i = 0; i < definition.actions.count(); ++i)
	{
		if (definition.actions.at(i).action == QLatin1String("separator"))
		{
			addSeparator();
		}
		else
		{
			addWidget(createWidget(definition.actions.at(i), m_window, this));
		}
	}
}

void ToolBarWidget::toolBarModified(const QString &identifier)
{
	if (identifier == m_identifier)
	{
		setup();
	}
}

void ToolBarWidget::toolBarRemoved(const QString &identifier)
{
	if (identifier == m_identifier)
	{
		deleteLater();
	}
}

void ToolBarWidget::notifyAreaChanged()
{
	if (m_mainWindow)
	{
		emit areaChanged(m_mainWindow->toolBarArea(this));
	}
}

void ToolBarWidget::notifyWindowChanged(qint64 identifier)
{
	m_window = m_mainWindow->getWindowsManager()->getWindowByIdentifier(identifier);

	emit windowChanged(m_window);
}

void ToolBarWidget::updateBookmarks()
{
	const ToolBarDefinition definition = ActionsManager::getToolBarDefinition(m_identifier);

	clear();

	BookmarksItem *item = (definition.bookmarksPath.startsWith(QLatin1Char('#')) ? BookmarksManager::getBookmark(definition.bookmarksPath.mid(1).toULongLong()) : BookmarksManager::getModel()->getItem(definition.bookmarksPath));

	if (!item)
	{
		return;
	}

	for (int i = 0; i < item->rowCount(); ++i)
	{
		BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(item->child(i));

		if (bookmark)
		{
			if (static_cast<BookmarksModel::BookmarkType>(bookmark->data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::SeparatorBookmark)
			{
				addSeparator();
			}
			else
			{
				addWidget(new BookmarkWidget(bookmark, this));
			}
		}
	}
}

QWidget* ToolBarWidget::createWidget(const ToolBarActionDefinition &definition, Window *window, ToolBarWidget *toolBar)
{
	if (definition.action == QLatin1String("spacer"))
	{
		QWidget *spacer = new QWidget(toolBar);
		spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

		return spacer;
	}

	if (definition.action == QLatin1String("ClosedWindowsWidget"))
	{
		QAction *closedWindowsAction = new QAction(Utils::getIcon(QLatin1String("user-trash")), tr("Closed Tabs"), toolBar);
		closedWindowsAction->setMenu(new Menu(Menu::ClosedWindowsMenu, toolBar));
		closedWindowsAction->setEnabled(false);

		QToolButton *closedWindowsMenuButton = new QToolButton(toolBar);
		closedWindowsMenuButton->setDefaultAction(closedWindowsAction);
		closedWindowsMenuButton->setAutoRaise(true);
		closedWindowsMenuButton->setPopupMode(QToolButton::InstantPopup);

		return closedWindowsMenuButton;
	}

	if (definition.action == QLatin1String("MenuButtonWidget"))
	{
		return new MenuButtonWidget(toolBar);
	}

	if (definition.action == QLatin1String("AddressWidget"))
	{
		return new AddressWidget(window, toolBar);
	}

	if (definition.action == QLatin1String("PanelChooserWidget"))
	{
		return new PanelChooserWidget(toolBar);
	}

	if (definition.action == QLatin1String("SearchWidget"))
	{
		return new SearchWidget(window, toolBar);
	}

	if (definition.action == QLatin1String("StatusMessageWidget"))
	{
		return new StatusMessageWidget(toolBar);
	}

	if (definition.action == QLatin1String("TabBarWidget"))
	{
		if (!toolBar || toolBar->objectName() != QLatin1String("TabBar"))
		{
			return NULL;
		}

		MainWindow *mainWindow = MainWindow::findMainWindow(toolBar);

		if (!mainWindow || mainWindow->getTabBar())
		{
			return NULL;
		}

		return new TabBarWidget(toolBar);
	}

	if (definition.action == QLatin1String("ZoomWidget"))
	{
		return new ZoomWidget(toolBar);
	}

	if (definition.action.startsWith(QLatin1String("bookmarks:")))
	{
		BookmarksItem *bookmark = (definition.action.startsWith(QLatin1String("bookmarks:/")) ? BookmarksManager::getModel()->getItem(definition.action.mid(11)) : BookmarksManager::getBookmark(definition.action.mid(11).toULongLong()));

		if (bookmark)
		{
			return new BookmarkWidget(bookmark, toolBar);
		}
	}

	if (definition.action.endsWith(QLatin1String("Action")))
	{
		const int identifier = ActionsManager::getActionIdentifier(definition.action.left(definition.action.length() - 6));

		if (identifier >= 0)
		{
			if (identifier == Action::GoBackAction)
			{
				return new GoBackActionWidget(window, toolBar);
			}

			if (identifier == Action::GoForwardAction)
			{
				return new GoForwardActionWidget(window, toolBar);
			}

			return new ActionWidget(identifier, window, toolBar);
		}
	}

	return NULL;
}

int ToolBarWidget::getMaximumButtonSize() const
{
	return ActionsManager::getToolBarDefinition(m_identifier).maximumButtonSize;
}

}
