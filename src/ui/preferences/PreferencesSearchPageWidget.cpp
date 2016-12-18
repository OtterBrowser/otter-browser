/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "PreferencesSearchPageWidget.h"
#include "../SearchEnginePropertiesDialog.h"
#include "../../core/SessionsManager.h"
#include "../../core/SettingsManager.h"
#include "../../core/ThemesManager.h"
#include "../../core/Utils.h"

#include "ui_PreferencesSearchPageWidget.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtGui/QRegularExpressionValidator>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

namespace Otter
{

SearchKeywordDelegate::SearchKeywordDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void SearchKeywordDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	editor->setGeometry(option.rect);
}

void SearchKeywordDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QLineEdit *widget(qobject_cast<QLineEdit*>(editor));

	if (widget)
	{
		model->setData(index, widget->text());
	}
}

QWidget* SearchKeywordDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	const QStringList keywords(PreferencesSearchPageWidget::getKeywords(index.model(), index.row()));
	QLineEdit *widget(new QLineEdit(index.data(Qt::DisplayRole).toString(), parent));
	widget->setValidator(new QRegularExpressionValidator(QRegularExpression((keywords.isEmpty() ? QString() : QStringLiteral("(?!\\b(%1)\\b)").arg(keywords.join('|'))) + "[a-z0-9]*"), widget));

	return widget;
}

PreferencesSearchPageWidget::PreferencesSearchPageWidget(QWidget *parent) : QWidget(parent),
	m_defaultSearchEngine(SettingsManager::getValue(SettingsManager::Search_DefaultSearchEngineOption).toString()),
	m_ui(new Ui::PreferencesSearchPageWidget)
{
	m_ui->setupUi(this);

	QStandardItemModel *searchEnginesModel(new QStandardItemModel(this));
	searchEnginesModel->setHorizontalHeaderLabels(QStringList({tr("Name"), tr("Keyword")}));

	const QStringList searchEngines(SearchEnginesManager::getSearchEngines());

	for (int i = 0; i < searchEngines.count(); ++i)
	{
		const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(searchEngines.at(i)));

		if (searchEngine.identifier.isEmpty())
		{
			continue;
		}

		m_searchEngines[searchEngine.identifier] = qMakePair(false, searchEngine);

		searchEnginesModel->appendRow(createRow(searchEngine));
	}

	m_ui->searchViewWidget->setModel(searchEnginesModel);
	m_ui->searchViewWidget->setItemDelegateForColumn(1, new SearchKeywordDelegate(this));
	m_ui->searchSuggestionsCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Search_SearchEnginesSuggestionsOption).toBool());

	QMenu *addSearchMenu(new QMenu(m_ui->addSearchButton));
	addSearchMenu->addAction(tr("New…"), this, SLOT(createSearchEngine()));
	addSearchMenu->addAction(tr("File…"), this, SLOT(importSearchEngine()));
	addSearchMenu->addAction(tr("Readd"))->setMenu(new QMenu(m_ui->addSearchButton));

	m_ui->addSearchButton->setMenu(addSearchMenu);
	m_ui->moveDownSearchButton->setIcon(ThemesManager::getIcon(QLatin1String("arrow-down")));
	m_ui->moveUpSearchButton->setIcon(ThemesManager::getIcon(QLatin1String("arrow-up")));

	updateReaddSearchMenu();

	connect(m_ui->searchFilterLineEdit, SIGNAL(textChanged(QString)), m_ui->searchViewWidget, SLOT(setFilterString(QString)));
	connect(m_ui->searchViewWidget, SIGNAL(canMoveDownChanged(bool)), m_ui->moveDownSearchButton, SLOT(setEnabled(bool)));
	connect(m_ui->searchViewWidget, SIGNAL(canMoveUpChanged(bool)), m_ui->moveUpSearchButton, SLOT(setEnabled(bool)));
	connect(m_ui->searchViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateSearchActions()));
	connect(m_ui->searchViewWidget, SIGNAL(modified()), this, SIGNAL(settingsModified()));
	connect(m_ui->addSearchButton->menu()->actions().at(2)->menu(), SIGNAL(triggered(QAction*)), this, SLOT(readdSearchEngine(QAction*)));
	connect(m_ui->editSearchButton, SIGNAL(clicked()), this, SLOT(editSearchEngine()));
	connect(m_ui->removeSearchButton, SIGNAL(clicked()), this, SLOT(removeSearchEngine()));
	connect(m_ui->moveDownSearchButton, SIGNAL(clicked()), m_ui->searchViewWidget, SLOT(moveDownRow()));
	connect(m_ui->moveUpSearchButton, SIGNAL(clicked()), m_ui->searchViewWidget, SLOT(moveUpRow()));
}

PreferencesSearchPageWidget::~PreferencesSearchPageWidget()
{
	delete m_ui;
}

void PreferencesSearchPageWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void PreferencesSearchPageWidget::createSearchEngine()
{
	const QString identifier(Utils::createIdentifier(QString(), m_searchEngines.keys()));

	if (identifier.isEmpty())
	{
		return;
	}

	SearchEnginesManager::SearchEngineDefinition searchEngine;
	searchEngine.identifier = identifier;
	searchEngine.title = tr("New Search Engine");
	searchEngine.icon = ThemesManager::getIcon(QLatin1String("edit-find"));

	SearchEnginePropertiesDialog dialog(searchEngine, getKeywords(m_ui->searchViewWidget->getSourceModel()), false, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	searchEngine = dialog.getSearchEngine();

	m_searchEngines[identifier] = qMakePair(true, searchEngine);

	if (dialog.isDefault())
	{
		m_defaultSearchEngine = identifier;
	}

	m_ui->searchViewWidget->insertRow(createRow(searchEngine));

	emit settingsModified();
}

void PreferencesSearchPageWidget::importSearchEngine()
{
	const QString path(QFileDialog::getOpenFileName(this, tr("Select File"), QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0), Utils::formatFileTypes({tr("Open Search files (*.xml)")})));

	if (!path.isEmpty())
	{
		addSearchEngine(path, Utils::createIdentifier(QString(), m_searchEngines.keys()), false);
	}
}

void PreferencesSearchPageWidget::readdSearchEngine(QAction *action)
{
	if (action && !action->data().isNull())
	{
		addSearchEngine(SessionsManager::getReadableDataPath(QLatin1String("searches/") + action->data().toString() + QLatin1String(".xml")), action->data().toString(), true);
	}
}

void PreferencesSearchPageWidget::editSearchEngine()
{
	const QModelIndex index(m_ui->searchViewWidget->getIndex(m_ui->searchViewWidget->getCurrentRow(), 0));
	const QString identifier(index.data(Qt::UserRole).toString());

	if (identifier.isEmpty() || !m_searchEngines.contains(identifier))
	{
		return;
	}

	const QStringList keywords(getKeywords(m_ui->searchViewWidget->getSourceModel(), m_ui->searchViewWidget->getCurrentRow()));
	SearchEnginePropertiesDialog dialog(m_searchEngines[identifier].second, keywords, (identifier == m_defaultSearchEngine), this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	SearchEnginesManager::SearchEngineDefinition searchEngine(dialog.getSearchEngine());

	if (keywords.contains(searchEngine.keyword))
	{
		searchEngine.keyword = QString();
	}

	m_searchEngines[identifier] = qMakePair(true, searchEngine);

	if (dialog.isDefault())
	{
		m_defaultSearchEngine = identifier;
	}

	m_ui->searchViewWidget->setData(index, searchEngine.title, Qt::DisplayRole);
	m_ui->searchViewWidget->setData(index, searchEngine.description, Qt::ToolTipRole);
	m_ui->searchViewWidget->setData(m_ui->searchViewWidget->getIndex(index.row(), 1), searchEngine.keyword, Qt::DisplayRole);

	if (searchEngine.icon.isNull())
	{
		m_ui->searchViewWidget->setData(index, QColor(Qt::transparent), Qt::DecorationRole);
	}
	else
	{
		m_ui->searchViewWidget->setData(index, searchEngine.icon, Qt::DecorationRole);
	}

	emit settingsModified();
}

void PreferencesSearchPageWidget::removeSearchEngine()
{
	const QString identifier(m_ui->searchViewWidget->getIndex(m_ui->searchViewWidget->getCurrentRow(), 0).data(Qt::UserRole).toString());

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

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("searches/") + identifier + QLatin1String(".xml")));

	if (QFile::exists(path))
	{
		messageBox.setCheckBox(new QCheckBox(tr("Delete search engine permanently")));
	}

	if (messageBox.exec() == QMessageBox::Yes)
	{
		if (messageBox.checkBox() && messageBox.checkBox()->isChecked())
		{
			m_filesToRemove.append(path);
		}

		m_searchEngines.remove(identifier);

		m_ui->searchViewWidget->removeRow();

		updateReaddSearchMenu();

		emit settingsModified();
	}
}

void PreferencesSearchPageWidget::addSearchEngine(const QString &path, const QString &identifier, bool isReadding)
{
	QFile file(path);

	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(this, tr("Error"), tr("Failed to open Open Search file."));

		return;
	}

	SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::loadSearchEngine(&file, identifier, false));

	file.close();

	if (searchEngine.identifier.isEmpty() || m_searchEngines.contains(identifier))
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

		searchEngine.keyword = QString();
	}

	m_searchEngines[identifier] = qMakePair(false, searchEngine);

	m_ui->searchViewWidget->insertRow(createRow(searchEngine));

	if (isReadding)
	{
		updateReaddSearchMenu();
	}

	emit settingsModified();
}

void PreferencesSearchPageWidget::updateSearchActions()
{
	const QModelIndex index(m_ui->searchViewWidget->currentIndex());
	const int currentRow(m_ui->searchViewWidget->getCurrentRow());
	const bool isSelected(currentRow >= 0 && currentRow < m_ui->searchViewWidget->getRowCount());

	if (index.column() != 1)
	{
		m_ui->searchViewWidget->setCurrentIndex(index.sibling(index.row(), 1));
	}

	m_ui->editSearchButton->setEnabled(isSelected);
	m_ui->removeSearchButton->setEnabled(isSelected);
}

void PreferencesSearchPageWidget::updateReaddSearchMenu()
{
	if (!m_ui->addSearchButton->menu())
	{
		return;
	}

	QStringList availableIdentifiers;
	QList<SearchEnginesManager::SearchEngineDefinition> availableSearchEngines;
	QList<QFileInfo> allSearchEngines(QDir(SessionsManager::getReadableDataPath(QLatin1String("searches"))).entryInfoList(QDir::Files));
	allSearchEngines.append(QDir(SessionsManager::getReadableDataPath(QLatin1String("searches"), true)).entryInfoList(QDir::Files));

	for (int i = 0; i < allSearchEngines.count(); ++i)
	{
		const QString identifier(allSearchEngines.at(i).baseName());

		if (!m_searchEngines.contains(identifier) && !availableIdentifiers.contains(identifier))
		{
			QFile file(allSearchEngines.at(i).absoluteFilePath());

			if (file.open(QIODevice::ReadOnly))
			{
				const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::loadSearchEngine(&file, identifier));

				if (!searchEngine.identifier.isEmpty())
				{
					availableIdentifiers.append(identifier);

					availableSearchEngines.append(searchEngine);
				}

				file.close();
			}
		}
	}

	m_ui->addSearchButton->menu()->actions().at(2)->menu()->clear();
	m_ui->addSearchButton->menu()->actions().at(2)->menu()->setEnabled(!availableSearchEngines.isEmpty());

	for (int i = 0; i < availableSearchEngines.count(); ++i)
	{
		m_ui->addSearchButton->menu()->actions().at(2)->menu()->addAction(availableSearchEngines.at(i).icon, (availableSearchEngines.at(i).title.isEmpty() ? tr("(Untitled)") : availableSearchEngines.at(i).title))->setData(availableSearchEngines.at(i).identifier);
	}
}

void PreferencesSearchPageWidget::save()
{
	for (int i = 0; i < m_filesToRemove.count(); ++i)
	{
		QFile::remove(m_filesToRemove.at(i));
	}

	m_filesToRemove.clear();

	QStringList searchEnginesOrder;

	for (int i = 0; i < m_ui->searchViewWidget->getRowCount(); ++i)
	{
		const QString identifier(m_ui->searchViewWidget->getIndex(i, 0).data(Qt::UserRole).toString());
		const QString keyword(m_ui->searchViewWidget->getIndex(i, 1).data(Qt::DisplayRole).toString());

		if (!identifier.isEmpty())
		{
			searchEnginesOrder.append(identifier);
		}

		if (m_searchEngines.contains(identifier) && m_searchEngines[identifier].second.keyword != keyword)
		{
			m_searchEngines[identifier].first = true;
			m_searchEngines[identifier].second.keyword = keyword;
		}
	}

	QHash<QString, QPair<bool, SearchEnginesManager::SearchEngineDefinition> >::iterator searchEnginesIterator;

	for (searchEnginesIterator = m_searchEngines.begin(); searchEnginesIterator != m_searchEngines.end(); ++searchEnginesIterator)
	{
		if (searchEnginesIterator.value().first)
		{
			SearchEnginesManager::saveSearchEngine(searchEnginesIterator.value().second);
		}
	}

	if (SettingsManager::getValue(SettingsManager::Search_SearchEnginesOrderOption).toStringList() == searchEnginesOrder)
	{
		SearchEnginesManager::loadSearchEngines();
	}
	else
	{
		SettingsManager::setValue(SettingsManager::Search_SearchEnginesOrderOption, searchEnginesOrder);
	}

	SettingsManager::setValue(SettingsManager::Search_DefaultSearchEngineOption, m_defaultSearchEngine);
	SettingsManager::setValue(SettingsManager::Search_SearchEnginesSuggestionsOption, m_ui->searchSuggestionsCheckBox->isChecked());

	updateReaddSearchMenu();
}

QList<QStandardItem*> PreferencesSearchPageWidget::createRow(const SearchEnginesManager::SearchEngineDefinition &searchEngine) const
{
	QList<QStandardItem*> items({new QStandardItem(searchEngine.icon, searchEngine.title), new QStandardItem(searchEngine.keyword)});
	items[0]->setData(QColor(Qt::transparent), Qt::DecorationRole);
	items[0]->setData(searchEngine.identifier, Qt::UserRole);
	items[0]->setToolTip(searchEngine.description);
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);

	if (!searchEngine.icon.isNull())
	{
		items[0]->setIcon(searchEngine.icon);
	}

	return items;
}

QStringList PreferencesSearchPageWidget::getKeywords(const QAbstractItemModel *model, int excludeRow)
{
	QStringList keywords;

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

}
