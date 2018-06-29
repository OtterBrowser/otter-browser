/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "PreferencesGeneralPageWidget.h"
#include "AcceptLanguageDialog.h"
#include "../MainWindow.h"
#include "../Menu.h"
#include "../../../3rdparty/columnresizer/ColumnResizer.h"
#include "../../core/Application.h"
#include "../../core/BookmarksManager.h"
#include "../../core/BookmarksModel.h"
#include "../../core/PlatformIntegration.h"
#include "../../core/SessionModel.h"
#include "../../core/SessionsManager.h"
#include "../../core/SettingsManager.h"

#include "ui_PreferencesGeneralPageWidget.h"

namespace Otter
{

PreferencesGeneralPageWidget::PreferencesGeneralPageWidget(QWidget *parent) : QWidget(parent),
	m_acceptLanguage(SettingsManager::getOption(SettingsManager::Network_AcceptLanguageOption).toString()),
	m_ui(new Ui::PreferencesGeneralPageWidget)
{
	m_ui->setupUi(this);
	m_ui->startupBehaviorComboBox->addItem(tr("Show windows and tabs from the last time"), QLatin1String("continuePrevious"));
	m_ui->startupBehaviorComboBox->addItem(tr("Show startup dialog"), QLatin1String("showDialog"));
	m_ui->startupBehaviorComboBox->addItem(tr("Show home page"), QLatin1String("startHomePage"));
	m_ui->startupBehaviorComboBox->addItem(tr("Show start page"), QLatin1String("startStartPage"));
	m_ui->startupBehaviorComboBox->addItem(tr("Show empty page"), QLatin1String("startEmpty"));

	const int startupBehaviorIndex(m_ui->startupBehaviorComboBox->findData(SettingsManager::getOption(SettingsManager::Browser_StartupBehaviorOption).toString()));

	m_ui->startupBehaviorComboBox->setCurrentIndex((startupBehaviorIndex < 0) ? 0 : startupBehaviorIndex);
	m_ui->homePageLineEditWidget->setText(SettingsManager::getOption(SettingsManager::Browser_HomePageOption).toString());

	Menu *bookmarksMenu(new Menu(Menu::BookmarkSelectorMenu, m_ui->useBookmarkAsHomePageButton));

	m_ui->useBookmarkAsHomePageButton->setMenu(bookmarksMenu);
	m_ui->useBookmarkAsHomePageButton->setEnabled(BookmarksManager::getModel()->getRootItem()->rowCount() > 0);
	m_ui->downloadsFilePathWidget->setSelectFile(false);
	m_ui->downloadsFilePathWidget->setPath(SettingsManager::getOption(SettingsManager::Paths_DownloadsOption).toString());
	m_ui->alwaysAskCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_AlwaysAskWhereToSaveDownloadOption).toBool());
	m_ui->tabsInsteadOfWindowsCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_OpenLinksInNewTabOption).toBool());
	m_ui->delayTabsLoadingCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Sessions_DeferTabsLoadingOption).toBool());
	m_ui->reuseCurrentTabCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_ReuseCurrentTabOption).toBool());
	m_ui->openNextToActiveheckBox->setChecked(SettingsManager::getOption(SettingsManager::TabBar_OpenNextToActiveOption).toBool());

	PlatformIntegration *platformIntegration(Application::getPlatformIntegration());

	if (!platformIntegration || !platformIntegration->canSetAsDefaultBrowser())
	{
		m_ui->setDefaultButton->setEnabled(false);
	}
	else
	{
		connect(m_ui->setDefaultButton, &QPushButton::clicked, platformIntegration, &PlatformIntegration::setAsDefaultBrowser);
	}

	ColumnResizer *columnResizer(new ColumnResizer(this));
	columnResizer->addWidgetsFromFormLayout(m_ui->startupLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->downloadsLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->tabsLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->languageLayout, QFormLayout::LabelRole);

	connect(bookmarksMenu, &Menu::triggered, this, &PreferencesGeneralPageWidget::useBookmarkAsHomePage);
	connect(m_ui->useCurrentAsHomePageButton, &QPushButton::clicked, this, &PreferencesGeneralPageWidget::useCurrentAsHomePage);
	connect(m_ui->restoreHomePageButton, &QPushButton::clicked, this, &PreferencesGeneralPageWidget::restoreHomePage);
	connect(m_ui->acceptLanguageButton, &QPushButton::clicked, this, &PreferencesGeneralPageWidget::setupAcceptLanguage);
}

PreferencesGeneralPageWidget::~PreferencesGeneralPageWidget()
{
	delete m_ui;
}

void PreferencesGeneralPageWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void PreferencesGeneralPageWidget::useCurrentAsHomePage()
{
	SessionItem *item(SessionsManager::getModel()->getMainWindowItem(MainWindow::findMainWindow(this)));

	if (item)
	{
		m_ui->homePageLineEditWidget->setText(item->data(SessionModel::UrlRole).toUrl().toString(QUrl::RemovePassword));
	}
}

void PreferencesGeneralPageWidget::useBookmarkAsHomePage(QAction *action)
{
	if (action)
	{
		const BookmarksModel::Bookmark *bookmark(BookmarksManager::getModel()->getBookmark(action->data().toULongLong()));
		const QString url(bookmark ? bookmark->getUrl().toDisplayString() : QString());

		if (url.isEmpty())
		{
			m_ui->homePageLineEditWidget->setText(QLatin1String("bookmarks:") + QString::number(action->data().toULongLong()));
		}
		else
		{
			m_ui->homePageLineEditWidget->setText(url);
		}
	}
}

void PreferencesGeneralPageWidget::restoreHomePage()
{
	m_ui->homePageLineEditWidget->setText(SettingsManager::getOptionDefinition(SettingsManager::Browser_HomePageOption).defaultValue.toString());
}

void PreferencesGeneralPageWidget::setupAcceptLanguage()
{
	AcceptLanguageDialog dialog(m_acceptLanguage, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		m_acceptLanguage = dialog.getLanguages();

		emit settingsModified();
	}
}

void PreferencesGeneralPageWidget::save()
{
	SettingsManager::setOption(SettingsManager::Browser_StartupBehaviorOption, m_ui->startupBehaviorComboBox->currentData().toString());
	SettingsManager::setOption(SettingsManager::Browser_HomePageOption, m_ui->homePageLineEditWidget->text());
	SettingsManager::setOption(SettingsManager::Paths_DownloadsOption, m_ui->downloadsFilePathWidget->getPath());
	SettingsManager::setOption(SettingsManager::Browser_AlwaysAskWhereToSaveDownloadOption, m_ui->alwaysAskCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Browser_OpenLinksInNewTabOption, m_ui->tabsInsteadOfWindowsCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Sessions_DeferTabsLoadingOption, m_ui->delayTabsLoadingCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Browser_ReuseCurrentTabOption, m_ui->reuseCurrentTabCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::TabBar_OpenNextToActiveOption, m_ui->openNextToActiveheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Network_AcceptLanguageOption, m_acceptLanguage);
}

}
