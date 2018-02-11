/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QCoreApplication>
#include <QtCore/QVariantMap>
#include <QtCore/QVector>

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
		BookmarksBar,
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

	enum ToolBarsMode
	{
		NormalMode = 0,
		FullScreenMode
	};

	struct ToolBarDefinition final
	{
		struct Entry final
		{
			QString action;
			QVariantMap options;
			QVariantMap parameters;
			QVector<Entry> entries;
		};

		QString title;
		QString bookmarksPath;
		QString currentPanel;
		QStringList panels;
		QVector<Entry> entries;
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

		void setVisibility(ToolBarsMode mode, ToolBarVisibility visibility)
		{
			switch (mode)
			{
				case NormalMode:
					normalVisibility = visibility;

					break;
				case FullScreenMode:
					fullScreenVisibility = visibility;

					break;
				default:
					break;
			}
		}

		QString getTitle() const
		{
			return (isDefault ? QCoreApplication::translate("actions", title.toUtf8()) : title);
		}

		ToolBarVisibility getVisibility(ToolBarsMode mode) const
		{
			return ((mode == FullScreenMode) ? fullScreenVisibility : normalVisibility);
		}

		bool isGlobal() const
		{
			return (identifier != AddressBar && identifier != ProgressBar);
		}

		bool isValid() const
		{
			return (identifier >= 0);
		}
	};

	static void createInstance();
	static void addToolBar(ToolBarType type);
	static void configureToolBar(int identifier);
	static void resetToolBar(int identifier);
	static void removeToolBar(int identifier);
	static void resetToolBars();
	static void setToolBar(ToolBarsManager::ToolBarDefinition definition);
	static ToolBarsManager* getInstance();
	static QString getToolBarName(int identifier);
	static QVector<ToolBarsManager::ToolBarDefinition> getToolBarDefinitions(Qt::ToolBarAreas areas = Qt::AllToolBarAreas);
	static ToolBarsManager::ToolBarDefinition getToolBarDefinition(int identifier);
	static int getToolBarIdentifier(const QString &name);
	static bool areToolBarsLocked();

protected:
	explicit ToolBarsManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	static void ensureInitialized();
	static QJsonValue encodeEntry(const ToolBarDefinition::Entry &definition);
	static ToolBarDefinition::Entry decodeEntry(const QJsonValue &value);
	static QHash<QString, ToolBarDefinition> loadToolBars(const QString &path, bool isDefault);

protected slots:
	void scheduleSave();
	void handleOptionChanged(int identifier, const QVariant &value);

private:
	int m_saveTimer;

	static ToolBarsManager *m_instance;
	static QMap<int, QString> m_identifiers;
	static QVector<ToolBarDefinition> m_definitions;
	static int m_toolBarIdentifierEnumerator;
	static bool m_areToolBarsLocked;
	static bool m_isLoading;

signals:
	void toolBarAdded(int identifier);
	void toolBarModified(int identifier);
	void toolBarMoved(int identifier);
	void toolBarRemoved(int identifier);
	void toolBarsLockedChanged(bool areLocked);
};

}

#endif
