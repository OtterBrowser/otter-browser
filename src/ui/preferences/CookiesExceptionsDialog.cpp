/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "CookiesExceptionsDialog.h"

#include "ui_CookiesExceptionsDialog.h"

namespace Otter
{

CookiesExceptionsDialog::CookiesExceptionsDialog(const QStringList &acceptedHosts, const QStringList &rejectedHosts, QWidget *parent) : Dialog(parent),
	m_ui(new Ui::CookiesExceptionsDialog)
{
	m_ui->setupUi(this);

	QStandardItemModel *acceptedHostsModel(new QStandardItemModel(this));

	for (int i = 0; i < acceptedHosts.count(); ++i)
	{
		if (!acceptedHosts.at(i).isEmpty())
		{
			acceptedHostsModel->appendRow(new QStandardItem(acceptedHosts.at(i)));
		}
	}

	m_ui->acceptedHostsItemView->setModel(acceptedHostsModel);

	QStandardItemModel *rejectedHostsModel(new QStandardItemModel(this));

	for (int i = 0; i < rejectedHosts.count(); ++i)
	{
		if (!rejectedHosts.at(i).isEmpty())
		{
			rejectedHostsModel->appendRow(new QStandardItem(rejectedHosts.at(i)));
		}
	}

	m_ui->rejectedHostsItemView->setModel(rejectedHostsModel);

	updateAcceptedHostsActions();
	updateRejectedHostsActions();

	connect(m_ui->acceptedHostsItemView, &ItemViewWidget::needsActionsUpdate, this, &CookiesExceptionsDialog::updateAcceptedHostsActions);
	connect(m_ui->addAcceptedHostsButton, &QPushButton::clicked, this, &CookiesExceptionsDialog::addAcceptedHost);
	connect(m_ui->editAcceptedHostsButton, &QPushButton::clicked, this, &CookiesExceptionsDialog::editAcceptedHost);
	connect(m_ui->removeAcceptedHostsButton, &QPushButton::clicked, this, &CookiesExceptionsDialog::removeAcceptedHost);
	connect(m_ui->rejectedHostsItemView, &ItemViewWidget::needsActionsUpdate, this, &CookiesExceptionsDialog::updateRejectedHostsActions);
	connect(m_ui->addRejectedHostsButton, &QPushButton::clicked, this, &CookiesExceptionsDialog::addRejectedHost);
	connect(m_ui->editRejectedHostsButton, &QPushButton::clicked, this, &CookiesExceptionsDialog::editRejectedHost);
	connect(m_ui->removeRejectedHostsButton, &QPushButton::clicked, this, &CookiesExceptionsDialog::removeRejectedHost);
}

CookiesExceptionsDialog::~CookiesExceptionsDialog()
{
	delete m_ui;
}

void CookiesExceptionsDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void CookiesExceptionsDialog::addAcceptedHost()
{
	m_ui->acceptedHostsItemView->insertRow();

	editAcceptedHost();
}

void CookiesExceptionsDialog::addRejectedHost()
{
	m_ui->rejectedHostsItemView->insertRow();

	editRejectedHost();
}

void CookiesExceptionsDialog::editAcceptedHost()
{
	m_ui->acceptedHostsItemView->edit(m_ui->acceptedHostsItemView->getIndex(m_ui->acceptedHostsItemView->getCurrentRow()));
}

void CookiesExceptionsDialog::editRejectedHost()
{
	m_ui->rejectedHostsItemView->edit(m_ui->rejectedHostsItemView->getIndex(m_ui->rejectedHostsItemView->getCurrentRow()));
}

void CookiesExceptionsDialog::removeAcceptedHost()
{
	m_ui->acceptedHostsItemView->removeRow();
	m_ui->acceptedHostsItemView->setFocus();

	updateAcceptedHostsActions();
}

void CookiesExceptionsDialog::removeRejectedHost()
{
	m_ui->rejectedHostsItemView->removeRow();
	m_ui->rejectedHostsItemView->setFocus();

	updateRejectedHostsActions();
}

void CookiesExceptionsDialog::updateAcceptedHostsActions()
{
	const bool isEditable(m_ui->acceptedHostsItemView->getCurrentRow() >= 0);

	m_ui->editAcceptedHostsButton->setEnabled(isEditable);
	m_ui->removeAcceptedHostsButton->setEnabled(isEditable);
}

void CookiesExceptionsDialog::updateRejectedHostsActions()
{
	const bool isEditable(m_ui->rejectedHostsItemView->getCurrentRow() >= 0);

	m_ui->editRejectedHostsButton->setEnabled(isEditable);
	m_ui->removeRejectedHostsButton->setEnabled(isEditable);
}

QStringList CookiesExceptionsDialog::getAcceptedHosts() const
{
	QStringList entries;

	for (int i = 0; i < m_ui->acceptedHostsItemView->model()->rowCount(); ++i)
	{
		const QString entry(m_ui->acceptedHostsItemView->getIndex(i).data(Qt::DisplayRole).toString());

		if (!entry.isEmpty() && !entries.contains(entry))
		{
			entries.append(entry);
		}
	}

	return entries;
}

QStringList CookiesExceptionsDialog::getRejectedHosts() const
{
	QStringList entries;

	for (int i = 0; i < m_ui->rejectedHostsItemView->model()->rowCount(); ++i)
	{
		const QString entry(m_ui->rejectedHostsItemView->getIndex(i).data(Qt::DisplayRole).toString());

		if (!entry.isEmpty() && !entries.contains(entry))
		{
			entries.append(entry);
		}
	}

	return entries;
}

}
