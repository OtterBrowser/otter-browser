/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "NavigationActionWidget.h"
#include "../../../core/Application.h"
#include "../../../core/GesturesManager.h"
#include "../../../core/HistoryManager.h"
#include "../../../ui/Action.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtGui/QHelpEvent>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>

namespace Otter
{

NavigationActionWidget::NavigationActionWidget(Window *window, const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : ActionWidget(((definition.action == QLatin1String("GoBackAction")) ? ActionsManager::GoBackAction : ActionsManager::GoForwardAction), window, definition, parent)
{
	setMenu(new QMenu(this));
	setPopupMode(QToolButton::DelayedPopup);
	setContextMenuPolicy(Qt::DefaultContextMenu);

	menu()->installEventFilter(this);

	connect(menu(), SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
	connect(menu(), SIGNAL(triggered(QAction*)), this, SLOT(goToHistoryIndex(QAction*)));
}

void NavigationActionWidget::goToHistoryIndex(QAction *action)
{
	if (getWindow() && action && action->data().type() == QVariant::Int)
	{
		getWindow()->getContentsWidget()->goToHistoryIndex(action->data().toInt());
	}
}

void NavigationActionWidget::addMenuEntry(int index, const WindowHistoryEntry &entry)
{
	QString title(entry.title);
	QAction *action(menu()->addAction(HistoryManager::getIcon(QUrl(entry.url)), (title.isEmpty() ? tr("(Untitled)") : title.replace(QLatin1Char('&'), QLatin1String("&&")))));
	action->setData(index);
	action->setStatusTip(entry.url);
}

void NavigationActionWidget::updateMenu()
{
	if (!menu() || !getWindow())
	{
		return;
	}

	menu()->clear();

	const WindowHistoryInformation history(getWindow()->getContentsWidget()->getHistory());

	if (getIdentifier() == ActionsManager::GoBackAction)
	{
		for (int i = (history.index - 1); i >= 0; --i)
		{
			addMenuEntry(i, history.entries.at(i));
		}
	}
	else
	{
		for (int i = (history.index + 1); i < history.entries.count(); ++i)
		{
			addMenuEntry(i, history.entries.at(i));
		}
	}
}

bool NavigationActionWidget::event(QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::Wheel)
	{
		GesturesManager::startGesture(this, event, {GesturesManager::ToolBarContext, GesturesManager::GenericContext});
	}

	if (event->type() == QEvent::ContextMenu)
	{
		QContextMenuEvent *contextMenuEvent(static_cast<QContextMenuEvent*>(event));

		if (contextMenuEvent)
		{
			if (contextMenuEvent->reason() == QContextMenuEvent::Mouse)
			{
				contextMenuEvent->accept();

				return true;
			}

			event->accept();

			Window *window(getWindow());
			QMenu menu(this);
			menu.addAction(window ? window->getContentsWidget()->createAction(ActionsManager::ClearTabHistoryAction) : Application::createAction(ActionsManager::ClearTabHistoryAction, this));
			menu.addAction(window ? window->getContentsWidget()->createAction(ActionsManager::PurgeTabHistoryAction) : Application::createAction(ActionsManager::PurgeTabHistoryAction, this));

			ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

			if (toolBar)
			{
				menu.addSeparator();
				menu.addActions(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), QVector<QAction*>(), &menu)->actions());
			}

			menu.exec(contextMenuEvent->globalPos());

			return true;
		}

		return false;
	}

	if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent(dynamic_cast<QHelpEvent*>(event));

		if (helpEvent)
		{
			const QVector<QKeySequence> shortcuts(ActionsManager::getActionDefinition(getIdentifier()).shortcuts);
			QString toolTip(text() + (shortcuts.isEmpty() ? QString() : QLatin1String(" (") + shortcuts.at(0).toString(QKeySequence::NativeText) + QLatin1Char(')')));

			if (getWindow())
			{
				const WindowHistoryInformation history(getWindow()->getContentsWidget()->getHistory());

				if (!history.entries.isEmpty())
				{
					int index(-1);

					if (getIdentifier() == ActionsManager::GoBackAction && history.index > 0)
					{
						index = (history.index - 1);
					}
					else if (getIdentifier() == ActionsManager::GoForwardAction && history.index < (history.entries.count() - 1))
					{
						index = (history.index + 1);
					}

					if (index >= 0)
					{
						QString title(history.entries.at(index).title);
						title = (title.isEmpty() ? tr("(Untitled)") : title.replace(QLatin1Char('&'), QLatin1String("&&")));

						toolTip = title + QLatin1String(" (") + text() + (shortcuts.isEmpty() ? QString() : QLatin1String(" - ") + shortcuts.at(0).toString(QKeySequence::NativeText)) + QLatin1Char(')');
					}
				}
			}

			QToolTip::showText(helpEvent->globalPos(), toolTip);
		}

		return true;
	}

	return ActionWidget::event(event);
}

bool NavigationActionWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::ContextMenu)
	{
		QContextMenuEvent *contextMenuEvent(dynamic_cast<QContextMenuEvent*>(event));

		if (contextMenuEvent)
		{
			QAction *action(menu()->activeAction());

			if (action && action->data().type() == QVariant::Int)
			{
				QMenu contextMenu(menu());
				QAction *removeEntryAction(contextMenu.addAction(tr("Remove Entry"), nullptr, nullptr, QKeySequence(Qt::Key_Delete)));
				QAction *purgeEntryAction(contextMenu.addAction(tr("Purge Entry"), nullptr, nullptr, QKeySequence(Qt::ShiftModifier | Qt::Key_Delete)));
				QAction *selectedAction(contextMenu.exec(contextMenuEvent->globalPos()));

				if (selectedAction == removeEntryAction)
				{
					menu()->close();

					getWindow()->getContentsWidget()->removeHistoryIndex(action->data().toInt());
				}
				else if (selectedAction == purgeEntryAction)
				{
					menu()->close();

					getWindow()->getContentsWidget()->removeHistoryIndex(action->data().toInt(), true);
				}
			}
		}
	}
	else if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent(dynamic_cast<QKeyEvent*>(event));

		if (keyEvent && keyEvent->key() == Qt::Key_Delete && getWindow())
		{
			QAction *action(menu()->activeAction());

			if (action && action->data().type() == QVariant::Int)
			{
				menu()->close();

				getWindow()->getContentsWidget()->removeHistoryIndex(action->data().toInt(), keyEvent->modifiers().testFlag(Qt::ShiftModifier));
			}
		}
	}

	return QObject::eventFilter(object, event);
}

}
