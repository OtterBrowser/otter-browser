/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../core/ItemModel.h"
#include "../core/SearchEnginesManager.h"
#include "../core/ThemesManager.h"

#include "ui_ToolBarDialog.h"

namespace Otter
{

ToolBarDialog::ToolBarDialog(const ToolBarsManager::ToolBarDefinition &definition, QWidget *parent) : Dialog(parent),
	m_currentEntriesModel(new QStandardItemModel(this)),
	m_definition(definition),
	m_ui(new Ui::ToolBarDialog)
{
	m_ui->setupUi(this);
	m_ui->stackedWidget->setCurrentIndex(definition.type);
	m_ui->titleLineEditWidget->setText(m_definition.getTitle());
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
	m_ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(m_definition.canReset());

	if (definition.identifier == ToolBarsManager::MenuBar || definition.identifier == ToolBarsManager::ProgressBar || definition.identifier == ToolBarsManager::StatusBar)
	{
		m_ui->hasToggleCheckBox->hide();
	}

	connect(m_ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &ToolBarDialog::restoreDefaults);

	switch (definition.type)
	{
		case ToolBarsManager::BookmarksBarType:
			setWindowTitle(tr("Edit Bookmarks Bar"));

			if (definition.title.isEmpty())
			{
				m_definition.title = tr("Bookmarks Bar");
			}

			m_ui->folderComboBox->setCurrentFolder(BookmarksManager::getModel()->getBookmarkByPath(definition.bookmarksPath));

			connect(m_ui->newFolderButton, &QPushButton::clicked, m_ui->folderComboBox, &BookmarksComboBoxWidget::createFolder);

			return;
		case ToolBarsManager::SideBarType:
			{
				setWindowTitle(tr("Edit Sidebar"));

				if (definition.title.isEmpty())
				{
					m_definition.title = tr("Sidebar");
				}

				ItemModel *panelsModel(new ItemModel(this));
				const QStringList specialPages(AddonsManager::getSpecialPages(AddonsManager::SpecialPageInformation::SidebarPanelType));

				for (int i = 0; i < specialPages.count(); ++i)
				{
					const QString specialPage(specialPages.at(i));
					ItemModel::Item *item(new ItemModel::Item(SidebarWidget::getPanelTitle(specialPage)));
					item->setCheckable(true);
					item->setCheckState(definition.panels.contains(specialPage) ? Qt::Checked : Qt::Unchecked);
					item->setData(specialPage, ItemModel::UserRole);

					panelsModel->insertRow(item);
				}

				for (int i = 0; i < definition.panels.count(); ++i)
				{
					const QString panel(definition.panels.at(i));

					if (!specialPages.contains(panel))
					{
						ItemModel::Item *item(new ItemModel::Item(SidebarWidget::getPanelTitle(panel)));
						item->setCheckable(true);
						item->setCheckState(Qt::Checked);
						item->setData(panel, ItemModel::UserRole);

						panelsModel->insertRow(item);
					}
				}

				m_ui->panelsViewWidget->setModel(panelsModel);
			}

			return;
		default:
			setWindowTitle(tr("Edit Toolbar"));

			if (definition.title.isEmpty())
			{
				m_definition.title = tr("Toolbar");
			}

			break;
	}

	m_ui->removeButton->setIcon(ThemesManager::createIcon(isLeftToRight() ? QLatin1String("go-previous") : QLatin1String("go-next")));
	m_ui->addButton->setIcon(ThemesManager::createIcon(isLeftToRight() ? QLatin1String("go-next") : QLatin1String("go-previous")));
	m_ui->moveUpButton->setIcon(ThemesManager::createIcon(QLatin1String("go-up")));
	m_ui->moveDownButton->setIcon(ThemesManager::createIcon(QLatin1String("go-down")));
	m_ui->addBookmarkButton->setMenu(new QMenu(m_ui->addBookmarkButton));

	QStandardItemModel *availableEntriesModel(new QStandardItemModel(this));
	availableEntriesModel->appendRow(createEntry(QLatin1String("separator")));
	availableEntriesModel->appendRow(createEntry(QLatin1String("spacer")));

	const QStringList widgets({QLatin1String("CustomMenu"), QLatin1String("ClosedWindowsMenu"), QLatin1String("AddressWidget"), QLatin1String("ConfigurationOptionWidget"), QLatin1String("ContentBlockingInformationWidget"), QLatin1String("MenuButtonWidget"), QLatin1String("PanelChooserWidget"), QLatin1String("PrivateWindowIndicatorWidget"), QLatin1String("SearchWidget"), QLatin1String("SizeGripWidget"), QLatin1String("StatusMessageWidget"), QLatin1String("TransfersWidget"), QLatin1String("ZoomWidget")});

	for (int i = 0; i < widgets.count(); ++i)
	{
		const QString widget(widgets.at(i));

		availableEntriesModel->appendRow(createEntry(widget));

		if (widget == QLatin1String("SearchWidget"))
		{
			const QStringList searchEngines(SearchEnginesManager::getSearchEngines());

			for (int j = 0; j < searchEngines.count(); ++j)
			{
				availableEntriesModel->appendRow(createEntry(widget, {{QLatin1String("searchEngine"), searchEngines.at(j)}}));
			}
		}
	}

	const QVector<ActionsManager::ActionDefinition> actions(ActionsManager::getActionDefinitions());

	for (int i = 0; i < actions.count(); ++i)
	{
		const ActionsManager::ActionDefinition action(actions.at(i));

		if (action.flags.testFlag(ActionsManager::ActionDefinition::IsDeprecatedFlag) || action.flags.testFlag(ActionsManager::ActionDefinition::RequiresParameters))
		{
			continue;
		}

		const QString name(ActionsManager::getActionName(action.identifier) + QLatin1String("Action"));
		QStandardItem *item(new QStandardItem(action.getText(true)));
		item->setData(ItemModel::createDecoration(action.defaultState.icon), Qt::DecorationRole);
		item->setData(name, IdentifierRole);
		item->setData(true, HasOptionsRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);
		item->setToolTip(QStringLiteral("%1 (%2)").arg(item->text(), name));

		availableEntriesModel->appendRow(item);
	}

	m_ui->availableEntriesItemView->setModel(availableEntriesModel);
	m_ui->availableEntriesItemView->setFilterRoles({Qt::DisplayRole, IdentifierRole});
	m_ui->availableEntriesItemView->viewport()->installEventFilter(this);
	m_ui->currentEntriesItemView->setModel(m_currentEntriesModel);
	m_ui->currentEntriesItemView->setFilterRoles({Qt::DisplayRole, IdentifierRole});
	m_ui->currentEntriesItemView->setViewMode(ItemViewWidget::TreeView);
	m_ui->currentEntriesItemView->setRootIsDecorated(false);

	for (int i = 0; i < m_definition.entries.count(); ++i)
	{
		addEntry(m_definition.entries.at(i));
	}

	m_definition.entries.clear();

	Menu *bookmarksMenu(new Menu(Menu::BookmarkSelectorMenu, m_ui->addBookmarkButton));

	m_ui->addBookmarkButton->setMenu(bookmarksMenu);
	m_ui->addBookmarkButton->setEnabled(BookmarksManager::getModel()->getRootItem()->rowCount() > 0);

	connect(bookmarksMenu, &Menu::triggered, this, &ToolBarDialog::addBookmark);
	connect(m_ui->addButton, &QToolButton::clicked, this, &ToolBarDialog::addNewEntry);
	connect(m_ui->removeButton, &QToolButton::clicked, m_ui->currentEntriesItemView, &ItemViewWidget::removeRow);
	connect(m_ui->moveDownButton, &QToolButton::clicked, m_ui->currentEntriesItemView, &ItemViewWidget::moveDownRow);
	connect(m_ui->moveUpButton, &QToolButton::clicked, m_ui->currentEntriesItemView, &ItemViewWidget::moveUpRow);
	connect(m_ui->availableEntriesItemView, &ItemViewWidget::customContextMenuRequested, this, &ToolBarDialog::showAvailableEntriesContextMenu);
	connect(m_ui->availableEntriesItemView, &ItemViewWidget::needsActionsUpdate, this, &ToolBarDialog::updateActions);
	connect(m_ui->currentEntriesItemView, &ItemViewWidget::customContextMenuRequested, this, &ToolBarDialog::showCurrentEntriesContextMenu);
	connect(m_ui->currentEntriesItemView, &ItemViewWidget::needsActionsUpdate, this, &ToolBarDialog::updateActions);
	connect(m_ui->currentEntriesItemView, &ItemViewWidget::canMoveRowDownChanged, m_ui->moveDownButton, &QToolButton::setEnabled);
	connect(m_ui->currentEntriesItemView, &ItemViewWidget::canMoveRowUpChanged, m_ui->moveUpButton, &QToolButton::setEnabled);
	connect(m_ui->availableEntriesFilterLineEditWidget, &LineEditWidget::textChanged, m_ui->availableEntriesItemView, &ItemViewWidget::setFilterString);
	connect(m_ui->currentEntriesFilterLineEditWidget, &LineEditWidget::textChanged, m_ui->currentEntriesItemView, &ItemViewWidget::setFilterString);
	connect(m_ui->editEntryButton, &QPushButton::clicked, this, &ToolBarDialog::editEntry);
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
		m_ui->normalVisibilityComboBox->setItemText(0, tr("Always visible"));
		m_ui->normalVisibilityComboBox->setItemText(1, tr("Always hidden"));
		m_ui->normalVisibilityComboBox->setItemText(2, tr("Visible only when needed"));
		m_ui->fullScreenVisibilityComboBox->setItemText(0, tr("Always visible"));
		m_ui->fullScreenVisibilityComboBox->setItemText(1, tr("Always hidden"));
		m_ui->fullScreenVisibilityComboBox->setItemText(2, tr("Visible only when needed"));
		m_ui->fullScreenVisibilityComboBox->setItemText(3, tr("Visible only when cursor is close to screen edge"));
		m_ui->buttonStyleComboBox->setItemText(0, tr("Follow style"));
		m_ui->buttonStyleComboBox->setItemText(1, tr("Icon only"));
		m_ui->buttonStyleComboBox->setItemText(2, tr("Text only"));
		m_ui->buttonStyleComboBox->setItemText(3, tr("Text beside icon"));
		m_ui->buttonStyleComboBox->setItemText(4, tr("Text under icon"));
	}
}

void ToolBarDialog::addEntry(const ToolBarsManager::ToolBarDefinition::Entry &entry, QStandardItem *parent)
{
	QStandardItem *item(createEntry(entry.action, entry.options, entry.parameters));

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

void ToolBarDialog::addNewEntry()
{
	const QStandardItem *sourceItem(m_ui->availableEntriesItemView->getItem(m_ui->availableEntriesItemView->getCurrentRow()));

	if (!sourceItem)
	{
		return;
	}

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
		m_ui->currentEntriesItemView->insertRow({newItem});
	}
}

void ToolBarDialog::editEntry()
{
	const QModelIndex index(m_ui->currentEntriesItemView->currentIndex());

	if (!index.data(HasOptionsRole).toBool())
	{
		return;
	}

	const QString identifier(index.data(IdentifierRole).toString());
	QVariantMap options(index.data(OptionsRole).toMap());
	QDialog dialog(this);
	dialog.setWindowTitle(tr("Edit Entry"));

	QDialogButtonBox *buttonBox(new QDialogButtonBox(&dialog));
	buttonBox->addButton(QDialogButtonBox::Cancel);
	buttonBox->addButton(QDialogButtonBox::Ok);

	QFormLayout *formLayout(new QFormLayout());
	QVBoxLayout *mainLayout(new QVBoxLayout(&dialog));
	mainLayout->addLayout(formLayout);
	mainLayout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	if (identifier == QLatin1String("SearchWidget"))
	{
		const QStringList searchEngines(SearchEnginesManager::getSearchEngines());
		QVector<SettingsManager::OptionDefinition::Choice> searchEngineChoices{{tr("All"), {}, {}}, {}};
		searchEngineChoices.reserve(searchEngines.count() + 2);

		for (int i = 0; i < searchEngines.count(); ++i)
		{
			const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(searchEngines.at(i)));

			searchEngineChoices.append({(searchEngine.title.isEmpty() ? tr("Unknown") : searchEngine.title), searchEngines.at(i), searchEngine.icon});
		}

		OptionWidget *searchEngineWidget(new OptionWidget(options.value(QLatin1String("searchEngine")), SettingsManager::EnumerationType, &dialog));
		searchEngineWidget->setChoices(searchEngineChoices);
		searchEngineWidget->setObjectName(QLatin1String("searchEngine"));

		OptionWidget *showSearchButtonWidget(new OptionWidget(options.value(QLatin1String("showSearchButton"), true), SettingsManager::BooleanType, &dialog));
		showSearchButtonWidget->setObjectName(QLatin1String("showSearchButton"));

		formLayout->addRow(tr("Show search engine:"), searchEngineWidget);
		formLayout->addRow(tr("Show search button:"), showSearchButtonWidget);
	}
	else if (identifier == QLatin1String("ConfigurationOptionWidget"))
	{
		const QStringList choices(SettingsManager::getOptions());
		OptionWidget *textWidget(new OptionWidget({}, SettingsManager::StringType, &dialog));
		textWidget->setDefaultValue(options.value(QLatin1String("optionName"), choices.value(0)).toString().section(QLatin1Char('/'), -1));
		textWidget->setValue(options.value(QLatin1String("text"), textWidget->getDefaultValue()));
		textWidget->setObjectName(QLatin1String("text"));

		OptionWidget *optionNameWidget(new OptionWidget(options.value(QLatin1String("optionName")), SettingsManager::EnumerationType, &dialog));
		optionNameWidget->setChoices(choices);
		optionNameWidget->setObjectName(QLatin1String("optionName"));

		OptionWidget *scopeWidget(new OptionWidget(options.value(QLatin1String("scope")), SettingsManager::EnumerationType, &dialog));
		scopeWidget->setChoices({{tr("Global"), QLatin1String("global"), {}}, {tr("Tab"), QLatin1String("window"), {}}});
		scopeWidget->setDefaultValue(QLatin1String("window"));
		scopeWidget->setObjectName(QLatin1String("scope"));

		formLayout->addRow(tr("Text:"), textWidget);
		formLayout->addRow(tr("Option:"), optionNameWidget);
		formLayout->addRow(tr("Scope:"), scopeWidget);

		connect(optionNameWidget, &OptionWidget::commitData, textWidget, [=]()
		{
			const bool needsReset(textWidget->isDefault());

			textWidget->setDefaultValue(optionNameWidget->getValue().toString().section(QLatin1Char('/'), -1));

			if (needsReset)
			{
				textWidget->setValue(textWidget->getDefaultValue());
			}
		});
	}
	else if (identifier == QLatin1String("ContentBlockingInformationWidget") || identifier == QLatin1String("MenuButtonWidget") || identifier == QLatin1String("PrivateWindowIndicatorWidget") || identifier == QLatin1String("TransfersWidget") || identifier.startsWith(QLatin1String("bookmarks:")) || identifier.endsWith(QLatin1String("Action")) || identifier.endsWith(QLatin1String("Menu")))
	{
		OptionWidget *iconWidget(new OptionWidget({}, SettingsManager::IconType, &dialog));
		iconWidget->setObjectName(QLatin1String("icon"));

		OptionWidget *textWidget(new OptionWidget({}, SettingsManager::StringType, &dialog));
		textWidget->setObjectName(QLatin1String("text"));

		if (identifier == QLatin1String("ClosedWindowsMenu"))
		{
			iconWidget->setDefaultValue(QLatin1String("user-trash"));
			textWidget->setDefaultValue(tr("Closed Tabs and Windows"));
		}
		else if (identifier == QLatin1String("ContentBlockingInformationWidget"))
		{
			iconWidget->setDefaultValue(QLatin1String("content-blocking"));
			textWidget->setDefaultValue(tr("Blocked Elements: {amount}"));
		}
		else if (identifier == QLatin1String("MenuButtonWidget"))
		{
			iconWidget->setDefaultValue(QGuiApplication::windowIcon());
			textWidget->setDefaultValue(tr("Menu"));
		}
		else if (identifier == QLatin1String("PrivateWindowIndicatorWidget"))
		{
			iconWidget->setDefaultValue(QLatin1String("window-private"));
			textWidget->setDefaultValue(tr("Private Window"));
		}
		else if (identifier == QLatin1String("TransfersWidget"))
		{
			iconWidget->setDefaultValue(QLatin1String("transfers"));
			textWidget->setDefaultValue(tr("Downloads"));
		}
		else if (identifier.startsWith(QLatin1String("bookmarks:")))
		{
			const BookmarksModel::Bookmark *bookmark(identifier.startsWith(QLatin1String("bookmarks:/")) ? BookmarksManager::getModel()->getBookmarkByPath(identifier.mid(11)) : BookmarksManager::getBookmark(identifier.mid(10).toULongLong()));

			if (bookmark)
			{
				iconWidget->setDefaultValue(bookmark->getIcon());
				textWidget->setDefaultValue(bookmark->getTitle());
			}
		}
		else if (identifier.endsWith(QLatin1String("Action")))
		{
			const int actionIdentifier(ActionsManager::getActionIdentifier(identifier.left(identifier.length() - 6)));

			if (actionIdentifier >= 0)
			{
				const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(actionIdentifier));

				iconWidget->setDefaultValue(definition.defaultState.icon);
				textWidget->setDefaultValue(definition.getText(true));
			}
		}

		iconWidget->setValue(options.value(QLatin1String("icon"), iconWidget->getDefaultValue()));
		textWidget->setValue(options.value(QLatin1String("text"), textWidget->getDefaultValue()));

		formLayout->addRow(tr("Icon:"), iconWidget);
		formLayout->addRow(tr("Text:"), textWidget);
	}

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	for (int i = 0; i < formLayout->rowCount(); ++i)
	{
		QLayoutItem *item(formLayout->itemAt(i, QFormLayout::FieldRole));

		if (!item)
		{
			continue;
		}

		const OptionWidget *widget(qobject_cast<OptionWidget*>(item->widget()));

		if (widget)
		{
			const QString option(widget->objectName());

			if (widget->isDefault())
			{
				options.remove(option);
			}
			else
			{
				options[option] = widget->getValue();
			}
		}
	}

	m_currentEntriesModel->setItemData(index, createEntryData(identifier, options, index.data(ParametersRole).toMap()));
}

void ToolBarDialog::addBookmark(QAction *action)
{
	if (action && action->data().type() == QVariant::ULongLong)
	{
		m_ui->currentEntriesItemView->insertRow({createEntry(QLatin1String("bookmarks:") + QString::number(action->data().toULongLong()))});
	}
}

void ToolBarDialog::restoreDefaults()
{
	ToolBarsManager::resetToolBar(m_definition.identifier);

	reject();
}

void ToolBarDialog::showAvailableEntriesContextMenu(const QPoint &position)
{
	const QModelIndex index(m_ui->availableEntriesItemView->indexAt(position));

	if (index.isValid())
	{
		QMenu menu(this);
		menu.addAction(tr("Add"), this, &ToolBarDialog::addNewEntry, QKeySequence(Qt::Key_Enter));
		menu.exec(m_ui->availableEntriesItemView->mapToGlobal(position));
	}
}

void ToolBarDialog::showCurrentEntriesContextMenu(const QPoint &position)
{
	QMenu menu(this);
	menu.addAction(tr("Remove"), m_ui->currentEntriesItemView, &ItemViewWidget::removeRow, QKeySequence(Qt::Key_Delete))->setEnabled(m_ui->removeButton->isEnabled());
	menu.exec(m_ui->currentEntriesItemView->mapToGlobal(position));
}

void ToolBarDialog::updateActions()
{
	const QModelIndex targetIndex(m_ui->currentEntriesItemView->currentIndex());
	const QString sourceIdentifier(m_ui->availableEntriesItemView->currentIndex().data(IdentifierRole).toString());
	const QString targetIdentifier(targetIndex.data(IdentifierRole).toString());

	m_ui->addButton->setEnabled(!sourceIdentifier.isEmpty() && (!(targetIndex.data(IdentifierRole).toString() == QLatin1String("CustomMenu") || targetIndex.parent().data(IdentifierRole).toString() == QLatin1String("CustomMenu")) || (sourceIdentifier == QLatin1String("separator") || sourceIdentifier.endsWith(QLatin1String("Action")) || sourceIdentifier.endsWith(QLatin1String("Menu")))));
	m_ui->removeButton->setEnabled(targetIndex.isValid() && targetIdentifier != QLatin1String("MenuBarWidget") && targetIdentifier != QLatin1String("TabBarWidget"));
	m_ui->editEntryButton->setEnabled(targetIndex.data(HasOptionsRole).toBool());
}

QStandardItem* ToolBarDialog::createEntry(const QString &identifier, const QVariantMap &options, const QVariantMap &parameters) const
{
	QStandardItem *item(new QStandardItem());

	if (identifier == QLatin1String("CustomMenu"))
	{
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

		m_ui->currentEntriesItemView->setRootIsDecorated(true);
	}
	else
	{
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);
	}

	const QMap<int, QVariant> entryData(createEntryData(identifier, options, parameters));
	QMap<int, QVariant>::const_iterator iterator;

	for (iterator = entryData.begin(); iterator != entryData.end(); ++iterator)
	{
		item->setData(iterator.value(), iterator.key());
	}

	return item;
}

ToolBarsManager::ToolBarDefinition::Entry ToolBarDialog::getEntry(QStandardItem *item) const
{
	ToolBarsManager::ToolBarDefinition::Entry definition;

	if (!item)
	{
		return definition;
	}

	definition.action = item->data(IdentifierRole).toString();
	definition.options = item->data(OptionsRole).toMap();
	definition.parameters = item->data(ParametersRole).toMap();

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
	definition.title = m_ui->titleLineEditWidget->text();
	definition.normalVisibility = static_cast<ToolBarsManager::ToolBarVisibility>(m_ui->normalVisibilityComboBox->currentData().toInt());
	definition.fullScreenVisibility = static_cast<ToolBarsManager::ToolBarVisibility>(m_ui->fullScreenVisibilityComboBox->currentData().toInt());
	definition.buttonStyle = static_cast<Qt::ToolButtonStyle>(m_ui->buttonStyleComboBox->currentData().toInt());
	definition.iconSize = m_ui->iconSizeSpinBox->value();
	definition.maximumButtonSize = m_ui->maximumButtonSizeSpinBox->value();
	definition.hasToggle = m_ui->hasToggleCheckBox->isChecked();

	switch (definition.type)
	{
		case ToolBarsManager::BookmarksBarType:
			definition.bookmarksPath = QLatin1Char('#') + QString::number(m_ui->folderComboBox->getCurrentFolder()->getIdentifier());

			break;
		case ToolBarsManager::SideBarType:
			definition.panels.clear();

			for (int i = 0; i < m_ui->panelsViewWidget->model()->rowCount(); ++i)
			{
				const QStandardItem *item(m_ui->panelsViewWidget->getItem(i));

				if (static_cast<Qt::CheckState>(item->data(Qt::CheckStateRole).toInt()) == Qt::Checked)
				{
					definition.panels.append(item->data(ItemModel::UserRole).toString());
				}
			}

			if (!definition.panels.contains(definition.currentPanel))
			{
				definition.currentPanel.clear();
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

QMap<int, QVariant> ToolBarDialog::createEntryData(const QString &identifier, const QVariantMap &options, const QVariantMap &parameters) const
{
	QMap<int, QVariant> entryData({{Qt::DecorationRole, ItemModel::createDecoration()}, {IdentifierRole, identifier}, {OptionsRole, options}, {ParametersRole, parameters}});

	if (identifier == QLatin1String("separator"))
	{
		entryData[Qt::DisplayRole] = tr("--- separator ---");
	}
	else if (identifier == QLatin1String("spacer"))
	{
		entryData[Qt::DisplayRole] = tr("--- spacer ---");
	}
	else if (identifier == QLatin1String("CustomMenu"))
	{
		entryData[Qt::DisplayRole] = tr("Arbitrary List of Actions");
	}
	else if (identifier == QLatin1String("ClosedWindowsMenu"))
	{
		entryData[HasOptionsRole] = true;
		entryData[Qt::DisplayRole] = tr("List of Closed Tabs and Windows");
		entryData[Qt::DecorationRole] = ThemesManager::createIcon(QLatin1String("user-trash"));
	}
	else if (identifier == QLatin1String("AddressWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Address Field");
		entryData[Qt::DecorationRole] = ThemesManager::createIcon(QLatin1String("document-open"));
	}
	else if (identifier == QLatin1String("ConfigurationOptionWidget"))
	{
		entryData[HasOptionsRole] = true;

		if (options.contains(QLatin1String("optionName")) && !options.value("optionName").toString().isEmpty())
		{
			entryData[Qt::DisplayRole] = tr("Configuration Widget (%1)").arg(options.value("optionName").toString());
		}
		else
		{
			entryData[Qt::DisplayRole] = tr("Configuration Widget");
		}
	}
	else if (identifier == QLatin1String("ContentBlockingInformationWidget"))
	{
		entryData[HasOptionsRole] = true;
		entryData[Qt::DisplayRole] = tr("Content Blocking Details");
		entryData[Qt::DecorationRole] = ThemesManager::createIcon(QLatin1String("content-blocking"));
	}
	else if (identifier == QLatin1String("ErrorConsoleWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Error Console");
	}
	else if (identifier == QLatin1String("MenuBarWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Menu Bar");
	}
	else if (identifier == QLatin1String("MenuButtonWidget"))
	{
		entryData[HasOptionsRole] = true;
		entryData[Qt::DisplayRole] = tr("Menu Button");
	}
	else if (identifier == QLatin1String("OpenInApplicationMenu"))
	{
		entryData[Qt::DisplayRole] = tr("Open with");
	}
	else if (identifier == QLatin1String("PanelChooserWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Sidebar Panel Chooser");
	}
	else if (identifier == QLatin1String("PrivateWindowIndicatorWidget"))
	{
		entryData[HasOptionsRole] = true;
		entryData[Qt::DisplayRole] = tr("Private Window Indicator");
		entryData[Qt::DecorationRole] = ThemesManager::createIcon(QLatin1String("window-private"));
	}
	else if (identifier == QLatin1String("ProgressInformationDocumentProgressWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Progress Information (Document Progress)");
	}
	else if (identifier == QLatin1String("ProgressInformationTotalSizeWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Progress Information (Total Progress)");
	}
	else if (identifier == QLatin1String("ProgressInformationElementsWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Progress Information (Loaded Elements)");
	}
	else if (identifier == QLatin1String("ProgressInformationSpeedWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Progress Information (Loading Speed)");
	}
	else if (identifier == QLatin1String("ProgressInformationElapsedTimeWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Progress Information (Elapsed Time)");
	}
	else if (identifier == QLatin1String("ProgressInformationMessageWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Progress Information (Status Message)");
	}
	else if (identifier == QLatin1String("SearchWidget"))
	{
		entryData[HasOptionsRole] = true;

		if (options.contains(QLatin1String("searchEngine")))
		{
			const SearchEnginesManager::SearchEngineDefinition definition(SearchEnginesManager::getSearchEngine(options[QLatin1String("searchEngine")].toString()));

			entryData[Qt::DisplayRole] = tr("Search Field (%1)").arg(definition.title.isEmpty() ? tr("Unknown") : definition.title);
			entryData[Qt::DecorationRole] = (definition.icon.isNull() ? ThemesManager::createIcon(QLatin1String("edit-find")) : definition.icon);
		}
		else
		{
			entryData[Qt::DisplayRole] = tr("Search Field");
			entryData[Qt::DecorationRole] = ThemesManager::createIcon(QLatin1String("edit-find"));
		}
	}
	else if (identifier == QLatin1String("SizeGripWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Window Resize Handle");
	}
	else if (identifier == QLatin1String("StatusMessageWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Status Message Field");
	}
	else if (identifier == QLatin1String("TabBarWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Tab Bar");
	}
	else if (identifier == QLatin1String("TransfersWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Downloads Progress Information");
		entryData[Qt::DecorationRole] = ThemesManager::createIcon(QLatin1String("transfers"));
	}
	else if (identifier == QLatin1String("ZoomWidget"))
	{
		entryData[Qt::DisplayRole] = tr("Zoom Slider");
	}
	else if (identifier.startsWith(QLatin1String("bookmarks:")))
	{
		const BookmarksModel::Bookmark *bookmark(identifier.startsWith(QLatin1String("bookmarks:/")) ? BookmarksManager::getModel()->getBookmarkByPath(identifier.mid(11)) : BookmarksManager::getBookmark(identifier.mid(10).toULongLong()));

		if (bookmark)
		{
			const QIcon icon(bookmark->getIcon());

			entryData[HasOptionsRole] = true;
			entryData[Qt::DisplayRole] = bookmark->getTitle();

			if (!icon.isNull())
			{
				entryData[Qt::DecorationRole] = icon;
			}
		}
		else
		{
			entryData[Qt::DisplayRole] = tr("Invalid Bookmark");
		}
	}
	else if (identifier.endsWith(QLatin1String("Action")))
	{
		const int actionIdentifier(ActionsManager::getActionIdentifier(identifier.left(identifier.length() - 6)));

		if (actionIdentifier < 0)
		{
			entryData[Qt::DisplayRole] = tr("Invalid Entry");
		}
		else
		{
			const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(actionIdentifier));

			entryData[HasOptionsRole] = true;
			entryData[Qt::DisplayRole] = definition.getText(true);

			if (!definition.defaultState.icon.isNull())
			{
				entryData[Qt::DecorationRole] = definition.defaultState.icon;
			}
		}
	}
	else
	{
		entryData[Qt::DisplayRole] = tr("Invalid Entry");
	}

	if (options.contains(QLatin1String("icon")))
	{
		entryData[Qt::DecorationRole] = ItemModel::createDecoration(ThemesManager::createIcon(options[QLatin1String("icon")].toString()));
	}

	if (options.contains(QLatin1String("text")))
	{
		entryData[Qt::DisplayRole] = options[QLatin1String("text")].toString();
	}

	entryData[Qt::ToolTipRole] = QStringLiteral("%1 (%2)").arg(entryData.value(Qt::DisplayRole).toString(), identifier);

	return entryData;
}

bool ToolBarDialog::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->availableEntriesItemView->viewport() && event->type() == QEvent::Drop)
	{
		event->accept();

		return true;
	}

	return Dialog::eventFilter(object, event);
}

}
