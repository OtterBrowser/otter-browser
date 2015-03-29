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

#ifndef OTTER_TOOLBARSMANAGER_H
#define OTTER_TOOLBARSMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>

namespace Otter
{

struct ToolBarActionDefinition
{
	QString action;
	QVariantMap options;
};

struct ToolBarDefinition
{
	QString identifier;
	QString title;
	QString bookmarksPath;
	QList<ToolBarActionDefinition> actions;
	Qt::ToolBarArea location;
	Qt::ToolButtonStyle buttonStyle;
	int iconSize;
	int maximumButtonSize;
	bool isDefault;

	ToolBarDefinition() : location(Qt::NoToolBarArea), buttonStyle(Qt::ToolButtonIconOnly), iconSize(-1), maximumButtonSize(-1), isDefault(false) {}
};

class ToolBarsManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static ToolBarsManager* getInstance();
	static QList<ToolBarDefinition> getToolBarDefinitions();
	static ToolBarDefinition getToolBarDefinition(const QString &identifier);

protected:
	static QHash<QString, ToolBarDefinition> loadToolBars(const QString &path, bool isDefault);

private:
	explicit ToolBarsManager(QObject *parent = NULL);

	static ToolBarsManager *m_instance;
	static QHash<QString, ToolBarDefinition> m_definitions;

signals:
	void toolBarAdded(const QString &identifier);
	void toolBarModified(const QString &identifier);
	void toolBarRemoved(const QString &identifier);
};

}

#endif
