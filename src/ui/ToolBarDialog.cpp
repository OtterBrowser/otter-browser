/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 - 2017 Piotr Wójcik <chocimier@tlen.pl>
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

#include "ToolBarDialog.h"
#include "Menu.h"
#include "OptionWidget.h"
#include "SidebarWidget.h"
#include "../core/ActionsManager.h"
#include "../core/AddonsManager.h"
#include "../core/BookmarksManager.h"
#include "../core/BookmarksModel.h"
#include "../core/SearchEnginesManager.h"
#include "../core/ThemesManager.h"
#include "../core/TreeModel.h"

#include "ui_ToolBarDialog.h"

namespace Otter
{

ToolBarDialog::ToolBarDialog(const ToolBarsManager::ToolBarDefinition &definition, QWidget *parent) : Dialog(parent),
	m_definition(definition),
	m_ui(new Ui::ToolBarDialog)
{
	if (definition.title.isEmpty())
	{
		switch (definition.type)
		{
			case ToolBarsManager::BookmarksBarType:
				m_definition.title = tr("Bookmarks Bar");

				break;
			case ToolBarsManager::SideBarType:
				m_definition.title = tr("Sidebar");

				break;
			default:
				m_definition.title = tr("Toolbar");

				break;
		}
	}

	m_ui->setupUi(this);
	m_ui->stackedWidget->setCurrentIndex(definition.type);
	m_ui->titleLineEdit->setText(m_definition.getTitle());
	m_ui->iconSizeSpinBox->setValue(qMax(0, m_definition.iconSize));
	m_ui->maximumButtonSizeSpinBox->setValue(qMax(0, m_definition.maximumButtonSize));
	m_ui->normalVisibilityComboBox->addItem(tr("Always visible"), ToolBarsManager::AlwaysVisibleToolBar);
	m_ui->normalVisibilityComboBox->addItem(tr("Always hidden"), ToolBarsManager::AlwaysHiddenToolBar);
	m_ui->normalVisibilityComboBox->addItem(tr("Visible only when needed"), ToolBarsManager::AutoVisibilityToolBar);

	const int normalVisibilityIndex(m_ui->normalVisibilityComboBox->findData(m_definition.normalVisibility));

	m_ui->normalVisibilityComboBox->setCurrentIndex((normalVisibilityIndex < 0) ? 0 : normalVisibilityIndex);
	m_ui->fullScreenVisibilityComboBox->addItem(tr("Always visible"), ToolBarsManager::AlwaysVisibleToolBar);
	m_ui->fullScreenVisibilityComboBox->addItem(tr("Always hidden"), ToolBarsManager::AlwaysHiddenToolBar);
	m_ui->fullScreenVisibilityComboBox->addItem(tr("Visible only when needed"), ToolBarsManager::AutoVisibilityToolBar);
	m_ui->fullScreenVisibilityComboBox->addItem(tr("Visible only when cursor is close to screen edge"), ToolBarsManager::OnHoverVisibleToolBar);

	const int fullScreenVisibilityIndex(m_ui->fullScreenVisibilityComboBox->findData(m_definition.fullScreenVisibility));

	m_ui->fullScreenVisibilityComboBox->setCurrentIndex((fullScreenVisibilityIndex < 0) ? 1 : fullScreenVisibilityIndex);
	m_ui->buttonStyleComboBox->addItem(tr("Follow style"), Qt::ToolButtonFollowStyle);
	m_ui->buttonStyleComboBox->addItem(tr("Icon only"), Qt::ToolButtonIconOnly);
	m_ui->buttonStyleComboBox->addItem(tr("Text only"), Qt::ToolButtonTextOnly);
	m_ui->buttonStyleComboBox->addItem(tr("Text beside icon"), Qt::ToolButtonTextBesideIcon);
	m_ui->buttonStyleComboBox->addItem(tr("Text under icon"), Qt::ToolButtonTextUnderIcon);

	const int buttonStyleIndex(m_ui->buttonStyleComboBox->findData(m_definition.buttonStyle));

	m_ui->buttonStyleComboBox->setCurrentIndex((buttonStyleIndex < 0) ? 1 : buttonStyleIndex);
	m_ui->hasToggleCheckBox->setChecked(definition.hasToggle);
	m_ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(m_definition.canReset);

	if (definition.identifier == ToolBarsManager::MenuBar || definition.identifier == ToolBarsManager::ProgressBar || definition.identifier == ToolBarsManager::StatusBar)
	{
		m_ui->hasToggleCheckBox->hide();
	}

	connect(m_ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked()), this, SLOT(restoreDefaults()));

	switch (definition.type)
	{
		case ToolBarsManager::BookmarksBarType:
			m_ui->folderComboBox->setCurrentFolder(BookmarksManager::getModel()->getItem(definition.bookmarksPath));

			return;
		case ToolBarsManager::SideBarType:
			{
				TreeModel *panelsModel(new TreeModel(this));
				const QStringList specialPages(AddonsManager::getSpecialPages());

				for (int i = 0; i < definition.panels.count(); ++i)
				{
					QStandardItem *item(new QStandardItem(SidebarWidget::getPanelTitle(definition.panels.at(i))));
					item->setCheckable(true);
					item->setCheckState(Qt::Checked);
					item->setData(definition.panels.at(i), TreeModel::UserRole);

					panelsModel->insertRow(item);
				}

				for (int i = 0; i < specialPages.count(); ++i)
				{
					if (!definition.panels.contains(specialPages.at(i)))
					{
						QStandardItem *item(new QStandardItem(SidebarWidget::getPanelTitle(specialPages.at(i))));
						item->setCheckable(true);
						item->setData(specialPages.at(i), TreeModel::UserRole);

						panelsModel->insertRow(item);
					}
				}

				m_ui->panelsViewWidget->setModel(panelsModel);
			}

			return;
		default:
			break;
	}

	m_ui->removeButton->setIcon(ThemesManager::getIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-previous") : QLatin1String("go-next")));
	m_ui->addButton->setIcon(ThemesManager::getIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-next") : QLatin1String("go-previous")));
	m_ui->moveUpButton->setIcon(ThemesManager::getIcon(QLatin1String("go-up")));
	m_ui->moveDownButton->setIcon(ThemesManager::getIcon(QLatin1String("go-down")));
	m_ui->addEntryButton->setMenu(new QMenu(m_ui->addEntryButton));

	QStandardItemModel *availableEntriesModel(new QStandardItemModel(this));
	availableEntriesModel->appendRow(createEntry(QLatin1String("separator")));
	availableEntriesModel->appendRow(createEntry(QLatin1String("spacer")));

	const QStringList widgets({QLatin1String("CustomMenu"), QLatin1String("ClosedWindowsMenu"), QLatin1String("AddressWidget"), QLatin1String("ConfigurationOptionWidget"), QLatin1String("ContentBlockingInformationWidget"), QLatin1String("MenuButtonWidget"), QLatin1String("PanelChooserWidget"), QLatin1String("SearchWidget"), QLatin1String("StatusMessageWidget"), QLatin1String("ZoomWidget")});

	for (int i = 0; i < widgets.count(); ++i)
	{
		availableEntriesModel->appendRow(createEntry(widgets.at(i)));

		if (widgets.at(i) == QLatin1String("SearchWidget"))
		{
			const QStringList searchEngines(SearchEnginesManager::getSearchEngines());

			for (int j = 0; j < searchEngines.count(); ++j)
			{
				QVariantMap options;
				options[QLatin1String("searchEngine")] = searchEngines.at(j);

				availableEntriesModel->appendRow(createEntry(widgets.at(i), options));
			}
		}
	}

	const QVector<ActionsManager::ActionDefinition> actions(ActionsManager::getActionDefinitions());

	for (int i = 0; i < actions.count(); ++i)
	{
		QStandardItem *item(new QStandardItem(QCoreApplication::translate("actions", (actions.at(i).description.isEmpty() ? actions.at(i).text : actions.at(i).description).toUtf8().constData())));
		item->setData(QColor(Qt::transparent), Qt::DecorationRole);
		item->setData(ActionsManager::getActionName(actions.at(i).identifier) + QLatin1String("Action"), IdentifierRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);
		item->setToolTip(item->text());

		if (!actions.at(i).icon.isNull())
		{
			item->setIcon(actions.at(i).icon);
		}

		availableEntriesModel->appendRow(item);
	}

	m_ui->availableEntriesItemView->setModel(availableEntriesModel);
	m_ui->availableEntriesItemView->viewport()->installEventFilter(this);
	m_ui->currentEntriesItemView->setModel(new QStandardItemModel(this));
	m_ui->currentEntriesItemView->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->currentEntriesItemView->setRootIsDecorated(false);

	for (int i = 0; i < m_definition.entries.count(); ++i)
	{
		addEntry(m_definition.entries.at(i));
	}

	m_definition.entries.clear();

	Menu *bookmarksMenu(new Menu(Menu::BookmarkSelectorMenuRole, m_ui->addEntryButton));

	m_ui->addEntryButton->setMenu(bookmarksMenu);
	m_ui->addEntryButton->setEnabled(BookmarksManager::getModel()->getRootItem()->rowCount() > 0);

	connect(bookmarksMenu, SIGNAL(triggered(QAction*)), this, SLOT(addBookmark(QAction*)));
	connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
	connect(m_ui->removeButton, SIGNAL(clicked()), m_ui->currentEntriesItemView, SLOT(removeRow()));
	connect(m_ui->moveDownButton, SIGNAL(clicked()), m_ui->currentEntriesItemView, SLOT(moveDownRow()));
	connect(m_ui->moveUpButton, SIGNAL(clicked()), m_ui->currentEntriesItemView, SLOT(moveUpRow()));
	connect(m_ui->availableEntriesItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateActions()));
	connect(m_ui->currentEntriesItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateActions()));
	connect(m_ui->currentEntriesItemView, SIGNAL(canMoveDownChanged(bool)), m_ui->moveDownButton, SLOT(setEnabled(bool)));
	connect(m_ui->currentEntriesItemView, SIGNAL(canMoveUpChanged(bool)), m_ui->moveUpButton, SLOT(setEnabled(bool)));
	connect(m_ui->availableEntriesFilterLineEdit, SIGNAL(textChanged(QString)), m_ui->availableEntriesItemView, SLOT(setFilterString(QString)));
	connect(m_ui->currentEntriesFilterLineEdit, SIGNAL(textChanged(QString)), m_ui->currentEntriesItemView, SLOT(setFilterString(QString)));
	connect(m_ui->editEntryButton, SIGNAL(clicked(bool)), this, SLOT(editEntry()));
}

ToolBarDialog::~ToolBarDialog()
{
	delete m_ui;
}

void ToolBarDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void ToolBarDialog::addEntry(const ActionsManager::ActionEntryDefinition &entry, QStandardItem *parent)
{
	QStandardItem *item(createEntry(entry.action, entry.options));

	if (parent)
	{
		parent->appendRow(item);
	}
	else
	{
		m_ui->currentEntriesItemView->getSourceModel()->appendRow(item);
	}

	if (entry.action == QLatin1String("CustomMenu"))
	{
		for (int i = 0; i < entry.entries.count(); ++i)
		{
			addEntry(entry.entries.at(i), item);
		}

		m_ui->currentEntriesItemView->expand(item->index());
	}
}

void ToolBarDialog::addEntry()
{
	QStandardItem *sourceItem(m_ui->availableEntriesItemView->getItem(m_ui->availableEntriesItemView->getCurrentRow()));

	if (sourceItem)
	{
		QStandardItem *newItem(sourceItem->clone());
		QStandardItem *targetItem(m_ui->currentEntriesItemView->getItem(m_ui->currentEntriesItemView->currentIndex()));

		if (targetItem && targetItem->data(IdentifierRole).toString() != QLatin1String("CustomMenu"))
		{
			targetItem = targetItem->parent();
		}

		if (targetItem && targetItem->data(IdentifierRole).toString() == QLatin1String("CustomMenu"))
		{
			int row(-1);

			if (targetItem->index() != m_ui->currentEntriesItemView->currentIndex())
			{
				row = m_ui->currentEntriesItemView->currentIndex().row();
			}

			if (row >= 0)
			{
				targetItem->insertRow((row + 1), newItem);
			}
			else
			{
				targetItem->appendRow(newItem);
			}

			m_ui->currentEntriesItemView->setCurrentIndex(newItem->index());
			m_ui->currentEntriesItemView->expand(targetItem->index());
		}
		else
		{
			m_ui->currentEntriesItemView->insertRow(newItem);
		}
	}
}

void ToolBarDialog::editEntry()
{
	const QModelIndex index(m_ui->currentEntriesItemView->currentIndex());
	const QString identifier(index.data(IdentifierRole).toString());
	QVariantMap options(index.data(OptionsRole).toMap());
	QVector<QPair<QString, OptionWidget*> > widgets;

	if (identifier == QLatin1String("SearchWidget"))
	{
		const QStringList searchEngines(SearchEnginesManager::getSearchEngines());
		QVector<SettingsManager::OptionDefinition::ChoiceDefinition> searchEngineChoices{{tr("All"), QString(), QIcon()}, SettingsManager::OptionDefinition::ChoiceDefinition()};
		OptionWidget *searchEngineWidget(new OptionWidget(QLatin1String("searchEngine"), options.value(QLatin1String("searchEngine")), SettingsManager::EnumerationType, this));

		for (int i = 0; i < searchEngines.count(); ++i)
		{
			const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(searchEngines.at(i)));

			searchEngineChoices.append({(searchEngine.title.isEmpty() ? tr("Unknown") : searchEngine.title), searchEngines.at(i), searchEngine.icon});
		}

		searchEngineWidget->setChoices(searchEngineChoices);

		widgets.append({tr("Show search engine:"), searchEngineWidget});
		widgets.append({tr("Show search button:"), new OptionWidget(QLatin1String("showSearchButton"), options.value(QLatin1String("showSearchButton"), true), SettingsManager::BooleanType, this)});
	}
	else if (identifier == QLatin1String("ConfigurationOptionWidget") || identifier == QLatin1String("ContentBlockingInformationWidget") || identifier == QLatin1String("MenuButtonWidget") || identifier.startsWith(QLatin1String("bookmarks:")) || identifier.endsWith(QLatin1String("Action")) || identifier.endsWith(QLatin1String("Menu")))
	{
		OptionWidget *iconOptionWidget(new OptionWidget(QLatin1String("icon"), QVariant(), SettingsManager::IconType, this));
		OptionWidget *textOptionWidget(new OptionWidget(QLatin1String("text"), QVariant(), SettingsManager::StringType, this));

		if (identifier == QLatin1String("ClosedWindowsMenu"))
		{
			iconOptionWidget->setDefaultValue(ThemesManager::getIcon(QLatin1String("user-trash")));
		}
		else if (identifier == QLatin1String("ConfigurationOptionWidget"))
		{
			const QStringList choices(SettingsManager::getOptions());
			OptionWidget *optionNameWidget(new OptionWidget(QLatin1String("optionName"), options.value(QLatin1String("optionName")), SettingsManager::EnumerationType, this));
			optionNameWidget->setChoices(choices);

			widgets.append({tr("Option:"), optionNameWidget});

			OptionWidget *scopeWidget(new OptionWidget(QLatin1String("scope"), options.value(QLatin1String("scope")), SettingsManager::BooleanType, this));
			scopeWidget->setChoices(QVector<SettingsManager::OptionDefinition::ChoiceDefinition>{{tr("Global"), QLatin1String("global"), QIcon()}, {tr("Tab"), QLatin1String("window"), QIcon()}});
			scopeWidget->setDefaultValue(QLatin1String("window"));

			widgets.append({tr("Scope:"), scopeWidget});

			textOptionWidget->setDefaultValue(options.value(QLatin1String("optionName"), choices.first()).toString().section(QLatin1Char('/'), -1));

			connect(optionNameWidget, &OptionWidget::commitData, [&]()
			{
				textOptionWidget->setDefaultValue(optionNameWidget->getValue().toString().section(QLatin1Char('/'), -1));
			});
		}
		else if (identifier == QLatin1String("ContentBlockingInformationWidget"))
		{
			iconOptionWidget->setDefaultValue(ThemesManager::getIcon(QLatin1String("content-blocking")));
			textOptionWidget->setDefaultValue(tr("Blocked Elements: {amount}"));
		}
		else if (identifier == QLatin1String("MenuButtonWidget"))
		{
			iconOptionWidget->setDefaultValue(ThemesManager::getIcon(QLatin1String("otter-browser"), false));
			textOptionWidget->setDefaultValue(tr("Menu"));
		}
		else if (identifier.startsWith(QLatin1String("bookmarks:")))
		{
			BookmarksItem *bookmark(identifier.startsWith(QLatin1String("bookmarks:/")) ? BookmarksManager::getModel()->getItem(identifier.mid(11)) : BookmarksManager::getBookmark(identifier.mid(10).toULongLong()));

			if (bookmark)
			{
				iconOptionWidget->setDefaultValue(bookmark->data(Qt::DecorationRole).value<QIcon>());
				textOptionWidget->setDefaultValue(bookmark->data(BookmarksModel::TitleRole).isValid() ? bookmark->data(BookmarksModel::TitleRole).toString() : tr("(Untitled)"));
			}
		}
		else if (identifier.endsWith(QLatin1String("Action")))
		{
			const int actionIdentifier(ActionsManager::getActionIdentifier(identifier.left(identifier.length() - 6)));

			if (actionIdentifier >= 0)
			{
				const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(actionIdentifier));

				iconOptionWidget->setDefaultValue(definition.icon);
				textOptionWidget->setDefaultValue(QCoreApplication::translate("actions", (definition.description.isEmpty() ? definition.text : definition.description).toUtf8().constData()));
			}
		}

		iconOptionWidget->setValue(options.value(QLatin1String("icon"), iconOptionWidget->getDefaultValue()));
		textOptionWidget->setValue(options.value(QLatin1String("text"), textOptionWidget->getDefaultValue()));
		textOptionWidget->setButtons(textOptionWidget->getDefaultValue().isNull() ? OptionWidget::NoButtons : OptionWidget::ResetButton);

		widgets.append({tr("Icon:"), iconOptionWidget});
		widgets.append({tr("Text:"), textOptionWidget});
	}

	if (widgets.isEmpty())
	{
		return;
	}

	QDialog dialog(this);
	dialog.setWindowTitle(tr("Edit Entry"));

	QDialogButtonBox *buttonBox(new QDialogButtonBox(&dialog));
	buttonBox->addButton(QDialogButtonBox::Cancel);
	buttonBox->addButton(QDialogButtonBox::Ok);

	QFormLayout *formLayout(new QFormLayout());
	QVBoxLayout *mainLayout(new QVBoxLayout(&dialog));
	mainLayout->addLayout(formLayout);
	mainLayout->addWidget(buttonBox);

	for (int i = 0; i < widgets.count(); ++i)
	{
		formLayout->addRow(widgets.at(i).first, widgets.at(i).second);
	}

	connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	for (int i = 0; i < widgets.count(); ++i)
	{
		if (widgets.at(i).second->getValue() == widgets.at(i).second->getDefaultValue())
		{
			options.remove(widgets.at(i).second->getOption());
		}
		else
		{
			options[widgets.at(i).second->getOption()] = widgets.at(i).second->getValue();
		}
	}

	QStandardItem *item(createEntry(identifier, options));

	m_ui->currentEntriesItemView->setData(index, item->text(), Qt::DisplayRole);
	m_ui->currentEntriesItemView->setData(index, item->text(), Qt::ToolTipRole);
	m_ui->currentEntriesItemView->setData(index, item->icon(), Qt::DecorationRole);
	m_ui->currentEntriesItemView->setData(index, options, OptionsRole);

	delete item;
}

void ToolBarDialog::addBookmark(QAction *action)
{
	if (action && action->data().type() == QVariant::ModelIndex)
	{
		m_ui->currentEntriesItemView->insertRow(createEntry(QLatin1String("bookmarks:") + QString::number(action->data().toModelIndex().data(BookmarksModel::IdentifierRole).toULongLong())));
	}
}

void ToolBarDialog::restoreDefaults()
{
	ToolBarsManager::getInstance()->resetToolBar(m_definition.identifier);

	reject();
}

void ToolBarDialog::updateActions()
{
	const QString sourceIdentifier(m_ui->availableEntriesItemView->currentIndex().data(IdentifierRole).toString());
	const QString targetIdentifier(m_ui->currentEntriesItemView->currentIndex().data(IdentifierRole).toString());

	m_ui->addButton->setEnabled(!sourceIdentifier.isEmpty() && (!(m_ui->currentEntriesItemView->currentIndex().data(IdentifierRole).toString() == QLatin1String("CustomMenu") || m_ui->currentEntriesItemView->currentIndex().parent().data(IdentifierRole).toString() == QLatin1String("CustomMenu")) || (sourceIdentifier == QLatin1String("separator") || sourceIdentifier.endsWith(QLatin1String("Action")) || sourceIdentifier.endsWith(QLatin1String("Menu")))));
	m_ui->removeButton->setEnabled(m_ui->currentEntriesItemView->currentIndex().isValid() && targetIdentifier != QLatin1String("MenuBarWidget") && targetIdentifier != QLatin1String("TabBarWidget"));
	m_ui->editEntryButton->setEnabled(targetIdentifier == QLatin1String("ConfigurationOptionWidget") || targetIdentifier == QLatin1String("ContentBlockingInformationWidget") || targetIdentifier == QLatin1String("MenuButtonWidget") || targetIdentifier == QLatin1String("PanelChooserWidget") || targetIdentifier == QLatin1String("SearchWidget") || targetIdentifier.startsWith(QLatin1String("bookmarks:")) || targetIdentifier.endsWith(QLatin1String("Action")) || targetIdentifier.endsWith(QLatin1String("Menu")));
}

QStandardItem* ToolBarDialog::createEntry(const QString &identifier, const QVariantMap &options)
{
	QStandardItem *item(new QStandardItem());
	item->setData(QColor(Qt::transparent), Qt::DecorationRole);
	item->setData(identifier, IdentifierRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

	if (identifier == QLatin1String("separator"))
	{
		item->setText(tr("--- separator ---"));
	}
	else if (identifier == QLatin1String("spacer"))
	{
		item->setText(tr("--- spacer ---"));
	}
	else if (identifier == QLatin1String("CustomMenu"))
	{
		item->setText(tr("Arbitrary List of Actions"));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

		m_ui->currentEntriesItemView->setRootIsDecorated(true);
	}
	else if (identifier == QLatin1String("ClosedWindowsMenu"))
	{
		item->setText(tr("List of Closed Tabs and Windows"));
	}
	else if (identifier == QLatin1String("AddressWidget"))
	{
		item->setText(tr("Address Field"));
	}
	else if (identifier == QLatin1String("ConfigurationOptionWidget"))
	{
		if (options.contains(QLatin1String("optionName")) && !options.value("optionName").toString().isEmpty())
		{
			item->setText(tr("Configuration Widget (%1)").arg(options.value("optionName").toString()));
		}
		else
		{
			item->setText(tr("Configuration Widget"));
		}
	}
	else if (identifier == QLatin1String("ContentBlockingInformationWidget"))
	{
		item->setText(tr("Content Blocking Details"));
	}
	else if (identifier == QLatin1String("MenuBarWidget"))
	{
		item->setText(tr("Menu Bar"));
	}
	else if (identifier == QLatin1String("MenuButtonWidget"))
	{
		item->setText(tr("Menu Button"));
	}
	else if (identifier == QLatin1String("PanelChooserWidget"))
	{
		item->setText(tr("Sidebar Panel Chooser"));
	}
	else if (identifier == QLatin1String("ProgressInformationDocumentProgressWidget"))
	{
		item->setText(tr("Progress Information (Document Progress)"));
	}
	else if (identifier == QLatin1String("ProgressInformationTotalSizeWidget"))
	{
		item->setText(tr("Progress Information (Total Progress)"));
	}
	else if (identifier == QLatin1String("ProgressInformationElementsWidget"))
	{
		item->setText(tr("Progress Information (Loaded Elements)"));
	}
	else if (identifier == QLatin1String("ProgressInformationSpeedWidget"))
	{
		item->setText(tr("Progress Information (Loading Speed)"));
	}
	else if (identifier == QLatin1String("ProgressInformationElapsedTimeWidget"))
	{
		item->setText(tr("Progress Information (Elapsed Time)"));
	}
	else if (identifier == QLatin1String("ProgressInformationMessageWidget"))
	{
		item->setText(tr("Progress Information (Status Message)"));
	}
	else if (identifier == QLatin1String("SearchWidget"))
	{
		if (options.contains(QLatin1String("searchEngine")))
		{
			const SearchEnginesManager::SearchEngineDefinition definition(SearchEnginesManager::getSearchEngine(options[QLatin1String("searchEngine")].toString()));

			item->setText(tr("Search Field (%1)").arg(definition.title.isEmpty() ? tr("Unknown") : definition.title));

			if (!definition.icon.isNull())
			{
				item->setIcon(definition.icon);
			}
		}
		else
		{
			item->setText(tr("Search Field"));
		}
	}
	else if (identifier == QLatin1String("StatusMessageWidget"))
	{
		item->setText(tr("Status Message Field"));
	}
	else if (identifier == QLatin1String("TabBarWidget"))
	{
		item->setText(tr("Tab Bar"));
	}
	else if (identifier == QLatin1String("ZoomWidget"))
	{
		item->setText(tr("Zoom Slider"));
	}
	else if (identifier.startsWith(QLatin1String("bookmarks:")))
	{
		BookmarksItem *bookmark(identifier.startsWith(QLatin1String("bookmarks:/")) ? BookmarksManager::getModel()->getItem(identifier.mid(11)) : BookmarksManager::getBookmark(identifier.mid(10).toULongLong()));

		if (bookmark)
		{
			const QIcon icon(bookmark->data(Qt::DecorationRole).value<QIcon>());

			item->setText(bookmark->data(BookmarksModel::TitleRole).isValid() ? bookmark->data(BookmarksModel::TitleRole).toString() : tr("(Untitled)"));

			if (!icon.isNull())
			{
				item->setIcon(icon);
			}
		}
		else
		{
			item->setText(tr("Invalid Bookmark"));
		}
	}
	else if (identifier.endsWith(QLatin1String("Action")))
	{
		const int actionIdentifier(ActionsManager::getActionIdentifier(identifier.left(identifier.length() - 6)));

		if (actionIdentifier < 0)
		{
			item->setText(tr("Invalid Entry"));
		}
		else
		{
			const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(actionIdentifier));

			item->setText(QCoreApplication::translate("actions", (definition.description.isEmpty() ? definition.text : definition.description).toUtf8().constData()));

			if (!definition.icon.isNull())
			{
				item->setIcon(definition.icon);
			}
		}
	}
	else
	{
		item->setText(tr("Invalid Entry"));
	}

	if (!options.isEmpty())
	{
		item->setData(options, OptionsRole);

		if (options.contains(QLatin1String("icon")))
		{
			const QString data(options[QLatin1String("icon")].toString());
			QIcon icon;

			if (data.startsWith(QLatin1String("data:image/")))
			{
				icon = QIcon(QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8()))));
			}
			else
			{
				icon = ThemesManager::getIcon(data);
			}

			if (data.isEmpty())
			{
				item->setData(QColor(Qt::transparent), Qt::DecorationRole);
			}
			else if (!icon.isNull())
			{
				item->setIcon(icon);
			}
		}

		if (options.contains(QLatin1String("text")))
		{
			item->setText(options[QLatin1String("text")].toString());
		}
	}

	item->setToolTip(item->text());

	return item;
}

ActionsManager::ActionEntryDefinition ToolBarDialog::getEntry(QStandardItem *item) const
{
	ActionsManager::ActionEntryDefinition definition;

	if (!item)
	{
		return definition;
	}

	definition.action = item->data(IdentifierRole).toString();
	definition.options = item->data(OptionsRole).toMap();

	if (definition.action == QLatin1String("CustomMenu"))
	{
		definition.entries.reserve(item->rowCount());

		for (int i = 0; i < item->rowCount(); ++i)
		{
			definition.entries.append(getEntry(item->child(i, 0)));
		}
	}

	return definition;
}

ToolBarsManager::ToolBarDefinition ToolBarDialog::getDefinition() const
{
	ToolBarsManager::ToolBarDefinition definition(m_definition);
	definition.title = m_ui->titleLineEdit->text();
	definition.normalVisibility = static_cast<ToolBarsManager::ToolBarVisibility>(m_ui->normalVisibilityComboBox->currentData().toInt());
	definition.fullScreenVisibility = static_cast<ToolBarsManager::ToolBarVisibility>(m_ui->fullScreenVisibilityComboBox->currentData().toInt());
	definition.buttonStyle = static_cast<Qt::ToolButtonStyle>(m_ui->buttonStyleComboBox->currentData().toInt());
	definition.iconSize = m_ui->iconSizeSpinBox->value();
	definition.maximumButtonSize = m_ui->maximumButtonSizeSpinBox->value();
	definition.hasToggle = m_ui->hasToggleCheckBox->isChecked();

	switch (definition.type)
	{
		case ToolBarsManager::BookmarksBarType:
			definition.bookmarksPath = QLatin1Char('#') + QString::number(m_ui->folderComboBox->getCurrentFolder()->data(BookmarksModel::IdentifierRole).toULongLong());

			break;
		case ToolBarsManager::SideBarType:
			definition.currentPanel = m_definition.currentPanel;
			definition.panels.clear();

			for (int i = 0; i < m_ui->panelsViewWidget->model()->rowCount(); ++i)
			{
				QStandardItem *item(m_ui->panelsViewWidget->getItem(i));

				if (item->data(Qt::CheckStateRole).toInt() == Qt::Checked)
				{
					definition.panels.append(item->data(TreeModel::UserRole).toString());
				}
			}

			break;
		default:
			definition.entries.reserve(m_ui->currentEntriesItemView->model()->rowCount());

			for (int i = 0; i < m_ui->currentEntriesItemView->model()->rowCount(); ++i)
			{
				definition.entries.append(getEntry(m_ui->currentEntriesItemView->getItem(i)));
			}

			break;
	}

	return definition;
}

bool ToolBarDialog::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->availableEntriesItemView->viewport() && event->type() == QEvent::Drop)
	{
		event->accept();

		return true;
	}

	return Dialog::eventFilter(object, event);;
}

}
