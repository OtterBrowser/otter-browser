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

#ifndef OTTER_PREFERENCESSEARCHPAGEWIDGET_H
#define OTTER_PREFERENCESSEARCHPAGEWIDGET_H

#include "../ItemDelegate.h"
#include "../../core/SearchEnginesManager.h"

#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class PreferencesSearchPageWidget;
}

class SearchKeywordDelegate : public ItemDelegate
{
public:
	explicit SearchKeywordDelegate(QObject *parent);

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class PreferencesSearchPageWidget : public QWidget
{
	Q_OBJECT

public:
	explicit PreferencesSearchPageWidget(QWidget *parent = NULL);
	~PreferencesSearchPageWidget();

protected:
	void changeEvent(QEvent *event);
	void updateReaddSearchMenu();

protected slots:
	void addSearchEngine();
	void readdSearchEngine(QAction *action);
	void editSearchEngine();
	void removeSearchEngine();
	void updateSearchActions();
	void save();

private:
	QString m_defaultSearchEngine;
	QStringList m_filesToRemove;
	QHash<QString, QPair<bool, SearchEnginesManager::SearchEngineDefinition> > m_searchEngines;
	Ui::PreferencesSearchPageWidget *m_ui;

signals:
	void settingsModified();
};

}

#endif
