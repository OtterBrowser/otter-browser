/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_PASSIVENOTIFICATIONWIDGET_H
#define OTTER_PASSIVENOTIFICATIONWIDGET_H

#include "../core/NotificationsManager.h"

#include <QtWidgets/QLabel>

namespace Otter
{

class PassiveNotificationWidget final : public QWidget
{
public:
	explicit PassiveNotificationWidget(QWidget *parent = nullptr);

	void setMessage(const QString &message, Notification::Message::Level level = Notification::Message::InformationLevel);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	QLabel *m_iconLabel;
	QLabel *m_messageLabel;
};

}

#endif
