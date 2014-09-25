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

#include "Menu.h"
#include "../core/ActionsManager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>

namespace Otter
{

Menu::Menu(const QJsonObject &definition, QWidget *parent) : QMenu(parent),
	m_title(definition.value(QLatin1String("title")).toString())
{
	setObjectName(definition.value(QLatin1String("identifier")).toString());
	setTitle(m_title);

	const QJsonArray actions = definition.value(QLatin1String("actions")).toArray();

	for (int i = 0; i < actions.count(); ++i)
	{
		if (actions.at(i).isObject())
		{
			addMenu(new Menu(actions.at(i).toObject(), this));
		}
		else
		{
			const QString action = actions.at(i).toString();

			if (action == QLatin1String("separator"))
			{
				addSeparator();
			}
			else
			{
				addAction(ActionsManager::getAction(action, parent));
			}
		}
	}
}

bool Menu::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		setTitle(QCoreApplication::translate("actions", m_title.toUtf8().constData()));
	}

	return QMenu::event(event);
}

}
