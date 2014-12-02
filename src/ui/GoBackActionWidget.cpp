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

#include "GoBackActionWidget.h"
#include "ContentsWidget.h"
#include "Window.h"
#include "../core/SettingsManager.h"
#include "../core/WebBackend.h"
#include "../core/WebBackendsManager.h"

#include <QtWidgets/QMenu>

namespace Otter
{

GoBackActionWidget::GoBackActionWidget(Window *window, QWidget *parent) : ActionWidget(GoBackAction, window, parent),
	m_window(window)
{
	setMenu(new QMenu(this));

	if (SettingsManager::getValue(QLatin1String("Interface/ToolButtonArrow")).toString() == QLatin1String("inside"))
	{
		setPopupMode(QToolButton::DelayedPopup);
	}
	else if (SettingsManager::getValue(QLatin1String("Interface/ToolButtonArrow")).toString() == QLatin1String("outside"))
	{
		setPopupMode(QToolButton::MenuButtonPopup);
	}

	optionChanged(QLatin1String("Interface/ToolButtonArrow"), SettingsManager::getValue(QLatin1String("Interface/ToolButtonArrow")));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));

	connect(menu(), SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
	connect(menu(), SIGNAL(triggered(QAction*)), this, SLOT(goToHistoryIndex(QAction*)));
}

void GoBackActionWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Interface/ToolButtonArrow"))
	{
		if (value.toString() == QLatin1String("inside"))
		{
			setPopupMode(QToolButton::DelayedPopup);
		}
		else if (value.toString() == QLatin1String("outside"))
		{
			setPopupMode(QToolButton::MenuButtonPopup);
		}
	}
}

void GoBackActionWidget::goToHistoryIndex(QAction *action)
{
	if (m_window && action && action->data().type() == QVariant::Int)
	{
		m_window->getContentsWidget()->goToHistoryIndex(action->data().toInt());
	}
}

void GoBackActionWidget::updateMenu()
{
	if (!menu() || !m_window)
	{
		return;
	}

	menu()->clear();

	WebBackend *backend = WebBackendsManager::getBackend();
	const WindowHistoryInformation history = m_window->getContentsWidget()->getHistory();

	for (int i = (history.index - 1); i >= 0; --i)
	{
		QString title = history.entries.at(i).title;

		menu()->addAction(backend->getIconForUrl(QUrl(history.entries.at(i).url)), (title.isEmpty() ? tr("(Untitled)") : title.replace(QLatin1Char('&'), QLatin1String("&&"))))->setData(i);
	}
}

}
