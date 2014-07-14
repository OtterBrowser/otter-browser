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

#ifndef OTTER_SHORTCUTSMANAGER_H
#define OTTER_SHORTCUTSMANAGER_H

#include <QtCore/QObject>
#include <QtWidgets/QAction>

namespace Otter
{

class ShortcutsManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void loadProfiles();
	static ShortcutsManager* getInstance();
	static QHash<QString, QList<QKeySequence> > getShortcuts();
	static QKeySequence getNativeShortcut(const QString &action);

protected:
	explicit ShortcutsManager(QObject *parent = NULL);

	void timerEvent(QTimerEvent *event);

protected slots:
	void optionChanged(const QString &option);
	void triggerMacro();

private:
	int m_reloadTimer;

	static ShortcutsManager *m_instance;
	static QHash<QAction*, QStringList> m_macros;
	static QHash<QString, QList<QKeySequence> > m_shortcuts;
	static QHash<QString, QKeySequence> m_nativeShortcuts;

signals:
	void shortcutsChanged();
};

}

#endif
