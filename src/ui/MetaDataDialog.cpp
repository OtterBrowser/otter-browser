/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2021 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "MetaDataDialog.h"

#include "ui_MetaDataDialog.h"

namespace Otter
{

MetaDataDialog::MetaDataDialog(const Addon::MetaData &metaData, QWidget *parent) : Dialog(parent),
	m_isModified(false),
	m_ui(new Ui::MetaDataDialog)
{
	m_ui->setupUi(this);
	m_ui->titleLineEditWidget->setText(metaData.title);
	m_ui->descriptionLineEditWidget->setText(metaData.description);
	m_ui->versionLineEditWidget->setText(metaData.version);
	m_ui->authorLineEditWidget->setText(metaData.author);
	m_ui->homepageLineEditWidget->setText(metaData.homePage.toString());

	const QList<QLineEdit*> lineEdits(findChildren<QLineEdit*>());

	for (int i = 0; i < lineEdits.count(); ++i)
	{
		connect(lineEdits.at(i), &QLineEdit::textChanged, this, [&]()
		{
			m_isModified = true;
		});
	}
}

MetaDataDialog::~MetaDataDialog()
{
	delete m_ui;
}

void MetaDataDialog::changeEvent(QEvent *event)
{
	Dialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

Addon::MetaData MetaDataDialog::getMetaData() const
{
	Addon::MetaData metaData;
	metaData.title = m_ui->titleLineEditWidget->text();
	metaData.description = m_ui->descriptionLineEditWidget->text();
	metaData.version = m_ui->versionLineEditWidget->text();
	metaData.author = m_ui->authorLineEditWidget->text();
	metaData.homePage = QUrl(m_ui->homepageLineEditWidget->text());

	return metaData;
}

bool MetaDataDialog::isModified() const
{
	return m_isModified;
}

}
