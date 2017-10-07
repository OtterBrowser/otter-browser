/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "CookiePropertiesDialog.h"

#include "ui_CookiePropertiesDialog.h"

namespace Otter
{

CookiePropertiesDialog::CookiePropertiesDialog(const QNetworkCookie &cookie, QWidget *parent) : Dialog(parent),
	m_cookie(cookie),
	m_ui(new Ui::CookiePropertiesDialog)
{
	m_ui->setupUi(this);

	if (cookie.name().isEmpty())
	{
		setWindowTitle(tr("Add Cookie"));
	}
	else
	{
		setWindowTitle(tr("Edit Cookie"));
	}

	m_ui->nameLineEditWidget->setText(QString(cookie.name()));
	m_ui->valueLineEditWidget->setText(QString(cookie.value()));
	m_ui->domainLineEditWidget->setText(cookie.domain());
	m_ui->pathLineEditWidget->setText(cookie.path());
	m_ui->isSessionOnlyCheckBox->setChecked(!cookie.expirationDate().isValid());
	m_ui->expiresDateTimeEdit->setDateTime(cookie.expirationDate());
	m_ui->expiresDateTimeEdit->setEnabled(cookie.expirationDate().isValid());
	m_ui->isSecureCheckBox->setChecked(cookie.isSecure());
	m_ui->isHttpOnlyCheckBox->setChecked(cookie.isHttpOnly());

	connect(m_ui->isSessionOnlyCheckBox, &QCheckBox::clicked, m_ui->expiresDateTimeEdit, &CookiePropertiesDialog::setDisabled);
}

CookiePropertiesDialog::~CookiePropertiesDialog()
{
	delete m_ui;
}

void CookiePropertiesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

QNetworkCookie CookiePropertiesDialog::getOriginalCookie() const
{
	return m_cookie;
}

QNetworkCookie CookiePropertiesDialog::getModifiedCookie() const
{
	QNetworkCookie cookie(m_cookie);
	cookie.setName(m_ui->nameLineEditWidget->text().toUtf8());
	cookie.setValue(m_ui->valueLineEditWidget->text().toUtf8());
	cookie.setDomain(m_ui->domainLineEditWidget->text());
	cookie.setPath(m_ui->pathLineEditWidget->text());
	cookie.setExpirationDate(m_ui->isSessionOnlyCheckBox->isChecked() ? QDateTime() : m_ui->expiresDateTimeEdit->dateTime());
	cookie.setSecure(m_ui->isSecureCheckBox->isChecked());
	cookie.setHttpOnly(m_ui->isHttpOnlyCheckBox->isChecked());

	return cookie;
}

bool CookiePropertiesDialog::isModified() const
{
	return (m_cookie != getModifiedCookie());
}

}
