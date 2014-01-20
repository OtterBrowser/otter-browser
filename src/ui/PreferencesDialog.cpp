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
#include "preferences/ShortcutDelegate.h"
#include "../core/FileSystemCompleterModel.h"
#include "../core/SettingsManager.h"
#include "../core/SearchesManager.h"
#include "../core/Utils.h"

#include "ui_PreferencesDialog.h"

#include <QtWidgets/QCompleter>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>

namespace Otter
{

PreferencesDialog::PreferencesDialog(const QLatin1String &section, QWidget *parent) : QDialog(parent),
	m_defaultSearch(SettingsManager::getValue("Browser/DefaultSearchEngine").toString()),
	m_clearSettings(SettingsManager::getValue(QLatin1String("History/ClearOnClose")).toStringList()),
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

	m_ui->downloadsLineEdit->setCompleter(new QCompleter(new FileSystemCompleterModel(this), this));
	m_ui->downloadsLineEdit->setText(SettingsManager::getValue(QLatin1String("Paths/Downloads")).toString());
	m_ui->alwaysAskCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/AlwaysAskWhereToSaveDownload")).toBool());
	m_ui->tabsInsteadOfWindowsCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/OpenLinksInNewTab")).toBool());
	m_ui->openNextToActiveheckBox->setChecked(SettingsManager::getValue(QLatin1String("TabBar/OpenNextToActive")).toBool());

	m_ui->defaultZoomSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/DefaultZoom")).toInt());
	m_ui->zoomTextOnlyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Content/ZoomTextOnly")).toBool());
	m_ui->proportionalFontSizeSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/DefaultFontSize")).toInt());
	m_ui->fixedFontSizeSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/DefaultFixedFontSize")).toInt());
	m_ui->minimumFontSizeSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/MinimumFontSize")).toInt());

	QList<QLatin1String> fonts;
	fonts << QLatin1String("StandardFont") << QLatin1String("FixedFont") << QLatin1String("SerifFont") << QLatin1String("SansSerifFont") << QLatin1String("CursiveFont") << QLatin1String("FantasyFont");

	QStringList fontCategories;
	fontCategories << tr("Standard Font") << tr("Fixed Font") << tr("Serif Font") << tr("Sans Serif Font") << tr("Cursive Font") << tr("Fantasy Font");

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

	const QString doNotTrackPolicyString = SettingsManager::getValue(QLatin1String("Browser/DoNotTrackPolicy")).toString();
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
	m_ui->searchViewWidget->setItemDelegateForColumn(1, new ShortcutDelegate(this));
	m_ui->searchSuggestionsCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/SearchEnginesSuggestions")).toBool());
	m_ui->moveDownSearchButton->setIcon(Utils::getIcon(QLatin1String("arrow-down")));
	m_ui->moveUpSearchButton->setIcon(Utils::getIcon(QLatin1String("arrow-up")));

	m_ui->suggestBookmarksCheckBox->setChecked(SettingsManager::getValue(QLatin1String("AddressField/SuggestBookmarks")).toBool());

	m_ui->actionShortcutsMoveDownButton->setIcon(Utils::getIcon(QLatin1String("arrow-down")));
	m_ui->actionShortcutsMoveUpButton->setIcon(Utils::getIcon(QLatin1String("arrow-up")));
	m_ui->actionMacrosMoveDownButton->setIcon(Utils::getIcon(QLatin1String("arrow-down")));
	m_ui->actionMacrosMoveUpButton->setIcon(Utils::getIcon(QLatin1String("arrow-up")));

	const QString proxyString = SettingsManager::getValue(QLatin1String("Proxy/ProxyMode")).toString();
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
	if (proxyIndex == 2)
	{
		proxyModeChanged(proxyIndex);
	}

	m_ui->allProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseCommon")).toBool());
	m_ui->httpProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseHttp")).toBool());
	m_ui->httpsProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseHttps")).toBool());
	m_ui->ftpProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseFtp")).toBool());
	m_ui->socksProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseSocks")).toBool());
	m_ui->allProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/CommonAddress")).toString());
	m_ui->httpProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/HttpAddress")).toString());
	m_ui->httpsProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/HttpsAddress")).toString());
	m_ui->ftpProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/FtpAddress")).toString());
	m_ui->socksProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/SocksAddress")).toString());
	m_ui->allProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/CommonPort")).toInt());
	m_ui->httpProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/HttpPort")).toInt());
	m_ui->httpsProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/HttpsPort")).toInt());
	m_ui->ftpProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/FtpPort")).toInt());
	m_ui->socksProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/SocksPort")).toInt());

	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(save()));
	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
	connect(m_ui->downloadsBrowseButton, SIGNAL(clicked()), this, SLOT(browseDownloadsPath()));
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
	connect(m_ui->addSearchButton, SIGNAL(clicked()), this, SLOT(addSearch()));
	connect(m_ui->editSearchButton, SIGNAL(clicked()), this, SLOT(editSearch()));
	connect(m_ui->removeSearchButton, SIGNAL(clicked()), m_ui->searchViewWidget, SLOT(removeRow()));
	connect(m_ui->moveDownSearchButton, SIGNAL(clicked()), m_ui->searchViewWidget, SLOT(moveDownRow()));
	connect(m_ui->moveUpSearchButton, SIGNAL(clicked()), m_ui->searchViewWidget, SLOT(moveUpRow()));
	connect(m_ui->advancedListWidget, SIGNAL(currentRowChanged(int)), m_ui->advancedStackedWidget, SLOT(setCurrentIndex(int)));
	connect(m_ui->proxyModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(proxyModeChanged(int)));
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

void PreferencesDialog::browseDownloadsPath()
{
	const QString path = QFileDialog::getExistingDirectory(this, tr("Select Directory"), m_ui->downloadsLineEdit->text());

	if (!path.isEmpty())
	{
		m_ui->downloadsLineEdit->setText(path);
	}
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
}

void PreferencesDialog::setupClearHistory()
{
	ClearHistoryDialog dialog(m_clearSettings, true, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		m_clearSettings = dialog.getClearSettings();
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
	items.append(new QStandardItem(engineData[QLatin1String("shortcut")].toString()));

	m_ui->searchViewWidget->insertRow(items);
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
		const QString shortcut = m_ui->searchViewWidget->getIndex(i, 0).data(Qt::DisplayRole).toString();

		if (m_ui->searchViewWidget->getCurrentRow() != i && !shortcut.isEmpty())
		{
			shortcuts.append(shortcut);
		}
	}

	SearchPropertiesDialog dialog(engineData, shortcuts, this);

	if (dialog.exec() == QDialog::Accepted)
	{
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
	}
}

void PreferencesDialog::updateSearchActions()
{
	const int currentRow = m_ui->searchViewWidget->getCurrentRow();
	const bool isSelected = (currentRow >= 0 && currentRow < m_ui->searchViewWidget->getRowCount());

	m_ui->editSearchButton->setEnabled(isSelected);
	m_ui->removeSearchButton->setEnabled(isSelected);
}

void PreferencesDialog::save()
{
	SettingsManager::setValue(QLatin1String("Paths/Downloads"), m_ui->downloadsLineEdit->text());
	SettingsManager::setValue(QLatin1String("Browser/AlwaysAskWhereSaveFile"), m_ui->alwaysAskCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Browser/OpenLinksInNewTab"), m_ui->tabsInsteadOfWindowsCheckBox->isChecked());
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

	SettingsManager::setValue(QLatin1String("Browser/DoNotTrackPolicy"), doNotTrackPolicyString);
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
		SettingsManager::setValue(QLatin1String("Browser/DefaultSearchEngine"), m_defaultSearch);
	}

	SettingsManager::setValue(QLatin1String("Browser/SearchEnginesSuggestions"), m_ui->searchSuggestionsCheckBox->isChecked());

	SettingsManager::setValue(QLatin1String("AddressField/SuggestBookmarks"), m_ui->suggestBookmarksCheckBox->isChecked());

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

	SettingsManager::setValue(QLatin1String("Proxy/ProxyMode"), proxyString);

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

	SettingsManager::setValue(QLatin1String("Proxy/CommonAddress"), m_ui->allProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/HttpAddress"), m_ui->httpProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/HttpsAddress"), m_ui->httpsProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/FtpAddress"), m_ui->ftpProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/SocksAddress"), m_ui->socksProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/CommonPort"), m_ui->allProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/HttpPort"), m_ui->httpProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/HttpsPort"), m_ui->httpsProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/FtpPort"), m_ui->ftpProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/SocksPort"), m_ui->socksProxyPortSpinBox->value());

	close();
}

}
