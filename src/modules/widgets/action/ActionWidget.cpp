/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ActionWidget.h"
#include "../../../core/Application.h"
#include "../../../ui/Action.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QToolTip>

namespace Otter
{

ActionWidget::ActionWidget(int identifier, Window *window, const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_action(new Action(identifier, definition.parameters, ActionExecutor::Object(window, window), this))
{
	installGesturesFilter(this, this, {GesturesManager::ToolBarContext, GesturesManager::GenericContext});
	setDefaultAction(m_action);
	setWindow(window);

	const QVariantMap options(definition.options);

	if (options.contains(QLatin1String("icon")))
	{
		const QVariant data(options[QLatin1String("icon")]);

		if (data.type() == QVariant::Icon)
		{
			m_action->setIconOverride(data.value<QIcon>());
		}
		else
		{
			m_action->setIconOverride(data.toString());
		}
	}

	if (options.contains(QLatin1String("text")))
	{
		m_action->setTextOverride(options[QLatin1String("text")].toString());
	}

	switch (identifier)
	{
		case ActionsManager::NewTabAction:
		case ActionsManager::NewTabPrivateAction:
		case ActionsManager::NewWindowAction:
		case ActionsManager::NewWindowPrivateAction:
			setAcceptDrops(true);

			break;
		default:
			break;
	}

	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getDefinition().isGlobal())
	{
		connect(toolBar, &ToolBarWidget::windowChanged, this, &ActionWidget::setWindow);
	}
}

void ActionWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
	{
		ToolButtonWidget::mouseReleaseEvent(event);

		return;
	}

	int identifier(m_action->getIdentifier());
	QVariantMap parameters(m_action->getParameters());

	switch (identifier)
	{
		case ActionsManager::NewTabAction:
		case ActionsManager::NewTabPrivateAction:
		case ActionsManager::NewWindowAction:
		case ActionsManager::NewWindowPrivateAction:
			{
				const bool isNewWindow(identifier == ActionsManager::NewWindowAction || identifier == ActionsManager::NewWindowPrivateAction);
				SessionsManager::OpenHints hints(SessionsManager::calculateOpenHints((isNewWindow ? SessionsManager::NewWindowOpen : SessionsManager::NewTabOpen), event->button(), event->modifiers()));

				if (identifier == ActionsManager::NewTabPrivateAction || identifier == ActionsManager::NewWindowPrivateAction)
				{
					hints |= SessionsManager::PrivateOpen;
				}

				parameters[QLatin1String("hints")] = QVariant(hints);

				identifier = ActionsManager::OpenUrlAction;
			}

			break;
		default:
			break;
	}

	if (isCheckable())
	{
		parameters[QLatin1String("isChecked")] = !isChecked();
	}

	Application::triggerAction(identifier, parameters, this);

	setDefaultAction(nullptr);

	ToolButtonWidget::mouseReleaseEvent(event);

	setDefaultAction(m_action);
}

void ActionWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls())
	{
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void ActionWidget::dropEvent(QDropEvent *event)
{
	if (!event->mimeData()->hasUrls())
	{
		event->ignore();
	}

	QVariantMap parameters(getParameters());
	const QVector<QUrl> urls(Utils::extractUrls(event->mimeData()));
	const int identifier(getIdentifier());
	const bool isNewTab(identifier == ActionsManager::NewTabPrivateAction || identifier == ActionsManager::NewWindowPrivateAction);
	SessionsManager::OpenHints hints(SessionsManager::calculateOpenHints((isNewTab ? SessionsManager::NewWindowOpen : SessionsManager::NewTabOpen), Qt::LeftButton, event->keyboardModifiers()));

	if (isNewTab)
	{
		hints |= SessionsManager::PrivateOpen;
	}

	parameters[QLatin1String("hints")] = QVariant(hints);

	for (int i = 0; i < urls.count(); ++i)
	{
		QVariantMap actionParameters(parameters);
		actionParameters[QLatin1String("url")] = urls.at(i);

		Application::triggerAction(ActionsManager::OpenUrlAction, actionParameters, this);
	}

	event->accept();
}

void ActionWidget::setWindow(Window *window)
{
	if (m_action->getDefinition().scope == ActionsManager::ActionDefinition::WindowScope)
	{
		m_action->setExecutor(ActionExecutor::Object(window, window));
	}
	else
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(this));

		m_action->setExecutor(ActionExecutor::Object(mainWindow, mainWindow));
	}
}

int ActionWidget::getIdentifier() const
{
	return m_action->getIdentifier();
}

bool ActionWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), Utils::appendShortcut(text(), m_action->shortcut()));

		return true;
	}

	return ToolButtonWidget::event(event);
}

NavigationActionWidget::NavigationActionWidget(Window *window, const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : ActionWidget(((definition.action == QLatin1String("GoBackAction")) ? ActionsManager::GoBackAction : ActionsManager::GoForwardAction), window, definition, parent),
	m_window(window)
{
	setMenu(new QMenu(this));
	setPopupMode(QToolButton::DelayedPopup);
	setContextMenuPolicy(Qt::DefaultContextMenu);

	menu()->installEventFilter(this);

	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getDefinition().isGlobal())
	{
		connect(toolBar, &ToolBarWidget::windowChanged, this, [&](Window *window)
		{
			m_window = window;
		});
	}

	connect(menu(), &QMenu::aboutToShow, this, &NavigationActionWidget::updateMenu);
}

void NavigationActionWidget::addMenuEntry(int index, const Session::Window::History::Entry &entry)
{
	Action *action(new Action(ActionsManager::GoToHistoryIndexAction, {{QLatin1String("index"), index}}, ActionExecutor::Object(m_window, m_window), this));
	action->setStatusTip(entry.url);

	menu()->addAction(action);
}

void NavigationActionWidget::updateMenu()
{
	if (!menu() || !m_window)
	{
		return;
	}

	menu()->clear();

	const Session::Window::History history(m_window->getContentsWidget()->getHistory());

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
	switch (event->type())
	{
		case QEvent::ContextMenu:
			{
				QContextMenuEvent *contextMenuEvent(static_cast<QContextMenuEvent*>(event));

				if (contextMenuEvent->reason() == QContextMenuEvent::Mouse)
				{
					contextMenuEvent->accept();

					return true;
				}

				event->accept();

				ActionExecutor::Object executor(m_window, m_window);
				QMenu menu(this);
				menu.addAction(new Action(ActionsManager::ClearTabHistoryAction, {}, executor, &menu));
				menu.addAction(new Action(ActionsManager::ClearTabHistoryAction, {{QLatin1String("clearGlobalHistory"), true}}, executor, &menu));

				const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

				if (toolBar)
				{
					menu.addSeparator();
					menu.addActions(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), {}, &menu)->actions());
				}

				menu.exec(contextMenuEvent->globalPos());

				return true;
			}
		case QEvent::ToolTip:
			{
				const int identifier(getIdentifier());
				const QKeySequence shortcut(ActionsManager::getActionShortcut(identifier));
				QString toolTip(Utils::appendShortcut(text(), shortcut));

				if (m_window)
				{
					const Session::Window::History history(m_window->getContentsWidget()->getHistory());

					if (!history.entries.isEmpty())
					{
						int index(-1);

						if (identifier == ActionsManager::GoBackAction && history.index > 0)
						{
							index = (history.index - 1);
						}
						else if (identifier == ActionsManager::GoForwardAction && history.index < (history.entries.count() - 1))
						{
							index = (history.index + 1);
						}

						if (index >= 0)
						{
							toolTip = history.entries.at(index).getTitle().replace(QLatin1Char('&'), QLatin1String("&&")) + QLatin1String(" (") + text() + (shortcut.isEmpty() ? QString() : QLatin1String(" - ") + shortcut.toString(QKeySequence::NativeText)) + QLatin1Char(')');
						}
					}
				}

				QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), toolTip);
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
		const Action *action(qobject_cast<Action*>(menu()->activeAction()));

		if (action && action->getIdentifier() == ActionsManager::GoToHistoryIndexAction)
		{
			const ActionExecutor::Object executor(m_window, m_window);
			const int index(action->getParameters().value(QLatin1String("index")).toInt());
			QMenu contextMenu(menu());
			Action *removeEntryAction(new Action(ActionsManager::RemoveHistoryIndexAction, {{QLatin1String("index"), index}}, executor, &contextMenu));
			removeEntryAction->setTextOverride(QT_TRANSLATE_NOOP("actions", "Remove Entry"));
			removeEntryAction->setShortcut(QKeySequence(Qt::Key_Delete));

			Action *purgeEntryAction(new Action(ActionsManager::RemoveHistoryIndexAction, {{QLatin1String("index"), index}}, executor, &contextMenu));
			purgeEntryAction->setTextOverride(QT_TRANSLATE_NOOP("actions", "Purge Entry"));
			purgeEntryAction->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_Delete));

			contextMenu.addAction(removeEntryAction);
			contextMenu.addAction(purgeEntryAction);

			const QAction *selectedAction(contextMenu.exec(static_cast<QContextMenuEvent*>(event)->globalPos()));

			if (selectedAction == removeEntryAction || selectedAction == purgeEntryAction)
			{
				menu()->close();
			}
		}
	}
	else if (event->type() == QEvent::KeyPress)
	{
		const QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent->key() == Qt::Key_Delete && m_window)
		{
			const Action *action(qobject_cast<Action*>(menu()->activeAction()));

			if (action && action->getIdentifier() == ActionsManager::GoToHistoryIndexAction)
			{
				menu()->close();

				m_window->triggerAction(ActionsManager::RemoveHistoryIndexAction, {{QLatin1String("index"), action->getParameters().value(QLatin1String("index"), -1).toInt()}, {QLatin1String("clearGlobalHistory"), keyEvent->modifiers().testFlag(Qt::ShiftModifier)}});
			}
		}
	}

	return QObject::eventFilter(object, event);
}

}
