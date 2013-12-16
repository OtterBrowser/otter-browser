#include "PreferencesDialog.h"
#include "../core/SettingsManager.h"
#include "../core/SearchesManager.h"

#include "ui_PreferencesDialog.h"

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

	m_ui->tabsInsteadOfWindowsCheckBox->setChecked(SettingsManager::getValue("Browser/OpenLinksInNewTab").toBool());
	m_ui->defaultZoomSpinBox->setValue(SettingsManager::getValue("Browser/DefaultZoom").toInt());
	m_ui->zoomTextOnlyCheckBox->setChecked(SettingsManager::getValue("Browser/ZoomTextOnly").toBool());

	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(save()));
	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
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

void PreferencesDialog::save()
{
	SettingsManager::setValue("Browser/OpenLinksInNewTab", m_ui->tabsInsteadOfWindowsCheckBox->isChecked());
	SettingsManager::setValue("Browser/DefaultZoom", m_ui->defaultZoomSpinBox->value());
	SettingsManager::setValue("Browser/ZoomTextOnly", m_ui->zoomTextOnlyCheckBox->isChecked());

	close();
}

}
