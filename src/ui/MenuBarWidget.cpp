/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "MenuBarWidget.h"
#include "MainWindow.h"
#include "Menu.h"
#include "ToolBarWidget.h"
#include "../core/ActionsManager.h"
#include "../core/SessionsManager.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOptionMenuItem>

namespace Otter
{

MenuBarWidget::MenuBarWidget(MainWindow *parent) : QMenuBar(parent)
{
	const QString path = (SessionsManager::getProfilePath() + QLatin1String("/menuBar.json"));
	QFile file(QFile::exists(path) ? path : QLatin1String(":/other/menuBar.json"));
	file.open(QFile::ReadOnly);

	const QJsonArray definition = QJsonDocument::fromJson(file.readAll()).array();

	for (int i = 0; i < definition.count(); ++i)
	{
		Menu *menu = new Menu(this);
		menu->load(definition.at(i).toObject());

		addMenu(menu);
	}

	if (!isNativeMenuBar())
	{
		setup();

		connect(parent->getActionsManager(), SIGNAL(toolBarModified(QString)), this, SLOT(toolBarModified(QString)));
	}
}

void MenuBarWidget::setup()
{
	const ToolBarDefinition definition = ActionsManager::getToolBarDefinition(QLatin1String("MenuBar"));
	QStringList actions;

	for (int i = 0; i < definition.actions.count(); ++i)
	{
		actions.append(definition.actions.at(i).action);
	}

	if (actions.count() == 1 && actions.at(0) == QLatin1String("MenuBarWidget"))
	{
		if (cornerWidget(Qt::TopLeftCorner))
		{
			cornerWidget(Qt::TopLeftCorner)->deleteLater();

			setCornerWidget(NULL, Qt::TopLeftCorner);
		}

		if (cornerWidget(Qt::TopRightCorner))
		{
			cornerWidget(Qt::TopRightCorner)->deleteLater();

			setCornerWidget(NULL, Qt::TopRightCorner);
		}

		return;
	}

	ToolBarWidget *leftToolBar = qobject_cast<ToolBarWidget*>(cornerWidget(Qt::TopLeftCorner));
	ToolBarWidget *rightToolBar = qobject_cast<ToolBarWidget*>(cornerWidget(Qt::TopRightCorner));
	const int position = actions.indexOf(QLatin1String("MenuBarWidget"));
	const bool needsLeftToolbar = (position != 0);
	const bool needsRightToolbar = (position != (definition.actions.count() - 1));

	if (needsLeftToolbar && !leftToolBar)
	{
		leftToolBar = new ToolBarWidget(QString(), NULL, this);

		setCornerWidget(leftToolBar, Qt::TopLeftCorner);
	}
	else if (!needsLeftToolbar && leftToolBar)
	{
		leftToolBar->deleteLater();

		setCornerWidget(NULL, Qt::TopLeftCorner);
	}

	if (needsRightToolbar && !rightToolBar)
	{
		rightToolBar = new ToolBarWidget(QString(), NULL, this);

		setCornerWidget(rightToolBar, Qt::TopRightCorner);
	}
	else if (!needsRightToolbar && rightToolBar)
	{
		rightToolBar->deleteLater();

		setCornerWidget(NULL, Qt::TopRightCorner);
	}

	for (int i = 0; i < definition.actions.count(); ++i)
	{
		if (i != position)
		{
			ToolBarWidget *toolBar = ((i < position) ? leftToolBar : rightToolBar);

			if (definition.actions.at(i).action == QLatin1String("separator"))
			{
				toolBar->addSeparator();
			}
			else
			{
				toolBar->addWidget(ToolBarWidget::createWidget(definition.actions.at(i), NULL, toolBar));
			}
		}
	}
}

void MenuBarWidget::toolBarModified(const QString &identifier)
{
	if (identifier == QLatin1String("MenuBar"))
	{
		setup();
	}
}

}

