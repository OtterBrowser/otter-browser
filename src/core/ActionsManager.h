/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_ACTIONSMANAGER_H
#define OTTER_ACTIONSMANAGER_H

#include "Action.h"

#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtWidgets/QAction>
#include <QtWidgets/QShortcut>

namespace Otter
{

enum ToolBarLocation
{
	UnknownToolBarLocation = 0,
	TopToolBarLocation = 1,
	BottomBarArea = 2,
	LeftToolBarLocation = 3,
	RightToolBarLocation = 4,
	NavigationToolBarLocation = 5,
	StatusToolBarLocation = 6,
	TabsToolBarLocation = 7,
	MenuToolBarLocation = 8
};

struct ToolBarActionDefinition
{
	QString action;
	QVariantMap options;
};

struct ToolBarDefinition
{
	QString name;
	QString title;
	QList<ToolBarActionDefinition> actions;
	ToolBarLocation location;
};

struct ActionDefinition
{
	QString text;
	QString description;
	QIcon icon;
	QVector<QKeySequence> shortcuts;
	int identifier;
	bool isCheckable;
	bool isChecked;
	bool isEnabled;

	ActionDefinition() : identifier(-1), isCheckable(false), isChecked(false), isEnabled(true) {}
};

class ActionsManager;
class MainWindow;
class Window;

class ActionsManagerHelper : public QObject
{
	Q_OBJECT

protected:
	ActionsManagerHelper(QObject *parent);

	void timerEvent(QTimerEvent *event);
	int registerAction(int identifier, const QString &text, const QString &description = QString(), const QIcon &icon = QIcon(), bool isEnabled = true, bool isCheckable = false, bool isChecked = false);

	int reloadShortcutsTimer;
	QVector<ActionDefinition> actionDefinitions;
	QHash<QString, ToolBarDefinition> toolBarDefinitions;

protected slots:
	void optionChanged(const QString &option);

signals:
	void shortcutsChanged();

friend class ActionsManager;
};

class ActionsManager : public QObject
{
	Q_OBJECT

public:
	explicit ActionsManager(MainWindow *parent = NULL);

	void triggerAction(int identifier, bool checked = false);
	static void triggerAction(int identifier, QObject *parent, bool checked = false);
	static void loadShortcuts();
	void setCurrentWindow(Window *window);
	Action* getAction(int identifier);
	static Action* getAction(int identifier, QObject *parent);
	static QString getActionName(int identifier);
	static QList<ActionDefinition> getActionDefinitions();
	static QList<ToolBarDefinition> getToolBarDefinitions();
	static ActionDefinition getActionDefinition(int identifier);
	static ToolBarDefinition getToolBarDefinition(const QString &toolBar);
	static int getActionIdentifier(const QString &name);

protected:
	static void initialize();
	static ActionsManager* findManager(QObject *parent);

protected slots:
	void actionTriggered();
	void actionTriggered(bool checked);
	void updateShortcuts();

private:
	MainWindow *m_mainWindow;
	QPointer<Window> m_window;
	QVector<Action*> m_standardActions;
	QVector<QPair<int, QVector<QShortcut*> > > m_actionShortcuts;

	static ActionsManagerHelper *m_helper;
	static Action *m_dummyAction;
	static QMutex m_mutex;
};

}

#endif
