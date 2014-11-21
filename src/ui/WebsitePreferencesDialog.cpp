/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "WebsitePreferencesDialog.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/SettingsManager.h"

#include "ui_WebsitePreferencesDialog.h"

#include <QtCore/QDateTime>
#include <QtCore/QTextCodec>

namespace Otter
{

WebsitePreferencesDialog::WebsitePreferencesDialog(const QUrl &url, const QList<QNetworkCookie> &cookies, QWidget *parent) : QDialog(parent),
	m_ui(new Ui::WebsitePreferencesDialog)
{
	QList<int> textCodecs;
	textCodecs << 106 << 1015 << 1017 << 4 << 5 << 6 << 7 << 8 << 82 << 10 << 85 << 12 << 13 << 109 << 110 << 112 << 2250 << 2251 << 2252 << 2253 << 2254 << 2255 << 2256 << 2257 << 2258 << 18 << 39 << 17 << 38 << 2026;

	m_ui->setupUi(this);
	m_ui->websiteLineEdit->setText(url.isLocalFile() ? QLatin1String("localhost") : url.host());

	m_ui->encodingComboBox->addItem(tr("Auto Detect"));

	for (int i = 0; i < textCodecs.count(); ++i)
	{
		QTextCodec *codec = QTextCodec::codecForMib(textCodecs.at(i));

		if (!codec)
		{
			continue;
		}

		m_ui->encodingComboBox->addItem(codec->name(), textCodecs.at(i));
	}

	m_ui->encodingComboBox->setCurrentIndex(qMax(0, m_ui->encodingComboBox->findText(SettingsManager::getValue(QLatin1String("Content/DefaultEncoding"), url).toString())));
	m_ui->enableImagesCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/EnableImages"), url).toBool());
	m_ui->enableJavaCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/EnableJava"), url).toBool());

	const QString enablePlugins = SettingsManager::getValue(QLatin1String("Browser/EnablePlugins"), url).toString();

	m_ui->pluginsComboBox->addItem(tr("Enabled"), QLatin1String("enabled"));
	m_ui->pluginsComboBox->addItem(tr("On Demand"), QLatin1String("onDemand"));
	m_ui->pluginsComboBox->addItem(tr("Disabled"), QLatin1String("disabled"));

	if (enablePlugins == QLatin1String("disabled"))
	{
		m_ui->pluginsComboBox->setCurrentIndex(2);
	}
	else if (enablePlugins == QLatin1String("onDemand"))
	{
		m_ui->pluginsComboBox->setCurrentIndex(1);
	}
	else
	{
		m_ui->pluginsComboBox->setCurrentIndex(0);
	}

	m_ui->userStyleSheetFilePathWidget->setPath(SettingsManager::getValue(QLatin1String("Content/UserStyleSheet"), url).toString());
	m_ui->enableJavaScriptCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/EnableJavaScript"), url).toBool());
	m_ui->javaSriptCanAccessClipboardCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/JavaSriptCanAccessClipboard"), url).toBool());
	m_ui->javaScriptCanShowStatusMessagesCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanShowStatusMessages"), url).toBool());

	QStringList cookiesLabels;
	cookiesLabels << tr("Domain") << tr("Path") << tr("Value") << tr("Expiration date");

	QStandardItemModel *cookiesModel = new QStandardItemModel(this);
	cookiesModel->setHorizontalHeaderLabels(cookiesLabels);

	for (int i = 0; i < cookies.count(); ++i)
	{
		QList<QStandardItem*> items;
		items.append(new QStandardItem(cookies.at(i).domain()));
		items.append(new QStandardItem(cookies.at(i).path()));
		items.append(new QStandardItem(QString(cookies.at(i).value())));
		items.append(new QStandardItem(cookies.at(i).expirationDate().toString()));

		cookiesModel->appendRow(items);
	}

	m_ui->cookiesTableWidget->setModel(cookiesModel);

	const QStringList userAgents = NetworkManagerFactory::getUserAgents();

	m_ui->userAgentComboBox->addItem(tr("Default"), QLatin1String("default"));

	for (int i = 0; i < userAgents.count(); ++i)
	{
		const UserAgentInformation userAgent = NetworkManagerFactory::getUserAgent(userAgents.at(i));
		const QString title = userAgent.title;

		m_ui->userAgentComboBox->addItem((title.isEmpty() ? tr("(Untitled)") : title), userAgents.at(i));
		m_ui->userAgentComboBox->setItemData((i + 1), userAgent.value, (Qt::UserRole + 1));
	}

	m_ui->userAgentComboBox->setCurrentIndex(m_ui->userAgentComboBox->findData(SettingsManager::getValue(QLatin1String("Network/UserAgent"), url).toString()));

	connect(m_ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
}

WebsitePreferencesDialog::~WebsitePreferencesDialog()
{
	delete m_ui;
}

void WebsitePreferencesDialog::changeEvent(QEvent *event)
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

void WebsitePreferencesDialog::buttonClicked(QAbstractButton *button)
{
	QUrl url;
	url.setHost(m_ui->websiteLineEdit->text());

	switch (m_ui->buttonBox->buttonRole(button))
	{
		case QDialogButtonBox::AcceptRole:
			SettingsManager::setValue(QLatin1String("Content/DefaultEncoding"), ((m_ui->encodingComboBox->currentIndex() == 0) ? QString() : m_ui->encodingComboBox->currentText()), url);
			SettingsManager::setValue(QLatin1String("Browser/EnableImages"), m_ui->enableImagesCheckBox->isChecked(), url);
			SettingsManager::setValue(QLatin1String("Browser/EnableJava"), m_ui->enableJavaCheckBox->isChecked(), url);
			SettingsManager::setValue(QLatin1String("Browser/EnablePlugins"), m_ui->pluginsComboBox->currentData(Qt::UserRole).toString(), url);
			SettingsManager::setValue(QLatin1String("Content/UserStyleSheet"), m_ui->userStyleSheetFilePathWidget->getPath(), url);
			SettingsManager::setValue(QLatin1String("Browser/EnableJavaScript"), m_ui->enableJavaScriptCheckBox->isChecked(), url);
			SettingsManager::setValue(QLatin1String("Browser/JavaSriptCanAccessClipboard"), m_ui->javaSriptCanAccessClipboardCheckBox->isChecked(), url);
			SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanShowStatusMessages"), m_ui->javaScriptCanShowStatusMessagesCheckBox->isChecked(), url);
			SettingsManager::setValue(QLatin1String("Network/UserAgent"), ((m_ui->userAgentComboBox->currentIndex() == 0) ? QString() : m_ui->userAgentComboBox->currentData(Qt::UserRole).toString()), url);

			accept();

			break;
		case QDialogButtonBox::ResetRole:
			SettingsManager::removeOverride(url);
			accept();

			break;
		case QDialogButtonBox::RejectRole:
			reject();

			break;
		default:
			break;
	}
}

}
