/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "ContentFiltersContentsWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/Window.h"

#include "ui_ContentFiltersContentsWidget.h"

#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

namespace Otter
{

ContentFiltersContentsWidget::ContentFiltersContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ActiveWindowObserverContentsWidget(parameters, window, parent),
	m_isSettingsPageInitialized(false),
	m_ui(new Ui::ContentFiltersContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->hostComboBox->addItem(tr("Default"));
	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->profilesViewWidget->setFilterRoles({Qt::DisplayRole, ContentFiltersViewWidget::UpdateUrlRole});

	if (isSidebarPanel())
	{
		m_ui->stackedWidget->setCurrentIndex(0);
		m_ui->detailsWidget->hide();
	}
	else
	{
		initializeSettingsPage();
	}
}

ContentFiltersContentsWidget::~ContentFiltersContentsWidget()
{
	delete m_ui;
}

void ContentFiltersContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void ContentFiltersContentsWidget::initializeSettingsPage()
{
	if (m_isSettingsPageInitialized)
	{
		return;
	}

	m_isSettingsPageInitialized = true;

	const QStringList hosts(SettingsManager::getOverrideHosts(SettingsManager::ContentBlocking_ProfilesOption));

	if (!hosts.isEmpty())
	{
		m_ui->hostComboBox->insertSeparator(1);
		m_ui->hostComboBox->addItems(hosts);
	}

	if (isSidebarPanel())
	{
		const MainWindow *mainWindow(MainWindow::findMainWindow(parentWidget()));

		if (mainWindow && mainWindow->getActiveWindow())
		{
			const QString host(Utils::extractHost(mainWindow->getActiveWindow()->getUrl()));

			if (SettingsManager::hasOverride(host, SettingsManager::ContentBlocking_ProfilesOption))
			{
				m_ui->hostComboBox->setCurrentIndex(host.isEmpty() ? 0 : m_ui->hostComboBox->findText(host));
				m_ui->profilesViewWidget->setHost(host);
			}
		}
	}

	QMenu *menu(new QMenu(m_ui->addlButton));
	menu->addAction(tr("New…"), m_ui->profilesViewWidget, &ContentFiltersViewWidget::addProfile);
	menu->addAction(tr("File…"), m_ui->profilesViewWidget, &ContentFiltersViewWidget::importProfileFromFile);
	menu->addAction(tr("URL…"), m_ui->profilesViewWidget, &ContentFiltersViewWidget::importProfileFromUrl);

	m_ui->addlButton->setMenu(menu);

	connect(m_ui->hostComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [&](int index)
	{
		const QString host((index == 0) ? QString() : m_ui->hostComboBox->itemText(index));

		if (m_ui->profilesViewWidget->areProfilesModified() && host != m_ui->profilesViewWidget->getHost())
		{
			const int result(QMessageBox::question(this, tr("Question"), tr("The settings have been changed.\nDo you want to save them?"), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel));

			if (result == QMessageBox::Cancel)
			{
				m_ui->hostComboBox->setCurrentIndex(host.isEmpty() ? 0 : m_ui->hostComboBox->findText(host));

				return;
			}

			if (result == QMessageBox::Yes)
			{
				m_ui->profilesViewWidget->save();
			}
		}

		m_ui->profilesViewWidget->setHost(host);
		m_ui->removeHostButton->setEnabled(index > 0);
	});
	connect(m_ui->addHostButton, &QPushButton::clicked, this, [&]()
	{
		const QString host(QInputDialog::getText(this, tr("Select Website"), tr("Enter website name:")).simplified());

		if (!host.isEmpty())
		{
			if (m_ui->hostComboBox->findText(host) < 0)
			{
				if (m_ui->hostComboBox->count() == 1)
				{
					m_ui->hostComboBox->insertSeparator(1);
				}

				m_ui->hostComboBox->addItem(host);
			}

			m_ui->hostComboBox->setCurrentIndex(host.isEmpty() ? 0 : m_ui->hostComboBox->findText(host));
		}
	});
	connect(m_ui->removeHostButton, &QPushButton::clicked, this, [&]()
	{
		if (m_ui->hostComboBox->currentIndex() > 1)
		{
			SettingsManager::removeOverride(m_ui->hostComboBox->currentText(), SettingsManager::ContentBlocking_ProfilesOption);

			m_ui->hostComboBox->removeItem(m_ui->hostComboBox->currentIndex());

			if (m_ui->hostComboBox->count() == 2)
			{
				m_ui->hostComboBox->removeItem(1);
			}
		}
	});
	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->profilesViewWidget, &ContentFiltersViewWidget::setFilterString);
	connect(m_ui->profilesViewWidget, &ContentFiltersViewWidget::needsActionsUpdate, this, &ContentFiltersContentsWidget::updateActions);
	connect(m_ui->profilesViewWidget, &ContentFiltersViewWidget::areProfilesModifiedChanged, this, &ContentFiltersContentsWidget::setModified);
	connect(m_ui->profilesViewWidget, &ContentFiltersViewWidget::areProfilesModifiedChanged, m_ui->saveButton, &QPushButton::setEnabled);
	connect(m_ui->saveButton, &QPushButton::clicked, m_ui->profilesViewWidget, &ContentFiltersViewWidget::save);
	connect(m_ui->editButton, &QPushButton::clicked, m_ui->profilesViewWidget, &ContentFiltersViewWidget::editProfile);
}

void ContentFiltersContentsWidget::print(QPrinter *printer)
{
	m_ui->profilesViewWidget->render(printer);
}

void ContentFiltersContentsWidget::updateActions()
{
	const QModelIndex index(m_ui->profilesViewWidget->currentIndex().sibling(m_ui->profilesViewWidget->currentIndex().row(), 0));
	const bool isProfile(index.isValid() && index.flags().testFlag(Qt::ItemNeverHasChildren));

	m_ui->editButton->setEnabled(isProfile);

	if (isProfile)
	{
		const int updateInterval(index.sibling(index.row(), 1).data(Qt::DisplayRole).toInt());

		m_ui->titleLabelWidget->setText(index.data(Qt::DisplayRole).toString());
		m_ui->updateUrlLabelWidget->setText(index.data(ContentFiltersViewWidget::UpdateUrlRole).toUrl().toString());
		m_ui->updateIntervalLabelWidget->setText((updateInterval > 0) ? tr("%n day(s)", "", updateInterval) : tr("Never"));
		m_ui->lastUpdateLabelWidget->setText(Utils::formatDateTime(index.data(ContentFiltersViewWidget::UpdateTimeRole).toDateTime()));
	}
	else
	{
		m_ui->titleLabelWidget->clear();
		m_ui->updateUrlLabelWidget->clear();
		m_ui->updateIntervalLabelWidget->clear();
		m_ui->lastUpdateLabelWidget->clear();
	}
}

QString ContentFiltersContentsWidget::getTitle() const
{
	return tr("Content Filters");
}

QLatin1String ContentFiltersContentsWidget::getType() const
{
	return QLatin1String("contentFilters");
}

QUrl ContentFiltersContentsWidget::getUrl() const
{
	return {QLatin1String("about:content-filters")};
}

QIcon ContentFiltersContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("content-blocking"), false);
}

bool ContentFiltersContentsWidget::canClose()
{
	const int result(QMessageBox::question(this, tr("Question"), tr("The settings have been changed.\nDo you want to save them?"), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel));

	if (result == QMessageBox::Cancel)
	{
		return false;
	}

	if (result == QMessageBox::Yes)
	{
		m_ui->profilesViewWidget->save();
	}

	return true;
}

}
