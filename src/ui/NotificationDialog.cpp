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

#include "NotificationDialog.h"
#include "../core/NotificationsManager.h"
#include "../core/SettingsManager.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QStyleOption>

namespace Otter
{

NotificationDialog::NotificationDialog(Notification *notification, QWidget *parent) : QFrame(parent),
	m_notification(notification),
	m_closeLabel(new QLabel(this)),
	m_iconLabel(new QLabel(this)),
	m_messageLabel(new QLabel(this)),
	m_closeTimer(0)
{
	m_iconLabel->setStyleSheet(QLatin1String("padding:5px;"));
	m_iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

	m_messageLabel->setStyleSheet(QLatin1String("padding:5px;font-size:13px;"));
	m_messageLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_messageLabel->setWordWrap(true);

	QStyleOption option;
	option.rect = {0, 0, 16, 16};
	option.state = (QStyle::State_Enabled | QStyle::State_AutoRaise);

	QPixmap pixmap(16, 16);
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);

	style()->drawPrimitive(QStyle::PE_IndicatorTabClose, &option, &painter, this);

	m_closeLabel->setToolTip(tr("Close"));
	m_closeLabel->setPixmap(pixmap);
	m_closeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_closeLabel->setAlignment(Qt::AlignTop);
	m_closeLabel->setContentsMargins(5, 5, 5, 5);

	QBoxLayout *layout(new QBoxLayout(QBoxLayout::LeftToRight));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->setSizeConstraint(QLayout::SetMinimumSize);
	layout->addWidget(m_iconLabel);
	layout->addWidget(m_messageLabel);
	layout->addWidget(m_closeLabel);

	setLayout(layout);
	setObjectName(QLatin1String("notificationFrame"));
	setStyleSheet(QLatin1String("#notificationFrame {padding:5px;border:1px solid #CCC;border-radius:10px;background:#F0F0f0;}"));
	setCursor(QCursor(Qt::PointingHandCursor));
	setFixedWidth(400);
	setMinimumHeight(50);
	setMaximumHeight(150);
	setWindowOpacity(0);
	setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool | Qt::FramelessWindowHint);
	setFocusPolicy(Qt::NoFocus);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setAttribute(Qt::WA_ShowWithoutActivating, true);
	updateMessage();
	adjustSize();

	m_animation = new QPropertyAnimation(this, QByteArrayLiteral("windowOpacity"));
	m_animation->setDuration(500);
	m_animation->setStartValue(0.0);
	m_animation->setEndValue(1.0);
	m_animation->start();

	const int visibilityDuration(SettingsManager::getOption(SettingsManager::Interface_NotificationVisibilityDurationOption).toInt());

	if (visibilityDuration > 0)
	{
		m_closeTimer = startTimer(visibilityDuration * 1000);
	}

	connect(notification, &Notification::modified, this, &NotificationDialog::updateMessage);
	connect(notification, &Notification::requestedClose, this, &NotificationDialog::close);
}

void NotificationDialog::changeEvent(QEvent *event)
{
	QFrame::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_closeLabel->setToolTip(tr("Close"));

		updateMessage();
	}
}

void NotificationDialog::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_closeTimer)
	{
		killTimer(m_closeTimer);

		m_closeTimer = 0;

		m_animation->setStartValue(1.0);
		m_animation->setEndValue(0.0);
		m_animation->start();

		connect(m_animation, &QPropertyAnimation::finished, this, [&]()
		{
			m_notification->markAsIgnored();

			close();
		});
	}
}

void NotificationDialog::showEvent(QShowEvent *event)
{
	QFrame::showEvent(event);

	m_notification->markAsShown();
}

void NotificationDialog::resizeEvent(QResizeEvent *event)
{
	QFrame::resizeEvent(event);

	const QScreen *screen(window()->windowHandle()->screen());

	if (screen)
	{
		QRect geometry(screen->availableGeometry());
		geometry.setRight(geometry.right() - 20);
		geometry.setBottom(geometry.bottom() - 20);

		setGeometry(QStyle::alignedRect(Qt::LeftToRight, (Qt::AlignBottom | Qt::AlignRight), size(), geometry));
	}

	const int radius(8);
	const int cornerSize(radius * 2);
	const QRect rectangle(rect());
	QRect corner(rectangle.topLeft(), QSize(cornerSize, cornerSize));
	QRegion region;
	region += rectangle.adjusted(radius, 0, -radius, 0);
	region += rectangle.adjusted(0, radius, 0, -radius);
	region += QRegion(corner, QRegion::Ellipse);

	corner.moveRight(rectangle.right() - 1);

	region += QRegion(corner, QRegion::Ellipse);

	corner.moveBottom(rectangle.bottom() - 1);
	corner.moveLeft(rectangle.left());

	region += QRegion(corner, QRegion::Ellipse);

	corner.moveRight(rectangle.right() - 1);

	region += QRegion(corner, QRegion::Ellipse);

	setMask(region);
}

void NotificationDialog::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
	{
		return;
	}

	m_animation->stop();

	if (m_closeLabel->geometry().contains(event->pos()))
	{
		m_notification->markAsIgnored();

		close();

		return;
	}

	m_notification->markAsClicked();

	close();
}

void NotificationDialog::updateMessage()
{
	const Notification::Message message(m_notification->getMessage());

	m_iconLabel->setPixmap(message.getIcon().pixmap(32, 32));

	m_messageLabel->setText(QLatin1String("<strong>") + message.getTitle() + QLatin1String("</strong><br>") + message.message);
}

}
