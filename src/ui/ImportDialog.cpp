/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ImportDialog.h"
#include "../core/ThemesManager.h"
#include "../modules/importers/html/HtmlBookmarksImporter.h"
#include "../modules/importers/opera/OperaBookmarksImporter.h"
#include "../modules/importers/opera/OperaNotesImporter.h"
#include "../modules/importers/opera/OperaSearchEnginesImporter.h"
#include "../modules/importers/opera/OperaSessionImporter.h"
#include "../modules/importers/opml/OpmlImporter.h"

#include "ui_ImportDialog.h"

#include <QtGui/QCloseEvent>
#include <QtWidgets/QMessageBox>

namespace Otter
{

ImportDialog::ImportDialog(Importer *importer, QWidget *parent) : Dialog(parent),
	m_importer(importer),
	m_ui(new Ui::ImportDialog)
{
	m_ui->setupUi(this);
	m_ui->importPathWidget->setFilters(importer->getFileFilters());
	m_ui->importPathWidget->setPath(importer->getSuggestedPath());

	m_importer->setParent(this);

	QWidget *widget(m_importer->createOptionsWidget(this));

	if (widget)
	{
		m_ui->extraOptionsLayout->addWidget(widget);
	}

	setWindowTitle(m_importer->getTitle());

	connect(m_ui->importPathWidget, &FilePathWidget::pathChanged, this, &ImportDialog::setPath);
	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &ImportDialog::handleImportRequested);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &ImportDialog::reject);
}

void ImportDialog::closeEvent(QCloseEvent *event)
{
	if (m_ui->buttonBox->button(QDialogButtonBox::Abort))
	{
		m_importer->cancel();
	}

	event->accept();
}

ImportDialog::~ImportDialog()
{
	delete m_ui;
}

void ImportDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		setWindowTitle(m_importer->getTitle());
	}
}

void ImportDialog::createDialog(const QString &importerName, QWidget *parent)
{
	Importer *importer(nullptr);

	if (importerName == QLatin1String("HtmlBookmarks"))
	{
		importer = new HtmlBookmarksImporter();
	}
	else if (importerName == QLatin1String("OperaBookmarks"))
	{
		importer = new OperaBookmarksImporter();
	}
	else if (importerName == QLatin1String("OperaNotes"))
	{
		importer = new OperaNotesImporter();
	}
	else if (importerName == QLatin1String("OperaSearchEngines"))
	{
		importer = new OperaSearchEnginesImporter();
	}
	else if (importerName == QLatin1String("OperaSession"))
	{
		importer = new OperaSessionImporter();
	}
	else if (importerName == QLatin1String("OpmlFeeds"))
	{
		importer = new OpmlImporter();
	}

	if (importer)
	{
		ImportDialog dialog(importer, parent);
		dialog.exec();
	}
	else
	{
		QMessageBox::critical(parent, tr("Error"), tr("Unable to import selected type."));
	}
}

void ImportDialog::setPath(const QString &path)
{
	m_path = path;
}

void ImportDialog::handleImportRequested()
{
	m_ui->messageLayout->setDirection(isLeftToRight() ? QBoxLayout::LeftToRight : QBoxLayout::RightToLeft);
	m_ui->messageIconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("task-ongoing")).pixmap(32, 32));
	m_ui->buttonBox->clear();
	m_ui->buttonBox->addButton(QDialogButtonBox::Abort);
	m_ui->buttonBox->setEnabled(m_importer->canCancel());
	m_ui->stackedWidget->setCurrentIndex(1);

	disconnect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &ImportDialog::reject);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, m_importer, &Importer::cancel);
	connect(m_importer, &Importer::importStarted, this, &ImportDialog::handleImportStarted);
	connect(m_importer, &Importer::importProgress, this, &ImportDialog::handleImportProgress);
	connect(m_importer, &Importer::importFinished, this, &ImportDialog::handleImportFinished);

	m_importer->import(m_path);
}

void ImportDialog::handleImportStarted(Importer::ImportType type, int total)
{
	Q_UNUSED(type)

	handleImportProgress(type, total, 0);

	m_ui->messageTextLabel->setText(tr("Processing…"));
}

void ImportDialog::handleImportProgress(Importer::ImportType type, int total, int amount)
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

void ImportDialog::handleImportFinished(Importer::ImportType type, Importer::ImportResult result, int total)
{
	handleImportProgress(type, total, total);

	m_ui->messageIconLabel->setPixmap(ThemesManager::createIcon((result == Importer::SuccessfullImport) ? QLatin1String("task-complete") : QLatin1String("task-reject")).pixmap(32, 32));

	switch (result)
	{
		case Importer::FailedImport:
			m_ui->messageTextLabel->setText(tr("Failed to import data."));

			break;
		case Importer::CancelledImport:
			m_ui->messageTextLabel->setText(tr("Import cancelled by the user."));

			break;
		default:
			m_ui->messageTextLabel->setText(tr("Import finished successfully."));

			break;
	}

	m_ui->buttonBox->clear();
	m_ui->buttonBox->addButton(QDialogButtonBox::Close);
	m_ui->buttonBox->setEnabled(true);

	disconnect(m_ui->buttonBox, &QDialogButtonBox::rejected, m_importer, &Importer::cancel);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &ImportDialog::close);
}

}
