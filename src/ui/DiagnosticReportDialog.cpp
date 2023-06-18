/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
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

#include "DiagnosticReportDialog.h"
#include "../core/Application.h"

#include "ui_DiagnosticReportDialog.h"

#include <QtCore/QRegularExpression>
#include <QtGui/QClipboard>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QPushButton>

namespace Otter
{

DiagnosticReportDialog::DiagnosticReportDialog(Application::ReportOptions options, QWidget *parent) : Dialog(parent),
	m_copyButton(nullptr),
	m_ui(new Ui::DiagnosticReportDialog)
{
	m_ui->setupUi(this);
	m_ui->reportLabel->setText(QLatin1String("<div style=\"white-space:pre;\">") + Application::createReport(options).trimmed() + QLatin1String("</div>"));
	m_ui->reportLabel->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	m_copyButton = m_ui->buttonBox->addButton(tr("Copy"), QDialogButtonBox::ActionRole);

	connect(this, &DiagnosticReportDialog::finished, this, &DiagnosticReportDialog::deleteLater);
	connect(m_copyButton, &QPushButton::clicked, this, [&]()
	{
		QGuiApplication::clipboard()->setText(m_ui->reportLabel->text().remove(QRegularExpression(QLatin1String("<[^>]*>"))));
	});
}

DiagnosticReportDialog::~DiagnosticReportDialog()
{
	delete m_ui;
}

void DiagnosticReportDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		m_copyButton->setText(tr("Copy"));
	}
}

}
