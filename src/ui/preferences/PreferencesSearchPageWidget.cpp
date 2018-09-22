/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PreferencesSearchPageWidget.h"
#include "../Animation.h"
#include "../LineEditWidget.h"
#include "../SearchEnginePropertiesDialog.h"
#include "../../core/ItemModel.h"
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

SearchEngineTitleDelegate::SearchEngineTitleDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void SearchEngineTitleDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	ItemDelegate::initStyleOption(option, index);

	if (index.data(PreferencesSearchPageWidget::IsUpdatingRole).toBool())
	{
		const Animation *animation(PreferencesSearchPageWidget::getUpdateAnimation());

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

	const QStringList keywords(PreferencesSearchPageWidget::getKeywords(index.model(), index.row()));
	LineEditWidget *widget(new LineEditWidget(index.data(Qt::DisplayRole).toString(), parent));
	widget->setValidator(new QRegularExpressionValidator(QRegularExpression((keywords.isEmpty() ? QString() : QStringLiteral("(?!\\b(%1)\\b)").arg(keywords.join('|'))) + "[a-z0-9]*"), widget));
	widget->setFocus();

	return widget;
}

Animation* PreferencesSearchPageWidget::m_updateAnimation = nullptr;

PreferencesSearchPageWidget::PreferencesSearchPageWidget(QWidget *parent) : QWidget(parent),
	m_ui(new Ui::PreferencesSearchPageWidget)
{
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

		if (!searchEngine.isValid())
		{
			continue;
		}

		m_searchEngines[searchEngine.identifier] = {false, searchEngine};

		searchEnginesModel->appendRow(createRow(searchEngine, (searchEngine.identifier == defaultSearchEngine)));
	}

	m_ui->searchViewWidget->setModel(searchEnginesModel);
	m_ui->searchViewWidget->setItemDelegateForColumn(0, new SearchEngineTitleDelegate(this));
	m_ui->searchViewWidget->setItemDelegateForColumn(1, new SearchEngineKeywordDelegate(this));
	m_ui->searchViewWidget->setExclusive(true);
	m_ui->searchSuggestionsCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Search_SearchEnginesSuggestionsOption).toBool());

	QMenu *addSearchEngineMenu(new QMenu(m_ui->addSearchButton));
	addSearchEngineMenu->addAction(tr("New…"), this, &PreferencesSearchPageWidget::createSearchEngine);
	addSearchEngineMenu->addAction(tr("File…"), this, &PreferencesSearchPageWidget::importSearchEngine);
	addSearchEngineMenu->addAction(tr("Readd"))->setMenu(new QMenu(m_ui->addSearchButton));

	m_ui->addSearchButton->setMenu(addSearchEngineMenu);
	m_ui->moveDownSearchButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-down")));
	m_ui->moveUpSearchButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-up")));

	updateReaddSearchEngineMenu();

	connect(m_ui->searchFilterLineEditWidget, &LineEditWidget::textChanged, m_ui->searchViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->searchViewWidget, &ItemViewWidget::canMoveRowDownChanged, m_ui->moveDownSearchButton, &QToolButton::setEnabled);
	connect(m_ui->searchViewWidget, &ItemViewWidget::canMoveRowUpChanged, m_ui->moveUpSearchButton, &QToolButton::setEnabled);
	connect(m_ui->searchViewWidget, &ItemViewWidget::needsActionsUpdate, this, &PreferencesSearchPageWidget::updateSearchEngineActions);
	connect(m_ui->searchViewWidget, &ItemViewWidget::modified, this, &PreferencesSearchPageWidget::settingsModified);
	connect(m_ui->addSearchButton->menu()->actions().at(2)->menu(), &QMenu::triggered, this, &PreferencesSearchPageWidget::readdSearchEngine);
	connect(m_ui->editSearchButton, &QPushButton::clicked, this, &PreferencesSearchPageWidget::editSearchEngine);
	connect(m_ui->updateSearchButton, &QPushButton::clicked, this, &PreferencesSearchPageWidget::updateSearchEngine);
	connect(m_ui->removeSearchButton, &QPushButton::clicked, this, &PreferencesSearchPageWidget::removeSearchEngine);
	connect(m_ui->moveDownSearchButton, &QToolButton::clicked, m_ui->searchViewWidget, &ItemViewWidget::moveDownRow);
	connect(m_ui->moveUpSearchButton, &QToolButton::clicked, m_ui->searchViewWidget, &ItemViewWidget::moveUpRow);
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
		m_ui->searchViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Name"), tr("Keyword")});
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
	searchEngine.icon = ThemesManager::createIcon(QLatin1String("edit-find"));

	SearchEnginePropertiesDialog dialog(searchEngine, getKeywords(m_ui->searchViewWidget->getSourceModel()), this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	searchEngine = dialog.getSearchEngine();

	m_searchEngines[identifier] = {true, searchEngine};

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
		addSearchEngine(SessionsManager::getReadableDataPath(QLatin1String("searchEngines/") + action->data().toString() + QLatin1String(".xml")), action->data().toString(), true);
	}
}

void PreferencesSearchPageWidget::editSearchEngine()
{
	const QModelIndex index(m_ui->searchViewWidget->getIndex(m_ui->searchViewWidget->getCurrentRow(), 0));
	const QString identifier(index.data(IdentifierRole).toString());

	if (identifier.isEmpty() || !m_searchEngines.contains(identifier))
	{
		return;
	}

	const QStringList keywords(getKeywords(m_ui->searchViewWidget->getSourceModel(), m_ui->searchViewWidget->getCurrentRow()));
	SearchEnginePropertiesDialog dialog(m_searchEngines[identifier].second, keywords, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	SearchEnginesManager::SearchEngineDefinition searchEngine(dialog.getSearchEngine());

	if (keywords.contains(searchEngine.keyword))
	{
		searchEngine.keyword.clear();
	}

	m_searchEngines[identifier] = {true, searchEngine};

	m_ui->searchViewWidget->setData(index, searchEngine.title, Qt::DisplayRole);
	m_ui->searchViewWidget->setData(index, searchEngine.title, Qt::ToolTipRole);
	m_ui->searchViewWidget->setData(m_ui->searchViewWidget->getIndex(index.row(), 1), searchEngine.keyword, Qt::DisplayRole);
	m_ui->searchViewWidget->setData(m_ui->searchViewWidget->getIndex(index.row(), 1), searchEngine.keyword, Qt::ToolTipRole);

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

void PreferencesSearchPageWidget::updateSearchEngine()
{
	const QModelIndex index(m_ui->searchViewWidget->getIndex(m_ui->searchViewWidget->getCurrentRow(), 0));
	const QString identifier(index.data(IdentifierRole).toString());

	if (!identifier.isEmpty() && m_searchEngines.contains(identifier) && !m_updateJobs.contains(identifier))
	{
		if (!m_updateAnimation)
		{
			const QString path(ThemesManager::getAnimationPath(QLatin1String("spinner")));

			if (path.isEmpty())
			{
				m_updateAnimation = new SpinnerAnimation(this);
			}
			else
			{
				m_updateAnimation = new GenericAnimation(path, this);
			}

			m_updateAnimation->start();

			connect(m_updateAnimation, &Animation::frameChanged, m_ui->searchViewWidget->viewport(), static_cast<void(QWidget::*)()>(&QWidget::update));
		}

		m_ui->searchViewWidget->setData(index, true, IsUpdatingRole);
		m_ui->searchViewWidget->update();

		SearchEngineFetchJob *job(new SearchEngineFetchJob(m_searchEngines[identifier].second.selfUrl, identifier, false, this));

		m_updateJobs[identifier] = job;

		connect(job, &SearchEngineFetchJob::jobFinished, this, &PreferencesSearchPageWidget::handleSearchEngineUpdate);

		job->start();
	}
}

void PreferencesSearchPageWidget::removeSearchEngine()
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

	if (messageBox.exec() == QMessageBox::Yes)
	{
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

	m_searchEngines[identifier] = {false, searchEngine};

	m_ui->searchViewWidget->insertRow(createRow(searchEngine));

	if (isReadding)
	{
		updateReaddSearchEngineMenu();
	}

	emit settingsModified();
}

void PreferencesSearchPageWidget::handleSearchEngineUpdate(bool isSuccess)
{
	SearchEngineFetchJob *job(qobject_cast<SearchEngineFetchJob*>(sender()));

	if (!job)
	{
		return;
	}

	SearchEnginesManager::SearchEngineDefinition searchEngine(job->getSearchEngine());
	const QString identifier(searchEngine.isValid() ? searchEngine.identifier : m_updateJobs.key(job));

	if (!identifier.isEmpty())
	{
		for (int i = 0; i < m_ui->searchViewWidget->getRowCount(); ++i)
		{
			const QModelIndex index(m_ui->searchViewWidget->getIndex(i, 0));

			if (index.data(IdentifierRole).toString() == identifier)
			{
				if (isSuccess)
				{
					m_ui->searchViewWidget->setData(index, searchEngine.title, Qt::DisplayRole);
					m_ui->searchViewWidget->setData(index, searchEngine.title, Qt::ToolTipRole);

					if (searchEngine.icon.isNull())
					{
						m_ui->searchViewWidget->setData(index, QColor(Qt::transparent), Qt::DecorationRole);
					}
					else
					{
						m_ui->searchViewWidget->setData(index, searchEngine.icon, Qt::DecorationRole);
					}
				}

				m_ui->searchViewWidget->setData(index, false, IsUpdatingRole);

				break;
			}
		}

		m_updateJobs.remove(identifier);

		if (m_updateJobs.isEmpty())
		{
			m_updateAnimation->deleteLater();
			m_updateAnimation = nullptr;
		}
	}

	if (!isSuccess)
	{
		QMessageBox::warning(this, tr("Error"), tr("Failed to update search engine."), QMessageBox::Close);

		return;
	}

	if (m_searchEngines.contains(identifier))
	{
		searchEngine.keyword = m_searchEngines[identifier].second.keyword;

		m_searchEngines[identifier] = {true, searchEngine};
	}
}

void PreferencesSearchPageWidget::updateSearchEngineActions()
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
	m_ui->updateSearchButton->setEnabled(isSelected && m_searchEngines.contains(identifier) && m_searchEngines[identifier].second.selfUrl.isValid());
	m_ui->removeSearchButton->setEnabled(isSelected);
}

void PreferencesSearchPageWidget::updateReaddSearchEngineMenu()
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
	searchEnginesOrder.reserve(m_ui->searchViewWidget->getRowCount());

	QString defaultSearchEngine;

	for (int i = 0; i < m_ui->searchViewWidget->getRowCount(); ++i)
	{
		const QString identifier(m_ui->searchViewWidget->getIndex(i, 0).data(IdentifierRole).toString());
		const QString keyword(m_ui->searchViewWidget->getIndex(i, 1).data(Qt::DisplayRole).toString());

		if (!identifier.isEmpty())
		{
			searchEnginesOrder.append(identifier);

			if (m_ui->searchViewWidget->getIndex(i, 0).data(Qt::CheckStateRole) == Qt::Checked)
			{
				defaultSearchEngine = identifier;
			}
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

	if (SettingsManager::getOption(SettingsManager::Search_SearchEnginesOrderOption).toStringList() == searchEnginesOrder)
	{
		SearchEnginesManager::loadSearchEngines();
	}
	else
	{
		SettingsManager::setOption(SettingsManager::Search_SearchEnginesOrderOption, searchEnginesOrder);
	}

	SettingsManager::setOption(SettingsManager::Search_DefaultSearchEngineOption, defaultSearchEngine);
	SettingsManager::setOption(SettingsManager::Search_SearchEnginesSuggestionsOption, m_ui->searchSuggestionsCheckBox->isChecked());

	updateReaddSearchEngineMenu();
}

Animation* PreferencesSearchPageWidget::getUpdateAnimation()
{
	return m_updateAnimation;
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

QList<QStandardItem*> PreferencesSearchPageWidget::createRow(const SearchEnginesManager::SearchEngineDefinition &searchEngine, bool isDefault) const
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
		items[0]->setData(QColor(Qt::transparent), Qt::DecorationRole);
	}

	return items;
}

}
