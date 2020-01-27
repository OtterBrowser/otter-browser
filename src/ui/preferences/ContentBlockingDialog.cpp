/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "ContentBlockingDialog.h"
#include "../../core/AdblockContentFiltersProfile.h"
#include "../../core/Console.h"
#include "../../core/ContentFiltersManager.h"
#include "../../core/SessionsManager.h"
#include "../../core/SettingsManager.h"
#include "../../core/ThemesManager.h"

#include "ui_ContentBlockingDialog.h"

#include <QtCore/QBuffer>

namespace Otter
{

ContentBlockingDialog::ContentBlockingDialog(QWidget *parent) : Dialog(parent),
	m_ui(new Ui::ContentBlockingDialog)
{
	m_ui->setupUi(this);
	m_ui->cosmeticFiltersComboBox->addItem(tr("All"), QLatin1String("all"));
	m_ui->cosmeticFiltersComboBox->addItem(tr("Domain specific only"), QLatin1String("domainOnly"));
	m_ui->cosmeticFiltersComboBox->addItem(tr("None"), QLatin1String("none"));

	const int cosmeticFiltersIndex(m_ui->cosmeticFiltersComboBox->findData(SettingsManager::getOption(SettingsManager::ContentBlocking_CosmeticFiltersModeOption).toString()));

	m_ui->cosmeticFiltersComboBox->setCurrentIndex((cosmeticFiltersIndex < 0) ? 0 : cosmeticFiltersIndex);
	m_ui->enableCustomRulesCheckBox->setChecked(SettingsManager::getOption(SettingsManager::ContentBlocking_ProfilesOption).toStringList().contains(QLatin1String("custom")));

	QStandardItemModel *customRulesModel(new QStandardItemModel(this));
	QFile file(SessionsManager::getWritableDataPath("contentBlocking/custom.txt"));

	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream stream(&file);

		if (stream.readLine().trimmed().startsWith(QLatin1String("[Adblock Plus"), Qt::CaseInsensitive))
		{
			while (!stream.atEnd())
			{
				QStandardItem *item(new QStandardItem(stream.readLine().trimmed()));
				item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

				customRulesModel->appendRow(item);
			}
		}
		else
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to load custom rules: invalid adblock header"), Console::OtherCategory, Console::ErrorLevel, file.fileName());
		}

		file.close();
	}

	m_ui->customRulesViewWidget->setModel(customRulesModel);
	m_ui->enableWildcardsCheckBox->setChecked(SettingsManager::getOption(SettingsManager::ContentBlocking_EnableWildcardsOption).toBool());

	connect(m_ui->profilesViewWidget->selectionModel(), &QItemSelectionModel::currentChanged, this, &ContentBlockingDialog::updateProfilesActions);
	connect(m_ui->addProfileButton, &QPushButton::clicked, m_ui->profilesViewWidget, &ContentFiltersViewWidget::addProfile);
	connect(m_ui->editProfileButton, &QPushButton::clicked, m_ui->profilesViewWidget, &ContentFiltersViewWidget::editProfile);
	connect(m_ui->updateProfileButton, &QPushButton::clicked, m_ui->profilesViewWidget, &ContentFiltersViewWidget::updateProfile);
	connect(m_ui->removeProfileButton, &QPushButton::clicked, m_ui->profilesViewWidget, &ContentFiltersViewWidget::removeProfile);
	connect(m_ui->confirmButtonBox, &QDialogButtonBox::accepted, this, &ContentBlockingDialog::save);
	connect(m_ui->confirmButtonBox, &QDialogButtonBox::rejected, this, &ContentBlockingDialog::close);
	connect(m_ui->enableCustomRulesCheckBox, &QCheckBox::toggled, this, &ContentBlockingDialog::updateRulesActions);
	connect(m_ui->customRulesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &ContentBlockingDialog::updateRulesActions);
	connect(m_ui->addRuleButton, &QPushButton::clicked, this, &ContentBlockingDialog::addRule);
	connect(m_ui->editRuleButton, &QPushButton::clicked, this, &ContentBlockingDialog::editRule);
	connect(m_ui->removeRuleButton, &QPushButton::clicked, this, &ContentBlockingDialog::removeRule);

	updateRulesActions();
}

ContentBlockingDialog::~ContentBlockingDialog()
{
	delete m_ui;
}

void ContentBlockingDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void ContentBlockingDialog::updateProfilesActions()
{
	const QModelIndex index(m_ui->profilesViewWidget->currentIndex().sibling(m_ui->profilesViewWidget->currentIndex().row(), 0));
	const bool isEditable(index.isValid() && index.flags().testFlag(Qt::ItemNeverHasChildren));

	m_ui->editProfileButton->setEnabled(isEditable);
	m_ui->removeProfileButton->setEnabled(isEditable);
	m_ui->updateProfileButton->setEnabled(index.isValid() && index.data(ContentFiltersViewWidget::UpdateUrlRole).toUrl().isValid());
}

void ContentBlockingDialog::addRule()
{
	m_ui->customRulesViewWidget->insertRow();

	editRule();
}

void ContentBlockingDialog::editRule()
{
	m_ui->customRulesViewWidget->edit(m_ui->customRulesViewWidget->getIndex(m_ui->customRulesViewWidget->getCurrentRow()));
}

void ContentBlockingDialog::removeRule()
{
	m_ui->customRulesViewWidget->removeRow();
	m_ui->customRulesViewWidget->setFocus();

	updateRulesActions();
}

void ContentBlockingDialog::updateRulesActions()
{
	m_ui->tabWidget->setTabEnabled(1, m_ui->enableCustomRulesCheckBox->isChecked());

	const bool isEditable(m_ui->customRulesViewWidget->getCurrentRow() >= 0);

	m_ui->editRuleButton->setEnabled(isEditable);
	m_ui->removeRuleButton->setEnabled(isEditable);
}

void ContentBlockingDialog::save()
{
	m_ui->profilesViewWidget->save(m_ui->enableCustomRulesCheckBox->isChecked());

	if (m_ui->enableCustomRulesCheckBox->isChecked())
	{
		QByteArray data;
		QBuffer buffer(&data);
		buffer.open(QIODevice::ReadWrite | QIODevice::Text);

		for (int i = 0; i < m_ui->customRulesViewWidget->getRowCount(); ++i)
		{
			buffer.write(m_ui->customRulesViewWidget->getIndex(i, 0).data().toString().toLatin1() + QStringLiteral("\n").toUtf8());
		}

		buffer.reset();

		AdblockContentFiltersProfile::create(QLatin1String("custom"), tr("Custom Rules"), {}, 0, ContentFiltersProfile::OtherCategory, &buffer, true);
	}

	SettingsManager::setOption(SettingsManager::ContentBlocking_EnableWildcardsOption, m_ui->enableWildcardsCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::ContentBlocking_CosmeticFiltersModeOption, m_ui->cosmeticFiltersComboBox->currentData().toString());

	close();
}

}
