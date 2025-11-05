/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../core/AdblockContentFiltersProfile.h"
#include "../core/Console.h"
#include "../core/Job.h"
#include "../core/Utils.h"
#include "../../3rdparty/columnresizer/ColumnResizer.h"

#include "ui_ContentBlockingProfileDialog.h"

#include <QtCore/QTemporaryFile>
#include <QtWidgets/QMessageBox>

namespace Otter
{

ContentBlockingProfileDialog::ContentBlockingProfileDialog(const ContentFiltersProfile::ProfileSummary &profileSummary, const QString &rulesPath, QWidget *parent) : Dialog(parent),
	m_dataFetchJob(nullptr),
	m_name(profileSummary.name),
	m_rulesPath(rulesPath),
	m_lastUpdate(profileSummary.lastUpdate),
	m_isSourceLoaded(false),
	m_isUsingTemporaryFile(false),
	m_ui(new Ui::ContentBlockingProfileDialog)
{
	m_ui->setupUi(this);
	m_ui->titleLineEdit->setText(profileSummary.title);
	m_ui->categoryComboBox->addItem(tr("Advertisements"), ContentFiltersProfile::AdvertisementsCategory);
	m_ui->categoryComboBox->addItem(tr("Annoyance"), ContentFiltersProfile::AnnoyanceCategory);
	m_ui->categoryComboBox->addItem(tr("Privacy"), ContentFiltersProfile::PrivacyCategory);
	m_ui->categoryComboBox->addItem(tr("Social"), ContentFiltersProfile::SocialCategory);
	m_ui->categoryComboBox->addItem(tr("Regional"), ContentFiltersProfile::RegionalCategory);
	m_ui->categoryComboBox->addItem(tr("Other"), ContentFiltersProfile::OtherCategory);
	m_ui->categoryComboBox->setCurrentIndex(m_ui->categoryComboBox->findData(profileSummary.category));
	m_ui->cosmeticFiltersComboBox->addItem(tr("All"), ContentFiltersManager::AllFilters);
	m_ui->cosmeticFiltersComboBox->addItem(tr("Domain specific only"), ContentFiltersManager::DomainOnlyFilters);
	m_ui->cosmeticFiltersComboBox->addItem(tr("None"), ContentFiltersManager::NoFilters);
	m_ui->cosmeticFiltersComboBox->setCurrentIndex(m_ui->cosmeticFiltersComboBox->findData(profileSummary.cosmeticFiltersMode));
	m_ui->enableWildcardsCheckBox->setChecked(profileSummary.areWildcardsEnabled);
	m_ui->updateUrLineEdit->setText(profileSummary.updateUrl.toString());
	m_ui->updateIntervalSpinBox->setValue(profileSummary.updateInterval);
	m_ui->progressBar->hide();
	m_ui->passiveNotificationWidget->setMessage(tr("Any changes made here are going to be lost during manual or automatic update."), Notification::Message::WarningLevel);
	m_ui->sourceEditWidget->setSyntax(SyntaxHighlighter::AdblockPlusSyntax);
	m_ui->saveButton->setIcon(ThemesManager::createIcon(QLatin1String("document-save")));

	if (profileSummary.lastUpdate.isValid())
	{
		m_ui->lastUpdateTextLabel->setText(Utils::formatDateTime(profileSummary.lastUpdate));
	}

	if (!profileSummary.name.isEmpty())
	{
		ContentFiltersProfile *profile(ContentFiltersManager::getProfile(profileSummary.name));

		if (profile)
		{
			m_ui->updateButton->setEnabled(profileSummary.updateUrl.isValid());

			connect(profile, &ContentFiltersProfile::profileModified, profile, [=]()
			{
				m_lastUpdate = profile->getLastUpdate();

				m_ui->lastUpdateTextLabel->setText(Utils::formatDateTime(m_lastUpdate));
			});
			connect(m_ui->updateUrLineEdit, &QLineEdit::textChanged, this, [&]()
			{
				m_ui->updateButton->setEnabled(QUrl(m_ui->updateUrLineEdit->text()).isValid());
			});
			connect(m_ui->updateButton, &QPushButton::clicked, profile, [=]()
			{
				const QUrl url(m_ui->updateUrLineEdit->text());

				if (url.isValid())
				{
					profile->update(url);
				}
			});
		}
	}
	else if (profileSummary.updateUrl.isValid())
	{
		m_ui->tabWidget->setTabEnabled(1, false);
		m_ui->titleLineEdit->setReadOnly(true);
		m_ui->updateUrLineEdit->setReadOnly(true);
		m_ui->lastUpdateLabel->hide();
		m_ui->lastUpdateTextLabel->hide();
		m_ui->updateButton->hide();
		m_ui->progressBar->show();
		m_ui->confirmButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

		m_dataFetchJob = new DataFetchJob(profileSummary.updateUrl, this);

		connect(m_dataFetchJob, &DataFetchJob::progressChanged, this, [&](int progress)
		{
			if (m_ui->progressBar->maximum() == 0)
			{
				m_ui->progressBar->setMaximum(100);
			}

			m_ui->progressBar->setValue(progress);
		});
		connect(m_dataFetchJob, &Job::jobFinished, this, &ContentBlockingProfileDialog::handleJobFinished);

		m_dataFetchJob->start();
	}

	ColumnResizer *columnResizer(new ColumnResizer(this));
	columnResizer->addWidgetsFromFormLayout(m_ui->settingsTopLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->settingsBottomLayout, QFormLayout::LabelRole);

	connect(m_ui->sourceEditWidget, &SourceEditWidget::textChanged, this, [&]()
	{
		m_ui->saveButton->setEnabled(true);
	});
	connect(m_ui->saveButton, &QPushButton::clicked, this, &ContentBlockingProfileDialog::saveSource);
	connect(m_ui->tabWidget, &QTabWidget::currentChanged, this, &ContentBlockingProfileDialog::handleCurrentTabChanged);
	connect(m_ui->confirmButtonBox, &QDialogButtonBox::accepted, this, &ContentBlockingProfileDialog::markAsAccepted);
}

ContentBlockingProfileDialog::~ContentBlockingProfileDialog()
{
	delete m_ui;

	if (m_dataFetchJob)
	{
		m_dataFetchJob->cancel();
	}

	if (m_isUsingTemporaryFile && !isAccepted() && !m_rulesPath.isEmpty())
	{
		QFile::remove(m_rulesPath);
	}
}

void ContentBlockingProfileDialog::closeEvent(QCloseEvent *event)
{
	if (m_ui->saveButton->isEnabled())
	{
		const int result(QMessageBox::question(this, tr("Question"), tr("The source has been modified.\nDo you want to save it?"), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel));

		if (result == QMessageBox::Cancel)
		{
			event->ignore();

			return;
		}

		if (result == QMessageBox::Yes)
		{
			saveSource();
		}
	}

	event->accept();
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
		m_ui->cosmeticFiltersComboBox->setItemText(0, tr("All"));
		m_ui->cosmeticFiltersComboBox->setItemText(1, tr("Domain specific only"));
		m_ui->cosmeticFiltersComboBox->setItemText(2, tr("None"));
		m_ui->passiveNotificationWidget->setMessage(tr("Any changes made here are going to be lost during manual or automatic update."), Notification::Message::WarningLevel);
	}
}

void ContentBlockingProfileDialog::handleCurrentTabChanged(int index)
{
	if (m_isSourceLoaded || index != 1)
	{
		return;
	}

	if (m_rulesPath.isEmpty())
	{
		m_ui->sourceEditWidget->setPlainText(QLatin1String("[AdBlock Plus 2.0]\n"));
		m_ui->sourceEditWidget->markAsLoaded();
		m_ui->saveButton->setEnabled(false);

		m_isSourceLoaded = true;

		return;
	}

	QFile file(m_rulesPath);

	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream stream(&file);

		m_ui->sourceEditWidget->setPlainText(stream.readAll());
		m_ui->sourceEditWidget->markAsLoaded();
		m_ui->saveButton->setEnabled(false);

		file.close();

		m_isSourceLoaded = true;
	}
	else
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to load content blocking profile: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		QMessageBox::critical(this, tr("Error"), tr("Failed to load profile file."), QMessageBox::Close);
	}
}

void ContentBlockingProfileDialog::handleJobFinished(bool isSuccess)
{
	QIODevice *device(m_dataFetchJob->getData());

	m_dataFetchJob = nullptr;

	if (!isSuccess || !device)
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to download content blocking profile."), Console::OtherCategory, Console::ErrorLevel);

		QMessageBox::critical(this, tr("Error"), tr("Failed to download profile file."), QMessageBox::Close);

		return;
	}

	const QString path(createTemporaryFile());

	if (path.isEmpty())
	{
		return;
	}

	QFile file(path);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to save content blocking profile: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		QMessageBox::critical(this, tr("Error"), tr("Failed to save profile file."), QMessageBox::Close);

		return;
	}

	file.write(device->readAll());
	file.close();

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to open content blocking profile: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		QMessageBox::critical(this, tr("Error"), tr("Failed to open content blocking profile file:\n%1").arg(file.errorString()));

		return;
	}

	const AdblockContentFiltersProfile::HeaderInformation information(AdblockContentFiltersProfile::loadHeader(&file));

	file.close();

	if (information.hasError())
	{
		Console::addMessage(information.errorString, Console::OtherCategory, Console::ErrorLevel, file.fileName());

		QMessageBox::critical(this, tr("Error"), information.errorString);

		return;
	}

	m_lastUpdate = QDateTime::currentDateTime();

	m_ui->tabWidget->setTabEnabled(1, true);
	m_ui->titleLineEdit->setReadOnly(false);
	m_ui->titleLineEdit->setText(information.title);
	m_ui->updateUrLineEdit->setReadOnly(false);
	m_ui->lastUpdateLabel->show();
	m_ui->lastUpdateTextLabel->setText(Utils::formatDateTime(m_lastUpdate));
	m_ui->lastUpdateTextLabel->show();
	m_ui->updateButton->show();
	m_ui->progressBar->hide();
	m_ui->confirmButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

	m_rulesPath = path;
}

void ContentBlockingProfileDialog::saveSource()
{
	if (m_rulesPath.isEmpty())
	{
		m_rulesPath = createTemporaryFile();
	}

	QFile file(m_rulesPath);

	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);
		stream << m_ui->sourceEditWidget->toPlainText();

		file.close();

		m_ui->sourceEditWidget->markAsSaved();
		m_ui->saveButton->setEnabled(false);
	}
	else
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to save content blocking profile: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		QMessageBox::critical(this, tr("Error"), tr("Failed to save profile file."), QMessageBox::Close);
	}
}

QString ContentBlockingProfileDialog::createTemporaryFile()
{
	QTemporaryFile file;
	file.setAutoRemove(false);

	if (!file.open())
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to save content blocking profile: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		QMessageBox::critical(this, tr("Error"), tr("Failed to save profile file."), QMessageBox::Close);

		return {};
	}

	const QString path(file.fileName());

	file.close();

	m_isUsingTemporaryFile = true;

	return path;
}

QString ContentBlockingProfileDialog::getRulesPath() const
{
	return m_rulesPath;
}

ContentFiltersProfile::ProfileSummary ContentBlockingProfileDialog::getProfile() const
{
	ContentFiltersProfile::ProfileSummary profileSummary;
	profileSummary.name = m_name;
	profileSummary.title = m_ui->titleLineEdit->text();
	profileSummary.lastUpdate = m_lastUpdate;
	profileSummary.updateUrl = QUrl(m_ui->updateUrLineEdit->text());
	profileSummary.category = static_cast<ContentFiltersProfile::ProfileCategory>(m_ui->categoryComboBox->currentData().toInt());
	profileSummary.cosmeticFiltersMode = static_cast<ContentFiltersManager::CosmeticFiltersMode>(m_ui->cosmeticFiltersComboBox->currentData().toInt());
	profileSummary.updateInterval = m_ui->updateIntervalSpinBox->value();
	profileSummary.areWildcardsEnabled = m_ui->enableWildcardsCheckBox->isChecked();

	return profileSummary;
}

}
