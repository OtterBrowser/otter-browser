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

#include "ShortcutsManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "Utils.h"
#include "../ui/MainWindow.h"

#include <QtCore/QFile>
#include <QtCore/QSettings>

namespace Otter
{

ShortcutsManager* ShortcutsManager::m_instance = NULL;
QHash<QAction*, QStringList> ShortcutsManager::m_macros;
QHash<QString, QList<QKeySequence> > ShortcutsManager::m_shortcuts;
QHash<QString, QKeySequence> ShortcutsManager::m_nativeShortcuts;

ShortcutsManager::ShortcutsManager(QObject *parent) : QObject(parent),
	m_reloadTimer(0)
{
	m_instance = this;

	m_nativeShortcuts[QLatin1String("NewWindow")] = QKeySequence(QKeySequence::New);
	m_nativeShortcuts[QLatin1String("Open")] = QKeySequence(QKeySequence::Open);
	m_nativeShortcuts[QLatin1String("Save")] = QKeySequence(QKeySequence::Save);
	m_nativeShortcuts[QLatin1String("Exit")] = QKeySequence(QKeySequence::Quit);
	m_nativeShortcuts[QLatin1String("Undo")] = QKeySequence(QKeySequence::Undo);
	m_nativeShortcuts[QLatin1String("Redo")] = QKeySequence(QKeySequence::Redo);
	m_nativeShortcuts[QLatin1String("Redo")] = QKeySequence(QKeySequence::Redo);
	m_nativeShortcuts[QLatin1String("Cut")] = QKeySequence(QKeySequence::Cut);
	m_nativeShortcuts[QLatin1String("Copy")] = QKeySequence(QKeySequence::Copy);
	m_nativeShortcuts[QLatin1String("Paste")] = QKeySequence(QKeySequence::Paste);
	m_nativeShortcuts[QLatin1String("Delete")] = QKeySequence(QKeySequence::Delete);
	m_nativeShortcuts[QLatin1String("SelectAll")] = QKeySequence(QKeySequence::SelectAll);
	m_nativeShortcuts[QLatin1String("Find")] = QKeySequence(QKeySequence::Find);
	m_nativeShortcuts[QLatin1String("FindNext")] = QKeySequence(QKeySequence::FindNext);
	m_nativeShortcuts[QLatin1String("FindPrevious")] = QKeySequence(QKeySequence::FindPrevious);
	m_nativeShortcuts[QLatin1String("Reload")] = QKeySequence(QKeySequence::Refresh);
	m_nativeShortcuts[QLatin1String("ZoomIn")] = QKeySequence(QKeySequence::ZoomIn);
	m_nativeShortcuts[QLatin1String("ZoomOut")] = QKeySequence(QKeySequence::ZoomOut);
	m_nativeShortcuts[QLatin1String("Back")] = QKeySequence(QKeySequence::Back);
	m_nativeShortcuts[QLatin1String("Forward")] = QKeySequence(QKeySequence::Forward);
	m_nativeShortcuts[QLatin1String("Help")] = QKeySequence(QKeySequence::HelpContents);
	m_nativeShortcuts[QLatin1String("ApplicationConfiguration")] = QKeySequence(QKeySequence::Preferences);

	loadProfiles();

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), m_instance, SLOT(optionChanged(QString)));
}

void ShortcutsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_reloadTimer)
	{
		killTimer(m_reloadTimer);

		m_reloadTimer = 0;

		loadProfiles();
	}
}

void ShortcutsManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new ShortcutsManager(parent);
	}
}

void ShortcutsManager::optionChanged(const QString &option)
{
	if ((option == QLatin1String("Browser/ActionMacrosProfilesOrder") || option == QLatin1String("Browser/KeyboardShortcutsProfilesOrder")) && m_reloadTimer == 0)
	{
		m_reloadTimer = startTimer(50);
	}
}

void ShortcutsManager::loadProfiles()
{
	qDeleteAll(m_macros.keys());

	m_macros.clear();
	m_shortcuts.clear();

	QList<QKeySequence> shortcutsInUse;
	const QStringList shortcutProfiles = SettingsManager::getValue(QLatin1String("Browser/KeyboardShortcutsProfilesOrder")).toStringList();

	for (int i = 0; i < shortcutProfiles.count(); ++i)
	{
		const QString path = SessionsManager::getProfilePath() + QLatin1String("/keyboard/") + shortcutProfiles.at(i) + QLatin1String(".ini");
		const QSettings profile((QFile::exists(path) ? path : QLatin1String(":/keyboard/") + shortcutProfiles.at(i) + QLatin1String(".ini")), QSettings::IniFormat);
		const QStringList actions = profile.childGroups();

		for (int j = 0; j < actions.count(); ++j)
		{
			const QStringList rawShortcuts = profile.value(actions.at(j) + QLatin1String("/shortcuts"), QString()).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
			QList<QKeySequence> actionShortcuts;

			for (int k = 0; k < rawShortcuts.count(); ++k)
			{
				const QKeySequence shortcut = ((rawShortcuts.at(k) == QLatin1String("native")) ? m_nativeShortcuts.value(actions.at(j), QKeySequence()) : QKeySequence(rawShortcuts.at(k)));

				if (!shortcut.isEmpty() && !shortcutsInUse.contains(shortcut))
				{
					actionShortcuts.append(shortcut);
					shortcutsInUse.append(shortcut);
				}
			}

			if (!actionShortcuts.isEmpty())
			{
				m_shortcuts[actions.at(j)] = actionShortcuts;
			}
		}
	}

	const QStringList macroProfiles = SettingsManager::getValue(QLatin1String("Browser/ActionMacrosProfilesOrder")).toStringList();

	for (int i = 0; i < macroProfiles.count(); ++i)
	{
		const QString path = SessionsManager::getProfilePath() + QLatin1String("/macros/") + macroProfiles.at(i) + QLatin1String(".ini");
		const QSettings profile((QFile::exists(path) ? path : QLatin1String(":/macros/") + macroProfiles.at(i) + QLatin1String(".ini")), QSettings::IniFormat);
		const QStringList macros = profile.childGroups();

		for (int j = 0; j < macros.count(); ++j)
		{
			const QStringList actions = profile.value(macros.at(j) + QLatin1String("/actions"), QString()).toStringList();

///FIXME check if name is unique
			if (actions.isEmpty())
			{
				continue;
			}

			QAction *action = new QAction(QIcon(), profile.value(macros.at(j) + QLatin1String("/title"), QString()).toString(), SessionsManager::getActiveWindow());
			action->setObjectName(macros.at(j));

			connect(action, SIGNAL(triggered()), m_instance, SLOT(triggerMacro()));

			m_macros[action] = actions;

			const QStringList rawShortcuts = profile.value(macros.at(j) + QLatin1String("/shortcuts"), QString()).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
			QList<QKeySequence> actionShortcuts;

			for (int k = 0; k < rawShortcuts.count(); ++k)
			{
				const QKeySequence shortcut(rawShortcuts.at(k));

				if (!shortcut.isEmpty() && !shortcutsInUse.contains(shortcut))
				{
					actionShortcuts.append(shortcut);
					shortcutsInUse.append(shortcut);
				}
			}

			if (!actionShortcuts.isEmpty())
			{
				m_shortcuts[macros.at(j)] = actionShortcuts;

				action->setShortcuts(actionShortcuts);
				action->setShortcutContext(Qt::ApplicationShortcut);
			}
		}
	}

	emit m_instance->shortcutsChanged();
}

void ShortcutsManager::triggerMacro()
{
	QAction *action = qobject_cast<QAction*>(sender());
	MainWindow *window = qobject_cast<MainWindow*>(SessionsManager::getActiveWindow());

	if (action && window)
	{
		const QStringList actions = m_macros.value(action, QStringList());

		for (int i = 0; i < actions.count(); ++i)
		{
			window->getActionsManager()->triggerAction(actions.at(i));
		}
	}
}

ShortcutsManager* ShortcutsManager::getInstance()
{
	return m_instance;
}

QHash<QString, QList<QKeySequence> > ShortcutsManager::getShortcuts()
{
	return m_shortcuts;
}

QKeySequence ShortcutsManager::getNativeShortcut(const QString &action)
{
	return m_nativeShortcuts.value(action, QKeySequence());
}

}
