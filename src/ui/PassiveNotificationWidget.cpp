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

#include "PassiveNotificationWidget.h"
#include "../core/ThemesManager.h"

#include <QtGui/QPainter>
#include <QtWidgets/QBoxLayout>

namespace Otter
{

PassiveNotificationWidget::PassiveNotificationWidget(QWidget *parent) : QWidget(parent),
	m_iconLabel(new QLabel(this)),
	m_messageLabel(new QLabel(this))
{
	m_messageLabel->setWordWrap(true);

	QBoxLayout *layout(new QBoxLayout(QBoxLayout::LeftToRight));
	layout->setSizeConstraint(QLayout::SetMinimumSize);
	layout->addWidget(m_iconLabel, 0);
	layout->addWidget(m_messageLabel, 1);

	setLayout(layout);
}

void PassiveNotificationWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
///FIXME Use palette
	painter.setBrush(QColor(189, 229, 248));
	painter.setPen(QColor(130, 157, 170));
	painter.fillRect(contentsRect(), Qt::transparent);
	painter.drawRoundedRect(contentsRect().adjusted(2, 2, -2, -2), 5, 5);
}

void PassiveNotificationWidget::setMessage(const QString &message, Notification::Message::Level level)
{
	QString iconName;

	switch (level)
	{
		case Notification::Message::ErrorLevel:
			iconName = QLatin1String("dialog-error");

			break;
		case Notification::Message::WarningLevel:
			iconName = QLatin1String("dialog-warning");

			break;
		default:
			iconName = QLatin1String("dialog-information");

			break;
	}

	m_iconLabel->setPixmap(ThemesManager::createIcon(iconName).pixmap(32, 32));

	m_messageLabel->setText(message);
}

}
