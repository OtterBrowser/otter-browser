/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

NavigationActionWidget::NavigationActionWidget(Window *window, const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : ActionWidget(((definition.action == QLatin1String("GoBackAction")) ? ActionsManager::GoBackAction : ActionsManager::GoForwardAction), window, definition, parent),
	m_window(window)
{
	setMenu(new QMenu(this));
	setPopupMode(QToolButton::DelayedPopup);
	setContextMenuPolicy(Qt::DefaultContextMenu);

	menu()->installEventFilter(this);

	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getIdentifier() != ToolBarsManager::AddressBar)
	{
		connect(toolBar, &ToolBarWidget::windowChanged, this, &NavigationActionWidget::setWindow);
	}

	connect(menu(), &QMenu::aboutToShow, this, &NavigationActionWidget::updateMenu);
	connect(menu(), &QMenu::triggered, this, &NavigationActionWidget::goToHistoryIndex);
}

void NavigationActionWidget::goToHistoryIndex(QAction *action)
{
	if (m_window && action && action->data().type() == QVariant::Int)
	{
		m_window->getContentsWidget()->goToHistoryIndex(action->data().toInt());
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
	if (!menu() || !m_window)
	{
		return;
	}

	menu()->clear();

	const WindowHistoryInformation history(m_window->getContentsWidget()->getHistory());

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

void NavigationActionWidget::setWindow(Window *window)
{
	m_window = window;
}

bool NavigationActionWidget::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::ContextMenu:
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

					ActionExecutor::Object executor(m_window, m_window);
					QMenu menu(this);
					menu.addAction(new Action(ActionsManager::ClearTabHistoryAction, {}, executor, &menu));
					menu.addAction(new Action(ActionsManager::ClearTabHistoryAction, {{QLatin1String("clearGlobalHistory"), true}}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Purge Tab History")}}, executor, &menu));

					const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

					if (toolBar)
					{
						menu.addSeparator();
						menu.addActions(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), {}, &menu)->actions());
					}

					menu.exec(contextMenuEvent->globalPos());

					return true;
				}
			}

			return false;
		case QEvent::MouseButtonDblClick:
		case QEvent::MouseButtonPress:
		case QEvent::Wheel:
			GesturesManager::startGesture(this, event, {GesturesManager::ToolBarContext, GesturesManager::GenericContext});

			break;
		case QEvent::ToolTip:
			{
				const QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));

				if (helpEvent)
				{
					const QKeySequence shortcut(ActionsManager::getActionShortcut(getIdentifier()));
					QString toolTip(text() + (shortcut.isEmpty() ? QString() : QLatin1String(" (") + shortcut.toString(QKeySequence::NativeText) + QLatin1Char(')')));

					if (m_window)
					{
						const WindowHistoryInformation history(m_window->getContentsWidget()->getHistory());

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

								toolTip = title + QLatin1String(" (") + text() + (shortcut.isEmpty() ? QString() : QLatin1String(" - ") + shortcut.toString(QKeySequence::NativeText)) + QLatin1Char(')');
							}
						}
					}

					QToolTip::showText(helpEvent->globalPos(), toolTip);
				}
			}

			return true;
		default:
			break;
	}

	return ActionWidget::event(event);
}

bool NavigationActionWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::ContextMenu)
	{
		const QContextMenuEvent *contextMenuEvent(static_cast<QContextMenuEvent*>(event));

		if (contextMenuEvent)
		{
			const QAction *action(menu()->activeAction());

			if (action && action->data().type() == QVariant::Int)
			{
				QMenu contextMenu(menu());
				const QAction *removeEntryAction(contextMenu.addAction(tr("Remove Entry"), nullptr, nullptr, QKeySequence(Qt::Key_Delete)));
				const QAction *purgeEntryAction(contextMenu.addAction(tr("Purge Entry"), nullptr, nullptr, QKeySequence(Qt::ShiftModifier | Qt::Key_Delete)));
				const QAction *selectedAction(contextMenu.exec(contextMenuEvent->globalPos()));

				if (selectedAction == removeEntryAction)
				{
					menu()->close();

					m_window->getContentsWidget()->removeHistoryIndex(action->data().toInt());
				}
				else if (selectedAction == purgeEntryAction)
				{
					menu()->close();

					m_window->getContentsWidget()->removeHistoryIndex(action->data().toInt(), true);
				}
			}
		}
	}
	else if (event->type() == QEvent::KeyPress)
	{
		const QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent && keyEvent->key() == Qt::Key_Delete && m_window)
		{
			const QAction *action(menu()->activeAction());

			if (action && action->data().type() == QVariant::Int)
			{
				menu()->close();

				m_window->getContentsWidget()->removeHistoryIndex(action->data().toInt(), keyEvent->modifiers().testFlag(Qt::ShiftModifier));
			}
		}
	}

	return QObject::eventFilter(object, event);
}

}
