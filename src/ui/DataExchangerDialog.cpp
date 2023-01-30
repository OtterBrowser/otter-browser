/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2023 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "DataExchangerDialog.h"
#include "../core/ThemesManager.h"
#include "../core/Utils.h"
#include "../modules/exporters/html/HtmlBookmarksExportDataExchanger.h"
#include "../modules/exporters/xbel/XbelBookmarksExportDataExchanger.h"
#include "../modules/importers/html/HtmlBookmarksImportDataExchanger.h"
#include "../modules/importers/opera/OperaBookmarksImportDataExchanger.h"
#include "../modules/importers/opera/OperaNotesImportDataExchanger.h"
#include "../modules/importers/opera/OperaSearchEnginesImportDataExchanger.h"
#include "../modules/importers/opera/OperaSessionImportDataExchanger.h"
#include "../modules/importers/opml/OpmlImportDataExchanger.h"

#include "ui_DataExchangerDialog.h"

#include <QtGui/QCloseEvent>
#include <QtWidgets/QMessageBox>

namespace Otter
{

DataExchangerDialog::DataExchangerDialog(ExportDataExchanger *exporter, QWidget *parent) : Dialog(parent),
	m_exporter(exporter),
	m_importer(nullptr),
	m_ui(new Ui::DataExchangerDialog)
{
	m_ui->setupUi(this);
	m_ui->stackedWidget->setCurrentWidget(m_ui->exportOptionsPage);
	m_ui->exportPathWidget->setFilters(exporter->getFileFilters());
	m_ui->exportPathWidget->setPath(exporter->getSuggestedPath(Utils::getStandardLocation(QStandardPaths::HomeLocation)));
	m_ui->exportPathWidget->setOpenMode(FilePathWidget::NewFileMode);

	exporter->setParent(this);

	if (exporter->hasOptions())
	{
		m_ui->extraExportOptionsLayout->addWidget(exporter->createOptionsWidget(this));
	}

	setWindowTitle(exporter->getTitle());
	setObjectName(exporter->metaObject()->className());
	adjustSize();

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, [&]()
	{
		const QString path(m_ui->exportPathWidget->getPath());
		const bool isManuallySpecified(m_ui->exportPathWidget->isManuallySpecified());
		const bool exists(QFile::exists(path));
		bool canExport(isManuallySpecified || !exists);

		if (exists && !isManuallySpecified)
		{
			canExport = QMessageBox::warning(this, tr("Warning"), tr("%1 already exists.\nDo you want to replace it?").arg(QFileInfo(path).fileName()), (QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes;
		}

		if (canExport)
		{
			setupResults(m_exporter);

			m_exporter->exportData(path, true);
		}
	});
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &DataExchangerDialog::reject);
}

DataExchangerDialog::DataExchangerDialog(ImportDataExchanger *importer, QWidget *parent) : Dialog(parent),
	m_exporter(nullptr),
	m_importer(importer),
	m_ui(new Ui::DataExchangerDialog)
{
	m_ui->setupUi(this);
	m_ui->stackedWidget->setCurrentWidget(m_ui->importOptionsPage);
	m_ui->importPathWidget->setFilters(importer->getFileFilters());
	m_ui->importPathWidget->setPath(importer->getSuggestedPath());
	m_ui->importPathWidget->setOpenMode(FilePathWidget::ExistingFileMode);

	importer->setParent(this);

	if (importer->hasOptions())
	{
		m_ui->extraImportOptionsLayout->addWidget(importer->createOptionsWidget(this));
	}

	setWindowTitle(importer->getTitle());
	setObjectName(importer->metaObject()->className());
	adjustSize();

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, [&]()
	{
		setupResults(m_importer);

		m_importer->importData(m_ui->importPathWidget->getPath());
	});
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &DataExchangerDialog::reject);
}

void DataExchangerDialog::closeEvent(QCloseEvent *event)
{
	if (m_ui->buttonBox->button(QDialogButtonBox::Abort))
	{
		if (m_exporter)
		{
			m_exporter->cancel();
		}
		else if (m_importer)
		{
			m_importer->cancel();
		}
	}

	event->accept();
}

DataExchangerDialog::~DataExchangerDialog()
{
	delete m_ui;
}

void DataExchangerDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		setWindowTitle(m_importer->getTitle());
	}
}

void DataExchangerDialog::createDialog(const QString &exchangerName, QWidget *parent)
{
	ExportDataExchanger *exportExchanger(nullptr);
	ImportDataExchanger *importExchanger(nullptr);

	if (exchangerName == QLatin1String("HtmlBookmarksExport"))
	{
		exportExchanger = new HtmlBookmarksExportDataExchanger();
	}
	else if (exchangerName == QLatin1String("XbelBookmarksExport"))
	{
		exportExchanger = new XbelBookmarksExportDataExchanger();
	}
	else if (exchangerName == QLatin1String("HtmlBookmarksImport"))
	{
		importExchanger = new HtmlBookmarksImportDataExchanger();
	}
	else if (exchangerName == QLatin1String("OperaBookmarksImport"))
	{
		importExchanger = new OperaBookmarksImportDataExchanger();
	}
	else if (exchangerName == QLatin1String("OperaNotesImport"))
	{
		importExchanger = new OperaNotesImportDataExchanger();
	}
	else if (exchangerName == QLatin1String("OperaSearchEnginesImport"))
	{
		importExchanger = new OperaSearchEnginesImportDataExchanger();
	}
	else if (exchangerName == QLatin1String("OperaSessionImport"))
	{
		importExchanger = new OperaSessionImportDataExchanger();
	}
	else if (exchangerName == QLatin1String("OpmlFeedsImport"))
	{
		importExchanger = new OpmlImportDataExchanger();
	}

	if (exportExchanger)
	{
		DataExchangerDialog dialog(exportExchanger, parent);
		dialog.exec();
	}
	else if (importExchanger)
	{
		DataExchangerDialog dialog(importExchanger, parent);
		dialog.exec();
	}
	else
	{
		QMessageBox::critical(parent, tr("Error"), tr("Unable to find handler for selected type."));
	}
}

void DataExchangerDialog::handleExchangeStarted(DataExchanger::ExchangeType type, int total)
{
	handleExchangeProgress(type, total, 0);

	m_ui->messageTextLabel->setText(tr("Processing…"));
}

void DataExchangerDialog::handleExchangeProgress(DataExchanger::ExchangeType type, int total, int amount)
{
	Q_UNUSED(type)

	if (total > 0)
	{
		m_ui->progressBar->setRange(0, total);
		m_ui->progressBar->setValue(amount);
	}
	else
	{
		m_ui->progressBar->setRange(0, 0);
		m_ui->progressBar->setValue(-1);
	}
}

void DataExchangerDialog::handleExchangeFinished(DataExchanger::ExchangeType type, DataExchanger::OperationResult result, int total)
{
	handleExchangeProgress(type, total, total);

	m_ui->messageIconLabel->setPixmap(ThemesManager::createIcon((result == DataExchanger::SuccessfullOperation) ? QLatin1String("task-complete") : QLatin1String("task-reject")).pixmap(32, 32));
	m_ui->buttonBox->clear();
	m_ui->buttonBox->addButton(QDialogButtonBox::Close);
	m_ui->buttonBox->setEnabled(true);

	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &DataExchangerDialog::close);
}

void DataExchangerDialog::handleExportFinished(DataExchanger::ExchangeType type, DataExchanger::OperationResult result, int total)
{
	handleExchangeFinished(type, result, total);

	switch (result)
	{
		case DataExchanger::FailedOperation:
			m_ui->messageTextLabel->setText(tr("Failed to export data."));

			break;
		case DataExchanger::CancelledOperation:
			m_ui->messageTextLabel->setText(tr("Export cancelled by the user."));

			break;
		default:
			m_ui->messageTextLabel->setText(tr("Export finished successfully."));

			break;
	}

	disconnect(m_ui->buttonBox, &QDialogButtonBox::rejected, m_exporter, &DataExchanger::cancel);
}

void DataExchangerDialog::handleImportFinished(DataExchanger::ExchangeType type, DataExchanger::OperationResult result, int total)
{
	handleExchangeFinished(type, result, total);

	switch (result)
	{
		case DataExchanger::FailedOperation:
			m_ui->messageTextLabel->setText(tr("Failed to import data."));

			break;
		case DataExchanger::CancelledOperation:
			m_ui->messageTextLabel->setText(tr("Import cancelled by the user."));

			break;
		default:
			m_ui->messageTextLabel->setText(tr("Import finished successfully."));

			break;
	}

	disconnect(m_ui->buttonBox, &QDialogButtonBox::rejected, m_importer, &DataExchanger::cancel);
}

void DataExchangerDialog::setupResults(DataExchanger *exchanger)
{
	m_ui->messageLayout->setDirection(isLeftToRight() ? QBoxLayout::LeftToRight : QBoxLayout::RightToLeft);
	m_ui->messageIconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("task-ongoing")).pixmap(32, 32));
	m_ui->buttonBox->clear();
	m_ui->buttonBox->addButton(QDialogButtonBox::Abort)->setEnabled(exchanger->canCancel());
	m_ui->stackedWidget->setCurrentWidget(m_ui->resultsPage);

	if (exchanger->getExchangeDirection() == DataExchanger::ImportDirection)
	{
		connect(exchanger, &DataExchanger::exchangeFinished, this, &DataExchangerDialog::handleImportFinished);
	}
	else
	{
		connect(exchanger, &DataExchanger::exchangeFinished, this, &DataExchangerDialog::handleExportFinished);
	}

	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, exchanger, &DataExchanger::cancel);
	connect(exchanger, &DataExchanger::exchangeStarted, this, &DataExchangerDialog::handleExchangeStarted);
	connect(exchanger, &DataExchanger::exchangeProgress, this, &DataExchangerDialog::handleExchangeProgress);
	disconnect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &DataExchangerDialog::reject);
}

}
