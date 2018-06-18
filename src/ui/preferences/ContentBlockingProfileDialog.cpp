/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2016 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../core/AdblockContentFiltersProfile.h"
#include "../../core/ContentFiltersManager.h"
#include "../../core/SessionsManager.h"
#include "../../core/Utils.h"

#include "ui_ContentBlockingProfileDialog.h"

#include <QtCore/QDir>
#include <QtWidgets/QMessageBox>

namespace Otter
{

ContentBlockingProfileDialog::ContentBlockingProfileDialog(QWidget *parent, ContentFiltersProfile *profile) : Dialog(parent),
	m_profile(profile),
	m_ui(new Ui::ContentBlockingProfileDialog)
{
	m_ui->setupUi(this);
	m_ui->categoryComboBox->addItem(tr("Advertisements"), ContentFiltersProfile::AdvertisementsCategory);
	m_ui->categoryComboBox->addItem(tr("Annoyance"), ContentFiltersProfile::AnnoyanceCategory);
	m_ui->categoryComboBox->addItem(tr("Privacy"), ContentFiltersProfile::PrivacyCategory);
	m_ui->categoryComboBox->addItem(tr("Social"), ContentFiltersProfile::SocialCategory);
	m_ui->categoryComboBox->addItem(tr("Regional"), ContentFiltersProfile::RegionalCategory);
	m_ui->categoryComboBox->addItem(tr("Other"), ContentFiltersProfile::OtherCategory);

	if (profile)
	{
		m_ui->titleLineEdit->setText(profile->getTitle());
		m_ui->updateUrLineEdit->setText(profile->getUpdateUrl().url());
		m_ui->categoryComboBox->setCurrentIndex(m_ui->categoryComboBox->findData(profile->getCategory()));
		m_ui->lastUpdateTextLabel->setText(Utils::formatDateTime(profile->getLastUpdate()));
		m_ui->updateIntervalSpinBox->setValue(profile->getUpdateInterval());
	}

	connect(m_ui->confirmButtonBox, &QDialogButtonBox::accepted, this, &ContentBlockingProfileDialog::save);
	connect(m_ui->confirmButtonBox, &QDialogButtonBox::rejected, this, &ContentBlockingProfileDialog::close);
}

ContentBlockingProfileDialog::~ContentBlockingProfileDialog()
{
	delete m_ui;
}

ContentFiltersProfile* ContentBlockingProfileDialog::getProfile()
{
	return m_profile;
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

void ContentBlockingProfileDialog::save()
{
	const ContentFiltersProfile::ProfileCategory category(static_cast<ContentFiltersProfile::ProfileCategory>(m_ui->categoryComboBox->itemData(m_ui->categoryComboBox->currentIndex()).toInt()));
	const QUrl url(m_ui->updateUrLineEdit->text());

	if (!url.isValid())
	{
		QMessageBox::critical(this, tr("Error"), tr("Valid update URL is required."), QMessageBox::Close);

		return;
	}

	if (m_profile)
	{
		m_profile->setCategory(category);
		m_profile->setTitle(m_ui->titleLineEdit->text());
		m_profile->setUpdateUrl(url);
		m_profile->setUpdateInterval(m_ui->updateIntervalSpinBox->value());
	}
	else
	{
		QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking")));

		const QString fileName(QFileInfo(url.fileName()).completeBaseName());
		QFile file(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking/%1.txt")).arg(fileName));

		if (file.exists())
		{
			QMessageBox::critical(this, tr("Error"), tr("Profile with name %1.txt already exists.").arg(fileName), QMessageBox::Close);

			return;
		}

		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QMessageBox::critical(this, tr("Error"), tr("Failed to create profile file: %1.").arg(file.errorString()), QMessageBox::Close);

			return;
		}

		file.write(QStringLiteral("[AdBlock Plus 2.0]\n").toUtf8());
		file.close();

		ContentFiltersProfile *profile(new AdblockContentFiltersProfile(fileName, m_ui->titleLineEdit->text(), url, {}, {}, m_ui->updateIntervalSpinBox->value(), category, (ContentFiltersProfile::HasCustomTitleFlag | ContentFiltersProfile::HasCustomUpdateUrlFlag), ContentFiltersManager::getInstance()));

		ContentFiltersManager::addProfile(profile);

		m_profile = profile;
	}

	accept();
}

}
