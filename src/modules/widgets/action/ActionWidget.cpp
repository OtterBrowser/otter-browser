/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QToolTip>

namespace Otter
{

ActionWidget::ActionWidget(int identifier, Window *window, const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_window(window),
	m_identifier(identifier)
{
	setWindow(window);

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
	{
		connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
	}
}

void ActionWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if ((m_identifier == ActionsManager::NewTabAction || m_identifier == ActionsManager::NewTabPrivateAction) && event->button() != Qt::RightButton)
	{
		WindowsManager::OpenHints hints(WindowsManager::calculateOpenHints(WindowsManager::NewTabOpen, event->button(), event->modifiers()));

		if (m_identifier == ActionsManager::NewTabPrivateAction)
		{
			hints |= WindowsManager::PrivateOpen;
		}

		QVariantMap parameters;
		parameters[QLatin1String("hints")] = static_cast<int>(hints);

		ActionsManager::triggerAction(ActionsManager::OpenUrlAction, this, parameters);

		QAction *action(defaultAction());

		setDefaultAction(nullptr);

		ToolButtonWidget::mouseReleaseEvent(event);

		setDefaultAction(action);
		setText(getText());
		setIcon(getIcon());

		return;
	}

	ToolButtonWidget::mouseReleaseEvent(event);
}

void ActionWidget::resetAction()
{
	Action *action(nullptr);

	if (m_window && Action::isLocal(m_identifier) && m_window->getContentsWidget() && m_window->getContentsWidget()->getAction(m_identifier))
	{
		action = m_window->getContentsWidget()->getAction(m_identifier);
	}
	else
	{
		action = ActionsManager::getAction(m_identifier, this);
	}

	removeAction(defaultAction());
	setDefaultAction(action);
	setText(getText());
	setIcon(getIcon());
}

void ActionWidget::setWindow(Window *window)
{
	if (m_window)
	{
		disconnect(m_window, SIGNAL(widgetChanged()), this, SLOT(resetAction()));
	}

	m_window = window;

	if (window)
	{
		connect(window, SIGNAL(widgetChanged()), this, SLOT(resetAction()));
	}

	resetAction();
}

Window* ActionWidget::getWindow() const
{
	return m_window;
}

int ActionWidget::getIdentifier() const
{
	return m_identifier;
}

bool ActionWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		Action *action(qobject_cast<Action*>(defaultAction()));
		QHelpEvent *helpEvent(dynamic_cast<QHelpEvent*>(event));

		if (action && helpEvent)
		{
			const QVector<QKeySequence> shortcuts(ActionsManager::getActionDefinition(action ? action->getIdentifier() : m_identifier).shortcuts);

			QToolTip::showText(helpEvent->globalPos(), text() + (shortcuts.isEmpty() ? QString() : QLatin1String(" (") + shortcuts.at(0).toString(QKeySequence::NativeText) + QLatin1Char(')')));
		}

		return true;
	}

	return ToolButtonWidget::event(event);
}

}
