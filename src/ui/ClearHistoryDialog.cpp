#include "ClearHistoryDialog.h"
#include "../core/HistoryManager.h"
#include "../core/NetworkAccessManager.h"
#include "../core/SettingsManager.h"
#include "../core/TransfersManager.h"

#include "ui_ClearHistoryDialog.h"

#include <QtWidgets/QPushButton>

namespace Otter
{

ClearHistoryDialog::ClearHistoryDialog(const QStringList &clearSettings, bool configureMode, QWidget *parent) : QDialog(parent),
	m_configureMode(configureMode),
	m_ui(new Ui::ClearHistoryDialog)
{
	m_ui->setupUi(this);

	QStringList settings = clearSettings;
	settings.removeAll(QString());

	if (settings.isEmpty())
	{
		settings << "browsing" << "cookies" << "forms" << "downloads" << "caches";
	}

	if (m_configureMode)
	{
		m_ui->periodWidget->hide();
	}
	else
	{
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Clear Now"));

		connect(this, SIGNAL(accepted()), this, SLOT(clearHistory()));
	}

	m_ui->clearBrowsingHistoryCheckBox->setChecked(settings.contains("browsing"));
	m_ui->clearCookiesCheckBox->setChecked(settings.contains("cookies"));
	m_ui->clearFormsHistoryCheckBox->setChecked(settings.contains("forms"));
	m_ui->clearDownloadsHistoryCheckBox->setChecked(settings.contains("downloads"));
	m_ui->clearSearchHistoryCheckBox->setChecked(settings.contains("search"));
	m_ui->clearCachesCheckBox->setChecked(settings.contains("caches"));
	m_ui->clearStorageCheckBox->setChecked(settings.contains("storage"));
	m_ui->clearPasswordsCheckBox->setChecked(settings.contains("passwords"));
}

ClearHistoryDialog::~ClearHistoryDialog()
{
	delete m_ui;
}

void ClearHistoryDialog::changeEvent(QEvent *event)
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

void ClearHistoryDialog::clearHistory()
{
	if (m_ui->clearBrowsingHistoryCheckBox->isChecked())
	{
		HistoryManager::clearHistory(m_ui->periodSpinBox->value());
	}

	if (m_ui->clearCookiesCheckBox->isChecked())
	{
		NetworkAccessManager::clearCookies(m_ui->periodSpinBox->value());
	}

	if (m_ui->clearDownloadsHistoryCheckBox->isChecked())
	{
		TransfersManager::clearTransfers(m_ui->periodSpinBox->value());
	}

	if (m_ui->clearCachesCheckBox->isChecked())
	{
		NetworkAccessManager::clearCache(m_ui->periodSpinBox->value());
	}
}

QStringList ClearHistoryDialog::getClearSettings() const
{
	QStringList clearSettings;

	if (m_ui->clearBrowsingHistoryCheckBox->isChecked())
	{
		clearSettings.append("browsing");
	}

	if (m_ui->clearCookiesCheckBox->isChecked())
	{
		clearSettings.append("cookies");
	}

	if (m_ui->clearFormsHistoryCheckBox->isChecked())
	{
		clearSettings.append("forms");
	}

	if (m_ui->clearDownloadsHistoryCheckBox->isChecked())
	{
		clearSettings.append("downloads");
	}

	if (m_ui->clearSearchHistoryCheckBox->isChecked())
	{
		clearSettings.append("search");
	}

	if (m_ui->clearCachesCheckBox->isChecked())
	{
		clearSettings.append("caches");
	}

	if (m_ui->clearStorageCheckBox->isChecked())
	{
		clearSettings.append("storage");
	}

	if (m_ui->clearPasswordsCheckBox->isChecked())
	{
		clearSettings.append("passwords");
	}

	return clearSettings;
}

}
