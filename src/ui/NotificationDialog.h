/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_NOTIFICATIONDIALOG_H
#define OTTER_NOTIFICATIONDIALOG_H

#include <QtCore/QPropertyAnimation>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>

namespace Otter
{

class Notification;

class NotificationDialog final : public QFrame
{
	Q_OBJECT

public:
	explicit NotificationDialog(Notification *notification, QWidget *parent = nullptr);

protected:
	void changeEvent(QEvent *event) override;
	void timerEvent(QTimerEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

protected slots:
	void updateMessage();

private:
	Notification *m_notification;
	QLabel *m_closeLabel;
	QLabel *m_iconLabel;
	QLabel *m_messageLabel;
	QPropertyAnimation *m_animation;
	int m_closeTimer;
};

}

#endif
