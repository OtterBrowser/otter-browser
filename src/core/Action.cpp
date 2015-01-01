/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "Action.h"
#include "ActionsManager.h"

#include <QtCore/QCoreApplication>

namespace Otter
{

Action::Action(int identifier, QObject *parent) : QAction(QIcon(), QString(), parent),
	m_identifier(identifier),
	m_isOverridingText(false)
{
	update(true);
}

void Action::setup(Action *action)
{
	if (!action)
	{
		action = qobject_cast<Action*>(sender());
	}

	if (!action)
	{
		update(true);
		setEnabled(false);

		return;
	}

	setEnabled(action->isEnabled());
	setText(action->text());
	setIcon(action->icon());
	setCheckable(action->isCheckable());
	setChecked(action->isChecked());
}

void Action::update(bool reset)
{
	if (m_identifier < 0)
	{
		return;
	}

	const ActionDefinition action = ActionsManager::getActionDefinition(m_identifier);
	QString text = QCoreApplication::translate("actions", (m_isOverridingText ? m_overrideText : action.text).toUtf8().constData());

	if (!action.shortcuts.isEmpty())
	{
		text += QLatin1Char('\t') + action.shortcuts.first().toString(QKeySequence::NativeText);
	}

	setText(text);

	if (reset)
	{
		setEnabled(action.isEnabled);
		setCheckable(action.isCheckable);
		setChecked(action.isChecked);
		setIcon(action.icon);
	}
}

void Action::setOverrideText(const QString &text)
{
	m_overrideText = text;
	m_isOverridingText = true;

	update();
}

QList<QKeySequence> Action::getShortcuts() const
{
	return ActionsManager::getActionDefinition(m_identifier).shortcuts.toList();
}

int Action::getIdentifier() const
{
	return m_identifier;
}

bool Action::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		update();
	}

	return QAction::event(event);
}

bool Action::isLocal(int identifier)
{
	switch (identifier)
	{
		case Action::SaveAction:
		case Action::CloneTabAction:
		case Action::CloseTabAction:
		case Action::OpenLinkAction:
		case Action::OpenLinkInCurrentTabAction:
		case Action::OpenLinkInNewTabAction:
		case Action::OpenLinkInNewTabBackgroundAction:
		case Action::OpenLinkInNewWindowAction:
		case Action::OpenLinkInNewWindowBackgroundAction:
		case Action::CopyLinkToClipboardAction:
		case Action::BookmarkLinkAction:
		case Action::SaveLinkToDiskAction:
		case Action::SaveLinkToDownloadsAction:
		case Action::OpenSelectionAsLinkAction:
		case Action::OpenFrameInCurrentTabAction:
		case Action::OpenFrameInNewTabAction:
		case Action::OpenFrameInNewTabBackgroundAction:
		case Action::CopyFrameLinkToClipboardAction:
		case Action::ReloadFrameAction:
		case Action::ViewFrameSourceAction:
		case Action::OpenImageInNewTabAction:
		case Action::SaveImageToDiskAction:
		case Action::CopyImageToClipboardAction:
		case Action::CopyImageUrlToClipboardAction:
		case Action::ReloadImageAction:
		case Action::ImagePropertiesAction:
		case Action::SaveMediaToDiskAction:
		case Action::CopyMediaUrlToClipboardAction:
		case Action::MediaControlsAction:
		case Action::MediaLoopAction:
		case Action::MediaPlayPauseAction:
		case Action::MediaMuteAction:
		case Action::GoBackAction:
		case Action::GoForwardAction:
		case Action::GoToParentDirectoryAction:
		case Action::RewindAction:
		case Action::FastForwardAction:
		case Action::StopAction:
		case Action::StopScheduledReloadAction:
		case Action::ReloadAction:
		case Action::ReloadOrStopAction:
		case Action::ReloadAndBypassCacheAction:
		case Action::ScheduleReloadAction:
		case Action::UndoAction:
		case Action::RedoAction:
		case Action::CutAction:
		case Action::CopyAction:
		case Action::CopyPlainTextAction:
		case Action::CopyAddressAction:
		case Action::PasteAction:
		case Action::PasteAndGoAction:
		case Action::DeleteAction:
		case Action::SelectAllAction:
		case Action::ClearAllAction:
		case Action::CheckSpellingAction:
		case Action::FindAction:
		case Action::FindNextAction:
		case Action::FindPreviousAction:
		case Action::QuickFindAction:
		case Action::SearchAction:
		case Action::SearchMenuAction:
		case Action::CreateSearchAction:
		case Action::ZoomInAction:
		case Action::ZoomOutAction:
		case Action::ZoomOriginalAction:
		case Action::ScrollToStartAction:
		case Action::ScrollToEndAction:
		case Action::ScrollPageUpAction:
		case Action::ScrollPageDownAction:
		case Action::ScrollPageLeftAction:
		case Action::ScrollPageRightAction:
		case Action::ActivateAddressFieldAction:
		case Action::ActivateSearchFieldAction:
		case Action::ActivateContentAction:
		case Action::ActivateTabOnLeftAction:
		case Action::ActivateTabOnRightAction:
		case Action::AddBookmarkAction:
		case Action::QuickBookmarkAccessAction:
		case Action::PopupsPolicyAction:
		case Action::ImagesPolicyAction:
		case Action::CookiesPolicyAction:
		case Action::ThirdPartyCookiesPolicyAction:
		case Action::PluginsPolicyAction:
		case Action::LoadPluginsAction:
		case Action::EnableJavaScriptAction:
		case Action::EnableJavaAction:
		case Action::EnableReferrerAction:
		case Action::ProxyMenuAction:
		case Action::EnableProxyAction:
		case Action::ViewSourceAction:
		case Action::ValidateAction:
		case Action::InspectPageAction:
		case Action::InspectElementAction:
		case Action::WebsitePreferencesAction:
		case Action::QuickPreferencesAction:
		case Action::ResetQuickPreferencesAction:
			return true;
		default:
			break;
	}

	return false;
}

}
