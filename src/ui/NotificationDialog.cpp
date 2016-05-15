/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "NotificationDialog.h"
#include "../core/NotificationsManager.h"
#include "../core/SettingsManager.h"
#include "../core/ThemesManager.h"

#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QStyleOptionFrame>

namespace Otter
{

NotificationDialog::NotificationDialog(Notification *notification, QWidget *parent) : QDialog(parent),
	m_notification(notification),
	m_closeLabel(NULL)
{
	QFrame *notificationFrame(new QFrame(this));
	notificationFrame->setObjectName(QLatin1String("notificationFrame"));
	notificationFrame->setStyleSheet(QLatin1String("#notificationFrame {padding:5px;border:1px solid #CCC;border-radius:10px;background:#F0F0f0;}"));
	notificationFrame->setCursor(QCursor(Qt::PointingHandCursor));
	notificationFrame->installEventFilter(this);

	QBoxLayout *mainLayout(new QBoxLayout(QBoxLayout::LeftToRight));
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
	mainLayout->addWidget(notificationFrame);

	QLabel *iconLabel(new QLabel(this));
	iconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("otter-browser-32")).pixmap(32, 32));
	iconLabel->setStyleSheet(QLatin1String("padding:5px;"));

	QLabel *messageLabel(new QLabel(this));
	messageLabel->setText(m_notification->getMessage());
	messageLabel->setStyleSheet(QLatin1String("padding:5px;font-size:13px;"));
	messageLabel->setWordWrap(true);

	QStyleOption option;
	option.rect = QRect(0, 0, 16, 16);

	QPixmap pixmap(16, 16);
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);

	style()->drawPrimitive(QStyle::PE_IndicatorTabClose, &option, &painter, this);

	m_closeLabel = new QLabel(notificationFrame);
	m_closeLabel->setToolTip(tr("Close"));
	m_closeLabel->setPixmap(pixmap);
	m_closeLabel->setAlignment(Qt::AlignTop);
	m_closeLabel->setMargin(5);
	m_closeLabel->installEventFilter(this);

	QBoxLayout *notificationLayout(new QBoxLayout(QBoxLayout::LeftToRight));
	notificationLayout->setContentsMargins(0, 0, 0, 0);
	notificationLayout->setSpacing(0);
	notificationLayout->setSizeConstraint(QLayout::SetMinimumSize);
	notificationLayout->addWidget(iconLabel);
	notificationLayout->addWidget(messageLabel);
	notificationLayout->addWidget(m_closeLabel);

	notificationFrame->setLayout(notificationLayout);

	setLayout(mainLayout);
	setFixedWidth(400);
	setMinimumHeight(50);
	setMaximumHeight(150);
	setWindowOpacity(0);
	setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool | Qt::FramelessWindowHint);
	setFocusPolicy(Qt::NoFocus);
	setAttribute(Qt::WA_ShowWithoutActivating);
	setAttribute(Qt::WA_TranslucentBackground);
	adjustSize();

	m_animation = new QPropertyAnimation(this, QStringLiteral("windowOpacity").toLatin1());
	m_animation->setDuration(500);
	m_animation->setStartValue(0.0);
	m_animation->setEndValue(1.0);
	m_animation->start();

	const int visibilityDuration(SettingsManager::getValue(QLatin1String("Interface/NotificationVisibilityDuration")).toInt());

	if (visibilityDuration > 0)
	{
		QTimer::singleShot((visibilityDuration * 1000), this, SLOT(aboutToClose()));
	}
}

void NotificationDialog::closeEvent(QCloseEvent *event)
{
	deleteLater();

	event->accept();
}

void NotificationDialog::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	QRect geometry(QApplication::desktop()->availableGeometry());
	geometry.setRight(geometry.right() - 20);
	geometry.setBottom(geometry.bottom() - 20);

	setGeometry(QStyle::alignedRect(Qt::LeftToRight, (Qt::AlignBottom | Qt::AlignRight), size(), geometry));
}

void NotificationDialog::aboutToClose()
{
	m_animation->setStartValue(1.0);
	m_animation->setEndValue(0.0);
	m_animation->start();

	connect(m_animation, SIGNAL(finished()), this, SLOT(clean()));
}

void NotificationDialog::clean()
{
	m_notification->markIgnored();

	close();
}

bool NotificationDialog::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
		{
			if (object == m_closeLabel)
			{
				m_notification->markIgnored();

				m_animation->stop();

				close();

				return true;
			}
			else
			{
				m_notification->markClicked();

				m_animation->stop();

				close();
			}
		}
	}

	return QWidget::eventFilter(object, event);
}

}
