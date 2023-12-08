/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2021 - 2023 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "WebsitesPreferencesPage.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../ui/WebsitePreferencesDialog.h"

#include "ui_WebsitesPreferencesPage.h"

#include <QtWidgets/QMessageBox>

namespace Otter
{

WebsitesPreferencesPage::WebsitesPreferencesPage(QWidget *parent) : CategoryPage(parent),
	m_ui(nullptr)
{
}

WebsitesPreferencesPage::~WebsitesPreferencesPage()
{
	if (wasLoaded())
	{
		delete m_ui;
	}
}

void WebsitesPreferencesPage::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && wasLoaded())
	{
		m_ui->retranslateUi(this);
	}
}

void WebsitesPreferencesPage::addWebsite()
{
	WebsitePreferencesDialog dialog({}, {}, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	const QString host(dialog.getHost());

	if (host.isEmpty())
	{
		return;
	}

	const QModelIndexList indexes(m_ui->websitesItemView->getSourceModel()->match(m_ui->websitesItemView->getSourceModel()->index(0, 0), Qt::DisplayRole, host));

	if (indexes.isEmpty())
	{
		QStandardItem *item(new QStandardItem(HistoryManager::getIcon(host), host));
		item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

		m_ui->websitesItemView->insertRow({item});
		m_ui->websitesItemView->sortByColumn(0, Qt::AscendingOrder);
	}
}

void WebsitesPreferencesPage::editWebsite()
{
	WebsitePreferencesDialog dialog(m_ui->websitesItemView->getCurrentIndex().data(Qt::DisplayRole).toString(), {}, this);
	dialog.exec();
}

void WebsitesPreferencesPage::removeWebsite()
{
	const QString host(m_ui->websitesItemView->getCurrentIndex().data(Qt::DisplayRole).toString());

	if (!host.isEmpty() && QMessageBox::question(this, tr("Question"), tr("Do you really want to remove preferences for this website?"), (QMessageBox::Ok | QMessageBox::Cancel)) == QMessageBox::Ok)
	{
		SettingsManager::removeOverride(host);

		m_ui->websitesItemView->removeRow();
	}
}

void WebsitesPreferencesPage::updateWebsiteActions()
{
	const QModelIndex index(m_ui->websitesItemView->getCurrentIndex());

	m_ui->websitesEditButton->setEnabled(index.isValid());
	m_ui->websitesRemoveButton->setEnabled(index.isValid());
}

void WebsitesPreferencesPage::load()
{
	if (wasLoaded())
	{
		return;
	}

	m_ui = new Ui::WebsitesPreferencesPage;
	m_ui->setupUi(this);

	QStandardItemModel *overridesModel(new QStandardItemModel(this));
	const QStringList overrideHosts(SettingsManager::getOverrideHosts());

	for (int i = 0; i < overrideHosts.count(); ++i)
	{
		QStandardItem *item(new QStandardItem(HistoryManager::getIcon(overrideHosts.at(i)), overrideHosts.at(i)));
		item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

		overridesModel->appendRow(item);
	}

	m_ui->websitesFilterLineEditWidget->setClearOnEscape(true);
	m_ui->websitesItemView->setModel(overridesModel);

	connect(m_ui->websitesFilterLineEditWidget, &LineEditWidget::textChanged, m_ui->websitesItemView, &ItemViewWidget::setFilterString);
	connect(m_ui->websitesItemView, &ItemViewWidget::needsActionsUpdate, this, &WebsitesPreferencesPage::updateWebsiteActions);
	connect(m_ui->websitesAddButton, &QPushButton::clicked, this, &WebsitesPreferencesPage::addWebsite);
	connect(m_ui->websitesEditButton, &QPushButton::clicked, this, &WebsitesPreferencesPage::editWebsite);
	connect(m_ui->websitesRemoveButton, &QPushButton::clicked, this, &WebsitesPreferencesPage::removeWebsite);

	markAsLoaded();
}

QString WebsitesPreferencesPage::getTitle() const
{
	return tr("Websites");
}

}
