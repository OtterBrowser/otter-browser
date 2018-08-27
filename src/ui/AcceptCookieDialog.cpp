/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "AcceptCookieDialog.h"

#include "ui_AcceptCookieDialog.h"

#include <QtCore/QDateTime>
#include <QtWidgets/QPushButton>

namespace Otter
{

AcceptCookieDialog::AcceptCookieDialog(const QNetworkCookie &cookie, CookieJar::CookieOperation operation, CookieJar *cookieJar, QWidget *parent) : Dialog(parent),
	m_cookieJar(cookieJar),
	m_cookie(cookie),
	m_operation(operation),
	m_ui(new Ui::AcceptCookieDialog)
{
	QString domain(cookie.domain());

	if (domain.startsWith(QLatin1Char('.')))
	{
		domain = domain.mid(1);
	}

	m_ui->setupUi(this);

	if (operation == CookieJar::InsertCookie)
	{
		m_ui->messageLabel->setText(tr("Website %1 requested to add new cookie.").arg(domain));
	}
	else if (operation == CookieJar::UpdateCookie)
	{
		m_ui->messageLabel->setText(tr("Website %1 requested to update existing cookie.").arg(domain));
	}
	else
	{
		m_ui->messageLabel->setText(tr("Website %1 requested to remove existing cookie.").arg(domain));
	}

	m_ui->domainValueLabelWidget->setText(cookie.domain());
	m_ui->nameValueLabelWidget->setText(QString(cookie.name()));
	m_ui->valueValueLabelWidget->setText(QString(cookie.value()));
	m_ui->expiresValueLabel->setText(cookie.expirationDate().isValid() ? cookie.expirationDate().toString(Qt::ISODate) : tr("This session only"));
	m_ui->isSecureValueLabel->setText(cookie.isSecure() ? tr("Secure connections only") : tr("Any type of connection"));
	m_ui->isHttpOnlyValueLabel->setText(cookie.isHttpOnly() ? tr("Yes") : tr("No"));
	m_ui->buttonBox->addButton(tr("Accept"), QDialogButtonBox::AcceptRole);

	if (operation != CookieJar::RemoveCookie && !cookie.isSessionCookie())
	{
		m_ui->buttonBox->addButton(tr("Accept For This Session Only"), QDialogButtonBox::AcceptRole)->setObjectName(QLatin1String("sessionOnly"));
	}

	m_ui->buttonBox->addButton(tr("Discard"), QDialogButtonBox::RejectRole);

	connect(m_ui->buttonBox, &QDialogButtonBox::clicked, this, &AcceptCookieDialog::handleButtonClicked);
}

AcceptCookieDialog::~AcceptCookieDialog()
{
	delete m_ui;
}

void AcceptCookieDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void AcceptCookieDialog::handleButtonClicked(QAbstractButton *button)
{
	const QDialogButtonBox::ButtonRole role(m_ui->buttonBox->buttonRole(button));
	const AcceptCookieResult result((role == QDialogButtonBox::AcceptRole) ? ((button->objectName() == QLatin1String("sessionOnly")) ? AcceptAsSessionCookie : AcceptCookie) : IgnoreCookie);

	if (m_operation == CookieJar::InsertCookie)
	{
		if (result == AcceptCookieDialog::AcceptAsSessionCookie)
		{
			m_cookie.setExpirationDate({});

			m_cookieJar->forceInsertCookie(m_cookie);
		}
		else if (result == AcceptCookieDialog::AcceptCookie)
		{
			m_cookieJar->forceInsertCookie(m_cookie);
		}
	}
	else if (m_operation == CookieJar::UpdateCookie)
	{
		if (result == AcceptCookieDialog::AcceptAsSessionCookie)
		{
			m_cookie.setExpirationDate({});

			m_cookieJar->forceUpdateCookie(m_cookie);
		}
		else if (result == AcceptCookieDialog::AcceptCookie)
		{
			m_cookieJar->forceUpdateCookie(m_cookie);
		}
	}
	else if (m_operation == CookieJar::InsertCookie && result != AcceptCookieDialog::IgnoreCookie)
	{
		m_cookieJar->forceDeleteCookie(m_cookie);
	}

	accept();
}

}
