/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_TRAYICON_H
#define OTTER_TRAYICON_H

#include "../core/NotificationsManager.h"

#include <QtWidgets/QSystemTrayIcon>

namespace Otter
{

class Application;

class TrayIcon final : public QObject
{
	Q_OBJECT

public:
	explicit TrayIcon(Application *parent);

	void showNotification(Notification *notification);
	bool event(QEvent *event) override;

public slots:
	void hide();

protected:
	void timerEvent(QTimerEvent *event) override;
	void showMessage(const Notification::Message &message);

protected slots:
	void toggleWindowsVisibility();
	void handleMessageClicked();

private:
	QSystemTrayIcon *m_trayIcon;
	Notification *m_notification;
	int m_autoHideTimer;
};

}

#endif
