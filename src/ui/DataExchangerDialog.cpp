/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2021 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

	exporter->setParent(this);

	if (exporter->hasOptions())
	{
		m_ui->extraImportOptionsLayout->addWidget(exporter->createOptionsWidget(this));
	}

	setWindowTitle(exporter->getTitle());
	setObjectName(exporter->metaObject()->className());
	adjustSize();
}

DataExchangerDialog::DataExchangerDialog(ImportDataExchanger *importer, QWidget *parent) : Dialog(parent),
	m_exporter(nullptr),
	m_importer(importer),
	m_ui(new Ui::DataExchangerDialog)
{
	m_ui->setupUi(this);
	m_ui->importPathWidget->setFilters(importer->getFileFilters());
	m_ui->importPathWidget->setPath(importer->getSuggestedPath());

	importer->setParent(this);

	if (importer->hasOptions())
	{
		m_ui->extraImportOptionsLayout->addWidget(importer->createOptionsWidget(this));
	}

	setWindowTitle(importer->getTitle());
	setObjectName(importer->metaObject()->className());
	adjustSize();

	connect(m_ui->importPathWidget, &FilePathWidget::pathChanged, this, [&](const QString &path)
	{
		m_path = path;
	});
	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &DataExchangerDialog::handleImportRequested);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &DataExchangerDialog::reject);
}

void DataExchangerDialog::closeEvent(QCloseEvent *event)
{
	if (m_ui->buttonBox->button(QDialogButtonBox::Abort))
	{
		m_importer->cancel();
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

	if (exchangerName == QLatin1String("XbelBookmarksExport"))
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
		QMessageBox::critical(parent, tr("Error"), (exchangerName.contains(QLatin1String("Import")) ? tr("Unable to import selected type.") : tr("Unable to export selected type.")));
	}
}

void DataExchangerDialog::handleImportRequested()
{
	m_ui->messageLayout->setDirection(isLeftToRight() ? QBoxLayout::LeftToRight : QBoxLayout::RightToLeft);
	m_ui->messageIconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("task-ongoing")).pixmap(32, 32));
	m_ui->buttonBox->clear();
	m_ui->buttonBox->addButton(QDialogButtonBox::Abort)->setEnabled(m_importer->canCancel());
	m_ui->stackedWidget->setCurrentIndex(1);

	disconnect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &DataExchangerDialog::reject);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, m_importer, &DataExchanger::cancel);
	connect(m_importer, &ImportDataExchanger::importStarted, this, &DataExchangerDialog::handleImportStarted);
	connect(m_importer, &ImportDataExchanger::importProgress, this, &DataExchangerDialog::handleImportProgress);
	connect(m_importer, &ImportDataExchanger::importFinished, this, &DataExchangerDialog::handleImportFinished);

	m_importer->importData(m_path);
}

void DataExchangerDialog::handleImportStarted(DataExchanger::ExchangeType type, int total)
{
	Q_UNUSED(type)

	handleImportProgress(type, total, 0);

	m_ui->messageTextLabel->setText(tr("Processing…"));
}

void DataExchangerDialog::handleImportProgress(DataExchanger::ExchangeType type, int total, int amount)
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

void DataExchangerDialog::handleImportFinished(DataExchanger::ExchangeType type, DataExchanger::OperationResult result, int total)
{
	handleImportProgress(type, total, total);

	m_ui->messageIconLabel->setPixmap(ThemesManager::createIcon((result == DataExchanger::SuccessfullOperation) ? QLatin1String("task-complete") : QLatin1String("task-reject")).pixmap(32, 32));

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

	m_ui->buttonBox->clear();
	m_ui->buttonBox->addButton(QDialogButtonBox::Close);
	m_ui->buttonBox->setEnabled(true);

	disconnect(m_ui->buttonBox, &QDialogButtonBox::rejected, m_importer, &DataExchanger::cancel);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &DataExchangerDialog::close);
}

}
