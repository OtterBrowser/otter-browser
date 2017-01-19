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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_PREFERENCESSEARCHPAGEWIDGET_H
#define OTTER_PREFERENCESSEARCHPAGEWIDGET_H

#include "../ItemDelegate.h"
#include "../../core/SearchEnginesManager.h"

#include <QtGui/QMovie>
#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class PreferencesSearchPageWidget;
}

class SearchEngineTitleDelegate : public ItemDelegate
{
public:
	explicit SearchEngineTitleDelegate(QObject *parent);

	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

class SearchEngineKeywordDelegate : public ItemDelegate
{
public:
	explicit SearchEngineKeywordDelegate(QObject *parent);

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class PreferencesSearchPageWidget : public QWidget
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		IsUpdatingRole
	};

	explicit PreferencesSearchPageWidget(QWidget *parent = nullptr);
	~PreferencesSearchPageWidget();

	static QMovie* getUpdateMovie();
	static QStringList getKeywords(const QAbstractItemModel *model, int excludeRow = -1);

protected:
	void changeEvent(QEvent *event) override;
	void addSearchEngine(const QString &path, const QString &identifier, bool isReadding);
	void updateReaddSearchEngineMenu();
	QList<QStandardItem*> createRow(const SearchEnginesManager::SearchEngineDefinition &searchEngine) const;

protected slots:
	void createSearchEngine();
	void importSearchEngine();
	void readdSearchEngine(QAction *action);
	void editSearchEngine();
	void updateSearchEngine();
	void removeSearchEngine();
	void handleSearchEngineUpdate(bool isSuccess);
	void updateSearchEngineActions();
	void save();

private:
	QString m_defaultSearchEngine;
	QStringList m_filesToRemove;
	QHash<QString, SearchEngineFetchJob*> m_updateJobs;
	QHash<QString, QPair<bool, SearchEnginesManager::SearchEngineDefinition> > m_searchEngines;
	Ui::PreferencesSearchPageWidget *m_ui;

	static QMovie *m_updateMovie;

signals:
	void settingsModified();
};

}

#endif
