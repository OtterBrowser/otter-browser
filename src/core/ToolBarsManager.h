/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TOOLBARSMANAGER_H
#define OTTER_TOOLBARSMANAGER_H

#include "ActionsManager.h"

namespace Otter
{

class ToolBarsManager : public QObject
{
	Q_OBJECT
	Q_ENUMS(ToolBarIdentifier)

public:
	enum ToolBarIdentifier
	{
		MenuBar = 0,
		TabBar,
		NavigationBar,
		ProgressBar,
		StatusBar,
		OtherToolBar
	};

	enum ToolBarVisibility
	{
		AlwaysVisibleToolBar = 0,
		OnHoverVisibleToolBar = 1,
		AutoVisibilityToolBar = 2,
		AlwaysHiddenToolBar = 3
	};

	struct ToolBarDefinition
	{
		QString title;
		QString bookmarksPath;
		QList<ActionsManager::ActionEntryDefinition> entries;
		ToolBarVisibility normalVisibility;
		ToolBarVisibility fullScreenVisibility;
		Qt::ToolBarArea location;
		Qt::ToolButtonStyle buttonStyle;
		int identifier;
		int iconSize;
		int maximumButtonSize;
		int row;
		bool canReset;
		bool isDefault;
		bool wasRemoved;

		ToolBarDefinition() : normalVisibility(AlwaysVisibleToolBar), fullScreenVisibility(AlwaysHiddenToolBar), location(Qt::NoToolBarArea), buttonStyle(Qt::ToolButtonIconOnly), identifier(-1), iconSize(-1), maximumButtonSize(-1), row(-1), canReset(false), isDefault(false), wasRemoved(false) {}
	};

	static void createInstance(QObject *parent = NULL);
	static void setToolBar(ToolBarsManager::ToolBarDefinition definition);
	static void setToolBarsLocked(bool locked);
	static ToolBarsManager* getInstance();
	static QString getToolBarName(int identifier);
	static QVector<ToolBarsManager::ToolBarDefinition> getToolBarDefinitions();
	static ToolBarsManager::ToolBarDefinition getToolBarDefinition(int identifier);
	static int getToolBarIdentifier(const QString &name);
	static bool areToolBarsLocked();

public slots:
	void addToolBar();
	void addBookmarksBar();
	void configureToolBar(int identifier = -1);
	void resetToolBar(int identifier = -1);
	void removeToolBar(int identifier = -1);
	void resetToolBars();

protected:
	void timerEvent(QTimerEvent *event);
	static QJsonValue encodeEntry(const ActionsManager::ActionEntryDefinition &definition);
	static ActionsManager::ActionEntryDefinition decodeEntry(const QJsonValue &value);
	static QHash<QString, ToolBarsManager::ToolBarDefinition> loadToolBars(const QString &path, bool isDefault);

protected slots:
	void optionChanged(const QString &key, const QVariant &value);
	void scheduleSave();

private:
	int m_saveTimer;

private:
	explicit ToolBarsManager(QObject *parent = NULL);

	static ToolBarsManager *m_instance;
	static QMap<int, QString> m_identifiers;
	static QVector<ToolBarsManager::ToolBarDefinition> m_definitions;
	static bool m_areToolBarsLocked;

signals:
	void toolBarAdded(int identifier);
	void toolBarModified(int identifier);
	void toolBarRemoved(int identifier);
	void toolBarsLockedChanged(bool locked);
};

}

#endif
