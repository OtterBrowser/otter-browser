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

#include "WebsiteInformationDialog.h"
#include "CertificateDialog.h"
#include "../core/ThemesManager.h"
#include "../core/Utils.h"

#include "ui_WebsiteInformationDialog.h"

namespace Otter
{

WebsiteInformationDialog::WebsiteInformationDialog(WebWidget *widget, QWidget *parent) : Dialog(parent),
	m_sslInformation(widget->getSslInformation()),
	m_ui(new Ui::WebsiteInformationDialog)
{
	m_ui->setupUi(this);

	const QVariantHash statistics(widget->getStatistics());
	const WindowsManager::ContentStates state(widget->getContentState());
	const QString characterEncoding(widget->getCharacterEncoding());
	QString host(widget->getUrl().host());

	if (host.isEmpty())
	{
		host = (widget->getUrl().scheme() == QLatin1String("file") ? QLatin1String("localhost") : tr("(unknown)"));
	}

	if (state.testFlag(WindowsManager::FraudContentState))
	{
		m_ui->stateLabel->setText(tr("This website was marked as fraud."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("badge-fraud"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WindowsManager::TrustedContentState))
	{
		m_ui->stateLabel->setText(tr("Your connection with this website is private."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("badge-trusted"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WindowsManager::SecureContentState))
	{
		m_ui->stateLabel->setText(tr("Your connection with this website is private."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("badge-secure"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WindowsManager::RemoteContentState))
	{
		m_ui->stateLabel->setText(tr("Your connection with this website is not private."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("badge-remote"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WindowsManager::LocalContentState))
	{
		m_ui->stateLabel->setText(tr("You are viewing content from your local filesystem."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("badge-local"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WindowsManager::ApplicationContentState))
	{
		m_ui->stateLabel->setText(tr("You are viewing safe page from Otter Browser."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("otter-browser"), false).pixmap(16, 16));
	}
	else
	{
		m_ui->stateLabel->setText(tr("No information."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::getIcon(QLatin1String("badge-unknown"), false).pixmap(16, 16));
	}

	m_ui->hostLabel->setText(host);
	m_ui->addressLabelWidget->setText(widget->getUrl().toString());
	m_ui->titleLabelWidget->setText(widget->getTitle());
	m_ui->encodingLabelWidget->setText(characterEncoding.isEmpty() ? tr("unknown") : characterEncoding);
	m_ui->sizeLabelWidget->setText(Utils::formatUnit(statistics.value(QLatin1String("bytesTotal")).toLongLong(), false, 1, true));
	m_ui->elementsLabelWidget->setText((statistics.value(QLatin1String("requestsBlocked")).toInt() > 0) ? tr("%1 (%n blocked)", "", statistics.value(QLatin1String("requestsBlocked")).toInt()).arg(statistics.value(QLatin1String("requestsStarted")).toInt()) : QString::number(statistics.value(QLatin1String("requestsStarted")).toInt()));
	m_ui->downloadDateLabelWidget->setText(Utils::formatDateTime(statistics.value(QLatin1String("dateDownloaded")).toDateTime()));

	const QString cookiesPolicy = widget->getOption(QLatin1String("Network/CookiesPolicy")).toString();

	if (cookiesPolicy == QLatin1String("acceptExisting"))
	{
		m_ui->cookiesValueLabel->setText(tr("Only existing"));
	}
	else if (cookiesPolicy == QLatin1String("readOnly"))
	{
		m_ui->cookiesValueLabel->setText(tr("Only read existing"));
	}
	else if (cookiesPolicy == QLatin1String("ignore"))
	{
		m_ui->cookiesValueLabel->setText(tr("Never"));
	}
	else
	{
		m_ui->cookiesValueLabel->setText(tr("Always"));
	}

	const QString thirdPartyCookiesPolicy(widget->getOption(QLatin1String("Network/ThirdPartyCookiesPolicy")).toString());

	if (thirdPartyCookiesPolicy == QLatin1String("acceptExisting"))
	{
		m_ui->thirdPartyCookiesValueLabel->setText(tr("Only existing"));
	}
	else if (thirdPartyCookiesPolicy == QLatin1String("ignore"))
	{
		m_ui->thirdPartyCookiesValueLabel->setText(tr("Never"));
	}
	else
	{
		m_ui->thirdPartyCookiesValueLabel->setText(tr("Always"));
	}

	const QString pluginsPolicy(widget->getOption(QLatin1String("Browser/EnablePlugins")).toString());

	if (pluginsPolicy == QLatin1String("enabled"))
	{
		m_ui->pluginsValueLabel->setText(tr("Always"));
	}
	else if (pluginsPolicy == QLatin1String("disabled"))
	{
		m_ui->pluginsValueLabel->setText(tr("Never"));
	}
	else
	{
		m_ui->pluginsValueLabel->setText(tr("On demand"));
	}

	m_ui->imagesValueLabel->setText(widget->getOption(QLatin1String("Browser/EnableImages")).toBool() ? tr("Always") : tr("Never"));
	m_ui->javascriptValueLabel->setText(widget->getOption(QLatin1String("Browser/EnableJavaScript")).toBool() ? tr("Always") : tr("Never"));

	const QString geolocationPolicy(widget->getOption(QLatin1String("Browser/EnableGeolocation")).toString());

	if (geolocationPolicy == QLatin1String("enabled"))
	{
		m_ui->geolocationValueLabel->setText(tr("Always"));
	}
	else if (geolocationPolicy == QLatin1String("disabled"))
	{
		m_ui->geolocationValueLabel->setText(tr("Never"));
	}
	else
	{
		m_ui->geolocationValueLabel->setText(tr("Always Ask"));
	}

	const QString notificationsPolicy(widget->getOption(QLatin1String("Browser/EnableNotifications")).toString());

	if (notificationsPolicy == QLatin1String("enabled"))
	{
		m_ui->notificationsValueLabel->setText(tr("Always"));
	}
	else if (notificationsPolicy == QLatin1String("disabled"))
	{
		m_ui->notificationsValueLabel->setText(tr("Never"));
	}
	else
	{
		m_ui->notificationsValueLabel->setText(tr("Always Ask"));
	}

	const QString popupsPolicy(widget->getOption(QLatin1String("Content/PopupsPolicy")).toString());

	if (popupsPolicy == QLatin1String("openAll"))
	{
		m_ui->popupsValueLabel->setText(tr("Always"));
	}
	else if (popupsPolicy == QLatin1String("openAllInBackground"))
	{
		m_ui->popupsValueLabel->setText(tr("Always (open in backgound)"));
	}
	else if (popupsPolicy == QLatin1String("blockAll"))
	{
		m_ui->popupsValueLabel->setText(tr("Never"));
	}
	else
	{
		m_ui->popupsValueLabel->setText(tr("Ask"));
	}

	if (m_sslInformation.certificates.isEmpty())
	{
		m_ui->tabWidget->setTabEnabled(2, false);
	}
	else
	{
		const QSslCertificate certificate(m_sslInformation.certificates.first());

		m_ui->certificateIssuedToLabelWidget->setText(certificate.subjectInfo(QSslCertificate::CommonName).join(QLatin1String(", ")));
		m_ui->certificateIssuedByLabelWidget->setText(certificate.issuerInfo(QSslCertificate::CommonName).join(QLatin1String(", ")));
		m_ui->certificateIssuedOnLabelWidget->setText(Utils::formatDateTime(certificate.effectiveDate()));
		m_ui->certificateExpiresOnLabelWidget->setText(Utils::formatDateTime(certificate.expiryDate()));
		m_ui->cipherProtocolLabelWidget->setText(m_sslInformation.cipher.protocolString());
		m_ui->cipherAuthenticationMethodLabelWidget->setText(m_sslInformation.cipher.authenticationMethod());
		m_ui->cipherEncryptionMethodLabelWidget->setText(m_sslInformation.cipher.encryptionMethod());
		m_ui->cipherKeyExchangeMethodLabelWidget->setText(m_sslInformation.cipher.keyExchangeMethod());
	}

	if (m_sslInformation.errors.isEmpty())
	{
		m_ui->sslErrorsHeaderLabel->hide();
		m_ui->sslErrorsViewWidget->hide();
	}
	else
	{
		QStringList sslErrorsLabels;
		sslErrorsLabels << tr("Error Message") << tr("URL");

		QStandardItemModel *sslErrorsModel(new QStandardItemModel(this));
		sslErrorsModel->setHorizontalHeaderLabels(sslErrorsLabels);

		for (int i = 0; i < m_sslInformation.errors.count(); ++i)
		{
			QList<QStandardItem*> items;
			items.append(new QStandardItem(m_sslInformation.errors.at(i).second.errorString()));
			items[0]->setToolTip(items[0]->text());
			items.append(new QStandardItem(m_sslInformation.errors.at(i).first.toDisplayString()));
			items[1]->setToolTip(items[1]->text());

			sslErrorsModel->appendRow(items);
		}

		m_ui->sslErrorsViewWidget->setModel(sslErrorsModel);
	}

	setWindowTitle(tr("Information for %1").arg(host));

	connect(m_ui->preferencesDetailsButton, SIGNAL(clicked(bool)), this, SLOT(showPreferences()));
	connect(m_ui->certificateDetailsButton, SIGNAL(clicked(bool)), this, SLOT(showCertificate()));
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

void WebsiteInformationDialog::showPreferences()
{
	ActionsManager::triggerAction(ActionsManager::WebsitePreferencesAction, this);
}

void WebsiteInformationDialog::showCertificate()
{
	CertificateDialog *dialog(new CertificateDialog(m_sslInformation.certificates));
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
}

}
