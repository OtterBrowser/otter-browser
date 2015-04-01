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

#include "AcceptCookieDialog.h"

#include "ui_AcceptCookieDialog.h"

#include <QtCore/QDateTime>
#include <QtWidgets/QPushButton>

namespace Otter
{

AcceptCookieDialog::AcceptCookieDialog(const QNetworkCookie &cookie, CookieOperation operation, QWidget *parent) : QDialog(parent),
	m_result(IgnoreCookie),
	m_ui(new Ui::AcceptCookieDialog)
{
	QString domain = cookie.domain();

	if (domain.startsWith(QLatin1Char('.')))
	{
		domain = domain.mid(1);
	}

	m_ui->setupUi(this);

	if (operation == InsertCookie)
	{
		m_ui->messageLabel->setText(tr("Website %1 requested to add new cookie.").arg(domain));
	}
	else if (operation == UpdateCookie)
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
	m_ui->expiresValueLabel->setText(cookie.expirationDate().isValid() ? cookie.expirationDate().toString(Qt::ISODate) : tr("This Session Only"));
	m_ui->secureValueLabel->setText(cookie.isSecure() ? tr("Yes") : tr("No"));
	m_ui->httpOnlyValueLabel->setText(cookie.isHttpOnly() ? tr("Yes") : tr("No"));
	m_ui->buttonBox->addButton(tr("Accept"), QDialogButtonBox::AcceptRole);

	if (operation != RemoveCookie && !cookie.isSessionCookie())
	{
		m_ui->buttonBox->addButton(tr("Accept For This Session Only"), QDialogButtonBox::AcceptRole)->setObjectName(QLatin1String("sessionOnly"));
	}

	m_ui->buttonBox->addButton(tr("Discard"), QDialogButtonBox::RejectRole);

	connect(m_ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
}

AcceptCookieDialog::~AcceptCookieDialog()
{
	delete m_ui;
}

void AcceptCookieDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void AcceptCookieDialog::buttonClicked(QAbstractButton *button)
{
	const QDialogButtonBox::ButtonRole role = m_ui->buttonBox->buttonRole(button);

	if (role == QDialogButtonBox::AcceptRole)
	{
		m_result = ((button->objectName() == QLatin1String("sessionOnly")) ? AcceptAsSessionCookie : AcceptCookie);
	}
	else
	{
		m_result = IgnoreCookie;
	}

	accept();
}

AcceptCookieDialog::AcceptCookieResult AcceptCookieDialog::getResult() const
{
	return m_result;
}

}
