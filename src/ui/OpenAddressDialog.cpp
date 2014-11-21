/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OpenAddressDialog.h"
#include "AddressWidget.h"

#include "ui_OpenAddressDialog.h"

namespace Otter
{

OpenAddressDialog::OpenAddressDialog(QWidget *parent) : QDialog(parent),
	m_addressWidget(NULL),
	m_ui(new Ui::OpenAddressDialog)
{
	m_ui->setupUi(this);

	m_addressWidget = new AddressWidget(NULL, true, this);
	m_addressWidget->setFocus();

	m_ui->verticalLayout->insertWidget(1, m_addressWidget);
	m_ui->label->setBuddy(m_addressWidget);

	connect(m_addressWidget, SIGNAL(requestedOpenUrl(QUrl)), this, SLOT(openUrl(QUrl)));
	connect(m_addressWidget, SIGNAL(requestedSearch(QString,QString)), this, SLOT(openSearch(QString,QString)));
	connect(this, SIGNAL(accepted()), this, SLOT(handleInput()));
}

OpenAddressDialog::~OpenAddressDialog()
{
	delete m_ui;
}

void OpenAddressDialog::handleInput()
{
	if (!m_addressWidget->text().trimmed().isEmpty())
	{
		m_addressWidget->handleUserInput(m_addressWidget->text());
	}
}

void OpenAddressDialog::changeEvent(QEvent *event)
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

void OpenAddressDialog::openUrl(const QUrl &url)
{
	emit requestedLoadUrl(url, NewTabOpen);

	close();
}

void OpenAddressDialog::openSearch(const QString &query, const QString &engine)
{
	emit requestedSearch(query, engine, NewTabOpen);

	close();
}

}
