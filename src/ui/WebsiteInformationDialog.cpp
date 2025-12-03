/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "WebsiteInformationDialog.h"
#include "CertificateDialog.h"
#include "../core/Application.h"
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

	const WebWidget::ContentStates state(widget->getContentState());
	const QString characterEncoding(widget->getCharacterEncoding());
	QString host(widget->getUrl().host());

	if (host.isEmpty())
	{
		host = (widget->getUrl().scheme() == QLatin1String("file") ? QLatin1String("localhost") : tr("(unknown)"));
	}

	setWindowTitle(tr("Information for %1").arg(host));

	if (state.testFlag(WebWidget::FraudContentState))
	{
		m_ui->stateLabel->setText(tr("This website was marked as fraud."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("badge-fraud"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WebWidget::MixedContentState))
	{
		m_ui->stateLabel->setText(tr("Your connection with this website is not private."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("badge-mixed"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WebWidget::SecureContentState))
	{
		m_ui->stateLabel->setText(tr("Your connection with this website is private."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("badge-secure"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WebWidget::RemoteContentState))
	{
		m_ui->stateLabel->setText(tr("Your connection with this website is not private."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("badge-remote"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WebWidget::LocalContentState))
	{
		m_ui->stateLabel->setText(tr("You are viewing content from your local filesystem."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("badge-local"), false).pixmap(16, 16));
	}
	else if (state.testFlag(WebWidget::ApplicationContentState))
	{
		m_ui->stateLabel->setText(tr("You are viewing safe page from Otter Browser."));
		m_ui->stateIconLabel->setPixmap(Application::windowIcon().pixmap(16, 16));
	}
	else
	{
		m_ui->stateLabel->setText(tr("No information."));
		m_ui->stateIconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("badge-unknown"), false).pixmap(16, 16));
	}

	m_ui->hostLabel->setText(host);
	m_ui->addressLabelWidget->setText(widget->getUrl().toString());
	m_ui->titleLabelWidget->setText(widget->getTitle());
	m_ui->encodingLabelWidget->setText(characterEncoding.isEmpty() ? tr("unknown") : characterEncoding);
	m_ui->sizeLabelWidget->setText(Utils::formatUnit(widget->getPageInformation(WebWidget::TotalBytesTotalInformation).toLongLong(), false, 1, true));
	m_ui->elementsLabelWidget->setText((widget->getPageInformation(WebWidget::RequestsBlockedInformation).toInt() > 0) ? tr("%1 (%n blocked)", "", widget->getPageInformation(WebWidget::RequestsBlockedInformation).toInt()).arg(widget->getPageInformation(WebWidget::RequestsStartedInformation).toInt()) : QString::number(widget->getPageInformation(WebWidget::RequestsStartedInformation).toInt()));
	m_ui->downloadDateLabelWidget->setText(Utils::formatDateTime(widget->getPageInformation(WebWidget::LoadingFinishedInformation).toDateTime()));

	const QString cookiesPolicy(widget->getOption(SettingsManager::Network_CookiesPolicyOption).toString());

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

	const QString thirdPartyCookiesPolicy(widget->getOption(SettingsManager::Network_ThirdPartyCookiesPolicyOption).toString());

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

	const QString pluginsPolicy(widget->getOption(SettingsManager::Permissions_EnablePluginsOption).toString());

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

	const QString imagesPolicy(widget->getOption(SettingsManager::Permissions_EnableImagesOption).toString());

	if (imagesPolicy == QLatin1String("onlyCached"))
	{
		m_ui->imagesValueLabel->setText(tr("Only cached"));
	}
	else if (imagesPolicy == QLatin1String("disabled"))
	{
		m_ui->imagesValueLabel->setText(tr("Never"));
	}
	else
	{
		m_ui->imagesValueLabel->setText(tr("Always"));
	}

	m_ui->javascriptValueLabel->setText(widget->getOption(SettingsManager::Permissions_EnableJavaScriptOption).toBool() ? tr("Always") : tr("Never"));

	const QString geolocationPolicy(widget->getOption(SettingsManager::Permissions_EnableGeolocationOption).toString());

	if (geolocationPolicy == QLatin1String("allow"))
	{
		m_ui->geolocationValueLabel->setText(tr("Always"));
	}
	else if (geolocationPolicy == QLatin1String("disallow"))
	{
		m_ui->geolocationValueLabel->setText(tr("Never"));
	}
	else
	{
		m_ui->geolocationValueLabel->setText(tr("Always ask"));
	}

	const QString fullScreenPolicy(widget->getOption(SettingsManager::Permissions_EnableFullScreenOption).toString());

	if (fullScreenPolicy == QLatin1String("allow"))
	{
		m_ui->fullScreenValueLabel->setText(tr("Always"));
	}
	else if (fullScreenPolicy == QLatin1String("disallow"))
	{
		m_ui->fullScreenValueLabel->setText(tr("Never"));
	}
	else
	{
		m_ui->fullScreenValueLabel->setText(tr("Always ask"));
	}

	const QString notificationsPolicy(widget->getOption(SettingsManager::Permissions_EnableNotificationsOption).toString());

	if (notificationsPolicy == QLatin1String("allow"))
	{
		m_ui->notificationsValueLabel->setText(tr("Always"));
	}
	else if (notificationsPolicy == QLatin1String("disallow"))
	{
		m_ui->notificationsValueLabel->setText(tr("Never"));
	}
	else
	{
		m_ui->notificationsValueLabel->setText(tr("Always ask"));
	}

	const QString popupsPolicy(widget->getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption).toString());

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

	connect(m_ui->preferencesDetailsButton, &QPushButton::clicked, this, [&]()
	{
		Application::triggerAction(ActionsManager::WebsitePreferencesAction, {}, this);
	});

	if (m_sslInformation.certificates.isEmpty())
	{
		m_ui->tabWidget->setTabEnabled(2, false);

		return;
	}

	const QSslCertificate certificate(m_sslInformation.certificates.value(0));

	m_ui->certificateIssuedToLabelWidget->setText(certificate.subjectInfo(QSslCertificate::CommonName).join(QLatin1String(", ")));
	m_ui->certificateIssuedByLabelWidget->setText(certificate.issuerInfo(QSslCertificate::CommonName).join(QLatin1String(", ")));
	m_ui->certificateIssuedOnLabelWidget->setText(Utils::formatDateTime(certificate.effectiveDate(), {}, false));
	m_ui->certificateExpiresOnLabelWidget->setText(Utils::formatDateTime(certificate.expiryDate(), {}, false));
	m_ui->cipherProtocolLabelWidget->setText(m_sslInformation.cipher.protocolString());
	m_ui->cipherAuthenticationMethodLabelWidget->setText(m_sslInformation.cipher.authenticationMethod());
	m_ui->cipherEncryptionMethodLabelWidget->setText(m_sslInformation.cipher.encryptionMethod());
	m_ui->cipherKeyExchangeMethodLabelWidget->setText(m_sslInformation.cipher.keyExchangeMethod());

	if (m_sslInformation.errors.isEmpty())
	{
		m_ui->sslErrorsHeaderLabel->hide();
		m_ui->sslErrorsViewWidget->hide();
	}
	else
	{
		QStandardItemModel *sslErrorsModel(new QStandardItemModel(this));
		sslErrorsModel->setHorizontalHeaderLabels({tr("Error Message"), tr("URL")});

		for (int i = 0; i < m_sslInformation.errors.count(); ++i)
		{
			QList<QStandardItem*> items({new QStandardItem(m_sslInformation.errors.at(i).error.errorString()), new QStandardItem(m_sslInformation.errors.at(i).url.toDisplayString())});
			items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			items[0]->setToolTip(items[0]->text());
			items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			items[1]->setToolTip(items[1]->text());

			sslErrorsModel->appendRow(items);
		}

		m_ui->sslErrorsViewWidget->setModel(sslErrorsModel);
	}

	connect(m_ui->certificateDetailsButton, &QPushButton::clicked, this, [&]()
	{
		CertificateDialog *dialog(new CertificateDialog(m_sslInformation.certificates));
		dialog->setAttribute(Qt::WA_DeleteOnClose);
		dialog->show();
	});
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

		if (m_ui->sslErrorsViewWidget->isVisible() && m_ui->sslErrorsViewWidget->getSourceModel())
		{
			m_ui->sslErrorsViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Error Message"), tr("URL")});
		}
	}
}

}
