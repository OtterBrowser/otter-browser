/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PasswordBarWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"

#include "ui_PasswordBarWidget.h"

namespace Otter
{

PasswordBarWidget::PasswordBarWidget(const PasswordsManager::PasswordInformation &password, bool isUpdate, QWidget *parent) : QWidget(parent),
	m_created(QDateTime::currentDateTimeUtc()),
	m_password(password),
	m_isUpdate(isUpdate),
	m_ui(new Ui::PasswordBarWidget)
{
	m_ui->setupUi(this);
	m_ui->iconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("dialog-password"), false).pixmap(m_ui->iconLabel->size()));
	m_ui->messageLabel->setText((isUpdate ? tr("Do you want to update login data for %1?") : tr("Do you want to save login data for %1?")).arg(Utils::extractHost(password.url)));

	connect(m_ui->okButton, &QToolButton::clicked, this, &PasswordBarWidget::handleAccepted);
	connect(m_ui->cancelButton, &QToolButton::clicked, this, &PasswordBarWidget::handleRejected);
}

PasswordBarWidget::~PasswordBarWidget()
{
	delete m_ui;
}

void PasswordBarWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
		m_ui->messageLabel->setText((m_isUpdate ? tr("Do you want to update login data for %1?") : tr("Do you want to save login data for %1?")).arg(Utils::extractHost(m_password.url)));
	}
}

void PasswordBarWidget::handleAccepted()
{
	hide();

	emit requestedClose();

	PasswordsManager::addPassword(m_password);
}

void PasswordBarWidget::handleRejected()
{
	hide();

	emit requestedClose();
}

bool PasswordBarWidget::shouldClose(const QUrl &url) const
{
	return (url.host() != m_password.url.host() || m_created.secsTo(QDateTime::currentDateTimeUtc()) > 10);
}

}
