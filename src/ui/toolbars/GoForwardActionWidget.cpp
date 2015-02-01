/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "GoForwardActionWidget.h"
#include "../ContentsWidget.h"
#include "../Window.h"
#include "../../core/AddonsManager.h"
#include "../../core/WebBackend.h"

#include <QtWidgets/QMenu>

namespace Otter
{

GoForwardActionWidget::GoForwardActionWidget(Window *window, QWidget *parent) : ActionWidget(Action::GoForwardAction, window, parent),
	m_window(window)
{
	setMenu(new QMenu(this));
	setPopupMode(QToolButton::DelayedPopup);

	connect(menu(), SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
	connect(menu(), SIGNAL(triggered(QAction*)), this, SLOT(goToHistoryIndex(QAction*)));
}

void GoForwardActionWidget::enterEvent(QEvent *event)
{
	ActionWidget::enterEvent(event);

	Action *action = qobject_cast<Action*>(defaultAction());

	if (m_window && action)
	{
		QString text = action->text();
		const WindowHistoryInformation history = m_window->getContentsWidget()->getHistory();
		const bool hasShortcut = text.contains(QLatin1Char('\t'));

		if (history.entries.isEmpty() || history.index == (history.entries.count() - 1))
		{
			setToolTip(hasShortcut ? text.replace(QLatin1Char('\t'), QStringLiteral(" (")) + QLatin1Char(')') : action->text());
		}
		else
		{
			QString title = history.entries.at(history.index + 1).title;
			title = (title.isEmpty() ? tr("(Untitled)") : title.replace(QLatin1Char('&'), QLatin1String("&&")));

			setToolTip(hasShortcut ? tr("%1 (Forward - %2)").arg(title).arg(action->getShortcuts().first().toString(QKeySequence::NativeText)) : tr("%1 (Forward)").arg(title));
		}

		return;
	}

	setToolTip(tr("Forward"));
}

void GoForwardActionWidget::goToHistoryIndex(QAction *action)
{
	if (m_window && action && action->data().type() == QVariant::Int)
	{
		m_window->getContentsWidget()->goToHistoryIndex(action->data().toInt());
	}
}

void GoForwardActionWidget::updateMenu()
{
	if (!menu() || !m_window)
	{
		return;
	}

	menu()->clear();

	WebBackend *backend = AddonsManager::getWebBackend();
	const WindowHistoryInformation history = m_window->getContentsWidget()->getHistory();

	for (int i = (history.index + 1); i < history.entries.count(); ++i)
	{
		QString title = history.entries.at(i).title;

		menu()->addAction(backend->getIconForUrl(QUrl(history.entries.at(i).url)), (title.isEmpty() ? tr("(Untitled)") : title.replace(QLatin1Char('&'), QLatin1String("&&"))))->setData(i);
	}
}

}
