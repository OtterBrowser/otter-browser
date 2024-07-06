/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "GeneralPreferencesPage.h"
#include "../../../core/Application.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/BookmarksModel.h"
#include "../../../core/PlatformIntegration.h"
#include "../../../core/SessionModel.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/Menu.h"
#include "../../../ui/preferences/AcceptLanguageDialog.h"
#include "../../../../3rdparty/columnresizer/ColumnResizer.h"

#include "ui_GeneralPreferencesPage.h"

namespace Otter
{

GeneralPreferencesPage::GeneralPreferencesPage(QWidget *parent) : CategoryPage(parent),
	m_acceptLanguage(SettingsManager::getOption(SettingsManager::Network_AcceptLanguageOption).toString()),
	m_ui(nullptr)
{
}

GeneralPreferencesPage::~GeneralPreferencesPage()
{
	if (wasLoaded())
	{
		delete m_ui;
	}
}

void GeneralPreferencesPage::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && wasLoaded())
	{
		m_ui->retranslateUi(this);
		m_ui->startupBehaviorComboBox->setItemText(0, tr("Show windows and tabs from the last time"));
		m_ui->startupBehaviorComboBox->setItemText(1, tr("Show startup dialog"));
		m_ui->startupBehaviorComboBox->setItemText(2, tr("Show home page"));
		m_ui->startupBehaviorComboBox->setItemText(3, tr("Show start page"));
		m_ui->startupBehaviorComboBox->setItemText(4, tr("Show empty page"));
	}
}

void GeneralPreferencesPage::load()
{
	if (wasLoaded())
	{
		return;
	}

	m_ui = new Ui::GeneralPreferencesPage();
	m_ui->setupUi(this);

	PlatformIntegration *platformIntegration(Application::getPlatformIntegration());
	const bool canSetAsDefaultBrowser(platformIntegration && platformIntegration->canSetAsDefaultBrowser());

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
	m_ui->downloadsFilePathWidget->setOpenMode(FilePathWidget::ExistingDirectoryMode);
	m_ui->downloadsFilePathWidget->setPath(SettingsManager::getOption(SettingsManager::Paths_DownloadsOption).toString());
	m_ui->alwaysAskCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_AlwaysAskWhereToSaveDownloadOption).toBool());
	m_ui->tabsInsteadOfWindowsCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_OpenLinksInNewTabOption).toBool());
	m_ui->delayTabsLoadingCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Sessions_DeferTabsLoadingOption).toBool());
	m_ui->reuseCurrentTabCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_ReuseCurrentTabOption).toBool());
	m_ui->openNextToActiveheckBox->setChecked(SettingsManager::getOption(SettingsManager::TabBar_OpenNextToActiveOption).toBool());
	m_ui->setDefaultButton->setEnabled(canSetAsDefaultBrowser);

	ColumnResizer *columnResizer(new ColumnResizer(this));
	columnResizer->addWidgetsFromFormLayout(m_ui->startupLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->downloadsLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->tabsLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->languageLayout, QFormLayout::LabelRole);

	if (canSetAsDefaultBrowser)
	{
		connect(m_ui->setDefaultButton, &QPushButton::clicked, platformIntegration, &PlatformIntegration::setAsDefaultBrowser);
	}

	connect(bookmarksMenu, &Menu::triggered, this, [&](QAction *action)
	{
		if (!action)
		{
			return;
		}

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
	});
	connect(m_ui->useCurrentAsHomePageButton, &QPushButton::clicked, this, [&]()
	{
		SessionItem *item(SessionsManager::getModel()->getMainWindowItem(MainWindow::findMainWindow(this)));

		if (item)
		{
			m_ui->homePageLineEditWidget->setText(item->data(SessionModel::UrlRole).toUrl().toString(QUrl::RemovePassword));
		}
	});
	connect(m_ui->restoreHomePageButton, &QPushButton::clicked, this, [&]()
	{
		m_ui->homePageLineEditWidget->setText(SettingsManager::getOptionDefinition(SettingsManager::Browser_HomePageOption).defaultValue.toString());
	});
	connect(m_ui->acceptLanguageButton, &QPushButton::clicked, this, [&]()
	{
		AcceptLanguageDialog dialog(m_acceptLanguage, this);

		if (dialog.exec() == QDialog::Accepted)
		{
			m_acceptLanguage = dialog.getLanguages();

			emit settingsModified();
		}
	});

	markAsLoaded();
}

void GeneralPreferencesPage::save()
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

QString GeneralPreferencesPage::getTitle() const
{
	return tr("General");
}

}
