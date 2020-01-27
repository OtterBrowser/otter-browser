/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2016 - 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ContentBlockingProfileDialog.h"
#include "../core/Utils.h"

#include "ui_ContentBlockingProfileDialog.h"

#include <QtWidgets/QMessageBox>

namespace Otter
{

ContentBlockingProfileDialog::ContentBlockingProfileDialog(const ProfileSummary &profile, QWidget *parent) : Dialog(parent),
	m_name(profile.name),
	m_ui(new Ui::ContentBlockingProfileDialog)
{
	m_ui->setupUi(this);
	m_ui->categoryComboBox->addItem(tr("Advertisements"), ContentFiltersProfile::AdvertisementsCategory);
	m_ui->categoryComboBox->addItem(tr("Annoyance"), ContentFiltersProfile::AnnoyanceCategory);
	m_ui->categoryComboBox->addItem(tr("Privacy"), ContentFiltersProfile::PrivacyCategory);
	m_ui->categoryComboBox->addItem(tr("Social"), ContentFiltersProfile::SocialCategory);
	m_ui->categoryComboBox->addItem(tr("Regional"), ContentFiltersProfile::RegionalCategory);
	m_ui->categoryComboBox->addItem(tr("Other"), ContentFiltersProfile::OtherCategory);
	m_ui->categoryComboBox->setCurrentIndex(m_ui->categoryComboBox->findData(profile.category));
	m_ui->titleLineEdit->setText(profile.title);
	m_ui->updateUrLineEdit->setText(profile.updateUrl.toString());
	m_ui->lastUpdateTextLabel->setText(Utils::formatDateTime(profile.lastUpdate));
	m_ui->updateIntervalSpinBox->setValue(profile.updateInterval);

	connect(m_ui->confirmButtonBox, &QDialogButtonBox::accepted, [&]()
	{
		if (QUrl(m_ui->updateUrLineEdit->text()).isValid())
		{
			accept();
		}
		else
		{
			QMessageBox::critical(this, tr("Error"), tr("Valid update URL is required."), QMessageBox::Close);
		}
	});
	connect(m_ui->confirmButtonBox, &QDialogButtonBox::rejected, this, &ContentBlockingProfileDialog::close);
}

ContentBlockingProfileDialog::~ContentBlockingProfileDialog()
{
	delete m_ui;
}

void ContentBlockingProfileDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
		m_ui->categoryComboBox->setItemText(0, tr("Advertisements"));
		m_ui->categoryComboBox->setItemText(1, tr("Annoyance"));
		m_ui->categoryComboBox->setItemText(2, tr("Privacy"));
		m_ui->categoryComboBox->setItemText(3, tr("Social"));
		m_ui->categoryComboBox->setItemText(4, tr("Regional"));
		m_ui->categoryComboBox->setItemText(5, tr("Other"));
	}
}

ContentBlockingProfileDialog::ProfileSummary ContentBlockingProfileDialog::getProfile() const
{
	ProfileSummary profile;
	profile.name = m_name;
	profile.title = m_ui->titleLineEdit->text();
	profile.updateUrl = QUrl(m_ui->updateUrLineEdit->text());
	profile.category = static_cast<ContentFiltersProfile::ProfileCategory>(m_ui->categoryComboBox->itemData(m_ui->categoryComboBox->currentIndex()).toInt());
	profile.updateInterval = m_ui->updateIntervalSpinBox->value();

	return profile;
}

}
