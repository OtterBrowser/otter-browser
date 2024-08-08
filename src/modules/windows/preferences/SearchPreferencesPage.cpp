/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "SearchPreferencesPage.h"
#include "../../../ui/Animation.h"
#include "../../../ui/LineEditWidget.h"
#include "../../../ui/SearchEnginePropertiesDialog.h"
#include "../../../core/ItemModel.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"

#include "ui_SearchPreferencesPage.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtGui/QRegularExpressionValidator>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

namespace Otter
{

SearchEngineTitleDelegate::SearchEngineTitleDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void SearchEngineTitleDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	ItemDelegate::initStyleOption(option, index);

	if (index.data(SearchPreferencesPage::IsUpdatingRole).toBool())
	{
		const Animation *animation(SearchPreferencesPage::getUpdateAnimation());

		if (animation)
		{
			option->icon = QIcon(animation->getCurrentPixmap());
		}
	}
}

SearchEngineKeywordDelegate::SearchEngineKeywordDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void SearchEngineKeywordDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QLineEdit *widget(qobject_cast<QLineEdit*>(editor));

	if (widget)
	{
		model->setData(index, widget->text(), Qt::DisplayRole);
		model->setData(index, widget->text(), Qt::ToolTipRole);
	}
}

QWidget* SearchEngineKeywordDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	const QStringList keywords(SearchPreferencesPage::getKeywords(index.model(), index.row()));
	LineEditWidget *widget(new LineEditWidget(index.data(Qt::DisplayRole).toString(), parent));
	widget->setValidator(new QRegularExpressionValidator(QRegularExpression((keywords.isEmpty() ? QString() : QStringLiteral("(?!\\b(%1)\\b)").arg(keywords.join('|'))) + QLatin1String("[a-z0-9]*")), widget));
	widget->setFocus();

	return widget;
}

Animation* SearchPreferencesPage::m_updateAnimation = nullptr;

SearchPreferencesPage::SearchPreferencesPage(QWidget *parent) : CategoryPage(parent),
	m_ui(nullptr)
{
}

SearchPreferencesPage::~SearchPreferencesPage()
{
	if (wasLoaded())
	{
		delete m_ui;
	}
}

void SearchPreferencesPage::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && wasLoaded())
	{
		m_ui->retranslateUi(this);
		m_ui->searchViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Name"), tr("Keyword")});
	}
}

void SearchPreferencesPage::createSearchEngine()
{
	SearchEnginesManager::SearchEngineDefinition searchEngine;
	searchEngine.title = tr("New Search Engine");
	searchEngine.icon = ThemesManager::createIcon(QLatin1String("edit-find"));

	SearchEnginePropertiesDialog dialog(searchEngine, getKeywords(m_ui->searchViewWidget->getSourceModel()), this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	searchEngine = dialog.getSearchEngine();
	searchEngine.identifier = searchEngine.createIdentifier(m_searchEngines.keys());

	m_searchEngines[searchEngine.identifier] = SearchEngine(searchEngine, true);

	m_ui->searchViewWidget->insertRow(createRow(searchEngine));

	emit settingsModified();
}

void SearchPreferencesPage::importSearchEngine()
{
	const QString path(QFileDialog::getOpenFileName(this, tr("Select File"), Utils::getStandardLocation(QStandardPaths::HomeLocation), Utils::formatFileTypes({tr("Open Search files (*.xml)")})));

	if (!path.isEmpty())
	{
		addSearchEngine(path, Utils::createIdentifier(QString(), m_searchEngines.keys()), false);
	}
}

void SearchPreferencesPage::readdSearchEngine(QAction *action)
{
	if (action && !action->data().isNull())
	{
		const QString identifier(action->data().toString());

		addSearchEngine(SessionsManager::getReadableDataPath(QLatin1String("searchEngines/") + identifier + QLatin1String(".xml")), identifier, true);
	}
}

void SearchPreferencesPage::editSearchEngine()
{
	const QModelIndex index(m_ui->searchViewWidget->getIndex(m_ui->searchViewWidget->getCurrentRow(), 0));
	const QString identifier(index.data(IdentifierRole).toString());

	if (identifier.isEmpty() || !m_searchEngines.contains(identifier))
	{
		return;
	}

	const QStringList keywords(getKeywords(m_ui->searchViewWidget->getSourceModel(), m_ui->searchViewWidget->getCurrentRow()));
	SearchEnginePropertiesDialog dialog(m_searchEngines[identifier].definition, keywords, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	SearchEnginesManager::SearchEngineDefinition searchEngine(dialog.getSearchEngine());

	if (keywords.contains(searchEngine.keyword))
	{
		searchEngine.keyword.clear();
	}

	m_searchEngines[identifier] = SearchEngine(searchEngine, true);

	m_ui->searchViewWidget->setData(index, searchEngine.title, Qt::DisplayRole);
	m_ui->searchViewWidget->setData(index, searchEngine.title, Qt::ToolTipRole);
	m_ui->searchViewWidget->setData(index, ItemModel::createDecoration(searchEngine.icon), Qt::DecorationRole);
	m_ui->searchViewWidget->setData(m_ui->searchViewWidget->getIndex(index.row(), 1), searchEngine.keyword, Qt::DisplayRole);
	m_ui->searchViewWidget->setData(m_ui->searchViewWidget->getIndex(index.row(), 1), searchEngine.keyword, Qt::ToolTipRole);

	emit settingsModified();
}

void SearchPreferencesPage::updateSearchEngine()
{
	const QPersistentModelIndex index(m_ui->searchViewWidget->getIndex(m_ui->searchViewWidget->getCurrentRow(), 0));
	const QString identifier(index.data(IdentifierRole).toString());

	if (identifier.isEmpty() || !m_searchEngines.contains(identifier) || m_updateJobs.contains(identifier))
	{
		return;
	}

	if (!m_updateAnimation)
	{
		m_updateAnimation = ThemesManager::createAnimation();
		m_updateAnimation->start();

		connect(m_updateAnimation, &Animation::frameChanged, m_ui->searchViewWidget->viewport(), static_cast<void(QWidget::*)()>(&QWidget::update));
	}

	m_ui->searchViewWidget->setData(index, true, IsUpdatingRole);
	m_ui->searchViewWidget->update();

	SearchEngineFetchJob *job(new SearchEngineFetchJob(m_searchEngines[identifier].definition.selfUrl, identifier, false, this));

	m_updateJobs[identifier] = job;

	connect(job, &SearchEngineFetchJob::jobFinished, this, [=](bool isSuccess)
	{
		SearchEnginesManager::SearchEngineDefinition searchEngine(job->getSearchEngine());

		if (index.isValid())
		{
			if (isSuccess)
			{
				m_ui->searchViewWidget->setData(index, searchEngine.title, Qt::DisplayRole);
				m_ui->searchViewWidget->setData(index, searchEngine.title, Qt::ToolTipRole);
				m_ui->searchViewWidget->setData(index, ItemModel::createDecoration(searchEngine.icon), Qt::DecorationRole);
			}

			m_ui->searchViewWidget->setData(index, false, IsUpdatingRole);
		}

		m_updateJobs.remove(identifier);

		if (m_updateJobs.isEmpty())
		{
			m_updateAnimation->deleteLater();
			m_updateAnimation = nullptr;
		}

		if (!isSuccess)
		{
			QMessageBox::warning(this, tr("Error"), tr("Failed to update search engine."), QMessageBox::Close);

			return;
		}

		if (m_searchEngines.contains(identifier))
		{
			searchEngine.keyword = m_searchEngines[identifier].definition.keyword;

			m_searchEngines[identifier] = SearchEngine(searchEngine, true);
		}
	});

	job->start();
}

void SearchPreferencesPage::removeSearchEngine()
{
	const QString identifier(m_ui->searchViewWidget->getIndex(m_ui->searchViewWidget->getCurrentRow(), 0).data(IdentifierRole).toString());

	if (identifier.isEmpty() || !m_searchEngines.contains(identifier))
	{
		return;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("Do you really want to remove this search engine?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Cancel);

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("searchEngines/") + identifier + QLatin1String(".xml")));

	if (QFile::exists(path))
	{
		messageBox.setCheckBox(new QCheckBox(tr("Delete search engine permanently")));
	}

	if (messageBox.exec() != QMessageBox::Yes)
	{
		return;
	}

	if (messageBox.checkBox() && messageBox.checkBox()->isChecked())
	{
		m_filesToRemove.append(path);
	}

	if (m_updateJobs.contains(identifier))
	{
		m_updateJobs[identifier]->cancel();
		m_updateJobs.remove(identifier);
	}

	m_searchEngines.remove(identifier);

	m_ui->searchViewWidget->removeRow();

	updateReaddSearchEngineMenu();

	emit settingsModified();
}

void SearchPreferencesPage::addSearchEngine(const QString &path, const QString &identifier, bool isReadding)
{
	QFile file(path);

	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(this, tr("Error"), tr("Failed to open Open Search file."));

		return;
	}

	SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::loadSearchEngine(&file, identifier, false));

	file.close();

	if (!searchEngine.isValid() || m_searchEngines.contains(identifier))
	{
		QMessageBox::warning(this, tr("Error"), tr("Failed to open Open Search file."));

		return;
	}

	const QStringList keywords(getKeywords(m_ui->searchViewWidget->getSourceModel()));

	if (keywords.contains(searchEngine.keyword))
	{
		QMessageBox messageBox;
		messageBox.setWindowTitle(tr("Question"));
		messageBox.setText(tr("Keyword is already in use. Do you want to continue anyway?"));
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		messageBox.setDefaultButton(QMessageBox::Cancel);

		if (messageBox.exec() == QMessageBox::Cancel)
		{
			return;
		}

		searchEngine.keyword.clear();
	}

	m_searchEngines[identifier] = SearchEngine(searchEngine);

	m_ui->searchViewWidget->insertRow(createRow(searchEngine));

	if (isReadding)
	{
		updateReaddSearchEngineMenu();
	}

	emit settingsModified();
}

void SearchPreferencesPage::updateSearchEngineActions()
{
	const QModelIndex index(m_ui->searchViewWidget->currentIndex());
	const QString identifier(index.sibling(index.row(), 0).data(IdentifierRole).toString());
	const int currentRow(m_ui->searchViewWidget->getCurrentRow());
	const bool isSelected(currentRow >= 0 && currentRow < m_ui->searchViewWidget->getRowCount());

	if (index.column() != 1)
	{
		m_ui->searchViewWidget->setCurrentIndex(index.sibling(index.row(), 1));
	}

	m_ui->editSearchButton->setEnabled(isSelected);
	m_ui->updateSearchButton->setEnabled(isSelected && m_searchEngines.contains(identifier) && m_searchEngines[identifier].definition.selfUrl.isValid());
	m_ui->removeSearchButton->setEnabled(isSelected);
}

void SearchPreferencesPage::updateReaddSearchEngineMenu()
{
	if (!m_ui->addSearchButton->menu())
	{
		return;
	}

	QStringList availableIdentifiers;
	QVector<SearchEnginesManager::SearchEngineDefinition> availableSearchEngines;
	const QList<QFileInfo> allSearchEngines(QDir(SessionsManager::getReadableDataPath(QLatin1String("searchEngines"))).entryInfoList(QDir::Files) + QDir(SessionsManager::getReadableDataPath(QLatin1String("searchEngines"), true)).entryInfoList(QDir::Files));

	for (int i = 0; i < allSearchEngines.count(); ++i)
	{
		const QString identifier(allSearchEngines.at(i).baseName());

		if (!m_searchEngines.contains(identifier) && !availableIdentifiers.contains(identifier))
		{
			const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(identifier));

			if (searchEngine.isValid())
			{
				availableIdentifiers.append(identifier);

				availableSearchEngines.append(searchEngine);
			}
		}
	}

	QMenu *menu(m_ui->addSearchButton->menu()->actions().at(2)->menu());
	menu->clear();
	menu->setEnabled(!availableSearchEngines.isEmpty());

	for (int i = 0; i < availableSearchEngines.count(); ++i)
	{
		const SearchEnginesManager::SearchEngineDefinition searchEngine(availableSearchEngines.at(i));

		menu->addAction(searchEngine.icon, (searchEngine.title.isEmpty() ? tr("(Untitled)") : searchEngine.title))->setData(searchEngine.identifier);
	}
}

void SearchPreferencesPage::load()
{
	if (wasLoaded())
	{
		return;
	}

	m_ui = new Ui::SearchPreferencesPage();
	m_ui->setupUi(this);

	ItemModel *searchEnginesModel(new ItemModel(this));
	searchEnginesModel->setHorizontalHeaderLabels({tr("Name"), tr("Keyword")});
	searchEnginesModel->setHeaderData(0, Qt::Horizontal, 250, HeaderViewWidget::WidthRole);
	searchEnginesModel->setExclusive(true);

	const QString defaultSearchEngine(SettingsManager::getOption(SettingsManager::Search_DefaultSearchEngineOption).toString());
	const QStringList searchEngines(SearchEnginesManager::getSearchEngines());

	for (int i = 0; i < searchEngines.count(); ++i)
	{
		const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(searchEngines.at(i)));

		if (searchEngine.isValid())
		{
			m_searchEngines[searchEngine.identifier] = SearchEngine(searchEngine);

			searchEnginesModel->appendRow(createRow(searchEngine, (searchEngine.identifier == defaultSearchEngine)));
		}
	}

	const QString suggestionsMode(SettingsManager::getOption(SettingsManager::Search_SearchEnginesSuggestionsModeOption).toString());

	m_ui->searchViewWidget->setModel(searchEnginesModel);
	m_ui->searchViewWidget->setItemDelegateForColumn(0, new SearchEngineTitleDelegate(this));
	m_ui->searchViewWidget->setItemDelegateForColumn(1, new SearchEngineKeywordDelegate(this));
	m_ui->searchViewWidget->setExclusive(true);
	m_ui->searchViewWidget->setRowsMovable(true);
	m_ui->enableSearchSuggestionsCheckBox->setChecked(suggestionsMode != QLatin1String("disabled"));
	m_ui->enableSearchSuggestionsInPrivateTabsCheckBox->setChecked(suggestionsMode == QLatin1String("enabled"));
	m_ui->enableSearchSuggestionsInPrivateTabsCheckBox->setEnabled(m_ui->enableSearchSuggestionsCheckBox->isChecked());

	QMenu *addSearchEngineMenu(new QMenu(m_ui->addSearchButton));
	addSearchEngineMenu->addAction(tr("New…"), this, &SearchPreferencesPage::createSearchEngine);
	addSearchEngineMenu->addAction(tr("File…"), this, &SearchPreferencesPage::importSearchEngine);
	addSearchEngineMenu->addAction(tr("Re-add"))->setMenu(new QMenu(m_ui->addSearchButton));

	m_ui->addSearchButton->setMenu(addSearchEngineMenu);
	m_ui->moveDownSearchButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-down")));
	m_ui->moveUpSearchButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-up")));

	updateReaddSearchEngineMenu();

	connect(m_ui->searchFilterLineEditWidget, &LineEditWidget::textChanged, m_ui->searchViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->searchViewWidget, &ItemViewWidget::canMoveRowDownChanged, m_ui->moveDownSearchButton, &QToolButton::setEnabled);
	connect(m_ui->searchViewWidget, &ItemViewWidget::canMoveRowUpChanged, m_ui->moveUpSearchButton, &QToolButton::setEnabled);
	connect(m_ui->searchViewWidget, &ItemViewWidget::needsActionsUpdate, this, &SearchPreferencesPage::updateSearchEngineActions);
	connect(m_ui->searchViewWidget, &ItemViewWidget::modified, this, &SearchPreferencesPage::settingsModified);
	connect(m_ui->addSearchButton->menu()->actions().at(2)->menu(), &QMenu::triggered, this, &SearchPreferencesPage::readdSearchEngine);
	connect(m_ui->editSearchButton, &QPushButton::clicked, this, &SearchPreferencesPage::editSearchEngine);
	connect(m_ui->updateSearchButton, &QPushButton::clicked, this, &SearchPreferencesPage::updateSearchEngine);
	connect(m_ui->removeSearchButton, &QPushButton::clicked, this, &SearchPreferencesPage::removeSearchEngine);
	connect(m_ui->moveDownSearchButton, &QToolButton::clicked, m_ui->searchViewWidget, &ItemViewWidget::moveDownRow);
	connect(m_ui->moveUpSearchButton, &QToolButton::clicked, m_ui->searchViewWidget, &ItemViewWidget::moveUpRow);
	connect(m_ui->enableSearchSuggestionsCheckBox, &QCheckBox::toggled, m_ui->enableSearchSuggestionsInPrivateTabsCheckBox, &QCheckBox::setEnabled);

	markAsLoaded();
}

void SearchPreferencesPage::save()
{
	for (int i = 0; i < m_filesToRemove.count(); ++i)
	{
		QFile::remove(m_filesToRemove.at(i));
	}

	m_filesToRemove.clear();

	QStringList searchEnginesOrder;
	searchEnginesOrder.reserve(m_ui->searchViewWidget->getRowCount());

	QString defaultSearchEngine;

	for (int i = 0; i < m_ui->searchViewWidget->getRowCount(); ++i)
	{
		const QString identifier(m_ui->searchViewWidget->getIndex(i, 0).data(IdentifierRole).toString());
		const QString keyword(m_ui->searchViewWidget->getIndex(i, 1).data(Qt::DisplayRole).toString());

		if (!identifier.isEmpty())
		{
			searchEnginesOrder.append(identifier);

			if (static_cast<Qt::CheckState>(m_ui->searchViewWidget->getIndex(i, 0).data(Qt::CheckStateRole).toInt()) == Qt::Checked)
			{
				defaultSearchEngine = identifier;
			}
		}

		if (m_searchEngines.contains(identifier))
		{
			SearchEngine *searchEngine(&m_searchEngines[identifier]);

			if (searchEngine->definition.keyword != keyword)
			{
				searchEngine->definition.keyword = keyword;
				searchEngine->isModified = true;
			}
		}
	}

	QHash<QString, SearchEngine>::iterator searchEnginesIterator;

	for (searchEnginesIterator = m_searchEngines.begin(); searchEnginesIterator != m_searchEngines.end(); ++searchEnginesIterator)
	{
		if (searchEnginesIterator.value().isModified)
		{
			SearchEnginesManager::saveSearchEngine(searchEnginesIterator.value().definition);
		}
	}

	if (SettingsManager::getOption(SettingsManager::Search_SearchEnginesOrderOption).toStringList() == searchEnginesOrder)
	{
		SearchEnginesManager::loadSearchEngines();
	}
	else
	{
		SettingsManager::setOption(SettingsManager::Search_SearchEnginesOrderOption, searchEnginesOrder);
	}

	QString suggestionsMode;

	if (m_ui->enableSearchSuggestionsCheckBox->isChecked())
	{
		suggestionsMode = (m_ui->enableSearchSuggestionsInPrivateTabsCheckBox->isChecked() ? QLatin1String("enabled") : QLatin1String("nonPrivateTabsOnly"));
	}
	else
	{
		suggestionsMode = QLatin1String("disabled");
	}

	SettingsManager::setOption(SettingsManager::Search_DefaultSearchEngineOption, defaultSearchEngine);
	SettingsManager::setOption(SettingsManager::Search_SearchEnginesSuggestionsModeOption, suggestionsMode);

	updateReaddSearchEngineMenu();
}

Animation* SearchPreferencesPage::getUpdateAnimation()
{
	return m_updateAnimation;
}

QString SearchPreferencesPage::getTitle() const
{
	return tr("Search");
}

QStringList SearchPreferencesPage::getKeywords(const QAbstractItemModel *model, int excludeRow)
{
	QStringList keywords;
	keywords.reserve(model->rowCount());

	for (int i = 0; i < model->rowCount(); ++i)
	{
		const QString keyword(model->index(i, 1).data(Qt::DisplayRole).toString());

		if (i != excludeRow && !keyword.isEmpty())
		{
			keywords.append(keyword);
		}
	}

	return keywords;
}

QList<QStandardItem*> SearchPreferencesPage::createRow(const SearchEnginesManager::SearchEngineDefinition &searchEngine, bool isDefault) const
{
	QList<QStandardItem*> items({new QStandardItem(searchEngine.icon, searchEngine.title), new QStandardItem(searchEngine.keyword)});
	items[0]->setCheckable(true);
	items[0]->setData(searchEngine.identifier, IdentifierRole);
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren);
	items[0]->setToolTip(searchEngine.title);
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);
	items[1]->setToolTip(searchEngine.keyword);

	if (isDefault)
	{
		items[0]->setData(Qt::Checked, Qt::CheckStateRole);
	}

	if (searchEngine.icon.isNull())
	{
		items[0]->setData(ItemModel::createDecoration(), Qt::DecorationRole);
	}

	return items;
}

}
