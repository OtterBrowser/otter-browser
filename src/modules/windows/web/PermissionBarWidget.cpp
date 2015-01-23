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

#include "PermissionBarWidget.h"
#include "../../../core/Utils.h"

#include "ui_PermissionBarWidget.h"

namespace Otter
{

PermissionBarWidget::PermissionBarWidget(const QString &option, const QUrl &url, QWidget *parent) : QWidget(parent),
	m_option(option),
	m_url(url),
	m_ui(new Ui::PermissionBarWidget)
{
	const QString domain = url.toString(QUrl::RemoveUserInfo | QUrl::RemovePath | QUrl::PreferLocalFile | QUrl::StripTrailingSlash);

	m_ui->setupUi(this);

	if (option == QLatin1String("Browser/EnableGeolocation"))
	{
		m_ui->iconLabel->setPixmap(Utils::getIcon(QLatin1String("permission-geolocation"), false).pixmap(m_ui->iconLabel->size()));
		m_ui->messageLabel->setText(tr("%1 wants access to your location.").arg(domain));
	}
	else if (option == QLatin1String("Browser/EnableNotifications"))
	{
		m_ui->iconLabel->setPixmap(Utils::getIcon(QLatin1String("permission-notifications"), false).pixmap(m_ui->iconLabel->size()));
		m_ui->messageLabel->setText(tr("%1 wants to show notifications.").arg(domain));
	}
	else if (option == QLatin1String("Browser/EnableMediaCaptureAudio"))
	{
		m_ui->iconLabel->setPixmap(Utils::getIcon(QLatin1String("permission-permission-capture-audio"), false).pixmap(m_ui->iconLabel->size()));
		m_ui->messageLabel->setText(tr("%1 wants to access your microphone.").arg(domain));
	}
	else if (option == QLatin1String("Browser/EnableMediaCaptureVideo"))
	{
		m_ui->iconLabel->setPixmap(Utils::getIcon(QLatin1String("permission-permission-capture-video"), false).pixmap(m_ui->iconLabel->size()));
		m_ui->messageLabel->setText(tr("%1 wants to access your camera.").arg(domain));
	}
	else if (option == QLatin1String("Browser/EnableMediaCaptureAudioVideo"))
	{
		m_ui->iconLabel->setPixmap(Utils::getIcon(QLatin1String("permission-permission-capture-audio-video"), false).pixmap(m_ui->iconLabel->size()));
		m_ui->messageLabel->setText(tr("%1 wants to access your microphone and camera.").arg(domain));
	}
	else
	{
		m_ui->iconLabel->setPixmap(Utils::getIcon(QLatin1String("dialog-error"), false).pixmap(m_ui->iconLabel->size()));
		m_ui->messageLabel->setText(tr("Invalid permission request from %1.").arg(domain));
		m_ui->permissionComboBox->hide();
		m_ui->okButton->hide();
	}

	connect(m_ui->okButton, SIGNAL(clicked()), this, SLOT(accepted()));
	connect(m_ui->cancelButton, SIGNAL(clicked()), this, SLOT(rejected()));
}

PermissionBarWidget::~PermissionBarWidget()
{
	delete m_ui;
}

void PermissionBarWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void PermissionBarWidget::accepted()
{
	hide();

	if (m_ui->permissionComboBox->currentIndex() == 0)
	{
		emit permissionChanged(WebWidget::GrantedPermission);
	}
	else
	{
		WebWidget::PermissionPolicies policies = WebWidget::RememberPermission;

		if (m_ui->permissionComboBox->currentIndex() == 1)
		{
			policies |= WebWidget::GrantedPermission;
		}
		else
		{
			policies |= WebWidget::DeniedPermission;
		}

		emit permissionChanged(policies);
	}
}

void PermissionBarWidget::rejected()
{
	hide();

	emit permissionChanged(WebWidget::DeniedPermission);
}

QString PermissionBarWidget::getOption() const
{
	return m_option;
}

QUrl PermissionBarWidget::getUrl() const
{
	return m_url;
}

}
