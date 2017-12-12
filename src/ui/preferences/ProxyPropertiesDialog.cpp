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

#include "ProxyPropertiesDialog.h"

#include "ui_ProxyPropertiesDialog.h"

namespace Otter
{

ProxyPropertiesDialog::ProxyPropertiesDialog(const ProxyDefinition &proxy, QWidget *parent) : Dialog(parent),
	m_proxy(proxy),
	m_ui(new Ui::ProxyPropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->titleLineEditWidget->setText(proxy.getTitle());

	if (proxy.type == ProxyDefinition::AutomaticProxy)
	{
		m_ui->automaticConfigurationCheckBox->setChecked(true);
		m_ui->automaticConfigurationFilePathWidget->setPath(proxy.path);
	}
	else
	{
		m_ui->manualConfigurationCheckBox->setChecked(true);

		if (proxy.servers.contains(ProxyDefinition::AnyProtocol))
		{
			m_ui->allCheckBox->setChecked(true);
			m_ui->allServersLineEditWidget->setText(proxy.servers[ProxyDefinition::AnyProtocol].hostName);
			m_ui->allPortSpinBox->setValue(proxy.servers[ProxyDefinition::AnyProtocol].port);
		}
		else
		{
			if (proxy.servers.contains(ProxyDefinition::HttpProtocol))
			{
				m_ui->httpCheckBox->setChecked(true);
				m_ui->httpServersLineEditWidget->setText(proxy.servers[ProxyDefinition::HttpProtocol].hostName);
				m_ui->httpPortSpinBox->setValue(proxy.servers[ProxyDefinition::HttpProtocol].port);
			}

			if (proxy.servers.contains(ProxyDefinition::HttpsProtocol))
			{
				m_ui->httpsCheckBox->setChecked(true);
				m_ui->httpsServersLineEditWidget->setText(proxy.servers[ProxyDefinition::HttpsProtocol].hostName);
				m_ui->httpsPortSpinBox->setValue(proxy.servers[ProxyDefinition::HttpsProtocol].port);
			}

			if (proxy.servers.contains(ProxyDefinition::FtpProtocol))
			{
				m_ui->ftpCheckBox->setChecked(true);
				m_ui->ftpServersLineEditWidget->setText(proxy.servers[ProxyDefinition::FtpProtocol].hostName);
				m_ui->ftpPortSpinBox->setValue(proxy.servers[ProxyDefinition::FtpProtocol].port);
			}

			if (proxy.servers.contains(ProxyDefinition::SocksProtocol))
			{
				m_ui->socksCheckBox->setChecked(true);
				m_ui->socksServersLineEditWidget->setText(proxy.servers[ProxyDefinition::SocksProtocol].hostName);
				m_ui->socksPortSpinBox->setValue(proxy.servers[ProxyDefinition::SocksProtocol].port);
			}
		}
	}

	m_ui->usesSystemAuthenticationCheckBox->setChecked(proxy.usesSystemAuthentication);

	QStandardItemModel *exceptionsModel(new QStandardItemModel(this));

	for (int i = 0; i < proxy.exceptions.count(); ++i)
	{
		exceptionsModel->appendRow(new QStandardItem(proxy.exceptions.at(i)));
	}

	m_ui->exceptionsItemViewWidget->setModel(exceptionsModel);

	updateProxyType();
	setWindowTitle(proxy.isValid() ? tr ("Edit Proxy") : tr("Add Proxy"));

	connect(m_ui->buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &ProxyPropertiesDialog::updateProxyType);
	connect(m_ui->allCheckBox, &QCheckBox::clicked, this, &ProxyPropertiesDialog::updateProxyType);
	connect(m_ui->exceptionsItemViewWidget, &ItemViewWidget::needsActionsUpdate, this, &ProxyPropertiesDialog::updateExceptionsActions);
	connect(m_ui->addExceptionButton, &QPushButton::clicked, this, &ProxyPropertiesDialog::addException);
	connect(m_ui->editExceptionButton, &QPushButton::clicked, this, &ProxyPropertiesDialog::editException);
	connect(m_ui->removeExceptionButton, &QPushButton::clicked, this, &ProxyPropertiesDialog::removeException);
}

ProxyPropertiesDialog::~ProxyPropertiesDialog()
{
	delete m_ui;
}

void ProxyPropertiesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void ProxyPropertiesDialog::addException()
{
	m_ui->exceptionsItemViewWidget->insertRow();

	editException();
}

void ProxyPropertiesDialog::editException()
{
	m_ui->exceptionsItemViewWidget->edit(m_ui->exceptionsItemViewWidget->getIndex(m_ui->exceptionsItemViewWidget->getCurrentRow()));
}

void ProxyPropertiesDialog::removeException()
{
	m_ui->exceptionsItemViewWidget->removeRow();
	m_ui->exceptionsItemViewWidget->setFocus();

	updateExceptionsActions();
}

void ProxyPropertiesDialog::updateExceptionsActions()
{
	const bool isEditable(m_ui->exceptionsItemViewWidget->getCurrentRow() >= 0);

	m_ui->editExceptionButton->setEnabled(isEditable);
	m_ui->removeExceptionButton->setEnabled(isEditable);
}

void ProxyPropertiesDialog::updateProxyType()
{
	if (m_ui->manualConfigurationCheckBox->isChecked())
	{
		const bool usesSeparateServers(!m_ui->allCheckBox->isChecked());

		m_ui->manualConfigurationWidget->setEnabled(true);
		m_ui->automaticConfigurationWidget->setEnabled(false);
		m_ui->httpCheckBox->setEnabled(usesSeparateServers);
		m_ui->httpServersLineEditWidget->setEnabled(usesSeparateServers);
		m_ui->httpPortSpinBox->setEnabled(usesSeparateServers);
		m_ui->httpsCheckBox->setEnabled(usesSeparateServers);
		m_ui->httpsServersLineEditWidget->setEnabled(usesSeparateServers);
		m_ui->httpsPortSpinBox->setEnabled(usesSeparateServers);
		m_ui->ftpCheckBox->setEnabled(usesSeparateServers);
		m_ui->ftpServersLineEditWidget->setEnabled(usesSeparateServers);
		m_ui->ftpPortSpinBox->setEnabled(usesSeparateServers);
		m_ui->socksCheckBox->setEnabled(usesSeparateServers);
		m_ui->socksServersLineEditWidget->setEnabled(usesSeparateServers);
		m_ui->socksPortSpinBox->setEnabled(usesSeparateServers);
	}
	else
	{
		m_ui->manualConfigurationWidget->setEnabled(false);
		m_ui->automaticConfigurationWidget->setEnabled(true);
	}
}

ProxyDefinition ProxyPropertiesDialog::getProxy() const
{
	ProxyDefinition proxy(m_proxy);
	proxy.title = m_ui->titleLineEditWidget->text();
	proxy.usesSystemAuthentication = m_ui->usesSystemAuthenticationCheckBox->isChecked();
	proxy.exceptions.clear();
	proxy.servers.clear();

	if (m_ui->automaticConfigurationCheckBox->isChecked())
	{
		proxy.type = ProxyDefinition::AutomaticProxy;
		proxy.path = m_ui->automaticConfigurationFilePathWidget->getPath();
	}
	else
	{
		proxy.type = ProxyDefinition::ManualProxy;

		if (m_ui->allCheckBox->isChecked())
		{
			proxy.servers[ProxyDefinition::AnyProtocol] = ProxyDefinition::ProxyServer(m_ui->allServersLineEditWidget->text(), static_cast<quint16>(m_ui->allPortSpinBox->value()));
		}
		else
		{
			if (m_ui->httpCheckBox->isChecked())
			{
				proxy.servers[ProxyDefinition::HttpProtocol] = ProxyDefinition::ProxyServer(m_ui->httpServersLineEditWidget->text(), static_cast<quint16>(m_ui->httpPortSpinBox->value()));
			}

			if (m_ui->httpsCheckBox->isChecked())
			{
				proxy.servers[ProxyDefinition::HttpsProtocol] = ProxyDefinition::ProxyServer(m_ui->httpsServersLineEditWidget->text(), static_cast<quint16>(m_ui->httpsPortSpinBox->value()));
			}

			if (m_ui->ftpCheckBox->isChecked())
			{
				proxy.servers[ProxyDefinition::FtpProtocol] = ProxyDefinition::ProxyServer(m_ui->ftpServersLineEditWidget->text(), static_cast<quint16>(m_ui->ftpPortSpinBox->value()));
			}

			if (m_ui->socksCheckBox->isChecked())
			{
				proxy.servers[ProxyDefinition::SocksProtocol] = ProxyDefinition::ProxyServer(m_ui->socksServersLineEditWidget->text(), static_cast<quint16>(m_ui->socksPortSpinBox->value()));
			}
		}
	}

	for (int i = 0; i < m_ui->exceptionsItemViewWidget->getRowCount(); ++i)
	{
		const QString exception(m_ui->exceptionsItemViewWidget->getIndex(i).data(Qt::DisplayRole).toString().simplified());

		if (!exception.isEmpty())
		{
			proxy.exceptions.append(exception);
		}
	}

	return proxy;
}

}
