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

#include "PasswordBarWidget.h"
#include "../../../core/ThemesManager.h"

#include "ui_PasswordBarWidget.h"

namespace Otter
{

PasswordBarWidget::PasswordBarWidget(const PasswordsManager::PasswordInformation &password, QWidget *parent) : QWidget(parent),
	m_created(QDateTime::currentDateTime()),
	m_ui(new Ui::PasswordBarWidget)
{
	m_ui->setupUi(this);
	m_ui->iconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("dialog-password"), false).pixmap(m_ui->iconLabel->size()));
	m_ui->messageLabel->setText(tr("Do you want to save login data for %1?").arg(password.url.host().isEmpty() ? QLatin1String("localhost") : password.url.host()));

	connect(m_ui->okButton, SIGNAL(clicked()), this, SLOT(accepted()));
	connect(m_ui->cancelButton, SIGNAL(clicked()), this, SLOT(rejected()));
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
	}
}

void PasswordBarWidget::accepted()
{
	hide();

	emit requestedClose();

	PasswordsManager::addPassword(m_password);
}

void PasswordBarWidget::rejected()
{
	hide();

	emit requestedClose();
}

bool PasswordBarWidget::shouldClose(const QUrl &url) const
{
	return (url.host() != m_password.url.host() || m_created.secsTo(QDateTime::currentDateTime()) > 10);
}

}
