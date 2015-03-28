/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ActionWidget.h"
#include "../ContentsWidget.h"
#include "../MainWindow.h"
#include "../ToolBarWidget.h"
#include "../Window.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QToolTip>

namespace Otter
{

ActionWidget::ActionWidget(int identifier, Window *window, QWidget *parent) : ToolButtonWidget(parent),
	m_window(window),
	m_identifier(identifier)
{
	setAutoRaise(true);
	setWindow(window);

	ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parent);

	if (toolBar)
	{
		connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
	}
}

void ActionWidget::actionEvent(QActionEvent *event)
{
	QToolButton::actionEvent(event);

	if (event->type() == QEvent::ActionChanged || event->type() == QEvent::ActionAdded)
	{
		Action *action = qobject_cast<Action*>(defaultAction());

		if (action)
		{
			setText(action->getText());
		}
	}
}

void ActionWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_identifier == Action::NewTabAction || m_identifier == Action::NewTabPrivateAction)
	{
		MainWindow *window = MainWindow::findMainWindow(this);

		if (window)
		{
			OpenHints hints = WindowsManager::calculateOpenHints(event->modifiers(), event->button(), NewTabOpen);

			if (m_identifier == Action::NewTabPrivateAction)
			{
				hints |= PrivateOpen;
			}

			window->getWindowsManager()->open(QUrl(), hints);
		}

		QAction *action = defaultAction();

		setDefaultAction(NULL);

		ToolButtonWidget::mouseReleaseEvent(event);

		setDefaultAction(action);

		return;
	}

	ToolButtonWidget::mouseReleaseEvent(event);
}

void ActionWidget::setWindow(Window *window)
{
	Action *action = NULL;

	m_window = window;

	if (window && Action::isLocal(m_identifier) && window->getContentsWidget()->getAction(m_identifier))
	{
		action = window->getContentsWidget()->getAction(m_identifier);
	}
	else
	{
		action = ActionsManager::getAction(m_identifier, this);
	}

	setDefaultAction(action);
	setText(action->getText());
}

Window* ActionWidget::getWindow() const
{
	return m_window;
}

bool ActionWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		Action *action = qobject_cast<Action*>(defaultAction());
		QHelpEvent *helpEvent = dynamic_cast<QHelpEvent*>(event);

		if (helpEvent)
		{
			const QVector<QKeySequence> shortcuts = ActionsManager::getActionDefinition(action ? action->getIdentifier() : m_identifier).shortcuts;

			QToolTip::showText(helpEvent->globalPos(), text() + (shortcuts.isEmpty() ? QString() : QLatin1String(" (") + shortcuts.at(0).toString(QKeySequence::NativeText) + QLatin1Char(')')));
		}

		return true;
	}

	return QToolButton::event(event);
}

}
