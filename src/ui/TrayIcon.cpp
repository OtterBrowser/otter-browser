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

#include "TrayIcon.h"
#include "MainWindow.h"
#include "../core/Application.h"
#include "../core/SessionsManager.h"

#include <QtWidgets/QMenu>

namespace Otter
{

TrayIcon::TrayIcon(Application *parent) : QObject(parent),
	m_icon(new QSystemTrayIcon(this))
{
	QMenu *menu = new QMenu();
	menu->addAction(tr("Show Window"))->setData(QLatin1String("toggleWindow"));
	menu->addSeparator();
	menu->addAction(tr("New Tab"))->setData(QLatin1String("newTab"));
	menu->addAction(tr("New Private Tab"))->setData(QLatin1String("newPrivateTab"));
	menu->addSeparator();
	menu->addAction(tr("Bookmarks"))->setData(QLatin1String("bookmarks"));
	menu->addAction(tr("Transfers"))->setData(QLatin1String("transfers"));
	menu->addAction(tr("History"))->setData(QLatin1String("history"));
	menu->addSeparator();
	menu->addAction(tr("Exit"))->setData(QLatin1String("exit"));

	m_icon->setIcon(parent->windowIcon());
	m_icon->setContextMenu(menu);
	m_icon->setToolTip(tr("Otter Browser"));
	m_icon->show();

	setParent(NULL);

	connect(this, SIGNAL(destroyed()), menu, SLOT(deleteLater()));
	connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
	connect(m_icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activated(QSystemTrayIcon::ActivationReason)));
}

void TrayIcon::activated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger)
	{
		MainWindow *window = SessionsManager::getActiveWindow();

		if (window)
		{
			if (window->isMinimized())
			{
				window->activateWindow();
				window->restoreWindowState();
			}
			else
			{
				window->storeWindowState();
				window->showMinimized();
			}
		}
	}
}

}
