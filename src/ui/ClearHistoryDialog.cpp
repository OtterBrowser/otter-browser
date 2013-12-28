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
		settings << QLatin1String("browsing") << QLatin1String("cookies") << QLatin1String("forms") << QLatin1String("downloads") << QLatin1String("caches");
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

	m_ui->clearBrowsingHistoryCheckBox->setChecked(settings.contains(QLatin1String("browsing")));
	m_ui->clearCookiesCheckBox->setChecked(settings.contains(QLatin1String("cookies")));
	m_ui->clearFormsHistoryCheckBox->setChecked(settings.contains(QLatin1String("forms")));
	m_ui->clearDownloadsHistoryCheckBox->setChecked(settings.contains(QLatin1String("downloads")));
	m_ui->clearSearchHistoryCheckBox->setChecked(settings.contains(QLatin1String("search")));
	m_ui->clearCachesCheckBox->setChecked(settings.contains(QLatin1String("caches")));
	m_ui->clearStorageCheckBox->setChecked(settings.contains(QLatin1String("storage")));
	m_ui->clearPasswordsCheckBox->setChecked(settings.contains(QLatin1String("passwords")));
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
		clearSettings.append(QLatin1String("browsing"));
	}

	if (m_ui->clearCookiesCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("cookies"));
	}

	if (m_ui->clearFormsHistoryCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("forms"));
	}

	if (m_ui->clearDownloadsHistoryCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("downloads"));
	}

	if (m_ui->clearSearchHistoryCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("search"));
	}

	if (m_ui->clearCachesCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("caches"));
	}

	if (m_ui->clearStorageCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("storage"));
	}

	if (m_ui->clearPasswordsCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("passwords"));
	}

	return clearSettings;
}

}
