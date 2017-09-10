/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#include "ActionsManager.h"
#include "JsonSettings.h"
#include "SessionsManager.h"
#include "ThemesManager.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaEnum>
#include <QtCore/QRegularExpression>
#include <QtCore/QTextStream>
#include <QtGui/QKeySequence>

namespace Otter
{

bool KeyboardProfile::Action::operator ==(const KeyboardProfile::Action &other) const
{
	return (shortcuts == other.shortcuts && parameters == other.parameters && action == other.action);
}

KeyboardProfile::KeyboardProfile(const QString &identifier, bool onlyMetaData) : Addon(),
	m_identifier(identifier),
	m_isModified(false)
{
	if (identifier.isEmpty())
	{
		return;
	}

	const JsonSettings settings(SessionsManager::getReadableDataPath(QLatin1String("keyboard/") + identifier + QLatin1String(".json")));
	const QStringList comments(settings.getComment().split(QLatin1Char('\n')));

	for (int i = 0; i < comments.count(); ++i)
	{
		const QString key(comments.at(i).section(QLatin1Char(':'), 0, 0).trimmed());
		const QString value(comments.at(i).section(QLatin1Char(':'), 1).trimmed());

		if (key == QLatin1String("Title"))
		{
			m_title = value;
		}
		else if (key == QLatin1String("Description"))
		{
			m_description = value;
		}
		else if (key == QLatin1String("Author"))
		{
			m_author = value;
		}
		else if (key == QLatin1String("Version"))
		{
			m_version = value;
		}
	}

	if (onlyMetaData)
	{
		return;
	}

	const QJsonArray contextsArray(settings.array());
	const bool enableSingleKeyShortcuts(SettingsManager::getOption(SettingsManager::Browser_EnableSingleKeyShortcutsOption).toBool());
	QRegularExpression functionKeyExpression(QLatin1String("F\\d+"));
	functionKeyExpression.optimize();

	for (int i = 0; i < contextsArray.count(); ++i)
	{
		const QJsonArray actionsArray(contextsArray.at(i).toObject().value(QLatin1String("actions")).toArray());
		QVector<Action> definitions;
		definitions.reserve(actionsArray.count());

		for (int j = 0; j < actionsArray.count(); ++j)
		{
			const QJsonObject actionObject(actionsArray.at(j).toObject());
			const int action(ActionsManager::getActionIdentifier(actionObject.value(QLatin1String("action")).toString()));

			if (action < 0)
			{
				continue;
			}

			const QJsonArray shortcutsArray(actionObject.value(QLatin1String("shortcuts")).toArray());
			QVector<QKeySequence> shortcuts;
			shortcuts.reserve(shortcutsArray.count());

			for (int k = 0; k < shortcutsArray.count(); ++k)
			{
				const QString rawShortcut(shortcutsArray.at(k).toString());

				if (!enableSingleKeyShortcuts && !functionKeyExpression.match(rawShortcut).hasMatch() && (!rawShortcut.contains(QLatin1Char('+')) || rawShortcut == QLatin1String("+")))
				{
					continue;
				}

				shortcuts.append(QKeySequence(rawShortcut));
			}

			if (shortcuts.isEmpty())
			{
				continue;
			}

			KeyboardProfile::Action definition;
			definition.shortcuts = shortcuts;
			definition.parameters = actionObject.value(QLatin1String("parameters")).toVariant().toMap();
			definition.action = action;

			definitions.append(definition);
		}

		m_definitions[ActionsManager::GenericContext] = definitions;
	}
}

void KeyboardProfile::setTitle(const QString &title)
{
	if (title != m_title)
	{
		m_title = title;
		m_isModified = true;
	}
}

void KeyboardProfile::setDescription(const QString &description)
{
	if (description != m_description)
	{
		m_description = description;
		m_isModified = true;
	}
}

void KeyboardProfile::setAuthor(const QString &author)
{
	if (author != m_author)
	{
		m_author = author;
		m_isModified = true;
	}
}

void KeyboardProfile::setVersion(const QString &version)
{
	if (version != m_version)
	{
		m_version = version;
		m_isModified = true;
	}
}

void KeyboardProfile::setDefinitions(const QHash<int, QVector<KeyboardProfile::Action> > &definitions)
{
	if (definitions != m_definitions)
	{
		m_definitions = definitions;
		m_isModified = true;
	}
}

void KeyboardProfile::setModified(bool isModified)
{
	m_isModified = isModified;
}

QString KeyboardProfile::getName() const
{
	return m_identifier;
}

QString KeyboardProfile::getTitle() const
{
	return (m_title.isEmpty() ? QCoreApplication::translate("Otter::KeyboardProfile", "(Untitled)") : m_title);
}

QString KeyboardProfile::getDescription() const
{
	return m_description;
}

QString KeyboardProfile::getAuthor() const
{
	return m_author;
}

QString KeyboardProfile::getVersion() const
{
	return m_version;
}

QHash<int, QVector<KeyboardProfile::Action> > KeyboardProfile::getDefinitions() const
{
	return m_definitions;
}

bool KeyboardProfile::isModified() const
{
	return m_isModified;
}

bool KeyboardProfile::save()
{
	JsonSettings settings(SessionsManager::getWritableDataPath(QLatin1String("keyboard/") + m_identifier + QLatin1String(".json")));
	QString comment;
	QTextStream stream(&comment);
	stream.setCodec("UTF-8");
	stream << QLatin1String("Title: ") << (m_title.isEmpty() ? QT_TR_NOOP("(Untitled)") : m_title) << QLatin1Char('\n');
	stream << QLatin1String("Description: ") << m_description << QLatin1Char('\n');
	stream << QLatin1String("Type: keyboard-profile\n");
	stream << QLatin1String("Author: ") << m_author << QLatin1Char('\n');
	stream << QLatin1String("Version: ") << m_version;

	settings.setComment(comment);

	QJsonArray contextsArray;
	QHash<int, QVector<KeyboardProfile::Action> >::const_iterator contextsIterator;

	for (contextsIterator = m_definitions.begin(); contextsIterator != m_definitions.end(); ++contextsIterator)
	{
		QJsonArray actionsArray;

		for (int i = 0; i < contextsIterator.value().count(); ++i)
		{
			const KeyboardProfile::Action &action(contextsIterator.value().at(i));
			QJsonArray shortcutsArray;

			for (int j = 0; j < action.shortcuts.count(); ++j)
			{
				shortcutsArray.append(action.shortcuts.at(j).toString());
			}

			QJsonObject actionObject{{QLatin1String("action"), ActionsManager::getActionName(action.action)}, {QLatin1String("shortcuts"), shortcutsArray}};

			if (!action.parameters.isEmpty())
			{
				actionObject.insert(QLatin1String("parameters"), QJsonObject::fromVariantMap(action.parameters));
			}

			actionsArray.append(actionObject);
		}

		contextsArray.append(QJsonObject({{QLatin1String("context"), QLatin1String("Generic")}, {QLatin1String("actions"), actionsArray}}));
	}

	settings.setArray(contextsArray);

	const bool result(settings.save());

	if (result)
	{
		m_isModified = false;
	}

	return result;
}

ActionsManager* ActionsManager::m_instance(nullptr);
QMap<int, QVector<QKeySequence> > ActionsManager::m_shortcuts;
QMultiMap<int, QPair<QVariantMap, QVector<QKeySequence> > > ActionsManager::m_extraShortcuts;
QVector<ActionsManager::ActionDefinition> ActionsManager::m_definitions;
int ActionsManager::m_actionIdentifierEnumerator(0);

ActionsManager::ActionsManager(QObject *parent) : QObject(parent),
	m_reloadTimer(0)
{
	m_definitions.reserve(ActionsManager::OtherAction);

	registerAction(RunMacroAction, QT_TRANSLATE_NOOP("actions", "Run Macro"), QT_TRANSLATE_NOOP("actions", "Run Arbitrary List of Actions"), QIcon(), ActionDefinition::ApplicationScope);
	registerAction(NewTabAction, QT_TRANSLATE_NOOP("actions", "New Tab"), QString(), ThemesManager::createIcon(QLatin1String("tab-new")), ActionDefinition::MainWindowScope);
	registerAction(NewTabPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Tab"), QString(), ThemesManager::createIcon(QLatin1String("tab-new-private")), ActionDefinition::MainWindowScope);
	registerAction(NewWindowAction, QT_TRANSLATE_NOOP("actions", "New Window"), QString(), ThemesManager::createIcon(QLatin1String("window-new")), ActionDefinition::ApplicationScope);
	registerAction(NewWindowPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Window"), QString(), ThemesManager::createIcon(QLatin1String("window-new-private")), ActionDefinition::ApplicationScope);
	registerAction(OpenAction, QT_TRANSLATE_NOOP("actions", "Open…"), QString(), ThemesManager::createIcon(QLatin1String("document-open")), ActionDefinition::MainWindowScope);
	registerAction(SaveAction, QT_TRANSLATE_NOOP("actions", "Save…"), QString(), ThemesManager::createIcon(QLatin1String("document-save")), ActionDefinition::WindowScope);
	registerAction(CloneTabAction, QT_TRANSLATE_NOOP("actions", "Clone Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(PinTabAction, QT_TRANSLATE_NOOP("actions", "Pin Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(DetachTabAction, QT_TRANSLATE_NOOP("actions", "Detach Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(MaximizeTabAction, QT_TRANSLATE_NOOP("actions", "Maximize"), QT_TRANSLATE_NOOP("actions", "Maximize Tab"), QIcon(), ActionDefinition::WindowScope);
	registerAction(MinimizeTabAction, QT_TRANSLATE_NOOP("actions", "Minimize"), QT_TRANSLATE_NOOP("actions", "Minimize Tab"), QIcon(), ActionDefinition::WindowScope);
	registerAction(RestoreTabAction, QT_TRANSLATE_NOOP("actions", "Restore"), QT_TRANSLATE_NOOP("actions", "Restore Tab"), QIcon(), ActionDefinition::WindowScope);
	registerAction(AlwaysOnTopTabAction, QT_TRANSLATE_NOOP("actions", "Stay on Top"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ClearTabHistoryAction, QT_TRANSLATE_NOOP("actions", "Clear Tab History"), QT_TRANSLATE_NOOP("actions", "Remove Local Tab History"), QIcon(), ActionDefinition::WindowScope, ActionDefinition::NoFlags, ActionsManager::ActionDefinition::NavigationCategory);
	registerAction(PurgeTabHistoryAction, QT_TRANSLATE_NOOP("actions", "Purge Tab History"), QT_TRANSLATE_NOOP("actions", "Remove Local and Global Tab History"), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsDeprecatedFlag, ActionsManager::ActionDefinition::NavigationCategory);
	registerAction(MuteTabMediaAction, QT_TRANSLATE_NOOP("actions", "Mute Tab Media"), QString(), ThemesManager::createIcon(QLatin1String("audio-volume-muted")), ActionDefinition::WindowScope);
	registerAction(SuspendTabAction, QT_TRANSLATE_NOOP("actions", "Suspend Tab"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(CloseTabAction, QT_TRANSLATE_NOOP("actions", "Close Tab"), QString(), ThemesManager::createIcon(QLatin1String("tab-close")), ActionDefinition::WindowScope);
	registerAction(CloseOtherTabsAction, QT_TRANSLATE_NOOP("actions", "Close Other Tabs"), QString(), ThemesManager::createIcon(QLatin1String("tab-close-other")), ActionDefinition::MainWindowScope);
	registerAction(ClosePrivateTabsAction, QT_TRANSLATE_NOOP("actions", "Close All Private Tabs"), QT_TRANSLATE_NOOP("actions", "Close All Private Tabs in Current Window"), QIcon(), ActionDefinition::MainWindowScope, ActionDefinition::NoFlags);
	registerAction(ClosePrivateTabsPanicAction, QT_TRANSLATE_NOOP("actions", "Close Private Tabs and Windows"), QString(), QIcon(), ActionDefinition::ApplicationScope);
	registerAction(ReopenTabAction, QT_TRANSLATE_NOOP("actions", "Reopen Previously Closed Tab"), QString(), QIcon(), ActionDefinition::MainWindowScope, ActionDefinition::NoFlags);
	registerAction(MaximizeAllAction, QT_TRANSLATE_NOOP("actions", "Maximize All"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(MinimizeAllAction, QT_TRANSLATE_NOOP("actions", "Minimize All"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(RestoreAllAction, QT_TRANSLATE_NOOP("actions", "Restore All"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(CascadeAllAction, QT_TRANSLATE_NOOP("actions", "Cascade"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(TileAllAction, QT_TRANSLATE_NOOP("actions", "Tile"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(CloseWindowAction, QT_TRANSLATE_NOOP("actions", "Close Window"), QString(), QIcon(), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(ReopenWindowAction, QT_TRANSLATE_NOOP("actions", "Reopen Previously Closed Window"), QString(), QIcon(), ActionDefinition::ApplicationScope, ActionDefinition::NoFlags);
	registerAction(SessionsAction, QT_TRANSLATE_NOOP("actions", "Manage Sessions…"), QString(), QIcon(), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(SaveSessionAction, QT_TRANSLATE_NOOP("actions", "Save Current Session…"), QString(), QIcon(), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(OpenUrlAction, QT_TRANSLATE_NOOP("actions", "Open URL"), QString(), QIcon(), ActionDefinition::MainWindowScope, ActionDefinition::RequiresParameters);
	registerAction(OpenLinkAction, QT_TRANSLATE_NOOP("actions", "Open"), QString(), ThemesManager::createIcon(QLatin1String("document-open")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(OpenLinkInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open in This Tab"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewWindowAction, QT_TRANSLATE_NOOP("actions", "Open in New Window"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewWindowBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Window"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewPrivateTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Tab"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewPrivateTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Background Tab"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewPrivateWindowAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Window"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewPrivateWindowBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Background Window"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(CopyLinkToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Link to Clipboard"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(BookmarkLinkAction, QT_TRANSLATE_NOOP("actions", "Bookmark Link…"), QString(), ThemesManager::createIcon(QLatin1String("bookmark-new")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(SaveLinkToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Link Target As…"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(SaveLinkToDownloadsAction, QT_TRANSLATE_NOOP("actions", "Save to Downloads"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(OpenSelectionAsLinkAction, QT_TRANSLATE_NOOP("actions", "Go to This Address"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenFrameInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open"), QT_TRANSLATE_NOOP("actions", "Open Frame in This Tab"), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::FrameCategory);
	registerAction(OpenFrameInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Tab"), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::FrameCategory);
	registerAction(OpenFrameInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Background Tab"), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::FrameCategory);
	registerAction(CopyFrameLinkToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Frame Link to Clipboard"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::FrameCategory);
	registerAction(ReloadFrameAction, QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload Frame"), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::FrameCategory);
	registerAction(ViewFrameSourceAction, QT_TRANSLATE_NOOP("actions", "View Frame Source"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::FrameCategory);
	registerAction(OpenImageInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open Image In New Tab"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(OpenImageInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open Image in New Background Tab"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::ImageCategory);
	registerAction(SaveImageToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Image…"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(CopyImageToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Image to Clipboard"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(CopyImageUrlToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Image Link to Clipboard"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(ReloadImageAction, QT_TRANSLATE_NOOP("actions", "Reload Image"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(ImagePropertiesAction, QT_TRANSLATE_NOOP("actions", "Image Properties…"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(SaveMediaToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Media…"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::MediaCategory);
	registerAction(CopyMediaUrlToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Media Link to Clipboard"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::MediaCategory);
	registerAction(MediaControlsAction, QT_TRANSLATE_NOOP("actions", "Show Controls"), QT_TRANSLATE_NOOP("actions", "Show Media Controls"), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag), ActionDefinition::MediaCategory);
	registerAction(MediaLoopAction, QT_TRANSLATE_NOOP("actions", "Looping"), QT_TRANSLATE_NOOP("actions", "Playback Looping"), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag), ActionDefinition::MediaCategory);
	registerAction(MediaPlayPauseAction, QT_TRANSLATE_NOOP("actions", "Play"), QT_TRANSLATE_NOOP("actions", "Play Media"), ThemesManager::createIcon(QLatin1String("media-playback-start")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::MediaCategory);
	registerAction(MediaMuteAction, QT_TRANSLATE_NOOP("actions", "Mute"), QT_TRANSLATE_NOOP("actions", "Mute Media"), ThemesManager::createIcon(QLatin1String("audio-volume-muted")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::MediaCategory);
	registerAction(MediaPlaybackRateAction, QT_TRANSLATE_NOOP("actions", "Playback Rate"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsCheckableFlag, ActionDefinition::MediaCategory);
	registerAction(FillPasswordAction, QT_TRANSLATE_NOOP("actions", "Log In"), QString(), ThemesManager::createIcon(QLatin1String("fill-password")), ActionDefinition::WindowScope, ActionDefinition::NoFlags, ActionDefinition::PageCategory);
	registerAction(GoAction, QT_TRANSLATE_NOOP("actions", "Go"), QT_TRANSLATE_NOOP("actions", "Go to URL"), ThemesManager::createIcon(QLatin1String("go-jump-locationbar")), ActionDefinition::MainWindowScope);
	registerAction(GoBackAction, QT_TRANSLATE_NOOP("actions", "Back"), QT_TRANSLATE_NOOP("actions", "Go Back"), ThemesManager::createIcon(QLatin1String("go-previous")), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag), ActionDefinition::NavigationCategory);
	registerAction(GoForwardAction, QT_TRANSLATE_NOOP("actions", "Forward"), QT_TRANSLATE_NOOP("actions", "Go Forward"), ThemesManager::createIcon(QLatin1String("go-next")), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag), ActionDefinition::NavigationCategory);
	registerAction(GoToPageAction, QT_TRANSLATE_NOOP("actions", "Go to Page or Search"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(GoToHomePageAction, QT_TRANSLATE_NOOP("actions", "Go to Home Page"), QString(), ThemesManager::createIcon(QLatin1String("go-home")), ActionDefinition::MainWindowScope);
	registerAction(GoToParentDirectoryAction, QT_TRANSLATE_NOOP("actions", "Go to Parent Directory"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(RewindAction, QT_TRANSLATE_NOOP("actions", "Rewind"), QT_TRANSLATE_NOOP("actions", "Rewind History"), ThemesManager::createIcon(QLatin1String("go-first")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(FastForwardAction, QT_TRANSLATE_NOOP("actions", "Fast Forward"), QString(), ThemesManager::createIcon(QLatin1String("go-last")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(StopAction, QT_TRANSLATE_NOOP("actions", "Stop"), QString(), ThemesManager::createIcon(QLatin1String("process-stop")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(StopScheduledReloadAction, QT_TRANSLATE_NOOP("actions", "Stop Scheduled Page Reload"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(StopAllAction, QT_TRANSLATE_NOOP("actions", "Stop All Pages"), QString(), ThemesManager::createIcon(QLatin1String("process-stop")), ActionDefinition::MainWindowScope);
	registerAction(ReloadAction, QT_TRANSLATE_NOOP("actions", "Reload"), QString(), ThemesManager::createIcon(QLatin1String("view-refresh")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(ReloadOrStopAction, QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload or Stop"), ThemesManager::createIcon(QLatin1String("view-refresh")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(ReloadAndBypassCacheAction, QT_TRANSLATE_NOOP("actions", "Reload and Bypass Cache"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ReloadAllAction, QT_TRANSLATE_NOOP("actions", "Reload All Tabs"), QString(), ThemesManager::createIcon(QLatin1String("view-refresh")), ActionDefinition::MainWindowScope);
	registerAction(ScheduleReloadAction, QT_TRANSLATE_NOOP("actions", "Schedule Page Reload"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ContextMenuAction, QT_TRANSLATE_NOOP("actions", "Show Context Menu"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(UndoAction, QT_TRANSLATE_NOOP("actions", "Undo"), QString(), ThemesManager::createIcon(QLatin1String("edit-undo")), ActionDefinition::WindowScope);
	registerAction(RedoAction, QT_TRANSLATE_NOOP("actions", "Redo"), QString(), ThemesManager::createIcon(QLatin1String("edit-redo")), ActionDefinition::WindowScope);
	registerAction(CutAction, QT_TRANSLATE_NOOP("actions", "Cut"), QString(), ThemesManager::createIcon(QLatin1String("edit-cut")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(CopyAction, QT_TRANSLATE_NOOP("actions", "Copy"), QString(), ThemesManager::createIcon(QLatin1String("edit-copy")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(CopyPlainTextAction, QT_TRANSLATE_NOOP("actions", "Copy as Plain Text"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::EditingCategory);
	registerAction(CopyAddressAction, QT_TRANSLATE_NOOP("actions", "Copy Address"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(CopyToNoteAction, QT_TRANSLATE_NOOP("actions", "Copy to Note"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(PasteAction, QT_TRANSLATE_NOOP("actions", "Paste"), QString(), ThemesManager::createIcon(QLatin1String("edit-paste")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(PasteAndGoAction, QT_TRANSLATE_NOOP("actions", "Paste and Go"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(DeleteAction, QT_TRANSLATE_NOOP("actions", "Delete"), QString(), ThemesManager::createIcon(QLatin1String("edit-delete")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(SelectAllAction, QT_TRANSLATE_NOOP("actions", "Select All"), QString(), ThemesManager::createIcon(QLatin1String("edit-select-all")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(UnselectAction, QT_TRANSLATE_NOOP("actions", "Deselect"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(ClearAllAction, QT_TRANSLATE_NOOP("actions", "Clear All"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(CheckSpellingAction, QT_TRANSLATE_NOOP("actions", "Check Spelling"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag), ActionDefinition::EditingCategory);
	registerAction(FindAction, QT_TRANSLATE_NOOP("actions", "Find…"), QString(), ThemesManager::createIcon(QLatin1String("edit-find")), ActionDefinition::WindowScope);
	registerAction(FindNextAction, QT_TRANSLATE_NOOP("actions", "Find Next"), QString(), ThemesManager::createIcon(QLatin1String("go-down")), ActionDefinition::WindowScope);
	registerAction(FindPreviousAction, QT_TRANSLATE_NOOP("actions", "Find Previous"), QString(), ThemesManager::createIcon(QLatin1String("go-up")), ActionDefinition::WindowScope);
	registerAction(QuickFindAction, QT_TRANSLATE_NOOP("actions", "Quick Find"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(SearchAction, QT_TRANSLATE_NOOP("actions", "Search"), QString(), ThemesManager::createIcon(QLatin1String("edit-find")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(CreateSearchAction, QT_TRANSLATE_NOOP("actions", "Create Search…"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(ZoomInAction, QT_TRANSLATE_NOOP("actions", "Zoom In"), QString(), ThemesManager::createIcon(QLatin1String("zoom-in")), ActionDefinition::WindowScope);
	registerAction(ZoomOutAction, QT_TRANSLATE_NOOP("actions", "Zoom Out"), QString(), ThemesManager::createIcon(QLatin1String("zoom-out")), ActionDefinition::WindowScope);
	registerAction(ZoomOriginalAction, QT_TRANSLATE_NOOP("actions", "Zoom Original"), QString(), ThemesManager::createIcon(QLatin1String("zoom-original")), ActionDefinition::WindowScope);
	registerAction(ScrollToStartAction, QT_TRANSLATE_NOOP("actions", "Go to Start of the Page"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ScrollToEndAction, QT_TRANSLATE_NOOP("actions", "Go to the End of the Page"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ScrollPageUpAction, QT_TRANSLATE_NOOP("actions", "Page Up"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ScrollPageDownAction, QT_TRANSLATE_NOOP("actions", "Page Down"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ScrollPageLeftAction, QT_TRANSLATE_NOOP("actions", "Page Left"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ScrollPageRightAction, QT_TRANSLATE_NOOP("actions", "Page Right"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(StartDragScrollAction, QT_TRANSLATE_NOOP("actions", "Enter Drag Scroll Mode"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(StartMoveScrollAction, QT_TRANSLATE_NOOP("actions", "Enter Move Scroll Mode"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(EndScrollAction, QT_TRANSLATE_NOOP("actions", "Exit Scroll Mode"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(PrintAction, QT_TRANSLATE_NOOP("actions", "Print…"), QString(), ThemesManager::createIcon(QLatin1String("document-print")), ActionDefinition::WindowScope);
	registerAction(PrintPreviewAction, QT_TRANSLATE_NOOP("actions", "Print Preview"), QString(), ThemesManager::createIcon(QLatin1String("document-print-preview")), ActionDefinition::WindowScope);
	registerAction(ActivateAddressFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Address Field"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateSearchFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Search Field"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateContentAction, QT_TRANSLATE_NOOP("actions", "Activate Content"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ActivatePreviouslyUsedTabAction, QT_TRANSLATE_NOOP("actions", "Go to Previously Used Tab"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateLeastRecentlyUsedTabAction, QT_TRANSLATE_NOOP("actions", "Go to Least Recently Used Tab"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateTabAction, QT_TRANSLATE_NOOP("actions", "Activate Tab"), QString(), QIcon(), ActionDefinition::MainWindowScope, ActionDefinition::RequiresParameters);
	registerAction(ActivateTabOnLeftAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Left"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateTabOnRightAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Right"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateWindowAction, QT_TRANSLATE_NOOP("actions", "Activate Window"), QString(), QIcon(), ActionDefinition::ApplicationScope, ActionDefinition::RequiresParameters);
	registerAction(BookmarksAction, QT_TRANSLATE_NOOP("actions", "Manage Bookmarks"), QString(), ThemesManager::createIcon(QLatin1String("bookmarks-organize")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(BookmarkPageAction, QT_TRANSLATE_NOOP("actions", "Bookmark Page…"), QString(), ThemesManager::createIcon(QLatin1String("bookmark-new")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::PageCategory);
	registerAction(BookmarkAllOpenPagesAction, QT_TRANSLATE_NOOP("actions", "Bookmark All Open Pages"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(OpenBookmarkAction, QT_TRANSLATE_NOOP("actions", "Open Bookmark"), QString(), QIcon(), ActionDefinition::MainWindowScope, ActionDefinition::RequiresParameters);
	registerAction(QuickBookmarkAccessAction, QT_TRANSLATE_NOOP("actions", "Quick Bookmark Access"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(CookiesAction, QT_TRANSLATE_NOOP("actions", "Cookies"), QString(), ThemesManager::createIcon(QLatin1String("cookies")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(LoadPluginsAction, QT_TRANSLATE_NOOP("actions", "Load All Plugins on the Page"), QString(), ThemesManager::createIcon(QLatin1String("preferences-plugin")), ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(EnableJavaScriptAction, QT_TRANSLATE_NOOP("actions", "Enable JavaScript"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag), ActionDefinition::PageCategory);
	registerAction(EnableReferrerAction, QT_TRANSLATE_NOOP("actions", "Enable Referrer"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag), ActionDefinition::PageCategory);
	registerAction(ViewSourceAction, QT_TRANSLATE_NOOP("actions", "View Source"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::NoFlags, ActionDefinition::NavigationCategory);
	registerAction(InspectPageAction, QT_TRANSLATE_NOOP("actions", "Inspect Page"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsCheckableFlag);
	registerAction(InspectElementAction, QT_TRANSLATE_NOOP("actions", "Inspect Element…"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(WorkOfflineAction, QT_TRANSLATE_NOOP("actions", "Work Offline"), QString(), QIcon(), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(FullScreenAction, QT_TRANSLATE_NOOP("actions", "Full Screen"), QString(), ThemesManager::createIcon(QLatin1String("view-fullscreen")), ActionDefinition::MainWindowScope);
	registerAction(ShowTabSwitcherAction, QT_TRANSLATE_NOOP("actions", "Show Tab Switcher"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ShowToolBarAction, QT_TRANSLATE_NOOP("actions", "Show Toolbar"), QString(), QIcon(), ActionDefinition::MainWindowScope, (ActionDefinition::IsCheckableFlag | ActionDefinition::RequiresParameters));
	registerAction(ShowMenuBarAction, QT_TRANSLATE_NOOP("actions", "Show Menubar"), QString(), QIcon(), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ShowTabBarAction, QT_TRANSLATE_NOOP("actions", "Show Tabbar"), QString(), QIcon(), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ShowSidebarAction, QT_TRANSLATE_NOOP("actions", "Show Sidebar"), QString(), ThemesManager::createIcon(QLatin1String("sidebar-show")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ShowErrorConsoleAction, QT_TRANSLATE_NOOP("actions", "Show Error Console"), QString(), QIcon(), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(LockToolBarsAction, QT_TRANSLATE_NOOP("actions", "Lock Toolbars"), QString(), QIcon(), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ResetToolBarsAction, QT_TRANSLATE_NOOP("actions", "Reset to Defaults…"), QT_TRANSLATE_NOOP("actions", "Reset Toolbars to Defaults…"), QIcon(), ActionDefinition::ApplicationScope);
	registerAction(OpenPanelAction, QT_TRANSLATE_NOOP("actions", "Open Panel as Tab"), QString(), ThemesManager::createIcon(QLatin1String("arrow-right")), ActionDefinition::MainWindowScope);
	registerAction(ClosePanelAction, QT_TRANSLATE_NOOP("actions", "Close Panel"), QString(), ThemesManager::createIcon(QLatin1String("window-close")), ActionDefinition::MainWindowScope);
	registerAction(ContentBlockingAction, QT_TRANSLATE_NOOP("actions", "Content Blocking…"), QString(), ThemesManager::createIcon(QLatin1String("content-blocking")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(HistoryAction, QT_TRANSLATE_NOOP("actions", "View History"), QString(), ThemesManager::createIcon(QLatin1String("view-history")), ActionDefinition::MainWindowScope);
	registerAction(ClearHistoryAction, QT_TRANSLATE_NOOP("actions", "Clear History…"), QString(), ThemesManager::createIcon(QLatin1String("edit-clear-history")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(AddonsAction, QT_TRANSLATE_NOOP("actions", "Addons"), QString(), ThemesManager::createIcon(QLatin1String("preferences-plugin")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(NotesAction, QT_TRANSLATE_NOOP("actions", "Notes"), QString(), ThemesManager::createIcon(QLatin1String("notes")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(PasswordsAction, QT_TRANSLATE_NOOP("actions", "Passwords"), QString(), ThemesManager::createIcon(QLatin1String("dialog-password")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(TransfersAction, QT_TRANSLATE_NOOP("actions", "Transfers"), QString(), ThemesManager::createIcon(QLatin1String("transfers")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(PreferencesAction, QT_TRANSLATE_NOOP("actions", "Preferences…"), QString(), QIcon(), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(WebsitePreferencesAction, QT_TRANSLATE_NOOP("actions", "Website Preferences…"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::PageCategory);
	registerAction(QuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Quick Preferences"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ResetQuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Reset Options"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(WebsiteInformationAction, QT_TRANSLATE_NOOP("actions", "Website Information…"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(WebsiteCertificateInformationAction, QT_TRANSLATE_NOOP("actions", "Website Certificate Information…"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(SwitchApplicationLanguageAction, QT_TRANSLATE_NOOP("actions", "Switch Application Language…"), QString(), ThemesManager::createIcon(QLatin1String("preferences-desktop-locale")), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(CheckForUpdatesAction, QT_TRANSLATE_NOOP("actions", "Check for Updates…"), QString(), QIcon(), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(DiagnosticReportAction, QT_TRANSLATE_NOOP("actions", "Diagnostic Report…"), QString(), QIcon(), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(AboutApplicationAction, QT_TRANSLATE_NOOP("actions", "About Otter…"), QString(), ThemesManager::createIcon(QLatin1String("otter-browser"), false), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(AboutQtAction, QT_TRANSLATE_NOOP("actions", "About Qt…"), QString(), ThemesManager::createIcon(QLatin1String("qt")), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(ExitAction, QT_TRANSLATE_NOOP("actions", "Exit"), QString(), ThemesManager::createIcon(QLatin1String("application-exit")), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));

	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int)));
}

void ActionsManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new ActionsManager(QCoreApplication::instance());
		m_actionIdentifierEnumerator = ActionsManager::staticMetaObject.indexOfEnumerator(QLatin1String("ActionIdentifier").data());

		loadProfiles();
	}
}

void ActionsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_reloadTimer)
	{
		killTimer(m_reloadTimer);

		m_reloadTimer = 0;

		loadProfiles();
	}
}

void ActionsManager::loadProfiles()
{
	m_shortcuts.clear();
	m_extraShortcuts.clear();

	QVector<QKeySequence> allShortcuts;
	const QStringList profiles(SettingsManager::getOption(SettingsManager::Browser_KeyboardShortcutsProfilesOrderOption).toStringList());

	for (int i = 0; i < profiles.count(); ++i)
	{
		const QHash<int, QVector<KeyboardProfile::Action> > definitions(KeyboardProfile(profiles.at(i)).getDefinitions());
		QHash<int, QVector<KeyboardProfile::Action> >::const_iterator iterator;

		for (iterator = definitions.constBegin(); iterator != definitions.constEnd(); ++iterator)
		{
			for (int j = 0; j < iterator.value().count(); ++j)
			{
				const KeyboardProfile::Action definition(iterator.value().at(j));
				QVector<QKeySequence> shortcuts;

				if (m_shortcuts.contains(definition.action))
				{
					shortcuts = m_shortcuts[definition.action];
				}

				for (int k = 0; k < definition.shortcuts.count(); ++k)
				{
					const QKeySequence shortcut(definition.shortcuts.at(k));

					if (!allShortcuts.contains(shortcut))
					{
						shortcuts.append(shortcut);
						allShortcuts.append(shortcut);
					}
				}

				if (!shortcuts.isEmpty())
				{
					if (definition.parameters.isEmpty())
					{
						m_shortcuts[definition.action] = shortcuts;
					}
					else
					{
						m_extraShortcuts.insert(definition.action, {definition.parameters, shortcuts});
					}
				}
			}
		}
	}

	emit m_instance->shortcutsChanged();
}

void ActionsManager::registerAction(int identifier, const QString &text, const QString &description, const QIcon &icon, ActionDefinition::ActionScope scope, ActionDefinition::ActionFlags flags, ActionDefinition::ActionCategory category)
{
	ActionsManager::ActionDefinition action;
	action.description = description;
	action.defaultState.text = text;
	action.defaultState.icon = icon;
	action.defaultState.isEnabled = flags.testFlag(ActionDefinition::IsEnabledFlag);
	action.identifier = identifier;
	action.flags = flags;
	action.category = category;
	action.scope = scope;

	m_definitions.append(action);
}

void ActionsManager::handleOptionChanged(int identifier)
{
	switch (identifier)
	{
		case SettingsManager::Browser_EnableSingleKeyShortcutsOption:
		case SettingsManager::Browser_KeyboardShortcutsProfilesOrderOption:
			if (m_reloadTimer == 0)
			{
				m_reloadTimer = startTimer(250);
			}

			break;
		default:
			break;
	}
}

ActionsManager* ActionsManager::getInstance()
{
	return m_instance;
}

QString ActionsManager::createReport()
{
	QString report;
	QTextStream stream(&report);
	stream.setFieldAlignment(QTextStream::AlignLeft);
	stream << QLatin1String("Keyboard Shortcuts:\n");

	for (int i = 0; i < m_definitions.count(); ++i)
	{
		if (m_shortcuts.contains(i))
		{
			const QVector<QKeySequence> shortcuts(m_shortcuts[i]);

			stream << QLatin1Char('\t');
			stream.setFieldWidth(30);
			stream << getActionName(i);
			stream.setFieldWidth(20);

			for (int j = 0; j < shortcuts.count(); ++j)
			{
				stream << shortcuts.at(j).toString(QKeySequence::PortableText);
			}

			stream.setFieldWidth(0);
			stream << QLatin1Char('\n');
		}

		if (m_extraShortcuts.contains(i))
		{
			const QList<QPair<QVariantMap, QVector<QKeySequence> > > definitions(m_extraShortcuts.values(i));

			if (!m_shortcuts.contains(i))
			{
				stream << QLatin1Char('\t');
				stream.setFieldWidth(30);
				stream << getActionName(i);
				stream.setFieldWidth(0);
				stream << QLatin1Char('\n');
			}

			for (int j = 0; j < definitions.count(); ++j)
			{
				const QVector<QKeySequence> shortcuts(definitions[j].second);

				stream << QLatin1Char('\t');
				stream.setFieldWidth(30);
				stream << QLatin1Char(' ') + QJsonDocument(QJsonObject::fromVariantMap(definitions[j].first)).toJson(QJsonDocument::Compact);
				stream.setFieldWidth(20);

				for (int j = 0; j < shortcuts.count(); ++j)
				{
					stream << shortcuts.at(j).toString(QKeySequence::PortableText);
				}

				stream.setFieldWidth(0);
				stream << QLatin1Char('\n');
			}
		}
	}

	stream << QLatin1Char('\n');

	return report;
}

QString ActionsManager::getActionName(int identifier)
{
	QString name(ActionsManager::staticMetaObject.enumerator(m_actionIdentifierEnumerator).valueToKey(identifier));

	if (!name.isEmpty())
	{
		name.chop(6);

		return name;
	}

	return QString();
}

QKeySequence ActionsManager::getActionShortcut(int identifier, const QVariantMap &parameters)
{
	if (parameters.isEmpty() && m_shortcuts.contains(identifier))
	{
		return m_shortcuts[identifier].first();
	}

	if (!parameters.isEmpty() && m_extraShortcuts.contains(identifier))
	{
		const QList<QPair<QVariantMap, QVector<QKeySequence> > > definitions(m_extraShortcuts.values(identifier));

		for (int i = 0; i < definitions.count(); ++i)
		{
			if (definitions.at(i).first == parameters)
			{
				return definitions.at(i).second.first();
			}
		}
	}

	return QKeySequence();
}

QVector<ActionsManager::ActionDefinition> ActionsManager::getActionDefinitions()
{
	return m_definitions;
}

QVector<KeyboardProfile::Action> ActionsManager::getShortcutDefinitions()
{
	QVector<KeyboardProfile::Action> definitions;
	definitions.reserve(m_shortcuts.count() + m_extraShortcuts.count());

	QMap<int, QVector<QKeySequence> >::iterator shortcutsIterator;

	for (shortcutsIterator = m_shortcuts.begin(); shortcutsIterator != m_shortcuts.end(); ++shortcutsIterator)
	{
		KeyboardProfile::Action definition;
		definition.shortcuts = shortcutsIterator.value();
		definition.action = shortcutsIterator.key();

		definitions.append(definition);
	}

	QMultiMap<int, QPair<QVariantMap, QVector<QKeySequence> > >::iterator extraShortcutsIterator;

	for (extraShortcutsIterator = m_extraShortcuts.begin(); extraShortcutsIterator != m_extraShortcuts.end(); ++extraShortcutsIterator)
	{
		KeyboardProfile::Action definition;
		definition.parameters = extraShortcutsIterator.value().first;
		definition.shortcuts = extraShortcutsIterator.value().second;
		definition.action = extraShortcutsIterator.key();

		definitions.append(definition);
	}

	return definitions;
}

ActionsManager::ActionDefinition ActionsManager::getActionDefinition(int identifier)
{
	if (identifier < 0 || identifier >= m_definitions.count())
	{
		return ActionsManager::ActionDefinition();
	}

	return m_definitions[identifier];
}

int ActionsManager::getActionIdentifier(const QString &name)
{
	if (!name.endsWith(QLatin1String("Action")))
	{
		return ActionsManager::staticMetaObject.enumerator(m_actionIdentifierEnumerator).keyToValue(QString(name + QLatin1String("Action")).toLatin1());
	}

	return ActionsManager::staticMetaObject.enumerator(m_actionIdentifierEnumerator).keyToValue(name.toLatin1());
}

ActionExecutor::Object::Object() : m_executor(nullptr)
{
}

ActionExecutor::Object::Object(QObject *object, ActionExecutor *executor) : m_object(object), m_executor(executor)
{
}

ActionExecutor::Object::Object(const Object &other) : m_object(other.m_object), m_executor(other.m_executor)
{
}

void ActionExecutor::Object::triggerAction(int identifier, const QVariantMap &parameters)
{
	if (!m_object.isNull())
	{
		m_executor->triggerAction(identifier, parameters);
	}
}

QObject* ActionExecutor::Object::getObject() const
{
	return m_object.data();
}

ActionExecutor::Object& ActionExecutor::Object::operator=(const Object &other)
{
	if (this != &other)
	{
		m_object = other.m_object;
		m_executor = other.m_executor;
	}

	return *this;
}

ActionsManager::ActionDefinition::State ActionExecutor::Object::getActionState(int identifier, const QVariantMap &parameters) const
{
	if (!m_object.isNull())
	{
		return m_executor->getActionState(identifier, parameters);
	}

	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());
	state.isEnabled = false;

	return state;
}

bool ActionExecutor::Object::isValid() const
{
	return !m_object.isNull();
}

ActionExecutor::ActionExecutor()
{
}

ActionExecutor::~ActionExecutor()
{
}

}
