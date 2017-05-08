/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TOOLBARSMANAGER_H
#define OTTER_TOOLBARSMANAGER_H

#include "ActionsManager.h"

#include <QtCore/QCoreApplication>

namespace Otter
{

class ToolBarsManager final : public QObject
{
	Q_OBJECT
	Q_ENUMS(ToolBarIdentifier)

public:
	enum ToolBarIdentifier
	{
		MenuBar = 0,
		TabBar,
		AddressBar,
		ProgressBar,
		SideBar,
		StatusBar,
		ErrorConsoleBar,
		OtherToolBar
	};

	enum ToolBarType
	{
		ActionsBarType = 0,
		BookmarksBarType,
		SideBarType
	};

	enum ToolBarVisibility
	{
		AlwaysVisibleToolBar = 0,
		OnHoverVisibleToolBar,
		AutoVisibilityToolBar,
		AlwaysHiddenToolBar
	};

	struct ToolBarDefinition
	{
		QString title;
		QString bookmarksPath;
		QString currentPanel;
		QStringList panels;
		QVector<ActionsManager::ActionEntryDefinition> entries;
		ToolBarType type = ActionsBarType;
		ToolBarVisibility normalVisibility = AlwaysVisibleToolBar;
		ToolBarVisibility fullScreenVisibility = AlwaysHiddenToolBar;
		Qt::ToolBarArea location = Qt::NoToolBarArea;
		Qt::ToolButtonStyle buttonStyle = Qt::ToolButtonIconOnly;
		int identifier = -1;
		int iconSize = -1;
		int maximumButtonSize = -1;
		int panelSize = -1;
		int row = -1;
		bool canReset = false;
		bool hasToggle = false;
		bool isDefault = false;
		bool wasRemoved = false;

		QString getTitle() const
		{
			return (isDefault ? QCoreApplication::translate("actions", title.toUtf8()) : title);
		}
	};

	static void createInstance();
	static void resetToolBars();
	static void setToolBar(ToolBarsManager::ToolBarDefinition definition);
	static void setToolBarsLocked(bool locked);
	static ToolBarsManager* getInstance();
	static QString getToolBarName(int identifier);
	static QVector<ToolBarsManager::ToolBarDefinition> getToolBarDefinitions(Qt::ToolBarAreas areas = Qt::AllToolBarAreas);
	static ToolBarsManager::ToolBarDefinition getToolBarDefinition(int identifier);
	static int getToolBarIdentifier(const QString &name);
	static bool areToolBarsLocked();

public slots:
	void addToolBar();
	void addBookmarksBar();
	void addSideBar();
	void configureToolBar(int identifier = -1);
	void resetToolBar(int identifier = -1);
	void removeToolBar(int identifier = -1);

protected:
	explicit ToolBarsManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	static QJsonValue encodeEntry(const ActionsManager::ActionEntryDefinition &definition);
	static ActionsManager::ActionEntryDefinition decodeEntry(const QJsonValue &value);
	static QHash<QString, ToolBarsManager::ToolBarDefinition> loadToolBars(const QString &path, bool isDefault);

protected slots:
	void scheduleSave();
	void handleOptionChanged(int identifier, const QVariant &value);

private:
	int m_saveTimer;

	static ToolBarsManager *m_instance;
	static QMap<int, QString> m_identifiers;
	static QVector<ToolBarsManager::ToolBarDefinition> m_definitions;
	static int m_toolBarIdentifierEnumerator;
	static bool m_areToolBarsLocked;

signals:
	void toolBarAdded(int identifier);
	void toolBarModified(int identifier);
	void toolBarMoved(int identifier);
	void toolBarRemoved(int identifier);
	void toolBarsLockedChanged(bool locked);
};

}

#endif
