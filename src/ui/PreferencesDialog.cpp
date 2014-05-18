/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "PreferencesDialog.h"
#include "ClearHistoryDialog.h"
#include "OptionDelegate.h"
#include "OptionWidget.h"
#include "SearchPropertiesDialog.h"
#include "UserAgentsManagerDialog.h"
#include "preferences/SearchShortcutDelegate.h"
#include "preferences/ShortcutsProfileDialog.h"
#include "../core/ActionsManager.h"
#include "../core/NetworkAccessManager.h"
#include "../core/SettingsManager.h"
#include "../core/SearchesManager.h"
#include "../core/SessionsManager.h"
#include "../core/Utils.h"
#include "../core/WindowsManager.h"

#include "ui_PreferencesDialog.h"

#include <QtCore/QSettings>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QSslCipher>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>

namespace Otter
{

PreferencesDialog::PreferencesDialog(const QLatin1String &section, QWidget *parent) : QDialog(parent),
	m_defaultSearch(SettingsManager::getValue("Search/DefaultSearchEngine").toString()),
	m_clearSettings(SettingsManager::getValue(QLatin1String("History/ClearOnClose")).toStringList()),
	m_userAgentsModified(false),
	m_ui(new Ui::PreferencesDialog)
{
	m_ui->setupUi(this);

	m_clearSettings.removeAll(QString());

	if (section == QLatin1String("content"))
	{
		m_ui->tabWidget->setCurrentIndex(1);
	}
	else if (section == QLatin1String("privacy"))
	{
		m_ui->tabWidget->setCurrentIndex(2);
	}
	else if (section == QLatin1String("search"))
	{
		m_ui->tabWidget->setCurrentIndex(3);
	}
	else if (section == QLatin1String("advanced"))
	{
		m_ui->tabWidget->setCurrentIndex(4);
	}
	else
	{
		m_ui->tabWidget->setCurrentIndex(0);
	}

	const QString startupBehaviorString = SettingsManager::getValue(QLatin1String("Browser/StartupBehavior")).toString();
	int startupBehaviorIndex = 0;

	if (startupBehaviorString == QLatin1String("showDialog"))
	{
		startupBehaviorIndex = 1;
	}
	else if (startupBehaviorString == QLatin1String("startHomePage"))
	{
		startupBehaviorIndex = 2;
	}
	else if (startupBehaviorString == QLatin1String("startEmpty"))
	{
		startupBehaviorIndex = 3;
	}

	m_ui->startupBehaviorComboBox->setCurrentIndex(startupBehaviorIndex);
	m_ui->homePageLineEdit->setText(SettingsManager::getValue(QLatin1String("Browser/StartPage")).toString());
	m_ui->downloadsFilePathWidget->setSelectFile(false);
	m_ui->downloadsFilePathWidget->setPath(SettingsManager::getValue(QLatin1String("Paths/Downloads")).toString());
	m_ui->alwaysAskCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/AlwaysAskWhereToSaveDownload")).toBool());
	m_ui->tabsInsteadOfWindowsCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/OpenLinksInNewTab")).toBool());
	m_ui->delayTabsLoadingCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/DelayRestoringOfBackgroundTabs")).toBool());
	m_ui->reuseCurrentTabCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool());
	m_ui->openNextToActiveheckBox->setChecked(SettingsManager::getValue(QLatin1String("TabBar/OpenNextToActive")).toBool());

	m_ui->defaultZoomSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/DefaultZoom")).toInt());
	m_ui->zoomTextOnlyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Content/ZoomTextOnly")).toBool());
	m_ui->proportionalFontSizeSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/DefaultFontSize")).toInt());
	m_ui->fixedFontSizeSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/DefaultFixedFontSize")).toInt());
	m_ui->minimumFontSizeSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/MinimumFontSize")).toInt());

	QList<QLatin1String> fonts;
	fonts << QLatin1String("StandardFont") << QLatin1String("FixedFont") << QLatin1String("SerifFont") << QLatin1String("SansSerifFont") << QLatin1String("CursiveFont") << QLatin1String("FantasyFont");

	QStringList fontCategories;
	fontCategories << tr("Standard font") << tr("Fixed-width font") << tr("Serif font") << tr("Sans-serif font") << tr("Cursive font") << tr("Fantasy font");

	OptionDelegate *fontsDelegate = new OptionDelegate(true, this);

	m_ui->fontsWidget->setRowCount(fonts.count());
	m_ui->fontsWidget->setItemDelegateForColumn(1, fontsDelegate);

	for (int i = 0; i < fonts.count(); ++i)
	{
		const QString family = SettingsManager::getValue(QLatin1String("Content/") + fonts.at(i)).toString();
		QTableWidgetItem *familyItem = new QTableWidgetItem(family);
		familyItem->setData(Qt::UserRole, QLatin1String("Content/") + fonts.at(i));
		familyItem->setData((Qt::UserRole + 1), QLatin1String("font"));

		QTableWidgetItem *previewItem = new QTableWidgetItem(tr("The quick brown fox jumps over the lazy dog"));
		previewItem->setFont(QFont(family));

		m_ui->fontsWidget->setItem(i, 0, new QTableWidgetItem(fontCategories.at(i)));
		m_ui->fontsWidget->setItem(i, 1, familyItem);
		m_ui->fontsWidget->setItem(i, 2, previewItem);
	}

	QList<QLatin1String> colors;
	colors << QLatin1String("BackgroundColor") << QLatin1String("TextColor") << QLatin1String("LinkColor") << QLatin1String("VisitedLinkColor");

	QStringList colorTypes;
	colorTypes << tr("Background Color") << tr("Text Color") << tr("Link Color") << tr("Visited Link Color");

	OptionDelegate *colorsDelegate = new OptionDelegate(true, this);

	m_ui->colorsWidget->setRowCount(colors.count());
	m_ui->colorsWidget->setItemDelegateForColumn(1, colorsDelegate);

	for (int i = 0; i < colors.count(); ++i)
	{
		const QString color = SettingsManager::getValue(QLatin1String("Content/") + colors.at(i)).toString();
		QTableWidgetItem *previewItem = new QTableWidgetItem(color);
		previewItem->setBackgroundColor(QColor(color));
		previewItem->setTextColor(Qt::transparent);
		previewItem->setData(Qt::UserRole, QLatin1String("Content/") + colors.at(i));
		previewItem->setData((Qt::UserRole + 1), QLatin1String("color"));

		m_ui->colorsWidget->setItem(i, 0, new QTableWidgetItem(colorTypes.at(i)));
		m_ui->colorsWidget->setItem(i, 1, previewItem);
	}

	const QString doNotTrackPolicyString = SettingsManager::getValue(QLatin1String("Network/DoNotTrackPolicy")).toString();
	int doNotTrackPolicyIndex = 2;

	if (doNotTrackPolicyString == QLatin1String("allow"))
	{
		doNotTrackPolicyIndex = 1;
	}
	else if (doNotTrackPolicyString == QLatin1String("doNotAllow"))
	{
		doNotTrackPolicyIndex = 0;
	}

	m_ui->doNotTrackComboBox->setCurrentIndex(doNotTrackPolicyIndex);
	m_ui->privateModeCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool());
	m_ui->historyWidget->setDisabled(m_ui->privateModeCheckBox->isChecked());
	m_ui->rememberBrowsingHistoryCheckBox->setChecked(SettingsManager::getValue(QLatin1String("History/RememberBrowsing")).toBool());
	m_ui->rememberDownloadsHistoryCheckBox->setChecked(SettingsManager::getValue(QLatin1String("History/RememberDownloads")).toBool());
	m_ui->acceptCookiesCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/EnableCookies")).toBool());
	m_ui->clearHistoryCheckBox->setChecked(!m_clearSettings.isEmpty());
	m_ui->clearHistoryButton->setEnabled(!m_clearSettings.isEmpty());

	QStringList searchEnginesLabels;
	searchEnginesLabels << tr("Name") << tr("Shortcut");

	QStandardItemModel *searchEnginesModel = new QStandardItemModel(this);
	searchEnginesModel->setHorizontalHeaderLabels(searchEnginesLabels);

	const QStringList searchEngines = SearchesManager::getSearchEngines();

	for (int i = 0; i < searchEngines.count(); ++i)
	{
		SearchInformation *engine = SearchesManager::getSearchEngine(searchEngines.at(i));

		if (!engine)
		{
			continue;
		}

		QVariantHash engineData;
		engineData[QLatin1String("identifier")] = engine->identifier;
		engineData[QLatin1String("isDefault")] = (engine->identifier == m_defaultSearch);
		engineData[QLatin1String("encoding")] = engine->encoding;
		engineData[QLatin1String("selfUrl")] = engine->selfUrl;
		engineData[QLatin1String("resultsUrl")] = engine->resultsUrl.url;
		engineData[QLatin1String("resultsEnctype")] = engine->resultsUrl.enctype;
		engineData[QLatin1String("resultsMethod")] = engine->resultsUrl.method;
		engineData[QLatin1String("resultsParameters")] = engine->resultsUrl.parameters.toString(QUrl::FullyDecoded);
		engineData[QLatin1String("suggestionsUrl")] = engine->suggestionsUrl.url;
		engineData[QLatin1String("suggestionsEnctype")] = engine->suggestionsUrl.enctype;
		engineData[QLatin1String("suggestionsMethod")] = engine->suggestionsUrl.method;
		engineData[QLatin1String("suggestionsParameters")] = engine->suggestionsUrl.parameters.toString(QUrl::FullyDecoded);

		QList<QStandardItem*> items;
		items.append(new QStandardItem(engine->icon, engine->title));
		items[0]->setToolTip(engine->description);
		items[0]->setData(engineData, Qt::UserRole);
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
		items.append(new QStandardItem(engine->shortcut));
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);

		searchEnginesModel->appendRow(items);
	}

	m_ui->searchViewWidget->setModel(searchEnginesModel);
	m_ui->searchViewWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_ui->searchViewWidget->setItemDelegateForColumn(0, new OptionDelegate(true, this));
	m_ui->searchViewWidget->setItemDelegateForColumn(1, new SearchShortcutDelegate(this));
	m_ui->searchSuggestionsCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Search/SearchEnginesSuggestions")).toBool());
	m_ui->moveDownSearchButton->setIcon(Utils::getIcon(QLatin1String("arrow-down")));
	m_ui->moveUpSearchButton->setIcon(Utils::getIcon(QLatin1String("arrow-up")));

	m_ui->suggestBookmarksCheckBox->setChecked(SettingsManager::getValue(QLatin1String("AddressField/SuggestBookmarks")).toBool());

	m_ui->sendReferrerCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Network/EnableReferrer")).toBool());

	const QStringList userAgents = NetworkAccessManager::getUserAgents();

	m_ui->userAgentComboBox->addItem(tr("Default"), QLatin1String("default"));

	for (int i = 0; i < userAgents.count(); ++i)
	{
		const UserAgentInformation userAgent = NetworkAccessManager::getUserAgent(userAgents.at(i));
		const QString title = userAgent.title;

		m_ui->userAgentComboBox->addItem((title.isEmpty() ? tr("(Untitled)") : title), userAgents.at(i));
		m_ui->userAgentComboBox->setItemData((i + 1), userAgent.value, (Qt::UserRole + 1));
	}

	m_ui->userAgentComboBox->setCurrentIndex(m_ui->userAgentComboBox->findData(SettingsManager::getValue(QLatin1String("Network/UserAgent")).toString()));

	const QString proxyString = SettingsManager::getValue(QLatin1String("Network/ProxyMode")).toString();
	int proxyIndex = 0;

	if (proxyString == QLatin1String("system"))
	{
		proxyIndex = 1;
	}
	else if (proxyString == QLatin1String("manual"))
	{
		proxyIndex = 2;
	}
	else if (proxyString == QLatin1String("automatic"))
	{
		proxyIndex = 3;
	}

	m_ui->proxyModeComboBox->setCurrentIndex(proxyIndex);

	if (proxyIndex == 2 || proxyIndex == 3)
	{
		proxyModeChanged(proxyIndex);
	}

	m_ui->allProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseCommon")).toBool());
	m_ui->httpProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseHttp")).toBool());
	m_ui->httpsProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseHttps")).toBool());
	m_ui->ftpProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseFtp")).toBool());
	m_ui->socksProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseSocks")).toBool());
	m_ui->allProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/CommonServers")).toString());
	m_ui->httpProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/HttpServers")).toString());
	m_ui->httpsProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/HttpsServers")).toString());
	m_ui->ftpProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/FtpServers")).toString());
	m_ui->socksProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/SocksServers")).toString());
	m_ui->allProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/CommonPort")).toInt());
	m_ui->httpProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/HttpPort")).toInt());
	m_ui->httpsProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/HttpsPort")).toInt());
	m_ui->ftpProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/FtpPort")).toInt());
	m_ui->socksProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/SocksPort")).toInt());
	m_ui->automaticProxyConfigurationLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/AutomaticConfigurationPath")).toString());

	QStandardItemModel *ciphersModel = new QStandardItemModel(this);
	const QStringList ciphers = SettingsManager::getValue(QLatin1String("Security/Ciphers")).toStringList();
	QStringList availableCiphers;
	const QList<QSslCipher>supportedCiphers = QSslSocket::supportedCiphers();

	for (int i = 0; i <supportedCiphers.count(); ++i)
	{
		if (ciphers.isEmpty() || ciphers.contains(supportedCiphers.at(i).name()))
		{
			QList<QStandardItem*> items;
			items.append(new QStandardItem(supportedCiphers.at(i).name()));
			items[0]->setFlags(Qt::ItemIsSelectable |Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

			ciphersModel->appendRow(items);
		}
	}

	m_ui->ciphersViewWidget->setModel(ciphersModel);
	m_ui->ciphersViewWidget->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
	m_ui->ciphersViewWidget->setItemDelegate(new OptionDelegate(true, this));

	m_ui->ciphersMoveDownButton->setIcon(Utils::getIcon(QLatin1String("arrow-down")));
	m_ui->ciphersMoveUpButton->setIcon(Utils::getIcon(QLatin1String("arrow-up")));


	m_ui->actionShortcutsMoveDownButton->setIcon(Utils::getIcon(QLatin1String("arrow-down")));
	m_ui->actionShortcutsMoveUpButton->setIcon(Utils::getIcon(QLatin1String("arrow-up")));
	m_ui->actionMacrosMoveDownButton->setIcon(Utils::getIcon(QLatin1String("arrow-down")));
	m_ui->actionMacrosMoveUpButton->setIcon(Utils::getIcon(QLatin1String("arrow-up")));

	loadProfiles(QLatin1String("keyboard"), QLatin1String("Browser/KeyboardShortcutsProfilesOrder"), m_ui->actionShortcutsViewWidget);
	loadProfiles(QLatin1String("macros"), QLatin1String("Browser/ActionMacrosProfilesOrder"), m_ui->actionMacrosViewWidget);

	QList<QLineEdit*> lineEdits = findChildren<QLineEdit*>();

	for (int i = 0; i < lineEdits.count(); ++i)
	{
		connect(lineEdits.at(i), SIGNAL(textChanged(QString)), this, SLOT(markModified()));
	}

	QList<QAbstractButton*> buttons = findChildren<QAbstractButton*>();

	for (int i = 0; i < buttons.count(); ++i)
	{
		connect(buttons.at(i), SIGNAL(toggled(bool)), this, SLOT(markModified()));
	}

	QList<QComboBox*> comboBoxes = findChildren<QComboBox*>();

	for (int i = 0; i < comboBoxes.count(); ++i)
	{
		connect(comboBoxes.at(i), SIGNAL(currentIndexChanged(int)), this, SLOT(markModified()));
	}

	m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

	connect(m_ui->buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(save()));
	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(save()));
	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
	connect(m_ui->useCurrentAsStartPageButton, SIGNAL(clicked()), this, SLOT(useCurrentAsStartPage()));
	connect(m_ui->restoreStartPageButton, SIGNAL(clicked()), this, SLOT(restoreStartPage()));
	connect(m_ui->fontsWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentFontChanged(int,int,int,int)));
	connect(fontsDelegate, SIGNAL(commitData(QWidget*)), this, SLOT(fontChanged(QWidget*)));
	connect(m_ui->colorsWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentColorChanged(int,int,int,int)));
	connect(colorsDelegate, SIGNAL(commitData(QWidget*)), this, SLOT(colorChanged(QWidget*)));
	connect(m_ui->privateModeCheckBox, SIGNAL(toggled(bool)), m_ui->historyWidget, SLOT(setDisabled(bool)));
	connect(m_ui->clearHistoryCheckBox, SIGNAL(toggled(bool)), m_ui->clearHistoryButton, SLOT(setEnabled(bool)));
	connect(m_ui->clearHistoryButton, SIGNAL(clicked()), this, SLOT(setupClearHistory()));
	connect(m_ui->searchFilterLineEdit, SIGNAL(textChanged(QString)), m_ui->searchViewWidget, SLOT(setFilter(QString)));
	connect(m_ui->searchViewWidget, SIGNAL(canMoveDownChanged(bool)), m_ui->moveDownSearchButton, SLOT(setEnabled(bool)));
	connect(m_ui->searchViewWidget, SIGNAL(canMoveUpChanged(bool)), m_ui->moveUpSearchButton, SLOT(setEnabled(bool)));
	connect(m_ui->searchViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateSearchActions()));
	connect(m_ui->searchViewWidget, SIGNAL(modified()), this, SLOT(markModified()));
	connect(m_ui->addSearchButton, SIGNAL(clicked()), this, SLOT(addSearch()));
	connect(m_ui->editSearchButton, SIGNAL(clicked()), this, SLOT(editSearch()));
	connect(m_ui->removeSearchButton, SIGNAL(clicked()), m_ui->searchViewWidget, SLOT(removeRow()));
	connect(m_ui->moveDownSearchButton, SIGNAL(clicked()), m_ui->searchViewWidget, SLOT(moveDownRow()));
	connect(m_ui->moveUpSearchButton, SIGNAL(clicked()), m_ui->searchViewWidget, SLOT(moveUpRow()));
	connect(m_ui->advancedListWidget, SIGNAL(currentRowChanged(int)), m_ui->advancedStackedWidget, SLOT(setCurrentIndex(int)));
	connect(m_ui->userAgentButton, SIGNAL(clicked()), this, SLOT(manageUserAgents()));
	connect(m_ui->proxyModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(proxyModeChanged(int)));
	connect(m_ui->automaticProxyConfigurationButton, SIGNAL(clicked()), this, SLOT(browseAutomaticProxyPath()));
	connect(m_ui->ciphersViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateCiphers()));
	connect(m_ui->ciphersMoveDownButton, SIGNAL(clicked()), m_ui->ciphersViewWidget, SLOT(moveDownRow()));
	connect(m_ui->ciphersMoveUpButton, SIGNAL(clicked()), m_ui->ciphersViewWidget, SLOT(moveUpRow()));
	connect(m_ui->ciphersRemoveButton, SIGNAL(clicked()), m_ui->ciphersViewWidget, SLOT(removeRow()));
	connect(m_ui->ciphersViewWidget, SIGNAL(canMoveDownChanged(bool)), m_ui->ciphersMoveDownButton, SLOT(setEnabled(bool)));
	connect(m_ui->ciphersViewWidget, SIGNAL(canMoveUpChanged(bool)), m_ui->ciphersMoveUpButton, SLOT(setEnabled(bool)));
	connect(m_ui->actionShortcutsViewWidget, SIGNAL(canMoveDownChanged(bool)), m_ui->actionShortcutsMoveDownButton, SLOT(setEnabled(bool)));
	connect(m_ui->actionShortcutsViewWidget, SIGNAL(canMoveUpChanged(bool)), m_ui->actionShortcutsMoveUpButton, SLOT(setEnabled(bool)));
	connect(m_ui->actionShortcutsViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateKeyboardProfleActions()));
	connect(m_ui->actionShortcutsViewWidget, SIGNAL(modified()), this, SLOT(markModified()));
	connect(m_ui->actionShortcutsAddButton, SIGNAL(clicked()), this, SLOT(addKeyboardProfile()));
	connect(m_ui->actionShortcutsEditButton, SIGNAL(clicked()), this, SLOT(editKeyboardProfile()));
	connect(m_ui->actionShortcutsCloneButton, SIGNAL(clicked()), this, SLOT(cloneKeyboardProfile()));
	connect(m_ui->actionShortcutsRemoveButton, SIGNAL(clicked()), this, SLOT(removeKeyboardProfile()));
	connect(m_ui->actionShortcutsMoveDownButton, SIGNAL(clicked()), m_ui->actionShortcutsViewWidget, SLOT(moveDownRow()));
	connect(m_ui->actionShortcutsMoveUpButton, SIGNAL(clicked()), m_ui->actionShortcutsViewWidget, SLOT(moveUpRow()));
	connect(m_ui->actionMacrosViewWidget, SIGNAL(canMoveDownChanged(bool)), m_ui->actionMacrosMoveDownButton, SLOT(setEnabled(bool)));
	connect(m_ui->actionMacrosViewWidget, SIGNAL(canMoveUpChanged(bool)), m_ui->actionMacrosMoveUpButton, SLOT(setEnabled(bool)));
	connect(m_ui->actionMacrosViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateMacrosProfleActions()));
	connect(m_ui->actionMacrosViewWidget, SIGNAL(modified()), this, SLOT(markModified()));
	connect(m_ui->actionMacrosAddButton, SIGNAL(clicked()), this, SLOT(addMacrosProfile()));
	connect(m_ui->actionMacrosEditButton, SIGNAL(clicked()), this, SLOT(editMacrosProfile()));
	connect(m_ui->actionMacrosCloneButton, SIGNAL(clicked()), this, SLOT(cloneMacrosProfile()));
	connect(m_ui->actionMacrosRemoveButton, SIGNAL(clicked()), this, SLOT(removeMacrosProfile()));
	connect(m_ui->actionMacrosMoveDownButton, SIGNAL(clicked()), m_ui->actionMacrosViewWidget, SLOT(moveDownRow()));
	connect(m_ui->actionMacrosMoveUpButton, SIGNAL(clicked()), m_ui->actionMacrosViewWidget, SLOT(moveUpRow()));
}

PreferencesDialog::~PreferencesDialog()
{
	delete m_ui;
}

void PreferencesDialog::changeEvent(QEvent *event)
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

void PreferencesDialog::useCurrentAsStartPage()
{
	WindowsManager *manager = SessionsManager::getWindowsManager();

	if (manager)
	{
		m_ui->homePageLineEdit->setText(manager->getUrl().toString(QUrl::RemovePassword));
	}
}

void PreferencesDialog::restoreStartPage()
{
	m_ui->homePageLineEdit->setText(SettingsManager::getDefaultValue(QLatin1String("Browser/StartPage")).toString());
}

void PreferencesDialog::currentFontChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
	Q_UNUSED(currentColumn)
	Q_UNUSED(previousColumn)

	QTableWidgetItem *previousItem = m_ui->fontsWidget->item(previousRow, 1);

	m_ui->fontsWidget->closePersistentEditor(previousItem);

	if (currentRow >= 0 && currentRow < m_ui->fontsWidget->rowCount())
	{
		m_ui->fontsWidget->openPersistentEditor(m_ui->fontsWidget->item(currentRow, 1));
	}
}

void PreferencesDialog::fontChanged(QWidget *editor)
{
	OptionWidget *widget = qobject_cast<OptionWidget*>(editor);

	if (widget && widget->getIndex().row() >= 0 && widget->getIndex().row() < m_ui->fontsWidget->rowCount())
	{
		m_ui->fontsWidget->item(widget->getIndex().row(), 1)->setText(m_ui->fontsWidget->item(widget->getIndex().row(), 1)->data(Qt::EditRole).toString());
		m_ui->fontsWidget->item(widget->getIndex().row(), 2)->setFont(QFont(widget->getValue().toString()));
	}

	markModified();
}

void PreferencesDialog::currentColorChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
	Q_UNUSED(currentColumn)
	Q_UNUSED(previousColumn)

	QTableWidgetItem *previousItem = m_ui->colorsWidget->item(previousRow, 1);

	m_ui->colorsWidget->closePersistentEditor(previousItem);

	if (currentRow >= 0 && currentRow < m_ui->colorsWidget->rowCount())
	{
		m_ui->colorsWidget->openPersistentEditor(m_ui->colorsWidget->item(currentRow, 1));
	}
}

void PreferencesDialog::colorChanged(QWidget *editor)
{
	OptionWidget *widget = qobject_cast<OptionWidget*>(editor);

	if (widget && widget->getIndex().row() >= 0 && widget->getIndex().row() < m_ui->colorsWidget->rowCount())
	{
		m_ui->colorsWidget->item(widget->getIndex().row(), 1)->setBackgroundColor(QColor(widget->getValue().toString()));
		m_ui->colorsWidget->item(widget->getIndex().row(), 1)->setData(Qt::EditRole, widget->getValue());
	}

	markModified();
}

void PreferencesDialog::setupClearHistory()
{
	ClearHistoryDialog dialog(m_clearSettings, true, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		m_clearSettings = dialog.getClearSettings();

		markModified();
	}

	m_ui->clearHistoryCheckBox->setChecked(!m_clearSettings.isEmpty());
	m_ui->clearHistoryButton->setEnabled(!m_clearSettings.isEmpty());
}

void PreferencesDialog::addSearch()
{
	QString identifier;
	QStringList identifiers;
	QStringList shortcuts;

	for (int i = 0; i < m_ui->searchViewWidget->getRowCount(); ++i)
	{
		identifiers.append(m_ui->searchViewWidget->getIndex(i, 0).data(Qt::UserRole).toHash().value(QLatin1String("identifier"), QString()).toString());

		const QString shortcut = m_ui->searchViewWidget->getIndex(i, 1).data(Qt::DisplayRole).toString();

		if (!shortcut.isEmpty())
		{
			shortcuts.append(shortcut);
		}
	}

	do
	{
		identifier = QInputDialog::getText(this, tr("Select Identifier"), tr("Input Unique Search Engine Identifier:"));

		if (identifier.isEmpty())
		{
			return;
		}
	}
	while (identifiers.contains(identifier));

	QVariantHash engineData;
	engineData[QLatin1String("identifier")] = identifier;
	engineData[QLatin1String("isDefault")] = false;
	engineData[QLatin1String("encoding")] = QLatin1String("UTF-8");
	engineData[QLatin1String("selfUrl")] = QString();
	engineData[QLatin1String("resultsUrl")] = QString();
	engineData[QLatin1String("resultsEnctype")] = QString();
	engineData[QLatin1String("resultsMethod")] = QLatin1String("get");
	engineData[QLatin1String("resultsParameters")] = QString();
	engineData[QLatin1String("suggestionsUrl")] = QString();
	engineData[QLatin1String("suggestionsEnctype")] = QString();
	engineData[QLatin1String("suggestionsMethod")] = QLatin1String("get");
	engineData[QLatin1String("suggestionsParameters")] = QString();
	engineData[QLatin1String("shortcut")] = QString();
	engineData[QLatin1String("title")] = tr("New Search Engine");
	engineData[QLatin1String("description")] = QString();
	engineData[QLatin1String("icon")] = Utils::getIcon(QLatin1String("edit-find"));

	SearchPropertiesDialog dialog(engineData, shortcuts, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	engineData = dialog.getEngineData();

	if (shortcuts.contains(engineData[QLatin1String("shortcut")].toString()))
	{
		engineData[QLatin1String("shortcut")] = QString();
	}

	if (engineData[QLatin1String("isDefault")].toBool())
	{
		m_defaultSearch = engineData[QLatin1String("identifier")].toString();
	}

	QList<QStandardItem*> items;
	items.append(new QStandardItem(engineData[QLatin1String("icon")].value<QIcon>(), engineData[QLatin1String("title")].toString()));
	items[0]->setToolTip(engineData[QLatin1String("description")].toString());
	items[0]->setData(engineData, Qt::UserRole);
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
	items.append(new QStandardItem(engineData[QLatin1String("shortcut")].toString()));
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);

	m_ui->searchViewWidget->insertRow(items);

	markModified();
}

void PreferencesDialog::editSearch()
{
	const QModelIndex index = m_ui->searchViewWidget->getIndex(m_ui->searchViewWidget->getCurrentRow(), 0);

	if (!index.isValid())
	{
		return;
	}

	QVariantHash engineData = index.data(Qt::UserRole).toHash();
	engineData[QLatin1String("shortcut")] = m_ui->searchViewWidget->getIndex(index.row(), 1).data(Qt::DisplayRole).toString();
	engineData[QLatin1String("title")] = index.data(Qt::DisplayRole).toString();
	engineData[QLatin1String("description")] = index.data(Qt::ToolTipRole).toString();
	engineData[QLatin1String("icon")] = index.data(Qt::DecorationRole).value<QIcon>();

	QStringList shortcuts;

	for (int i = 0; i < m_ui->searchViewWidget->getRowCount(); ++i)
	{
		const QString shortcut = m_ui->searchViewWidget->getIndex(i, 1).data(Qt::DisplayRole).toString();

		if (m_ui->searchViewWidget->getCurrentRow() != i && !shortcut.isEmpty())
		{
			shortcuts.append(shortcut);
		}
	}

	SearchPropertiesDialog dialog(engineData, shortcuts, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	engineData = dialog.getEngineData();

	if (shortcuts.contains(engineData[QLatin1String("shortcut")].toString()))
	{
		engineData[QLatin1String("shortcut")] = QString();
	}

	if (engineData[QLatin1String("isDefault")].toBool())
	{
		m_defaultSearch = engineData[QLatin1String("identifier")].toString();
	}

	m_ui->searchViewWidget->setData(index, engineData[QLatin1String("title")], Qt::DisplayRole);
	m_ui->searchViewWidget->setData(index, engineData[QLatin1String("description")], Qt::ToolTipRole);
	m_ui->searchViewWidget->setData(index, engineData[QLatin1String("icon")], Qt::DecorationRole);
	m_ui->searchViewWidget->setData(index, engineData, Qt::UserRole);
	m_ui->searchViewWidget->setData(m_ui->searchViewWidget->getIndex(index.row(), 1), engineData[QLatin1String("shortcut")], Qt::DisplayRole);

	markModified();
}

void PreferencesDialog::updateSearchActions()
{
	const int currentRow = m_ui->searchViewWidget->getCurrentRow();
	const bool isSelected = (currentRow >= 0 && currentRow < m_ui->searchViewWidget->getRowCount());

	m_ui->editSearchButton->setEnabled(isSelected);
	m_ui->removeSearchButton->setEnabled(isSelected);
}

void PreferencesDialog::manageUserAgents()
{
	QList<UserAgentInformation> userAgents;

	for (int i = 1; i < m_ui->userAgentComboBox->count(); ++i)
	{
		UserAgentInformation userAgent;
		userAgent.identifier = m_ui->userAgentComboBox->itemData(i, Qt::UserRole).toString();
		userAgent.title = m_ui->userAgentComboBox->itemText(i);
		userAgent.value = m_ui->userAgentComboBox->itemData(i, (Qt::UserRole + 1)).toString();

		userAgents.append(userAgent);
	}

	const QString selectedUserAgent = m_ui->userAgentComboBox->currentData().toString();

	UserAgentsManagerDialog dialog(userAgents, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		m_userAgentsModified = true;

		m_ui->userAgentComboBox->clear();

		userAgents = dialog.getUserAgents();

		m_ui->userAgentComboBox->addItem(tr("Default"), QLatin1String("default"));

		for (int i = 0; i < userAgents.count(); ++i)
		{
			const QString title = userAgents.at(i).title;

			m_ui->userAgentComboBox->addItem((title.isEmpty() ? tr("(Untitled)") : title), userAgents.at(i).identifier);
			m_ui->userAgentComboBox->setItemData((i + 1), userAgents.at(i).value, (Qt::UserRole + 1));
		}

		m_ui->userAgentComboBox->setCurrentIndex(m_ui->userAgentComboBox->findData(selectedUserAgent));
	}
}

void PreferencesDialog::proxyModeChanged(int index)
{
	if (index == 2)
	{
		m_ui->manualProxyConfigurationWidget->setEnabled(true);
		m_ui->allProxyCheckBox->setEnabled(true);
		m_ui->allProxyServersLineEdit->setEnabled(true);
		m_ui->allProxyPortSpinBox->setEnabled(true);

		if (!m_ui->allProxyCheckBox->isChecked())
		{
			m_ui->httpProxyCheckBox->setEnabled(true);
			m_ui->httpsProxyCheckBox->setEnabled(true);
			m_ui->ftpProxyCheckBox->setEnabled(true);
			m_ui->socksProxyCheckBox->setEnabled(true);
			m_ui->httpProxyServersLineEdit->setEnabled(true);
			m_ui->httpProxyPortSpinBox->setEnabled(true);
			m_ui->httpsProxyServersLineEdit->setEnabled(true);
			m_ui->httpsProxyPortSpinBox->setEnabled(true);
			m_ui->ftpProxyServersLineEdit->setEnabled(true);
			m_ui->ftpProxyPortSpinBox->setEnabled(true);
			m_ui->socksProxyServersLineEdit->setEnabled(true);
			m_ui->socksProxyPortSpinBox->setEnabled(true);
		}
	}
	else
	{
		m_ui->manualProxyConfigurationWidget->setDisabled(true);
		m_ui->allProxyCheckBox->setChecked(false);
		m_ui->httpProxyCheckBox->setChecked(false);
		m_ui->httpsProxyCheckBox->setChecked(false);
		m_ui->ftpProxyCheckBox->setChecked(false);
		m_ui->socksProxyCheckBox->setChecked(false);
	}

	if (index == 3)
	{
		m_ui->automaticProxyConfigurationWidget->setEnabled(true);
		m_ui->automaticProxyConfigurationLineEdit->setEnabled(true);
		m_ui->automaticProxyConfigurationButton->setEnabled(true);
	}
	else
	{
		m_ui->automaticProxyConfigurationWidget->setEnabled(false);
		m_ui->automaticProxyConfigurationLineEdit->setEnabled(false);
		m_ui->automaticProxyConfigurationButton->setEnabled(false);
	}
}

void PreferencesDialog::browseAutomaticProxyPath()
{
	const QString path = QFileDialog::getOpenFileName(this, tr("Select Proxy Automatic Configuration File"), QString(), tr("PAC files (*.pac)"));

	if (!path.isEmpty())
	{
		m_ui->automaticProxyConfigurationLineEdit->setText(path);
	}
}

void PreferencesDialog::updateCiphers()
{
	const int currentRow = m_ui->ciphersViewWidget->getCurrentRow();
	const bool isSelected = (currentRow >= 0 && currentRow < m_ui->ciphersViewWidget->getRowCount());

	m_ui->ciphersRemoveButton->setEnabled(isSelected);
}

void PreferencesDialog::addKeyboardProfile()
{
	const QString identifier = createProfileIdentifier(m_ui->actionShortcutsViewWidget);

	if (identifier.isEmpty())
	{
		return;
	}

	QHash<QString, QString> hash;
	hash[QLatin1String("Title")] = tr("(Untitled)");

	m_keyboardProfilesInformation[identifier] = hash;
	m_keyboardProfilesData[identifier] = QHash<QString, QVariantHash>();

	QList<QStandardItem*> items;
	items.append(new QStandardItem(tr("(Untitled)")));
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
	items.append(new QStandardItem(identifier));
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->actionShortcutsViewWidget->insertRow(items);

	markModified();
}

void PreferencesDialog::editKeyboardProfile()
{
	const QModelIndex index = m_ui->actionShortcutsViewWidget->getIndex(m_ui->actionShortcutsViewWidget->getCurrentRow(), 0);
	const QString profile = m_ui->actionShortcutsViewWidget->getIndex(index.row(), 1).data().toString();

	if (profile.isEmpty())
	{
		return;
	}

	const QString path = index.data(Qt::UserRole).toString();
	ShortcutsProfileDialog dialog((m_keyboardProfilesInformation.contains(profile) ? m_keyboardProfilesInformation[profile] : getProfileInformation(path)), (m_keyboardProfilesData.contains(profile) ? m_keyboardProfilesData[profile] : getProfileData(path)), getShortcuts(), false, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	m_keyboardProfilesInformation[profile] = dialog.getInformation();
	m_keyboardProfilesData[profile] = dialog.getData();

	const QString title = m_keyboardProfilesInformation[profile].value(QLatin1String("Title"), QString());

	m_ui->actionShortcutsViewWidget->setData(index, (title.isEmpty() ? tr("(Untitled)") : title), Qt::DisplayRole);
	m_ui->actionShortcutsViewWidget->setData(index, m_keyboardProfilesInformation[profile].value(QLatin1String("Description"), QString()), Qt::ToolTipRole);

	markModified();
}

void PreferencesDialog::cloneKeyboardProfile()
{
	const QModelIndex index = m_ui->actionShortcutsViewWidget->getIndex(m_ui->actionShortcutsViewWidget->getCurrentRow(), 1);

	if (!index.isValid())
	{
		return;
	}

	const QString profile = index.data().toString();
	const QString identifier = createProfileIdentifier(m_ui->actionShortcutsViewWidget, profile);

	if (identifier.isEmpty())
	{
		return;
	}

	const QString path = getProfilePath(QLatin1String("keyboard"), profile);

	m_keyboardProfilesInformation[identifier] = (m_keyboardProfilesInformation.contains(profile) ? m_keyboardProfilesInformation[profile] : getProfileInformation(path));
	m_keyboardProfilesData[identifier] = (m_keyboardProfilesData.contains(profile) ? m_keyboardProfilesData[profile] : getProfileData(path));

	QList<QStandardItem*> items;
	items.append(new QStandardItem(m_keyboardProfilesInformation[identifier].value(QLatin1String("Title"), tr("(Untitled)"))));
	items[0]->setToolTip(m_keyboardProfilesInformation[identifier].value(QLatin1String("Description"), QString()));
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
	items.append(new QStandardItem(identifier));
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->actionShortcutsViewWidget->insertRow(items);

	markModified();
}

void PreferencesDialog::removeKeyboardProfile()
{
	const QModelIndex index = m_ui->actionShortcutsViewWidget->getIndex(m_ui->actionShortcutsViewWidget->getCurrentRow(), 0);

	if (!index.isValid())
	{
		return;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("Do you really want to remove this profile?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Cancel);

	if (!index.data(Qt::UserRole).toString().isEmpty() && !index.data(Qt::UserRole).toString().startsWith(QLatin1Char(':')))
	{
		messageBox.setCheckBox(new QCheckBox(tr("Delete profile permanently")));
	}

	if (messageBox.exec() == QMessageBox::Yes)
	{
		if (messageBox.checkBox() && messageBox.checkBox()->isChecked())
		{
			m_removedProfiles.append(index.data(Qt::UserRole).toString());
		}

		const QString profile = m_ui->actionShortcutsViewWidget->getIndex(index.row(), 1).data().toString();

		m_keyboardProfilesInformation.remove(profile);
		m_keyboardProfilesData.remove(profile);

		m_ui->actionShortcutsViewWidget->removeRow();

		markModified();
	}
}

void PreferencesDialog::updateKeyboardProfleActions()
{
	const int currentRow = m_ui->actionShortcutsViewWidget->getCurrentRow();
	const QModelIndex index = m_ui->actionShortcutsViewWidget->getIndex(currentRow, 0);
	const bool isSelected = (currentRow >= 0 && currentRow < m_ui->actionShortcutsViewWidget->getRowCount());

	m_ui->actionShortcutsEditButton->setEnabled(isSelected && !index.data(Qt::UserRole).toString().startsWith(QLatin1Char(':')));
	m_ui->actionShortcutsCloneButton->setEnabled(isSelected);
	m_ui->actionShortcutsRemoveButton->setEnabled(isSelected);
}

void PreferencesDialog::addMacrosProfile()
{
	const QString identifier = createProfileIdentifier(m_ui->actionMacrosViewWidget);

	if (identifier.isEmpty())
	{
		return;
	}

	QHash<QString, QString> hash;
	hash[QLatin1String("Title")] = tr("(Untitled)");

	m_keyboardProfilesInformation[identifier] = hash;
	m_keyboardProfilesData[identifier] = QHash<QString, QVariantHash>();

	QList<QStandardItem*> items;
	items.append(new QStandardItem(tr("(Untitled)")));
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
	items.append(new QStandardItem(identifier));
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->actionMacrosViewWidget->insertRow(items);

	markModified();
}

void PreferencesDialog::editMacrosProfile()
{
	const QModelIndex index = m_ui->actionMacrosViewWidget->getIndex(m_ui->actionMacrosViewWidget->getCurrentRow(), 0);
	const QString profile = m_ui->actionMacrosViewWidget->getIndex(index.row(), 1).data().toString();

	if (profile.isEmpty())
	{
		return;
	}

	const QString path = index.data(Qt::UserRole).toString();
	ShortcutsProfileDialog dialog((m_macrosProfilesInformation.contains(profile) ? m_macrosProfilesInformation[profile] : getProfileInformation(path)), (m_macrosProfilesData.contains(profile) ? m_macrosProfilesData[profile] : getProfileData(path)), getShortcuts(), true, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	m_macrosProfilesInformation[profile] = dialog.getInformation();
	m_macrosProfilesData[profile] = dialog.getData();

	const QString title = m_macrosProfilesInformation[profile].value(QLatin1String("Title"), QString());

	m_ui->actionMacrosViewWidget->setData(index, (title.isEmpty() ? tr("(Untitled)") : title), Qt::DisplayRole);
	m_ui->actionMacrosViewWidget->setData(index, m_macrosProfilesInformation[profile].value(QLatin1String("Description"), QString()), Qt::ToolTipRole);

	markModified();
}

void PreferencesDialog::cloneMacrosProfile()
{
	const QModelIndex index = m_ui->actionMacrosViewWidget->getIndex(m_ui->actionMacrosViewWidget->getCurrentRow(), 1);

	if (!index.isValid())
	{
		return;
	}

	const QString profile = index.data().toString();
	const QString identifier = createProfileIdentifier(m_ui->actionMacrosViewWidget, profile);

	if (identifier.isEmpty())
	{
		return;
	}

	const QString path = getProfilePath(QLatin1String("macros"), profile);

	m_macrosProfilesInformation[identifier] = (m_macrosProfilesInformation.contains(profile) ? m_macrosProfilesInformation[profile] : getProfileInformation(path));
	m_macrosProfilesData[identifier] = (m_macrosProfilesData.contains(profile) ? m_macrosProfilesData[profile] : getProfileData(path));

	QList<QStandardItem*> items;
	items.append(new QStandardItem(m_macrosProfilesInformation[identifier].value(QLatin1String("Title"), tr("(Untitled)"))));
	items[0]->setToolTip(m_macrosProfilesInformation[identifier].value(QLatin1String("Description"), QString()));
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
	items.append(new QStandardItem(identifier));
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->actionMacrosViewWidget->insertRow(items);

	markModified();
}

void PreferencesDialog::removeMacrosProfile()
{
	const QModelIndex index = m_ui->actionMacrosViewWidget->getIndex(m_ui->actionMacrosViewWidget->getCurrentRow(), 0);

	if (!index.isValid())
	{
		return;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("Do you really want to remove this profile?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Cancel);

	if (!index.data(Qt::UserRole).toString().isEmpty() && !index.data(Qt::UserRole).toString().startsWith(QLatin1Char(':')))
	{
		messageBox.setCheckBox(new QCheckBox(tr("Delete profile permanently")));
	}

	if (messageBox.exec() == QMessageBox::Yes)
	{
		if (messageBox.checkBox() && messageBox.checkBox()->isChecked())
		{
			m_removedProfiles.append(index.data(Qt::UserRole).toString());
		}

		const QString profile = m_ui->actionMacrosViewWidget->getIndex(index.row(), 1).data().toString();

		m_macrosProfilesInformation.remove(profile);
		m_macrosProfilesData.remove(profile);

		m_ui->actionMacrosViewWidget->removeRow();

		markModified();
	}
}

void PreferencesDialog::updateMacrosProfleActions()
{
	const int currentRow = m_ui->actionMacrosViewWidget->getCurrentRow();
	const QModelIndex index = m_ui->actionMacrosViewWidget->getIndex(currentRow, 0);
	const bool isSelected = (currentRow >= 0 && currentRow < m_ui->actionMacrosViewWidget->getRowCount());

	m_ui->actionMacrosEditButton->setEnabled(isSelected && !index.data(Qt::UserRole).toString().startsWith(QLatin1Char(':')));
	m_ui->actionMacrosCloneButton->setEnabled(isSelected);
	m_ui->actionMacrosRemoveButton->setEnabled(isSelected);
}

void PreferencesDialog::loadProfiles(const QString &type, const QString &key, TableViewWidget *view)
{
	QStringList labels;
	labels << tr("Name") << tr("Identifier");

	QStandardItemModel *model = new QStandardItemModel(this);
	model->setHorizontalHeaderLabels(labels);

	const QStringList profiles = SettingsManager::getValue(key).toStringList();

	for (int i = 0; i < profiles.count(); ++i)
	{
		const QString path = getProfilePath(type, profiles.at(i));
		const QHash<QString, QString> information = getProfileInformation(path);

		if (information.isEmpty())
		{
			continue;
		}

		QList<QStandardItem*> items;
		items.append(new QStandardItem(information.value(QLatin1String("Title"), tr("(Untitled)"))));
		items[0]->setToolTip(information.value(QLatin1String("Description"), QString()));
		items[0]->setData(path, Qt::UserRole);
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
		items.append(new QStandardItem(profiles.at(i)));
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

		model->appendRow(items);
	}

	view->setModel(model);
	view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	view->setItemDelegate(new OptionDelegate(true, this));
}

void PreferencesDialog::markModified()
{
	m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void PreferencesDialog::save()
{
	const int startupBehaviorIndex = m_ui->startupBehaviorComboBox->currentIndex();
	QLatin1String startupBehaviorString = QLatin1String("continuePrevious");

	if (startupBehaviorIndex == 1)
	{
		startupBehaviorString = QLatin1String("showDialog");
	}
	else if (startupBehaviorIndex == 2)
	{
		startupBehaviorString = QLatin1String("startHomePage");
	}
	else if (startupBehaviorIndex == 3)
	{
		startupBehaviorString = QLatin1String("startEmpty");
	}

	SettingsManager::setValue(QLatin1String("Browser/StartupBehavior"), startupBehaviorString);
	SettingsManager::setValue(QLatin1String("Browser/StartPage"), m_ui->homePageLineEdit->text());
	SettingsManager::setValue(QLatin1String("Paths/Downloads"), m_ui->downloadsFilePathWidget->getPath());
	SettingsManager::setValue(QLatin1String("Browser/AlwaysAskWhereSaveFile"), m_ui->alwaysAskCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Browser/OpenLinksInNewTab"), m_ui->tabsInsteadOfWindowsCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Browser/DelayRestoringOfBackgroundTabs"), m_ui->delayTabsLoadingCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Browser/ReuseCurrentTab"), m_ui->reuseCurrentTabCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("TabBar/OpenNextToActive"), m_ui->openNextToActiveheckBox->isChecked());

	SettingsManager::setValue(QLatin1String("Content/DefaultZoom"), m_ui->defaultZoomSpinBox->value());
	SettingsManager::setValue(QLatin1String("Content/ZoomTextOnly"), m_ui->zoomTextOnlyCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Content/DefaultFontSize"), m_ui->proportionalFontSizeSpinBox->value());
	SettingsManager::setValue(QLatin1String("Content/DefaultFixedFontSize"), m_ui->fixedFontSizeSpinBox->value());
	SettingsManager::setValue(QLatin1String("Content/MinimumFontSize"), m_ui->minimumFontSizeSpinBox->value());

	for (int i = 0; i < m_ui->fontsWidget->rowCount(); ++i)
	{
		SettingsManager::setValue(m_ui->fontsWidget->item(i, 1)->data(Qt::UserRole).toString() , m_ui->fontsWidget->item(i, 1)->data(Qt::DisplayRole));
	}

	for (int i = 0; i < m_ui->colorsWidget->rowCount(); ++i)
	{
		SettingsManager::setValue(m_ui->colorsWidget->item(i, 1)->data(Qt::UserRole).toString() , m_ui->colorsWidget->item(i, 1)->data(Qt::EditRole));
	}

	const int doNotTrackPolicyIndex = m_ui->doNotTrackComboBox->currentIndex();
	QLatin1String doNotTrackPolicyString = QLatin1String("skip");

	if (doNotTrackPolicyIndex == 1)
	{
		doNotTrackPolicyString = QLatin1String("allow");
	}
	else if (doNotTrackPolicyIndex == 0)
	{
		doNotTrackPolicyString = QLatin1String("doNotAllow");
	}

	SettingsManager::setValue(QLatin1String("Network/DoNotTrackPolicy"), doNotTrackPolicyString);
	SettingsManager::setValue(QLatin1String("Browser/PrivateMode"), m_ui->privateModeCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("History/RememberBrowsing"), m_ui->rememberBrowsingHistoryCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("History/RememberDownloads"), m_ui->rememberDownloadsHistoryCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Browser/EnableCookies"), m_ui->acceptCookiesCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("History/ClearOnClose"), (m_ui->clearHistoryCheckBox->isChecked() ? m_clearSettings : QStringList()));

	QList<SearchInformation*> searchEngines;

	for (int i = 0; i < m_ui->searchViewWidget->getRowCount(); ++i)
	{
		const QModelIndex index = m_ui->searchViewWidget->getIndex(i, 0);

		if (!index.isValid())
		{
			continue;
		}

		const QVariantHash engineData = index.data(Qt::UserRole).toHash();
		SearchInformation *engine = new SearchInformation();
		engine->identifier = engineData[QLatin1String("identifier")].toString();
		engine->title = index.data(Qt::DisplayRole).toString();
		engine->description = index.data(Qt::ToolTipRole).toString();
		engine->shortcut = m_ui->searchViewWidget->getIndex(index.row(), 1).data(Qt::DisplayRole).toString();
		engine->encoding = engineData[QLatin1String("encoding")].toString();
		engine->selfUrl = engineData[QLatin1String("selfUrl")].toString();
		engine->resultsUrl.url = engineData[QLatin1String("resultsUrl")].toString();
		engine->resultsUrl.enctype = engineData[QLatin1String("resultsEnctype")].toString();
		engine->resultsUrl.method = engineData[QLatin1String("resultsMethod")].toString();
		engine->resultsUrl.parameters = QUrlQuery(engineData[QLatin1String("resultsParameters")].toString());
		engine->suggestionsUrl.url = engineData[QLatin1String("suggestionsUrl")].toString();
		engine->suggestionsUrl.enctype = engineData[QLatin1String("suggestionsEnctype")].toString();
		engine->suggestionsUrl.method = engineData[QLatin1String("suggestionsMethod")].toString();
		engine->suggestionsUrl.parameters = QUrlQuery(engineData[QLatin1String("suggestionsParameters")].toString());
		engine->icon = index.data(Qt::DecorationRole).value<QIcon>();

		searchEngines.append(engine);
	}

	if (SearchesManager::setSearchEngines(searchEngines))
	{
		SettingsManager::setValue(QLatin1String("Search/DefaultSearchEngine"), m_defaultSearch);
	}

	SettingsManager::setValue(QLatin1String("Search/SearchEnginesSuggestions"), m_ui->searchSuggestionsCheckBox->isChecked());

	SettingsManager::setValue(QLatin1String("AddressField/SuggestBookmarks"), m_ui->suggestBookmarksCheckBox->isChecked());

	SettingsManager::setValue(QLatin1String("Network/EnableReferrer"), m_ui->sendReferrerCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Network/UserAgent"), m_ui->userAgentComboBox->currentData().toString());

	const int proxyIndex = m_ui->proxyModeComboBox->currentIndex();
	QLatin1String proxyString = QLatin1String("noproxy");

	if (proxyIndex == 1)
	{
		proxyString = QLatin1String("system");
	}
	else if (proxyIndex == 2)
	{
		proxyString = QLatin1String("manual");
	}
	else if (proxyIndex == 3)
	{
		proxyString = QLatin1String("automatic");
	}

	SettingsManager::setValue(QLatin1String("Network/ProxyMode"), proxyString);

	if (!m_ui->allProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Proxy/UseCommon"), m_ui->allProxyCheckBox->isChecked());
	}

	if (!m_ui->httpProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Proxy/UseHttp"), m_ui->httpProxyCheckBox->isChecked());
	}

	if (!m_ui->httpsProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Proxy/UseHttps"), m_ui->httpsProxyCheckBox->isChecked());
	}

	if (!m_ui->ftpProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Proxy/UseFtp"), m_ui->ftpProxyCheckBox->isChecked());
	}

	if (!m_ui->socksProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Proxy/UseSocks"), m_ui->socksProxyCheckBox->isChecked());
	}

	SettingsManager::setValue(QLatin1String("Proxy/CommonServers"), m_ui->allProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/HttpServers"), m_ui->httpProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/HttpsServers"), m_ui->httpsProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/FtpServers"), m_ui->ftpProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/SocksServers"), m_ui->socksProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/CommonPort"), m_ui->allProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/HttpPort"), m_ui->httpProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/HttpsPort"), m_ui->httpsProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/FtpPort"), m_ui->ftpProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/SocksPort"), m_ui->socksProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/AutomaticConfigurationPath"), m_ui->automaticProxyConfigurationLineEdit->text());

	if (m_userAgentsModified)
	{
		QSettings userAgents(SessionsManager::getProfilePath() + QLatin1String("/userAgents.ini"), QSettings::IniFormat);
		userAgents.clear();

		for (int i = 1; i < m_ui->userAgentComboBox->count(); ++i)
		{
			userAgents.setValue(m_ui->userAgentComboBox->itemData(i, Qt::UserRole).toString() + "/title", m_ui->userAgentComboBox->itemText(i));
			userAgents.setValue(m_ui->userAgentComboBox->itemData(i, Qt::UserRole).toString() + "/value", m_ui->userAgentComboBox->itemData(i, (Qt::UserRole + 1)).toString());
		}

		userAgents.sync();

		NetworkAccessManager::loadUserAgents();
	}

	for (int i = 0; i < m_removedProfiles.count(); ++i)
	{
		QFile::remove(m_removedProfiles.at(i));
	}

	const QStringList modifiedKeyboardProfiles = m_keyboardProfilesInformation.keys();

	for (int i = 0; i < modifiedKeyboardProfiles.count(); ++i)
	{
		QDir().mkpath(SessionsManager::getProfilePath() + QLatin1String("/keyboard/"));
		QFile file(SessionsManager::getProfilePath() + QLatin1String("/keyboard/") + modifiedKeyboardProfiles.at(i) + QLatin1String(".ini"));

		if (!file.open(QIODevice::WriteOnly))
		{
			continue;
		}

		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		stream << QLatin1String("; Title: ") << m_keyboardProfilesInformation[modifiedKeyboardProfiles.at(i)].value(QLatin1String("Title"), tr("(Untitled)")) << QLatin1Char('\n');
		stream << QLatin1String("; Description: ") << m_keyboardProfilesInformation[modifiedKeyboardProfiles.at(i)].value(QLatin1String("Description"), QString()) << QLatin1Char('\n');
		stream << QLatin1String("; Type: keyboard-profile\n");
		stream << QLatin1String("; Author: ") << m_keyboardProfilesInformation[modifiedKeyboardProfiles.at(i)].value(QLatin1String("Author"), QString()) << QLatin1Char('\n');
		stream << QLatin1String("; Version: ") << m_keyboardProfilesInformation[modifiedKeyboardProfiles.at(i)].value(QLatin1String("Version"), QString()) << QLatin1String("\n\n");

		QHash<QString, QVariantHash>::iterator iterator;

		for (iterator = m_keyboardProfilesData[modifiedKeyboardProfiles.at(i)].begin(); iterator != m_keyboardProfilesData[modifiedKeyboardProfiles.at(i)].end(); ++iterator)
		{
			if (!iterator.key().isEmpty() && !iterator.value().value(QLatin1String("shortcuts")).isNull())
			{
				stream << QLatin1Char('[') << iterator.key() << QLatin1String("]\n");
				stream << QLatin1String("shortcuts=") << iterator.value().value(QLatin1String("shortcuts")).toString() << QLatin1String("\n\n");
			}
		}

		file.close();
	}

	const QStringList modifiedMacrosProfiles = m_macrosProfilesInformation.keys();

	for (int i = 0; i < modifiedMacrosProfiles.count(); ++i)
	{
		QDir().mkpath(SessionsManager::getProfilePath() + QLatin1String("/macros/"));
		QFile file(SessionsManager::getProfilePath() + QLatin1String("/macros/") + modifiedMacrosProfiles.at(i) + QLatin1String(".ini"));

		if (!file.open(QIODevice::WriteOnly))
		{
			continue;
		}

		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		stream << QLatin1String("; Title: ") << m_macrosProfilesInformation[modifiedMacrosProfiles.at(i)].value(QLatin1String("Title"), tr("(Untitled)")) << QLatin1Char('\n');
		stream << QLatin1String("; Description: ") << m_macrosProfilesInformation[modifiedMacrosProfiles.at(i)].value(QLatin1String("Description"), QString()) << QLatin1Char('\n');
		stream << QLatin1String("; Type: macros-profile\n");
		stream << QLatin1String("; Author: ") << m_macrosProfilesInformation[modifiedMacrosProfiles.at(i)].value(QLatin1String("Author"), QString()) << QLatin1Char('\n');
		stream << QLatin1String("; Version: ") << m_macrosProfilesInformation[modifiedMacrosProfiles.at(i)].value(QLatin1String("Version"), QString()) << QLatin1String("\n\n");

		QHash<QString, QVariantHash>::iterator iterator;

		for (iterator = m_macrosProfilesData[modifiedMacrosProfiles.at(i)].begin(); iterator != m_macrosProfilesData[modifiedMacrosProfiles.at(i)].end(); ++iterator)
		{
			if (!iterator.key().isEmpty() && !iterator.value().value(QLatin1String("actions")).isNull())
			{
				stream << QLatin1Char('[') << iterator.key() << QLatin1String("]\n");
				stream << Utils::formatConfigurationEntry(QLatin1String("title"), iterator.value().value(QLatin1String("title")).toString(), true);
				stream << QLatin1String("actions=") << iterator.value().value(QLatin1String("actions")).toString() << QLatin1Char('\n');
				stream << QLatin1String("shortcuts=") << iterator.value().value(QLatin1String("shortcuts")).toString() << QLatin1String("\n\n");
			}
		}

		file.close();
	}

	QStringList keyboardProfiles;

	for (int i = 0; i < m_ui->actionShortcutsViewWidget->getRowCount(); ++i)
	{
		const QModelIndex index = m_ui->actionShortcutsViewWidget->getIndex(i, 1);

		if (!index.data().toString().isEmpty())
		{
			keyboardProfiles.append(index.data().toString());
		}
	}

	SettingsManager::setValue(QLatin1String("Browser/KeyboardShortcutsProfilesOrder"), keyboardProfiles);

	QStringList macrosProfiles;

	for (int i = 0; i < m_ui->actionMacrosViewWidget->getRowCount(); ++i)
	{
		const QModelIndex index = m_ui->actionMacrosViewWidget->getIndex(i, 1);

		if (!index.data().toString().isEmpty())
		{
			macrosProfiles.append(index.data().toString());
		}
	}

	SettingsManager::setValue(QLatin1String("Browser/ActionMacrosProfilesOrder"), macrosProfiles);

	ActionsManager::loadProfiles();

	if (sender() == m_ui->buttonBox)
	{
		close();
	}
	else
	{
		m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	}
}

QString PreferencesDialog::createProfileIdentifier(TableViewWidget *view, QString identifier)
{
	QStringList identifiers;

	for (int i = 0; i < view->getRowCount(); ++i)
	{
		const QString profile = view->getIndex(i, 1).data().toString();

		if (!profile.isEmpty())
		{
			identifiers.append(profile);
		}
	}

	if (!identifier.isEmpty())
	{
		identifier.append(QLatin1String("-copy"));
	}

	do
	{
		identifier = QInputDialog::getText(this, tr("Select Identifier"), tr("Input Unique Profile Identifier:"), QLineEdit::Normal, identifier);

		if (identifier.isEmpty())
		{
			return QString();
		}
	}
	while (identifiers.contains(identifier));

	return identifier;
}

QString PreferencesDialog::getProfilePath(const QString &type, const QString &identifier)
{
	const QString directory = QLatin1Char('/') + type + QLatin1Char('/');
	const QString path = SessionsManager::getProfilePath() + directory + identifier + QLatin1String(".ini");

	return (QFile::exists(path) ? path : QLatin1Char(':') + directory + identifier + QLatin1String(".ini"));
}

QHash<QString, QString> PreferencesDialog::getProfileInformation(const QString &path) const
{
	QFile file(path);
	QHash<QString, QString> information;

	if (!file.open(QIODevice::ReadOnly))
	{
		return information;
	}

	const QRegularExpression expression(QLatin1String(";\\s*"));

	while (!file.atEnd())
	{
		const QString line = QString(file.readLine()).trimmed();

		if (!line.startsWith(QLatin1Char(';')))
		{
			break;
		}

		information[line.section(QLatin1Char(':'), 0, 0).remove(expression)] = line.section(QLatin1Char(':'), 1).trimmed();
	}

	return information;
}

QHash<QString, QVariantHash> PreferencesDialog::getProfileData(const QString &path) const
{
	QHash<QString, QVariantHash> data;
	QSettings settings(path, QSettings::IniFormat);
	const QStringList groups = settings.childGroups();

	for (int i = 0; i < groups.count(); ++i)
	{
		settings.beginGroup(groups.at(i));

		data[groups.at(i)] = QVariantHash();

		const QStringList keys = settings.childKeys();

		for (int j = 0; j < keys.count(); ++j)
		{
			data[groups.at(i)][keys.at(j)] = settings.value(keys.at(j));
		}

		settings.endGroup();
	}

	return data;
}

QHash<QString, QList<QKeySequence> > PreferencesDialog::getShortcuts() const
{
	QHash<QString, QList<QKeySequence> > shortcuts;

	for (int i = 0; i < m_ui->actionShortcutsViewWidget->getRowCount(); ++i)
	{
		const QModelIndex index = m_ui->actionShortcutsViewWidget->getIndex(i, 1);

		if (index.data().toString().isEmpty())
		{
			continue;
		}

		const QHash<QString, QVariantHash> data = (m_keyboardProfilesData.contains(index.data().toString()) ? m_keyboardProfilesData[index.data().toString()] : getProfileData(index.data(Qt::UserRole).toString()));
		QHash<QString, QVariantHash>::const_iterator iterator;

		for (iterator = data.constBegin(); iterator != data.constEnd(); ++iterator)
		{
			const QStringList rawShortcuts = iterator.value().value(QLatin1String("shortcuts")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
			QList<QKeySequence> actionShortcuts;

			for (int j = 0; j < rawShortcuts.count(); ++j)
			{
				const QKeySequence shortcut = ((rawShortcuts.at(j) == QLatin1String("native")) ? ActionsManager::getNativeShortcut(iterator.key()) : QKeySequence(rawShortcuts.at(j)));

				if (!shortcut.isEmpty())
				{
					actionShortcuts.append(shortcut);
				}
			}

			if (!actionShortcuts.isEmpty())
			{
				shortcuts[iterator.key()] = actionShortcuts;
			}
		}
	}

	for (int i = 0; i < m_ui->actionMacrosViewWidget->getRowCount(); ++i)
	{
		const QModelIndex index = m_ui->actionMacrosViewWidget->getIndex(i, 1);

		if (index.data().toString().isEmpty())
		{
			continue;
		}

		const QHash<QString, QVariantHash> data = (m_macrosProfilesData.contains(index.data().toString()) ? m_macrosProfilesData[index.data().toString()] : getProfileData(index.data(Qt::UserRole).toString()));
		QHash<QString, QVariantHash>::const_iterator iterator;

		for (iterator = data.constBegin(); iterator != data.constEnd(); ++iterator)
		{
			const QStringList rawShortcuts = iterator.value().value(QLatin1String("shortcuts")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
			QList<QKeySequence> actionShortcuts;

			for (int j = 0; j < rawShortcuts.count(); ++j)
			{
				const QKeySequence shortcut = ((rawShortcuts.at(j) == QLatin1String("native")) ? ActionsManager::getNativeShortcut(iterator.key()) : QKeySequence(rawShortcuts.at(j)));

				if (!shortcut.isEmpty())
				{
					actionShortcuts.append(shortcut);
				}
			}

			if (!actionShortcuts.isEmpty())
			{
				shortcuts[iterator.key()] = actionShortcuts;
			}
		}
	}

	return shortcuts;
}

}
