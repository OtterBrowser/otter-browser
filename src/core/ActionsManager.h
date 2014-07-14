/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QObject>
#include <QtWidgets/QAction>

#include "Action.h"

namespace Otter
{

struct ActionDefinition
{
	QString name;
	QString text;
	QIcon icon;
	ActionIdentifier identifier;
	ActionScope scope;
};

class MainWindow;

class ActionsManager : public QObject
{
	Q_OBJECT

public:
	explicit ActionsManager(MainWindow *parent = NULL);

	void triggerAction(const QString &action);
	void triggerAction(ActionIdentifier action);
	static void triggerAction(const QString &action, QObject *parent);
	static void triggerAction(ActionIdentifier action, QObject *parent);
	static void setupLocalAction(QAction *globalAction, QAction *localAction, bool connectTrigger = false);
	QAction* getAction(const QString &action);
	QAction* getAction(ActionIdentifier action);
	static QAction* getAction(const QString &action, QObject *parent);
	static QAction* getAction(ActionIdentifier action, QObject *parent);
	static QList<ActionDefinition> getActions();
	static bool registerAction(const QLatin1String &name, const QString &text, const QIcon &icon = QIcon(), ActionIdentifier identifier = UnknownAction);

public slots:
	void updateActions();

protected:
	static ActionsManager* findManager(QObject *parent);

private:
	MainWindow *m_window;
	QHash<QString, QAction*> m_actions;
	QHash<ActionIdentifier, QAction*> m_standardActions;

	static QHash<QString, ActionDefinition> m_definitions;
	static QHash<QObject*, MainWindow*> m_cache;
};

}

#endif
