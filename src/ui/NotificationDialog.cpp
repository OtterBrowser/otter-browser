/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include "../core/Notification.h"
#include "../core/Utils.h"

#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QStyleOptionFrame>

namespace Otter
{

NotificationDialog::NotificationDialog(Notification *notification, QWidget *parent) : QDialog(parent),
	m_notification(notification),
	m_closeLabel(new QLabel(this))
{
	QString notificationLevel = tr("Information");

	if (notification->getLevel() == WarningNotificationLevel)
	{
		notificationLevel = tr("Warning");
	}
	else if (notification->getLevel() == ErrorNotificationLevel)
	{
		notificationLevel = tr("Error");
	}

	QLabel *headerLabel = new QLabel(this);
	headerLabel->setStyleSheet(QLatin1String("QLabel {padding-left:5px;padding-top:8px;font-size:15px;font-weight:bold;}"));
	headerLabel->setText(notificationLevel);

	m_closeLabel->setToolTip(tr("Close"));
	m_closeLabel->setPixmap(Utils::getIcon(QLatin1String("window-close")).pixmap(16, 16));
	m_closeLabel->setMargin(5);
	m_closeLabel->installEventFilter(this);

	QLabel *iconLabel = new QLabel(this);
	iconLabel->setPixmap(Utils::getIcon(QLatin1String("otter-browser-32")).pixmap(32, 32));
	iconLabel->setStyleSheet(QLatin1String("QLabel{padding-left:5px;}"));
	iconLabel->setMargin(5);

	QBoxLayout *titleLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	titleLayout->setContentsMargins(0, 0, 0, 0);
	titleLayout->setSpacing(0);
	titleLayout->addWidget(headerLabel, 0, Qt::AlignLeft);
	titleLayout->addWidget(m_closeLabel, 1, Qt::AlignRight);

	QLabel *infoLabel = new QLabel(this);
	infoLabel->setText(m_notification->getMessage());
	infoLabel->setStyleSheet(QLatin1String("QLabel{padding-left:5px;font-size:13px;}"));
	infoLabel->setWordWrap(true);
	infoLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

	QBoxLayout *contentLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	contentLayout->setContentsMargins(0, 0, 0, 0);
	contentLayout->setSpacing(0);
	contentLayout->addLayout(titleLayout);
	contentLayout->addWidget(infoLabel);
	contentLayout->setSizeConstraint(QLayout::SetMinimumSize);

	QBoxLayout *allLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	allLayout->setContentsMargins(0, 0, 0, 0);
	allLayout->setSpacing(0);
	allLayout->addWidget(iconLabel);
	allLayout->addLayout(contentLayout);
	allLayout->setSizeConstraint(QLayout::SetMinimumSize);

	QFrame *frame = new QFrame(this);
	frame->setLayout(allLayout);
	frame->setObjectName(QLatin1String("notificationFrame"));
	frame->setStyleSheet(QLatin1String("#notificationFrame{border-radius:10;border: 1px solid #ccc;background-color:#f0f0f0;}"));
	frame->setCursor(QCursor(Qt::PointingHandCursor));
	frame->installEventFilter(this);

	QBoxLayout *mainLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(frame);
	mainLayout->setSizeConstraint(QLayout::SetMinimumSize);

	m_animation = new QPropertyAnimation(this, QStringLiteral("windowOpacity").toLatin1());
	m_animation->setDuration(4000);
	m_animation->setStartValue(1.0);
	m_animation->setEndValue(0.0);

	connect(m_animation, SIGNAL(finished()), this, SLOT(clean()));

	setLayout(mainLayout);
	setFixedWidth(350);
	setMinimumHeight(50);
	setMaximumHeight(150);
	setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool | Qt::FramelessWindowHint);
	setFocusPolicy(Qt::NoFocus);
	setAttribute(Qt::WA_ShowWithoutActivating);
	setAttribute(Qt::WA_TranslucentBackground);
	adjustSize();

	QTimer::singleShot((10 * 1000), this, SLOT(aboutToClose()));
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
	m_animation->start();
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
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

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
