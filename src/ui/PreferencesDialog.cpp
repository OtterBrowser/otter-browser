#include "PreferencesDialog.h"
#include "../core/FileSystemCompleterModel.h"
#include "../core/SettingsManager.h"
#include "../core/SearchesManager.h"

#include "ui_PreferencesDialog.h"

#include <QtWidgets/QCompleter>
#include <QtWidgets/QFileDialog>

namespace Otter
{

PreferencesDialog::PreferencesDialog(const QString &section, QWidget *parent) : QDialog(parent),
	m_ui(new Ui::PreferencesDialog)
{
	m_ui->setupUi(this);

	if (section == "content")
	{
		m_ui->tabWidget->setCurrentIndex(1);
	}
	else if (section == "privacy")
	{
		m_ui->tabWidget->setCurrentIndex(2);
	}
	else if (section == "search")
	{
		m_ui->tabWidget->setCurrentIndex(3);
	}
	else if (section == "advanced")
	{
		m_ui->tabWidget->setCurrentIndex(4);
	}
	else
	{
		m_ui->tabWidget->setCurrentIndex(0);
	}

	m_ui->downloadsLineEdit->setCompleter(new QCompleter(new FileSystemCompleterModel(this), this));
	m_ui->downloadsLineEdit->setText(SettingsManager::getValue("Paths/Downloads").toString());
	m_ui->alwaysAskCheckBox->setChecked(SettingsManager::getValue("Browser/AlwaysAskWhereToSaveDownload").toBool());
	m_ui->tabsInsteadOfWindowsCheckBox->setChecked(SettingsManager::getValue("Browser/OpenLinksInNewTab").toBool());
	m_ui->openNextToActiveheckBox->setChecked(SettingsManager::getValue("Tabs/OpenNextToActive").toBool());

	m_ui->defaultZoomSpinBox->setValue(SettingsManager::getValue("Browser/DefaultZoom").toInt());
	m_ui->zoomTextOnlyCheckBox->setChecked(SettingsManager::getValue("Browser/ZoomTextOnly").toBool());
	m_ui->proportionalFontSizeSpinBox->setValue(SettingsManager::getValue("Browser/DefaultFontSize").toInt());
	m_ui->fixedFontSizeSpinBox->setValue(SettingsManager::getValue("Browser/DefaultFixedFontSize").toInt());
	m_ui->minimumFontSizeSpinBox->setValue(SettingsManager::getValue("Browser/MinimumFontSize").toInt());

	m_ui->privateModeCheckBox->setChecked(SettingsManager::getValue("Browser/PrivateMode").toBool());
	m_ui->historyWidget->setDisabled(m_ui->privateModeCheckBox->isChecked());
	m_ui->rememberDownloadsHistoryCheckBox->setChecked(SettingsManager::getValue("Browser/RememberDownloads").toBool());
	m_ui->acceptCookiesCheckBox->setChecked(SettingsManager::getValue("Browser/EnableCookies").toBool());

	const QStringList engines = SearchesManager::getEngines();

	for (int i = 0; i < engines.count(); ++i)
	{
		SearchInformation *engine = SearchesManager::getEngine(engines.at(i));

		if (engine)
		{
			QTableWidgetItem *engineItem = new QTableWidgetItem(engine->icon, engine->title);
			engineItem->setToolTip(engine->description);

			const int row = m_ui->searchWidget->rowCount();

			m_ui->searchWidget->setRowCount(row + 1);
			m_ui->searchWidget->setItem(row, 0, engineItem);
			m_ui->searchWidget->setItem(row, 1, new QTableWidgetItem(engine->shortcut));
		}
	}

	m_ui->searchWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(save()));
	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
	connect(m_ui->downloadsBrowseButton, SIGNAL(clicked()), this, SLOT(browseDownloadsPath()));
	connect(m_ui->privateModeCheckBox, SIGNAL(toggled(bool)), m_ui->historyWidget, SLOT(setDisabled(bool)));
	connect(m_ui->searchFilterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterSearch(QString)));
}

PreferencesDialog::~PreferencesDialog()
{
	delete m_ui;
}

void PreferencesDialog::changeEvent(QEvent *event)
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

void PreferencesDialog::browseDownloadsPath()
{
	const QString path = QFileDialog::getExistingDirectory(this, tr("Select Directory"), m_ui->downloadsLineEdit->text());

	if (!path.isEmpty())
	{
		m_ui->downloadsLineEdit->setText(path);
	}
}

void PreferencesDialog::filterSearch(const QString &filter)
{
	for (int i = 0; i < m_ui->searchWidget->rowCount(); ++i)
	{
		m_ui->searchWidget->setRowHidden(i, (!filter.isEmpty() && !m_ui->searchWidget->item(i, 0)->text().contains(filter, Qt::CaseInsensitive) && !m_ui->searchWidget->item(i, 1)->text().contains(filter, Qt::CaseInsensitive)));
	}
}

void PreferencesDialog::save()
{
	SettingsManager::setValue("Paths/Downloads", m_ui->downloadsLineEdit->text());
	SettingsManager::setValue("Browser/AlwaysAskWhereSaveFile", m_ui->alwaysAskCheckBox->isChecked());
	SettingsManager::setValue("Browser/OpenLinksInNewTab", m_ui->tabsInsteadOfWindowsCheckBox->isChecked());
	SettingsManager::setValue("Tabs/OpenNextToActive", m_ui->openNextToActiveheckBox->isChecked());

	SettingsManager::setValue("Browser/DefaultZoom", m_ui->defaultZoomSpinBox->value());
	SettingsManager::setValue("Browser/ZoomTextOnly", m_ui->zoomTextOnlyCheckBox->isChecked());
	SettingsManager::setValue("Browser/DefaultFontSize", m_ui->proportionalFontSizeSpinBox->value());
	SettingsManager::setValue("Browser/DefaultFixedFontSize", m_ui->fixedFontSizeSpinBox->value());
	SettingsManager::setValue("Browser/MinimumFontSize", m_ui->minimumFontSizeSpinBox->value());

	SettingsManager::setValue("Browser/PrivateMode", m_ui->privateModeCheckBox->isChecked());
	SettingsManager::setValue("Browser/RememberDownloads", m_ui->rememberDownloadsHistoryCheckBox->isChecked());
	SettingsManager::setValue("Browser/EnableCookies", m_ui->acceptCookiesCheckBox->isChecked());

	close();
}

}
