/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
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

#include "ReportDialog.h"
#include "../core/Application.h"

#include "ui_ReportDialog.h"

#include <QtCore/QRegularExpression>
#include <QtGui/QClipboard>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QPushButton>

namespace Otter
{

ReportDialog::ReportDialog(Application::ReportOptions options, QWidget *parent) : Dialog(parent),
	m_ui(new Ui::ReportDialog)
{
	m_ui->setupUi(this);
	m_ui->reportLabel->setText(QLatin1String("<div style=\"white-space:pre;\">") + Application::createReport(options).trimmed() + QLatin1String("</div>"));
	m_ui->reportLabel->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	connect(this, &ReportDialog::finished, this, &ReportDialog::deleteLater);
	connect(m_ui->buttonBox->addButton(tr("Copy"), QDialogButtonBox::ActionRole), &QPushButton::clicked, this, &ReportDialog::copyReport);
}

ReportDialog::~ReportDialog()
{
	delete m_ui;
}

void ReportDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void ReportDialog::copyReport()
{
	QGuiApplication::clipboard()->setText(m_ui->reportLabel->text().remove(QRegularExpression(QLatin1String("<[^>]*>"))));
}

}
