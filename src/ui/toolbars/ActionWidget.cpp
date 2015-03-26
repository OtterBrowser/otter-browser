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
#include "../Window.h"

#include <QtGui/QMouseEvent>

namespace Otter
{

ActionWidget::ActionWidget(int identifier, Window *window, QWidget *parent) : ToolButtonWidget(parent),
	m_identifier(identifier)
{
	setAutoRaise(true);

	if (window && Action::isLocal(identifier) && window->getContentsWidget()->getAction(identifier))
	{
		setDefaultAction(window->getContentsWidget()->getAction(identifier));
	}
	else
	{
		setDefaultAction(ActionsManager::getAction(identifier, this));
	}
}

void ActionWidget::enterEvent(QEvent *event)
{
	QToolButton::enterEvent(event);

	Action *action = qobject_cast<Action*>(defaultAction());

	if (action)
	{
		QString text = action->text();

		if (text.contains(QLatin1Char('\t')))
		{
			setToolTip(text.replace(QLatin1Char('\t'), QStringLiteral(" (")) + QLatin1Char(')'));
		}
		else
		{
			setToolTip(text);
		}

		return;
	}

	setToolTip(QString());
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
	}

	ToolButtonWidget::mouseReleaseEvent(event);
}

}
