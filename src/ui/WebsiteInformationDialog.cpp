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

#include "WebsiteInformationDialog.h"
#include "../core/Utils.h"

#include "ui_WebsiteInformationDialog.h"

namespace Otter
{

WebsiteInformationDialog::WebsiteInformationDialog(WebWidget *widget, QWidget *parent) : Dialog(parent),
	m_sslInformation(widget->getSslInformation()),
	m_ui(new Ui::WebsiteInformationDialog)
{
	m_ui->setupUi(this);
	m_ui->tabWidget->setTabEnabled(1, false);

	const QVariantHash statistics = widget->getStatistics();
	const WindowsManager::ContentStates state = widget->getContentState();
	QString host = widget->getUrl().host();

	if (host.isEmpty())
	{
		host = (widget->getUrl().scheme() == QLatin1String("file") ? QLatin1String("localhost") : tr("(unknown)"));
	}

	if (state.testFlag(WindowsManager::FraudContentState))
	{
		m_ui->stateLabel->setText(tr("This website was marked as fraud."));
		m_ui->stateIconLabel->setPixmap(Utils::getIcon(QLatin1String("badge-fraud"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WindowsManager::TrustedContentState))
	{
		m_ui->stateLabel->setText(tr("Your connection with this website is private."));
		m_ui->stateIconLabel->setPixmap(Utils::getIcon(QLatin1String("badge-trusted"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WindowsManager::SecureContentState))
	{
		m_ui->stateLabel->setText(tr("Your connection with this website is private."));
		m_ui->stateIconLabel->setPixmap(Utils::getIcon(QLatin1String("badge-secure"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WindowsManager::RemoteContentState))
	{
		m_ui->stateLabel->setText(tr("Your connection with this website is not private."));
		m_ui->stateIconLabel->setPixmap(Utils::getIcon(QLatin1String("badge-remote"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WindowsManager::LocalContentState))
	{
		m_ui->stateLabel->setText(tr("You are viewing content from your local filesystem."));
		m_ui->stateIconLabel->setPixmap(Utils::getIcon(QLatin1String("badge-local"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WindowsManager::ApplicationContentState))
	{
		m_ui->stateLabel->setText(tr("You are viewing safe page from Otter Browser."));
		m_ui->stateIconLabel->setPixmap(Utils::getIcon(QLatin1String("otter-browser"), false).pixmap(16, 16));
	}
	else
	{
		m_ui->stateLabel->setText(tr("No information."));
		m_ui->stateIconLabel->setPixmap(Utils::getIcon(QLatin1String("badge-unknown"), false).pixmap(16, 16));
	}

	m_ui->hostLabel->setText(host);
	m_ui->addressLabelWidget->setText(widget->getUrl().toString());
	m_ui->titleLabelWidget->setText(widget->getTitle());
///FIXME
	m_ui->encodingLabelWidget->setText(tr("unknown"));
	m_ui->sizeLabelWidget->setText(Utils::formatUnit(statistics.value(QLatin1String("bytesTotal")).toLongLong(), false, 1, true));
	m_ui->elementsLabelWidget->setText((statistics.value(QLatin1String("requestsBlocked")).toInt() > 0) ? tr("%1 (%n blocked)", "", statistics.value(QLatin1String("requestsBlocked")).toInt()).arg(statistics.value(QLatin1String("requestsStarted")).toInt()) : QString::number(statistics.value(QLatin1String("requestsStarted")).toInt()));
	m_ui->downloadDateLabelWidget->setText(Utils::formatDateTime(statistics.value(QLatin1String("dateDownloaded")).toDateTime()));

	if (m_sslInformation.certificate.isNull())
	{
		m_ui->tabWidget->setTabEnabled(2, false);
	}
	else
	{
		m_ui->certificateNameLabelWidget->setText(m_sslInformation.certificate.subjectInfo(QSslCertificate::CommonName).join(QLatin1String(", ")));
		m_ui->certificateEffectiveDateLabelWidget->setText(Utils::formatDateTime(m_sslInformation.certificate.effectiveDate()));
		m_ui->certificateExpirationDateLabelWidget->setText(Utils::formatDateTime(m_sslInformation.certificate.expiryDate()));
		m_ui->cipherProtocolLabelWidget->setText(m_sslInformation.cipher.protocolString());
		m_ui->cipherAuthenticationMethodLabelWidget->setText(m_sslInformation.cipher.authenticationMethod());
		m_ui->cipherEncryptionMethodLabelWidget->setText(m_sslInformation.cipher.encryptionMethod());
		m_ui->cipherKeyExchangeMethodLabelWidget->setText(m_sslInformation.cipher.keyExchangeMethod());
	}

	setWindowTitle(tr("Information for %1").arg(host));
}

WebsiteInformationDialog::~WebsiteInformationDialog()
{
	delete m_ui;
}

void WebsiteInformationDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

}
