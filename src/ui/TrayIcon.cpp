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

namespace Otter
{

TrayIcon::TrayIcon(Application *parent) : QObject(parent),
	m_icon(new QSystemTrayIcon(this))
{
	m_icon->setIcon(parent->windowIcon());
	m_icon->setToolTip(tr("Otter Browser"));
	m_icon->show();

	setParent(NULL);

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
