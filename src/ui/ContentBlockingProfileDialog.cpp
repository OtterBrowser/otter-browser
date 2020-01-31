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

ContentBlockingProfileDialog::ContentBlockingProfileDialog(const ProfileSummary &profileSummary, QWidget *parent) : Dialog(parent),
	m_name(profileSummary.name),
	m_isSourceLoaded(false),
	m_ui(new Ui::ContentBlockingProfileDialog)
{
	m_ui->setupUi(this);
	m_ui->categoryComboBox->addItem(tr("Advertisements"), ContentFiltersProfile::AdvertisementsCategory);
	m_ui->categoryComboBox->addItem(tr("Annoyance"), ContentFiltersProfile::AnnoyanceCategory);
	m_ui->categoryComboBox->addItem(tr("Privacy"), ContentFiltersProfile::PrivacyCategory);
	m_ui->categoryComboBox->addItem(tr("Social"), ContentFiltersProfile::SocialCategory);
	m_ui->categoryComboBox->addItem(tr("Regional"), ContentFiltersProfile::RegionalCategory);
	m_ui->categoryComboBox->addItem(tr("Other"), ContentFiltersProfile::OtherCategory);
	m_ui->categoryComboBox->setCurrentIndex(m_ui->categoryComboBox->findData(profileSummary.category));
	m_ui->titleLineEdit->setText(profileSummary.title);
	m_ui->updateUrLineEdit->setText(profileSummary.updateUrl.toString());
	m_ui->updateIntervalSpinBox->setValue(profileSummary.updateInterval);
	m_ui->passiveNotificationWidget->setMessage(tr("Any changes made here are going to be lost during manual or automatic update."), Notification::Message::WarningLevel);
	m_ui->saveButton->setIcon(ThemesManager::createIcon(QLatin1String("document-save")));

	if (!profileSummary.name.isEmpty())
	{
		ContentFiltersProfile *profile(ContentFiltersManager::getProfile(profileSummary.name));

		if (profile)
		{
			m_ui->lastUpdateTextLabel->setText(Utils::formatDateTime(profile->getLastUpdate()));
			m_ui->updateButton->setEnabled(profileSummary.updateUrl.isValid());

			connect(profile, &ContentFiltersProfile::profileModified, [&]()
			{
				profile = ContentFiltersManager::getProfile(m_name);

				if (profile)
				{
					m_ui->lastUpdateTextLabel->setText(Utils::formatDateTime(profile->getLastUpdate()));
				}
			});
			connect(m_ui->updateUrLineEdit, &QLineEdit::textChanged, [&]()
			{
				m_ui->updateButton->setEnabled(QUrl(m_ui->updateUrLineEdit->text()).isValid());
			});
			connect(m_ui->updateButton, &QPushButton::clicked, [&]()
			{
				profile = ContentFiltersManager::getProfile(m_name);

				const QUrl url(m_ui->updateUrLineEdit->text());

				if (profile && url.isValid())
				{
					profile->update(url);
				}
			});
			connect(m_ui->tabWidget, &QTabWidget::currentChanged, this, &ContentBlockingProfileDialog::handleCurrentTabChanged);
		}
	}

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
	connect(m_ui->saveButton, &QPushButton::clicked, this, &ContentBlockingProfileDialog::saveSource);
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
		m_ui->passiveNotificationWidget->setMessage(tr("Any changes made here are going to be lost during manual or automatic update."), Notification::Message::WarningLevel);
	}
}

void ContentBlockingProfileDialog::handleCurrentTabChanged(int index)
{
	if (m_isSourceLoaded || index != 1 || m_name.isEmpty())
	{
		return;
	}

	ContentFiltersProfile *profile(ContentFiltersManager::getProfile(m_name));

	if (profile)
	{
		QFile file(profile->getPath());

		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream stream(&file);
			stream.setCodec("UTF-8");

			m_ui->sourceEditWidget->setPlainText(stream.readAll());
			m_ui->sourceEditWidget->markAsLoaded();

			file.close();

			m_isSourceLoaded = true;

			connect(m_ui->sourceEditWidget, &SourceEditWidget::textChanged, [&]()
			{
				m_ui->saveButton->setEnabled(true);
			});
		}
	}
}

void ContentBlockingProfileDialog::saveSource()
{
	ContentFiltersProfile *profile(ContentFiltersManager::getProfile(m_name));

	if (profile)
	{
		QFile file(profile->getPath());

		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream stream(&file);
			stream.setCodec("UTF-8");
			stream << m_ui->sourceEditWidget->toPlainText();

			file.close();

			m_ui->sourceEditWidget->markAsSaved();
			m_ui->saveButton->setEnabled(false);
		}
	}
}

ContentBlockingProfileDialog::ProfileSummary ContentBlockingProfileDialog::getProfile() const
{
	ProfileSummary profileSummary;
	profileSummary.name = m_name;
	profileSummary.title = m_ui->titleLineEdit->text();
	profileSummary.updateUrl = QUrl(m_ui->updateUrLineEdit->text());
	profileSummary.category = static_cast<ContentFiltersProfile::ProfileCategory>(m_ui->categoryComboBox->itemData(m_ui->categoryComboBox->currentIndex()).toInt());
	profileSummary.updateInterval = m_ui->updateIntervalSpinBox->value();

	return profileSummary;
}

}
