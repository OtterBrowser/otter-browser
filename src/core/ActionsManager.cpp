/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include <QtCore/QTextStream>
#include <QtGui/QKeySequence>

namespace Otter
{

bool KeyboardProfile::Action::operator ==(const KeyboardProfile::Action &other) const
{
	return (shortcuts == other.shortcuts && parameters == other.parameters && action == other.action);
}

KeyboardProfile::KeyboardProfile(const QString &identifier, LoadMode mode) : Addon(),
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

	if (mode == MetaDataOnlyMode)
	{
		return;
	}

	const QJsonArray contextsArray(settings.array());
	const bool areSingleKeyShortcutsAllowed((mode == FullMode) ? true : SettingsManager::getOption(SettingsManager::Browser_EnableSingleKeyShortcutsOption).toBool());

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
				const QKeySequence shortcut(shortcutsArray.at(k).toString());

				if (shortcut.isEmpty() || (!areSingleKeyShortcutsAllowed && !ActionsManager::isShortcutAllowed(shortcut, ActionsManager::DisallowSingleKeyShortcutCheck, false)))
				{
					continue;
				}

				shortcuts.append(shortcut);
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
QVector<QKeySequence> ActionsManager::m_disallowedShortcuts;
QVector<ActionsManager::ActionDefinition> ActionsManager::m_definitions;
int ActionsManager::m_actionIdentifierEnumerator(0);

ActionsManager::ActionsManager(QObject *parent) : QObject(parent),
	m_reloadTimer(0)
{
	m_definitions.reserve(ActionsManager::OtherAction);

	registerAction(RunMacroAction, QT_TRANSLATE_NOOP("actions", "Run Macro"), QT_TRANSLATE_NOOP("actions", "Run Arbitrary List of Actions"), {}, ActionDefinition::ApplicationScope, ActionDefinition::RequiresParameters);
	registerAction(SetOptionAction, QT_TRANSLATE_NOOP("actions", "Set Option"), QT_TRANSLATE_NOOP("actions", "Set, Reset or Toggle Option"), {}, ActionDefinition::ApplicationScope, ActionDefinition::RequiresParameters);
	registerAction(NewTabAction, QT_TRANSLATE_NOOP("actions", "New Tab"), {}, ThemesManager::createIcon(QLatin1String("tab-new")), ActionDefinition::MainWindowScope);
	registerAction(NewTabPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Tab"), {}, ThemesManager::createIcon(QLatin1String("tab-new-private")), ActionDefinition::MainWindowScope);
	registerAction(NewWindowAction, QT_TRANSLATE_NOOP("actions", "New Window"), {}, ThemesManager::createIcon(QLatin1String("window-new")), ActionDefinition::ApplicationScope);
	registerAction(NewWindowPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Window"), {}, ThemesManager::createIcon(QLatin1String("window-new-private")), ActionDefinition::ApplicationScope);
	registerAction(OpenAction, QT_TRANSLATE_NOOP("actions", "Open…"), {}, ThemesManager::createIcon(QLatin1String("document-open")), ActionDefinition::MainWindowScope);
	registerAction(SaveAction, QT_TRANSLATE_NOOP("actions", "Save…"), {}, ThemesManager::createIcon(QLatin1String("document-save")), ActionDefinition::WindowScope);
	registerAction(CloneTabAction, QT_TRANSLATE_NOOP("actions", "Clone Tab"), {}, {}, ActionDefinition::WindowScope);
	registerAction(PeekTabAction, QT_TRANSLATE_NOOP("actions", "Peek Tab"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(PinTabAction, QT_TRANSLATE_NOOP("actions", "Pin Tab"), {}, {}, ActionDefinition::WindowScope);
	registerAction(DetachTabAction, QT_TRANSLATE_NOOP("actions", "Detach Tab"), {}, {}, ActionDefinition::WindowScope);
	registerAction(MaximizeTabAction, QT_TRANSLATE_NOOP("actions", "Maximize"), QT_TRANSLATE_NOOP("actions", "Maximize Tab"), {}, ActionDefinition::WindowScope);
	registerAction(MinimizeTabAction, QT_TRANSLATE_NOOP("actions", "Minimize"), QT_TRANSLATE_NOOP("actions", "Minimize Tab"), {}, ActionDefinition::WindowScope);
	registerAction(RestoreTabAction, QT_TRANSLATE_NOOP("actions", "Restore"), QT_TRANSLATE_NOOP("actions", "Restore Tab"), {}, ActionDefinition::WindowScope);
	registerAction(AlwaysOnTopTabAction, QT_TRANSLATE_NOOP("actions", "Stay on Top"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ClearTabHistoryAction, QT_TRANSLATE_NOOP("actions", "Clear Tab History"), QT_TRANSLATE_NOOP("actions", "Remove Local Tab History"), {}, ActionDefinition::WindowScope, ActionDefinition::NoFlags, ActionsManager::ActionDefinition::NavigationCategory);
	registerAction(PurgeTabHistoryAction, QT_TRANSLATE_NOOP("actions", "Purge Tab History"), QT_TRANSLATE_NOOP("actions", "Remove Local and Global Tab History"), {}, ActionDefinition::WindowScope, ActionDefinition::IsDeprecatedFlag, ActionsManager::ActionDefinition::NavigationCategory);
	registerAction(MuteTabMediaAction, QT_TRANSLATE_NOOP("actions", "Mute Tab Media"), {}, ThemesManager::createIcon(QLatin1String("audio-volume-muted")), ActionDefinition::WindowScope);
	registerAction(SuspendTabAction, QT_TRANSLATE_NOOP("actions", "Suspend Tab"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(CloseTabAction, QT_TRANSLATE_NOOP("actions", "Close Tab"), {}, ThemesManager::createIcon(QLatin1String("tab-close")), ActionDefinition::WindowScope);
	registerAction(CloseOtherTabsAction, QT_TRANSLATE_NOOP("actions", "Close Other Tabs"), {}, ThemesManager::createIcon(QLatin1String("tab-close-other")), ActionDefinition::MainWindowScope);
	registerAction(ClosePrivateTabsAction, QT_TRANSLATE_NOOP("actions", "Close All Private Tabs"), QT_TRANSLATE_NOOP("actions", "Close All Private Tabs in Current Window"), {}, ActionDefinition::MainWindowScope, ActionDefinition::NoFlags);
	registerAction(ClosePrivateTabsPanicAction, QT_TRANSLATE_NOOP("actions", "Close Private Tabs and Windows"), {}, {}, ActionDefinition::ApplicationScope);
	registerAction(ReopenTabAction, QT_TRANSLATE_NOOP("actions", "Reopen Previously Closed Tab"), {}, {}, ActionDefinition::MainWindowScope, ActionDefinition::NoFlags);
	registerAction(MaximizeAllAction, QT_TRANSLATE_NOOP("actions", "Maximize All"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(MinimizeAllAction, QT_TRANSLATE_NOOP("actions", "Minimize All"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(RestoreAllAction, QT_TRANSLATE_NOOP("actions", "Restore All"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(CascadeAllAction, QT_TRANSLATE_NOOP("actions", "Cascade"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(TileAllAction, QT_TRANSLATE_NOOP("actions", "Tile"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(CloseWindowAction, QT_TRANSLATE_NOOP("actions", "Close Window"), {}, {}, ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(ReopenWindowAction, QT_TRANSLATE_NOOP("actions", "Reopen Previously Closed Window"), {}, {}, ActionDefinition::ApplicationScope, ActionDefinition::NoFlags);
	registerAction(SessionsAction, QT_TRANSLATE_NOOP("actions", "Manage Sessions…"), {}, {}, ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(SaveSessionAction, QT_TRANSLATE_NOOP("actions", "Save Current Session…"), {}, {}, ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(OpenUrlAction, QT_TRANSLATE_NOOP("actions", "Open URL"), {}, {}, ActionDefinition::MainWindowScope, ActionDefinition::RequiresParameters);
	registerAction(OpenLinkAction, QT_TRANSLATE_NOOP("actions", "Open"), {}, ThemesManager::createIcon(QLatin1String("document-open")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(OpenLinkInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open in This Tab"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewWindowAction, QT_TRANSLATE_NOOP("actions", "Open in New Window"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewWindowBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Window"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewPrivateTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Tab"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewPrivateTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Background Tab"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewPrivateWindowAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Window"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(OpenLinkInNewPrivateWindowBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Background Window"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::LinkCategory);
	registerAction(CopyLinkToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Link to Clipboard"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(BookmarkLinkAction, QT_TRANSLATE_NOOP("actions", "Bookmark Link…"), {}, ThemesManager::createIcon(QLatin1String("bookmark-new")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(SaveLinkToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Link Target As…"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(SaveLinkToDownloadsAction, QT_TRANSLATE_NOOP("actions", "Save to Downloads"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::LinkCategory);
	registerAction(OpenSelectionAsLinkAction, QT_TRANSLATE_NOOP("actions", "Go to This Address"), {}, {}, ActionDefinition::WindowScope);
	registerAction(OpenFrameAction, QT_TRANSLATE_NOOP("actions", "Open"), QT_TRANSLATE_NOOP("actions", "Open Frame"), {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::FrameCategory);
	registerAction(OpenFrameInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open"), QT_TRANSLATE_NOOP("actions", "Open Frame in This Tab"), {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::FrameCategory);
	registerAction(OpenFrameInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Tab"), {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::FrameCategory);
	registerAction(OpenFrameInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Background Tab"), {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::FrameCategory);
	registerAction(CopyFrameLinkToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Frame Link to Clipboard"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::FrameCategory);
	registerAction(ReloadFrameAction, QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload Frame"), {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::FrameCategory);
	registerAction(ViewFrameSourceAction, QT_TRANSLATE_NOOP("actions", "View Frame Source"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::FrameCategory);
	registerAction(OpenImageAction, QT_TRANSLATE_NOOP("actions", "Open Image"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(OpenImageInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open Image In New Tab"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::ImageCategory);
	registerAction(OpenImageInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open Image in New Background Tab"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::ImageCategory);
	registerAction(SaveImageToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Image…"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(CopyImageToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Image to Clipboard"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(CopyImageUrlToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Image Link to Clipboard"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(ReloadImageAction, QT_TRANSLATE_NOOP("actions", "Reload Image"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(ImagePropertiesAction, QT_TRANSLATE_NOOP("actions", "Image Properties…"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::ImageCategory);
	registerAction(SaveMediaToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Media…"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::MediaCategory);
	registerAction(CopyMediaUrlToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Media Link to Clipboard"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::MediaCategory);
	registerAction(MediaControlsAction, QT_TRANSLATE_NOOP("actions", "Show Controls"), QT_TRANSLATE_NOOP("actions", "Show Media Controls"), {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag), ActionDefinition::MediaCategory);
	registerAction(MediaLoopAction, QT_TRANSLATE_NOOP("actions", "Looping"), QT_TRANSLATE_NOOP("actions", "Playback Looping"), {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag), ActionDefinition::MediaCategory);
	registerAction(MediaPlayPauseAction, QT_TRANSLATE_NOOP("actions", "Play"), QT_TRANSLATE_NOOP("actions", "Play Media"), ThemesManager::createIcon(QLatin1String("media-playback-start")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::MediaCategory);
	registerAction(MediaMuteAction, QT_TRANSLATE_NOOP("actions", "Mute"), QT_TRANSLATE_NOOP("actions", "Mute Media"), ThemesManager::createIcon(QLatin1String("audio-volume-muted")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::MediaCategory);
	registerAction(MediaPlaybackRateAction, QT_TRANSLATE_NOOP("actions", "Playback Rate"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsCheckableFlag, ActionDefinition::MediaCategory);
	registerAction(FillPasswordAction, QT_TRANSLATE_NOOP("actions", "Log In"), {}, ThemesManager::createIcon(QLatin1String("fill-password")), ActionDefinition::WindowScope, ActionDefinition::NoFlags, ActionDefinition::PageCategory);
	registerAction(GoAction, QT_TRANSLATE_NOOP("actions", "Go"), QT_TRANSLATE_NOOP("actions", "Go to URL"), ThemesManager::createIcon(QLatin1String("go-jump-locationbar")), ActionDefinition::MainWindowScope);
	registerAction(GoBackAction, QT_TRANSLATE_NOOP("actions", "Back"), QT_TRANSLATE_NOOP("actions", "Go Back"), ThemesManager::createIcon(QLatin1String("go-previous")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(GoForwardAction, QT_TRANSLATE_NOOP("actions", "Forward"), QT_TRANSLATE_NOOP("actions", "Go Forward"), ThemesManager::createIcon(QLatin1String("go-next")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(GoToHistoryIndexAction, QT_TRANSLATE_NOOP("actions", "Go to History Entry"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::RequiresParameters, ActionDefinition::NavigationCategory);
	registerAction(GoToPageAction, QT_TRANSLATE_NOOP("actions", "Go to Page or Search"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(GoToHomePageAction, QT_TRANSLATE_NOOP("actions", "Go to Home Page"), {}, ThemesManager::createIcon(QLatin1String("go-home")), ActionDefinition::MainWindowScope);
	registerAction(GoToParentDirectoryAction, QT_TRANSLATE_NOOP("actions", "Go to Parent Directory"), {}, {}, ActionDefinition::WindowScope);
	registerAction(RewindAction, QT_TRANSLATE_NOOP("actions", "Rewind"), QT_TRANSLATE_NOOP("actions", "Rewind History"), ThemesManager::createIcon(QLatin1String("go-first")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(FastForwardAction, QT_TRANSLATE_NOOP("actions", "Fast Forward"), {}, ThemesManager::createIcon(QLatin1String("go-last")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(RemoveHistoryIndexAction, QT_TRANSLATE_NOOP("actions", "Remove History Entry"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::RequiresParameters, ActionDefinition::NavigationCategory);
	registerAction(StopAction, QT_TRANSLATE_NOOP("actions", "Stop"), {}, ThemesManager::createIcon(QLatin1String("process-stop")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(StopScheduledReloadAction, QT_TRANSLATE_NOOP("actions", "Stop Scheduled Page Reload"), {}, {}, ActionDefinition::WindowScope);
	registerAction(StopAllAction, QT_TRANSLATE_NOOP("actions", "Stop All Pages"), {}, ThemesManager::createIcon(QLatin1String("process-stop")), ActionDefinition::MainWindowScope);
	registerAction(ReloadAction, QT_TRANSLATE_NOOP("actions", "Reload"), {}, ThemesManager::createIcon(QLatin1String("view-refresh")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(ReloadOrStopAction, QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload or Stop"), ThemesManager::createIcon(QLatin1String("view-refresh")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::NavigationCategory);
	registerAction(ReloadAndBypassCacheAction, QT_TRANSLATE_NOOP("actions", "Reload and Bypass Cache"), {}, {}, ActionDefinition::WindowScope);
	registerAction(ReloadAllAction, QT_TRANSLATE_NOOP("actions", "Reload All Tabs"), {}, ThemesManager::createIcon(QLatin1String("view-refresh")), ActionDefinition::MainWindowScope);
	registerAction(ScheduleReloadAction, QT_TRANSLATE_NOOP("actions", "Schedule Page Reload"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ContextMenuAction, QT_TRANSLATE_NOOP("actions", "Show Context Menu"), {}, {}, ActionDefinition::WindowScope);
	registerAction(UndoAction, QT_TRANSLATE_NOOP("actions", "Undo"), {}, ThemesManager::createIcon(QLatin1String("edit-undo")), ActionDefinition::EditorScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(RedoAction, QT_TRANSLATE_NOOP("actions", "Redo"), {}, ThemesManager::createIcon(QLatin1String("edit-redo")), ActionDefinition::EditorScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(CutAction, QT_TRANSLATE_NOOP("actions", "Cut"), {}, ThemesManager::createIcon(QLatin1String("edit-cut")), ActionDefinition::EditorScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(CopyAction, QT_TRANSLATE_NOOP("actions", "Copy"), {}, ThemesManager::createIcon(QLatin1String("edit-copy")), ActionDefinition::EditorScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(CopyPlainTextAction, QT_TRANSLATE_NOOP("actions", "Copy as Plain Text"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag), ActionDefinition::EditingCategory);
	registerAction(CopyAddressAction, QT_TRANSLATE_NOOP("actions", "Copy Address"), {}, {}, ActionDefinition::WindowScope);
	registerAction(CopyToNoteAction, QT_TRANSLATE_NOOP("actions", "Copy to Note"), {}, {}, ActionDefinition::EditorScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(PasteAction, QT_TRANSLATE_NOOP("actions", "Paste"), {}, ThemesManager::createIcon(QLatin1String("edit-paste")), ActionDefinition::EditorScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(PasteAndGoAction, QT_TRANSLATE_NOOP("actions", "Paste and Go"), {}, {}, ActionDefinition::WindowScope);
	registerAction(DeleteAction, QT_TRANSLATE_NOOP("actions", "Delete"), {}, ThemesManager::createIcon(QLatin1String("edit-delete")), ActionDefinition::EditorScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(SelectAllAction, QT_TRANSLATE_NOOP("actions", "Select All"), {}, ThemesManager::createIcon(QLatin1String("edit-select-all")), ActionDefinition::EditorScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(UnselectAction, QT_TRANSLATE_NOOP("actions", "Deselect"), {}, {}, ActionDefinition::EditorScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(ClearAllAction, QT_TRANSLATE_NOOP("actions", "Clear All"), {}, {}, ActionDefinition::EditorScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(CheckSpellingAction, QT_TRANSLATE_NOOP("actions", "Check Spelling"), {}, {}, ActionDefinition::EditorScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag), ActionDefinition::EditingCategory);
	registerAction(FindAction, QT_TRANSLATE_NOOP("actions", "Find…"), {}, ThemesManager::createIcon(QLatin1String("edit-find")), ActionDefinition::WindowScope);
	registerAction(FindNextAction, QT_TRANSLATE_NOOP("actions", "Find Next"), {}, ThemesManager::createIcon(QLatin1String("go-down")), ActionDefinition::WindowScope);
	registerAction(FindPreviousAction, QT_TRANSLATE_NOOP("actions", "Find Previous"), {}, ThemesManager::createIcon(QLatin1String("go-up")), ActionDefinition::WindowScope);
	registerAction(QuickFindAction, QT_TRANSLATE_NOOP("actions", "Quick Find"), {}, {}, ActionDefinition::WindowScope);
	registerAction(SearchAction, QT_TRANSLATE_NOOP("actions", "Search"), {}, ThemesManager::createIcon(QLatin1String("edit-find")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(CreateSearchAction, QT_TRANSLATE_NOOP("actions", "Create Search…"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::EditingCategory);
	registerAction(ZoomInAction, QT_TRANSLATE_NOOP("actions", "Zoom In"), {}, ThemesManager::createIcon(QLatin1String("zoom-in")), ActionDefinition::WindowScope);
	registerAction(ZoomOutAction, QT_TRANSLATE_NOOP("actions", "Zoom Out"), {}, ThemesManager::createIcon(QLatin1String("zoom-out")), ActionDefinition::WindowScope);
	registerAction(ZoomOriginalAction, QT_TRANSLATE_NOOP("actions", "Zoom Original"), {}, ThemesManager::createIcon(QLatin1String("zoom-original")), ActionDefinition::WindowScope);
	registerAction(ScrollToStartAction, QT_TRANSLATE_NOOP("actions", "Go to Start of the Page"), {}, {}, ActionDefinition::WindowScope);
	registerAction(ScrollToEndAction, QT_TRANSLATE_NOOP("actions", "Go to the End of the Page"), {}, {}, ActionDefinition::WindowScope);
	registerAction(ScrollPageUpAction, QT_TRANSLATE_NOOP("actions", "Page Up"), {}, {}, ActionDefinition::WindowScope);
	registerAction(ScrollPageDownAction, QT_TRANSLATE_NOOP("actions", "Page Down"), {}, {}, ActionDefinition::WindowScope);
	registerAction(ScrollPageLeftAction, QT_TRANSLATE_NOOP("actions", "Page Left"), {}, {}, ActionDefinition::WindowScope);
	registerAction(ScrollPageRightAction, QT_TRANSLATE_NOOP("actions", "Page Right"), {}, {}, ActionDefinition::WindowScope);
	registerAction(StartDragScrollAction, QT_TRANSLATE_NOOP("actions", "Enter Drag Scroll Mode"), {}, {}, ActionDefinition::WindowScope);
	registerAction(StartMoveScrollAction, QT_TRANSLATE_NOOP("actions", "Enter Move Scroll Mode"), {}, {}, ActionDefinition::WindowScope);
	registerAction(EndScrollAction, QT_TRANSLATE_NOOP("actions", "Exit Scroll Mode"), {}, {}, ActionDefinition::WindowScope);
	registerAction(PrintAction, QT_TRANSLATE_NOOP("actions", "Print…"), {}, ThemesManager::createIcon(QLatin1String("document-print")), ActionDefinition::WindowScope);
	registerAction(PrintPreviewAction, QT_TRANSLATE_NOOP("actions", "Print Preview"), {}, ThemesManager::createIcon(QLatin1String("document-print-preview")), ActionDefinition::WindowScope);
	registerAction(TakeScreenshotAction, QT_TRANSLATE_NOOP("actions", "Take Screenshot"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(ActivateAddressFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Address Field"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(ActivateSearchFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Search Field"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(ActivateContentAction, QT_TRANSLATE_NOOP("actions", "Activate Content"), {}, {}, ActionDefinition::WindowScope);
	registerAction(ActivatePreviouslyUsedTabAction, QT_TRANSLATE_NOOP("actions", "Go to Previously Used Tab"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(ActivateLeastRecentlyUsedTabAction, QT_TRANSLATE_NOOP("actions", "Go to Least Recently Used Tab"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(ActivateTabAction, QT_TRANSLATE_NOOP("actions", "Activate Tab"), {}, {}, ActionDefinition::MainWindowScope, ActionDefinition::RequiresParameters);
	registerAction(ActivateTabOnLeftAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Left"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(ActivateTabOnRightAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Right"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(ActivateWindowAction, QT_TRANSLATE_NOOP("actions", "Activate Window"), {}, {}, ActionDefinition::ApplicationScope, ActionDefinition::RequiresParameters);
	registerAction(BookmarksAction, QT_TRANSLATE_NOOP("actions", "Manage Bookmarks"), {}, ThemesManager::createIcon(QLatin1String("bookmarks-organize")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(BookmarkPageAction, QT_TRANSLATE_NOOP("actions", "Bookmark Page…"), {}, ThemesManager::createIcon(QLatin1String("bookmark-new")), ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::PageCategory);
	registerAction(BookmarkAllOpenPagesAction, QT_TRANSLATE_NOOP("actions", "Bookmark All Open Pages"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(OpenBookmarkAction, QT_TRANSLATE_NOOP("actions", "Open Bookmark"), {}, {}, ActionDefinition::MainWindowScope, ActionDefinition::RequiresParameters);
	registerAction(QuickBookmarkAccessAction, QT_TRANSLATE_NOOP("actions", "Quick Bookmark Access"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(CookiesAction, QT_TRANSLATE_NOOP("actions", "Cookies"), {}, ThemesManager::createIcon(QLatin1String("cookies")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(LoadPluginsAction, QT_TRANSLATE_NOOP("actions", "Load All Plugins on the Page"), {}, ThemesManager::createIcon(QLatin1String("preferences-plugin")), ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(EnableJavaScriptAction, QT_TRANSLATE_NOOP("actions", "Enable JavaScript"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag), ActionDefinition::PageCategory);
	registerAction(EnableReferrerAction, QT_TRANSLATE_NOOP("actions", "Enable Referrer"), {}, {}, ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag), ActionDefinition::PageCategory);
	registerAction(ViewSourceAction, QT_TRANSLATE_NOOP("actions", "View Source"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::NoFlags, ActionDefinition::NavigationCategory);
	registerAction(InspectPageAction, QT_TRANSLATE_NOOP("actions", "Inspect Page"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsCheckableFlag);
	registerAction(InspectElementAction, QT_TRANSLATE_NOOP("actions", "Inspect Element…"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(WorkOfflineAction, QT_TRANSLATE_NOOP("actions", "Work Offline"), {}, {}, ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(FullScreenAction, QT_TRANSLATE_NOOP("actions", "Full Screen"), {}, ThemesManager::createIcon(QLatin1String("view-fullscreen")), ActionDefinition::MainWindowScope);
	registerAction(ShowTabSwitcherAction, QT_TRANSLATE_NOOP("actions", "Show Tab Switcher"), {}, {}, ActionDefinition::MainWindowScope);
	registerAction(ShowToolBarAction, QT_TRANSLATE_NOOP("actions", "Show Toolbar"), {}, {}, ActionDefinition::MainWindowScope, (ActionDefinition::IsCheckableFlag | ActionDefinition::RequiresParameters));
	registerAction(ShowMenuBarAction, QT_TRANSLATE_NOOP("actions", "Show Menubar"), {}, {}, ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ShowTabBarAction, QT_TRANSLATE_NOOP("actions", "Show Tabbar"), {}, {}, ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ShowSidebarAction, QT_TRANSLATE_NOOP("actions", "Show Sidebar"), {}, ThemesManager::createIcon(QLatin1String("sidebar-show")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsDeprecatedFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ShowErrorConsoleAction, QT_TRANSLATE_NOOP("actions", "Show Error Console"), {}, {}, ActionDefinition::MainWindowScope, (ActionDefinition::IsDeprecatedFlag | ActionDefinition::IsCheckableFlag));
	registerAction(LockToolBarsAction, QT_TRANSLATE_NOOP("actions", "Lock Toolbars"), {}, {}, ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ResetToolBarsAction, QT_TRANSLATE_NOOP("actions", "Reset to Defaults…"), QT_TRANSLATE_NOOP("actions", "Reset Toolbars to Defaults…"), {}, ActionDefinition::ApplicationScope);
	registerAction(ShowPanelAction, QT_TRANSLATE_NOOP("actions", "Show Panel"), QT_TRANSLATE_NOOP("actions", "Show Specified Panel in Sidebar"), {}, ActionDefinition::MainWindowScope);
	registerAction(OpenPanelAction, QT_TRANSLATE_NOOP("actions", "Open Panel as Tab"), QT_TRANSLATE_NOOP("actions", "Open Curent Sidebar Panel as Tab"), ThemesManager::createIcon(QLatin1String("arrow-right")), ActionDefinition::MainWindowScope);
	registerAction(ContentBlockingAction, QT_TRANSLATE_NOOP("actions", "Content Blocking…"), {}, ThemesManager::createIcon(QLatin1String("content-blocking")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(HistoryAction, QT_TRANSLATE_NOOP("actions", "View History"), {}, ThemesManager::createIcon(QLatin1String("view-history")), ActionDefinition::MainWindowScope);
	registerAction(ClearHistoryAction, QT_TRANSLATE_NOOP("actions", "Clear History…"), {}, ThemesManager::createIcon(QLatin1String("edit-clear-history")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(AddonsAction, QT_TRANSLATE_NOOP("actions", "Addons"), {}, ThemesManager::createIcon(QLatin1String("preferences-plugin")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(NotesAction, QT_TRANSLATE_NOOP("actions", "Notes"), {}, ThemesManager::createIcon(QLatin1String("notes")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(PasswordsAction, QT_TRANSLATE_NOOP("actions", "Passwords"), {}, ThemesManager::createIcon(QLatin1String("dialog-password")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(TransfersAction, QT_TRANSLATE_NOOP("actions", "Downloads"), {}, ThemesManager::createIcon(QLatin1String("transfers")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(PreferencesAction, QT_TRANSLATE_NOOP("actions", "Preferences…"), {}, {}, ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(WebsitePreferencesAction, QT_TRANSLATE_NOOP("actions", "Website Preferences…"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::IsEnabledFlag, ActionDefinition::PageCategory);
	registerAction(QuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Quick Preferences"), {}, {}, ActionDefinition::WindowScope);
	registerAction(ResetQuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Reset Options"), {}, {}, ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(WebsiteInformationAction, QT_TRANSLATE_NOOP("actions", "Website Information…"), {}, {}, ActionDefinition::WindowScope);
	registerAction(WebsiteCertificateInformationAction, QT_TRANSLATE_NOOP("actions", "Website Certificate Information…"), {}, {}, ActionDefinition::WindowScope);
	registerAction(SwitchApplicationLanguageAction, QT_TRANSLATE_NOOP("actions", "Switch Application Language…"), {}, ThemesManager::createIcon(QLatin1String("preferences-desktop-locale")), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(CheckForUpdatesAction, QT_TRANSLATE_NOOP("actions", "Check for Updates…"), {}, {}, ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(DiagnosticReportAction, QT_TRANSLATE_NOOP("actions", "Diagnostic Report…"), {}, {}, ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(AboutApplicationAction, QT_TRANSLATE_NOOP("actions", "About Otter…"), {}, ThemesManager::createIcon(QLatin1String("otter-browser"), false), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(AboutQtAction, QT_TRANSLATE_NOOP("actions", "About Qt…"), {}, ThemesManager::createIcon(QLatin1String("qt")), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));
	registerAction(ExitAction, QT_TRANSLATE_NOOP("actions", "Exit"), {}, ThemesManager::createIcon(QLatin1String("application-exit")), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsImmutableFlag));

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &ActionsManager::handleOptionChanged);
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

				if (definition.parameters.isEmpty() && m_shortcuts.contains(definition.action))
				{
					shortcuts = m_shortcuts[definition.action];
				}
				else if (!definition.parameters.isEmpty() && m_extraShortcuts.contains(definition.action))
				{
					const QList<QPair<QVariantMap, QVector<QKeySequence> > > extraDefinitions(m_extraShortcuts.values(definition.action));

					for (int k = 0; k < extraDefinitions.count(); ++k)
					{
						if (extraDefinitions.at(k).first == definition.parameters)
						{
							shortcuts = extraDefinitions.at(k).second;

							break;
						}
					}
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
				const QVector<QKeySequence> shortcuts(definitions.at(j).second);

				stream << QLatin1Char('\t');
				stream.setFieldWidth(30);
				stream << QLatin1Char(' ') + QJsonDocument(QJsonObject::fromVariantMap(definitions.at(j).first)).toJson(QJsonDocument::Compact);
				stream.setFieldWidth(20);

				for (int k = 0; k < shortcuts.count(); ++k)
				{
					stream << shortcuts.at(k).toString(QKeySequence::PortableText);
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

	return {};
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
		return ActionsManager::staticMetaObject.enumerator(m_actionIdentifierEnumerator).keyToValue((name + QLatin1String("Action")).toLatin1());
	}

	return ActionsManager::staticMetaObject.enumerator(m_actionIdentifierEnumerator).keyToValue(name.toLatin1());
}

bool ActionsManager::isShortcutAllowed(const QKeySequence &shortcut, ShortcutCheck check, bool areSingleKeyShortcutsAllowed)
{
	if (shortcut.isEmpty())
	{
		return false;
	}

	if ((check == AllChecks || check == DisallowSingleKeyShortcutCheck) && (!areSingleKeyShortcutsAllowed && (shortcut[0] == Qt::Key_Plus || !shortcut.toString(QKeySequence::PortableText).contains(QLatin1Char('+'))) && shortcut[0] != Qt::Key_Delete && !(shortcut[0] >= Qt::Key_F1 && shortcut[0] <= Qt::Key_F35)))
	{
		return false;
	}

	if (check == AllChecks || check == DisallowStandardShortcutCheck)
	{
		if (m_disallowedShortcuts.isEmpty())
		{
			m_disallowedShortcuts = {QKeySequence(QKeySequence::Copy), QKeySequence(QKeySequence::Cut), QKeySequence(QKeySequence::Delete), QKeySequence(QKeySequence::Paste), QKeySequence(QKeySequence::Redo), QKeySequence(QKeySequence::SelectAll), QKeySequence(QKeySequence::Undo)};
		}

		if (m_disallowedShortcuts.contains(shortcut))
		{
			return false;
		}
	}

	return true;
}

}
