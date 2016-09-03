/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
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

#include "PermissionBarWidget.h"
#include "../../../core/ThemesManager.h"

#include "ui_PermissionBarWidget.h"

namespace Otter
{

PermissionBarWidget::PermissionBarWidget(WebWidget::FeaturePermission feature, const QUrl &url, QWidget *parent) : QWidget(parent),
	m_url(url),
	m_feature(feature),
	m_ui(new Ui::PermissionBarWidget)
{
	const QString domain(url.host());

	m_ui->setupUi(this);

	switch (feature)
	{
		case WebWidget::FullScreenFeature:
			m_ui->iconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("permission-fullscreen"), false).pixmap(m_ui->iconLabel->size()));
			m_ui->messageLabel->setText(tr("%1 wants to enter full screen mode.").arg(domain));

			break;
		case WebWidget::GeolocationFeature:
			m_ui->iconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("permission-geolocation"), false).pixmap(m_ui->iconLabel->size()));
			m_ui->messageLabel->setText(tr("%1 wants access to your location.").arg(domain));

			break;
		case WebWidget::NotificationsFeature:
			m_ui->iconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("permission-notifications"), false).pixmap(m_ui->iconLabel->size()));
			m_ui->messageLabel->setText(tr("%1 wants to show notifications.").arg(domain));

			break;
		case WebWidget::PointerLockFeature:
			m_ui->iconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("permission-pointer-lock"), false).pixmap(m_ui->iconLabel->size()));
			m_ui->messageLabel->setText(tr("%1 wants to lock mouse pointer.").arg(domain));

			break;
		case WebWidget::CaptureAudioFeature:
			m_ui->iconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("permission-capture-audio"), false).pixmap(m_ui->iconLabel->size()));
			m_ui->messageLabel->setText(tr("%1 wants to access your microphone.").arg(domain));

			break;
		case WebWidget::CaptureVideoFeature:
			m_ui->iconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("permission-capture-video"), false).pixmap(m_ui->iconLabel->size()));
			m_ui->messageLabel->setText(tr("%1 wants to access your camera.").arg(domain));

			break;
		case WebWidget::CaptureAudioVideoFeature:
			m_ui->iconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("permission-capture-audio-video"), false).pixmap(m_ui->iconLabel->size()));
			m_ui->messageLabel->setText(tr("%1 wants to access your microphone and camera.").arg(domain));

			break;
		case WebWidget::PlaybackAudioFeature:
			m_ui->iconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("permission-playback-audio"), false).pixmap(m_ui->iconLabel->size()));
			m_ui->messageLabel->setText(tr("%1 wants to play audio.").arg(domain));

			break;
		default:
			m_ui->iconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("dialog-error"), false).pixmap(m_ui->iconLabel->size()));
			m_ui->messageLabel->setText(tr("Invalid permission request from %1.").arg(domain));
			m_ui->permissionComboBox->hide();
			m_ui->okButton->hide();

			break;
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

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void PermissionBarWidget::accepted()
{
	hide();

	if (m_ui->permissionComboBox->currentIndex() == 0)
	{
		emit permissionChanged(WebWidget::GrantedPermission | WebWidget::KeepAskingPermission);
	}
	else
	{
		emit permissionChanged((m_ui->permissionComboBox->currentIndex() == 1) ? WebWidget::GrantedPermission : WebWidget::DeniedPermission);
	}
}

void PermissionBarWidget::rejected()
{
	hide();

	emit permissionChanged(WebWidget::DeniedPermission);
}

QUrl PermissionBarWidget::getUrl() const
{
	return m_url;
}

WebWidget::FeaturePermission PermissionBarWidget::getFeature() const
{
	return m_feature;
}

}
