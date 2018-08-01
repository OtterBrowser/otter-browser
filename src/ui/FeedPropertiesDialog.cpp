/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "FeedPropertiesDialog.h"
#include "../core/FeedsManager.h"

#include "ui_FeedPropertiesDialog.h"

#include <QtWidgets/QMessageBox>

namespace Otter
{

FeedPropertiesDialog::FeedPropertiesDialog(Feed *feed, FeedsModel::Entry *folder, QWidget *parent) : Dialog(parent),
	m_feed(feed),
	m_ui(new Ui::FeedPropertiesDialog)
{
	m_ui->setupUi(this);

	if (feed)
	{
		m_ui->titleLineEditWidget->setText(feed->getTitle());
		m_ui->iconButton->setIcon(feed->getIcon());
		m_ui->urlLineEditWidget->setText(feed->getUrl().toString());

		if (feed->getUpdateInterval() > 0 || FeedsManager::getModel()->hasFeed(feed->getUrl()))
		{
			m_ui->updateIntervalSpinBox->setValue(feed->getUpdateInterval());
		}
	}
	else
	{
		setWindowTitle(tr("Add Feed"));
	}

	m_ui->folderComboBox->setCurrentFolder(folder);
	m_ui->iconButton->setDefaultIcon(QLatin1String("application-rss+xml"));

	connect(m_ui->newFolderButton, &QPushButton::clicked, m_ui->folderComboBox, &FeedsComboBoxWidget::createFolder);
	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &FeedPropertiesDialog::saveFeed);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &FeedPropertiesDialog::close);
}

FeedPropertiesDialog::~FeedPropertiesDialog()
{
	delete m_ui;
}

void FeedPropertiesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void FeedPropertiesDialog::saveFeed()
{
	if (m_ui->urlLineEditWidget->text().isEmpty() || !QUrl(m_ui->urlLineEditWidget->text()).isValid())
	{
		QMessageBox::critical(this, tr("Error"), tr("Valid address is required."), QMessageBox::Close);

		return;
	}

	if (m_feed)
	{
		m_feed->setTitle(m_ui->titleLineEditWidget->text());
		m_feed->setUrl(QUrl(m_ui->urlLineEditWidget->text()));
		m_feed->setIcon(m_ui->iconButton->icon());
		m_feed->setUpdateInterval(m_ui->updateIntervalSpinBox->value());
	}
	else
	{
		m_feed = FeedsManager::createFeed(QUrl(m_ui->urlLineEditWidget->text()), m_ui->titleLineEditWidget->text(), m_ui->iconButton->icon(), m_ui->updateIntervalSpinBox->value());
	}

	accept();
}

Feed* FeedPropertiesDialog::getFeed() const
{
	return m_feed;
}

FeedsModel::Entry* FeedPropertiesDialog::getFolder() const
{
	return m_ui->folderComboBox->getCurrentFolder();
}

}
