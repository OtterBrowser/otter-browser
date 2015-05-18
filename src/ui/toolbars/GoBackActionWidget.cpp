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

#include "GoBackActionWidget.h"
#include "../ContentsWidget.h"
#include "../Window.h"
#include "../../core/HistoryManager.h"

#include <QtGui/QHelpEvent>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>

namespace Otter
{

GoBackActionWidget::GoBackActionWidget(Window *window, QWidget *parent) : ActionWidget(ActionsManager::GoBackAction, window, parent)
{
	setMenu(new QMenu(this));
	setPopupMode(QToolButton::DelayedPopup);

	connect(menu(), SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
	connect(menu(), SIGNAL(triggered(QAction*)), this, SLOT(goToHistoryIndex(QAction*)));
}

void GoBackActionWidget::goToHistoryIndex(QAction *action)
{
	if (getWindow() && action && action->data().type() == QVariant::Int)
	{
		getWindow()->getContentsWidget()->goToHistoryIndex(action->data().toInt());
	}
}

void GoBackActionWidget::updateMenu()
{
	if (!menu() || !getWindow())
	{
		return;
	}

	menu()->clear();

	const WindowHistoryInformation history = getWindow()->getContentsWidget()->getHistory();

	for (int i = (history.index - 1); i >= 0; --i)
	{
		QString title = history.entries.at(i).title;
		QAction *action = menu()->addAction(HistoryManager::getIcon(QUrl(history.entries.at(i).url)), (title.isEmpty() ? tr("(Untitled)") : title.replace(QLatin1Char('&'), QLatin1String("&&"))));
		action->setData(i);
		action->setStatusTip(history.entries.at(i).url);
	}
}

bool GoBackActionWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = dynamic_cast<QHelpEvent*>(event);

		if (helpEvent)
		{
			const QVector<QKeySequence> shortcuts = ActionsManager::getActionDefinition(ActionsManager::GoBackAction).shortcuts;
			QString toolTip = text() + (shortcuts.isEmpty() ? QString() : QLatin1String(" (") + shortcuts.at(0).toString(QKeySequence::NativeText) + QLatin1Char(')'));

			if (getWindow())
			{
				const WindowHistoryInformation history = getWindow()->getContentsWidget()->getHistory();

				if (!history.entries.isEmpty() && history.index > 0)
				{
					QString title = history.entries.at(history.index - 1).title;
					title = (title.isEmpty() ? tr("(Untitled)") : title.replace(QLatin1Char('&'), QLatin1String("&&")));

					toolTip = title + QLatin1String(" (") + text() + (shortcuts.isEmpty() ? QString() : QLatin1String(" - ") + shortcuts.at(0).toString(QKeySequence::NativeText)) + QLatin1Char(')');
				}
			}

			QToolTip::showText(helpEvent->globalPos(), toolTip);
		}

		return true;
	}

	return ActionWidget::event(event);
}

}
