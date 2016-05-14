/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "ImportDialog.h"
#include "../modules/importers/html/HtmlBookmarksImporter.h"
#include "../modules/importers/opera/OperaBookmarksImporter.h"
#include "../modules/importers/opera/OperaNotesImporter.h"
#include "../modules/importers/opera/OperaSearchEnginesImporter.h"
#include "../modules/importers/opera/OperaSessionImporter.h"

#include "ui_ImportDialog.h"

#include <QtWidgets/QMessageBox>

namespace Otter
{

ImportDialog::ImportDialog(Importer *importer, QWidget *parent) : Dialog(parent),
	m_importer(importer),
	m_ui(new Ui::ImportDialog)
{
	m_ui->setupUi(this);
	m_ui->importPathWidget->setFilter(importer->getFileFilter());
	m_ui->importPathWidget->setPath(importer->getSuggestedPath());

	m_importer->setParent(this);

	QWidget *widget(m_importer->getOptionsWidget());

	if (widget)
	{
		m_ui->optionBox->addWidget(widget);
	}

	setWindowTitle(m_importer->getTitle());

	connect(m_ui->importPathWidget, SIGNAL(pathChanged(QString)), this, SLOT(setPath(QString)));
	connect(this, SIGNAL(accepted()), this, SLOT(import()));
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
	}
}

void ImportDialog::createDialog(const QString &importerName, QWidget *parent)
{
	Importer *importer = NULL;

	if (importerName == QLatin1String("OperaBookmarks"))
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
	else if (importerName == QLatin1String("HtmlBookmarks"))
	{
		importer = new HtmlBookmarksImporter();
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

void ImportDialog::import()
{
	if (!m_importer->import(m_path))
	{
		QMessageBox::critical(this, tr("Error"), tr("Failed to import selected type."));
	}
}

}
