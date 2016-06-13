/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ToolBarDialog.h"
#include "Menu.h"
#include "OptionWidget.h"
#include "../core/ActionsManager.h"
#include "../core/BookmarksManager.h"
#include "../core/BookmarksModel.h"
#include "../core/SearchEnginesManager.h"
#include "../core/ThemesManager.h"

#include "ui_ToolBarDialog.h"

#include <QtWidgets/QMenu>

namespace Otter
{

ToolBarDialog::ToolBarDialog(int identifier, QWidget *parent) : Dialog(parent),
	m_definition(ToolBarsManager::getToolBarDefinition(identifier)),
	m_ui(new Ui::ToolBarDialog)
{
	m_ui->setupUi(this);
	m_ui->removeButton->setIcon(ThemesManager::getIcon(QLatin1String("go-previous")));
	m_ui->addButton->setIcon(ThemesManager::getIcon(QLatin1String("go-next")));
	m_ui->moveUpButton->setIcon(ThemesManager::getIcon(QLatin1String("go-up")));
	m_ui->moveDownButton->setIcon(ThemesManager::getIcon(QLatin1String("go-down")));
	m_ui->addEntryButton->setMenu(new QMenu(m_ui->addEntryButton));
	m_ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(m_definition.canReset);
	m_ui->titleLineEdit->setText(m_definition.title.isEmpty() ? tr("Custom Toolbar") : m_definition.title);
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

	if (m_definition.bookmarksPath.isEmpty())
	{
		m_ui->folderLabel->setParent(NULL);
		m_ui->folderLabel->deleteLater();
		m_ui->folderComboBox->setParent(NULL);
		m_ui->folderComboBox->deleteLater();
	}
	else
	{
		m_ui->folderComboBox->setCurrentFolder(BookmarksManager::getModel()->getItem(m_definition.bookmarksPath));
		m_ui->optionsHeader->hide();
		m_ui->arrangementWidget->hide();
	}

	QStandardItemModel *availableEntriesModel(new QStandardItemModel(this));
	availableEntriesModel->appendRow(createEntry(QLatin1String("separator")));
	availableEntriesModel->appendRow(createEntry(QLatin1String("spacer")));

	const QStringList widgets({QLatin1String("CustomMenu"), QLatin1String("ClosedWindowsMenu"), QLatin1String("AddressWidget"), QLatin1String("MenuButtonWidget"), QLatin1String("PanelChooserWidget"), QLatin1String("SearchWidget"), QLatin1String("StatusMessageWidget"), QLatin1String("ZoomWidget")});

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

	const QVector<ActionsManager::ActionDefinition> actions = ActionsManager::getActionDefinitions();

	for (int i = 0; i < actions.count(); ++i)
	{
		QStandardItem *item(new QStandardItem(actions.at(i).icon, QCoreApplication::translate("actions", (actions.at(i).description.isEmpty() ? actions.at(i).text : actions.at(i).description).toUtf8().constData())));
		item->setData(ActionsManager::getActionName(actions.at(i).identifier) + QLatin1String("Action"), Qt::UserRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		item->setToolTip(item->text());

		availableEntriesModel->appendRow(item);
	}

	m_ui->availableEntriesItemView->setModel(availableEntriesModel);
	m_ui->availableEntriesItemView->viewport()->setAcceptDrops(false);
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
	connect(m_ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked()), this, SLOT(restoreDefaults()));
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

		if (targetItem && targetItem->data(Qt::UserRole).toString() != QLatin1String("CustomMenu"))
		{
			targetItem = targetItem->parent();
		}

		if (targetItem && targetItem->data(Qt::UserRole).toString() == QLatin1String("CustomMenu"))
		{
			int row = -1;

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
	const QString identifier(index.data(Qt::UserRole).toString());
	QVariantMap options(index.data(Qt::UserRole + 1).toMap());
	QList<QPair<QString, OptionWidget*> > widgets;

	if (identifier == QLatin1String("SearchWidget"))
	{
		const QStringList searchEngines(SearchEnginesManager::getSearchEngines());
		QList<OptionWidget::EnumerationChoice> searchEngineChoices;
		OptionWidget *searchEngineWidget(new OptionWidget(QLatin1String("searchEngine"), options.value(QLatin1String("searchEngine")), SettingsManager::EnumerationType, this));
		OptionWidget::EnumerationChoice defaultSearchEngineChoice;
		defaultSearchEngineChoice.text = tr("All");

		searchEngineChoices.append(defaultSearchEngineChoice);

		for (int i = 0; i < searchEngines.count(); ++i)
		{
			const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(searchEngines.at(i)));
			OptionWidget::EnumerationChoice searchEngineChoice;
			searchEngineChoice.text = (searchEngine.title.isEmpty() ? tr("Unknown") : searchEngine.title);
			searchEngineChoice.value = searchEngines.at(i);
			searchEngineChoice.icon = searchEngine.icon;

			searchEngineChoices.append(searchEngineChoice);
		}

		searchEngineWidget->setChoices(searchEngineChoices);

		widgets.append(qMakePair(tr("Show search engine:"), searchEngineWidget));
		widgets.append(qMakePair(tr("Show search button:"), new OptionWidget(QLatin1String("showSearchButton"), options.value(QLatin1String("showSearchButton"), true), SettingsManager::BooleanType, this)));
	}
	else if (identifier == QLatin1String("MenuButtonWidget") || identifier == QLatin1String("PanelChooserWidget") || identifier.startsWith(QLatin1String("bookmarks:")) || identifier.endsWith(QLatin1String("Action")) || identifier.endsWith(QLatin1String("Menu")))
	{
		widgets.append(qMakePair(tr("Custom icon:"), new OptionWidget(QLatin1String("icon"), options.value(QLatin1String("icon")), SettingsManager::IconType, this)));
		widgets.append(qMakePair(tr("Custom text:"), new OptionWidget(QLatin1String("text"), options.value(QLatin1String("text")), SettingsManager::StringType, this)));
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
		if (widgets.at(i).second->getValue().isNull())
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
	m_ui->currentEntriesItemView->setData(index, options, (Qt::UserRole + 1));

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
	const QString sourceIdentifier(m_ui->availableEntriesItemView->currentIndex().data(Qt::UserRole).toString());
	const QString targetIdentifier(m_ui->currentEntriesItemView->currentIndex().data(Qt::UserRole).toString());

	m_ui->addButton->setEnabled(!sourceIdentifier.isEmpty() && (!(m_ui->currentEntriesItemView->currentIndex().data(Qt::UserRole).toString() == QLatin1String("CustomMenu") || m_ui->currentEntriesItemView->currentIndex().parent().data(Qt::UserRole).toString() == QLatin1String("CustomMenu")) || (sourceIdentifier == QLatin1String("separator") || sourceIdentifier.endsWith(QLatin1String("Action")) || sourceIdentifier.endsWith(QLatin1String("Menu")))));
	m_ui->removeButton->setEnabled(m_ui->currentEntriesItemView->currentIndex().isValid() && targetIdentifier != QLatin1String("MenuBarWidget") && targetIdentifier != QLatin1String("TabBarWidget"));
	m_ui->editEntryButton->setEnabled(targetIdentifier == QLatin1String("MenuButtonWidget") || targetIdentifier == QLatin1String("PanelChooserWidget") || targetIdentifier == QLatin1String("SearchWidget") || targetIdentifier.startsWith(QLatin1String("bookmarks:")) || targetIdentifier.endsWith(QLatin1String("Action")) || targetIdentifier.endsWith(QLatin1String("Menu")));
}

QStandardItem* ToolBarDialog::createEntry(const QString &identifier, const QVariantMap &options)
{
	QStandardItem *item(new QStandardItem());
	item->setData(identifier, Qt::UserRole);
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
		item->setText(tr("List of Closed Windows and Tabs"));
	}
	else if (identifier == QLatin1String("AddressWidget"))
	{
		item->setText(tr("Address Field"));
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
	else if (identifier == QLatin1String("SearchWidget"))
	{
		if (options.contains(QLatin1String("searchEngine")))
		{
			const SearchEnginesManager::SearchEngineDefinition definition(SearchEnginesManager::getSearchEngine(options[QLatin1String("searchEngine")].toString()));

			item->setText(tr("Search Field (%1)").arg(definition.title.isEmpty() ? tr("Unknown") : definition.title));
			item->setIcon(definition.icon);
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
			item->setText(bookmark->data(BookmarksModel::TitleRole).isValid() ? bookmark->data(BookmarksModel::TitleRole).toString() : tr("(Untitled)"));
			item->setIcon(bookmark->data(Qt::DecorationRole).value<QIcon>());
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
			item->setIcon(definition.icon);
		}
	}
	else
	{
		item->setText(tr("Invalid Entry"));
	}

	if (!options.isEmpty())
	{
		item->setData(options, (Qt::UserRole + 1));

		if (options.contains(QLatin1String("icon")))
		{
			const QString data(options[QLatin1String("icon")].toString());

			if (data.startsWith(QLatin1String("data:image/")))
			{
				item->setIcon(QIcon(QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8())))));
			}
			else
			{
				item->setIcon(ThemesManager::getIcon(data));
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

	definition.action = item->data(Qt::UserRole).toString();
	definition.options = item->data(Qt::UserRole + 1).toMap();

	if (definition.action == QLatin1String("CustomMenu"))
	{
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

	for (int i = 0; i < m_ui->currentEntriesItemView->model()->rowCount(); ++i)
	{
		definition.entries.append(getEntry(m_ui->currentEntriesItemView->getItem(i)));
	}

	if (!definition.bookmarksPath.isEmpty())
	{
		definition.bookmarksPath = QLatin1Char('#') + QString::number(m_ui->folderComboBox->getCurrentFolder()->data(BookmarksModel::IdentifierRole).toULongLong());
	}

	return definition;
}

}
