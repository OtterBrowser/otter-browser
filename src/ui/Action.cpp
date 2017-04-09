/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "Action.h"
#include "../core/ThemesManager.h"

#include <QtGui/QGuiApplication>

namespace Otter
{

Action::Action(int identifier, QObject *parent) : QAction(parent),
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

	m_identifier = action->getIdentifier();

	setCheckable(action->isCheckable());
	setState(action->getState());
}

void Action::update(bool reset)
{
	if (m_identifier < 0)
	{
		if (m_isOverridingText)
		{
			setText(QCoreApplication::translate("actions", m_overrideText.toUtf8().constData()));
		}

		return;
	}

	const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(m_identifier));
	ActionsManager::ActionDefinition::State state;

	if (reset)
	{
		state = definition.defaultState;
		state.text = QCoreApplication::translate("actions", state.text.toUtf8().constData());

		switch (m_identifier)
		{
			case ActionsManager::GoBackAction:
				state.icon = ThemesManager::getIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-previous") : QLatin1String("go-next"));

				break;
			case ActionsManager::GoForwardAction:
				state.icon = ThemesManager::getIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-next") : QLatin1String("go-previous"));

				break;
			case ActionsManager::RewindAction:
				state.icon = ThemesManager::getIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-first") : QLatin1String("go-last"));

				break;
			case ActionsManager::FastForwardAction:
				state.icon = ThemesManager::getIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-last") : QLatin1String("go-first"));

				break;
			default:
				break;
		}

		setCheckable(definition.flags.testFlag(ActionsManager::ActionDefinition::IsCheckableFlag));
	}
	else
	{
		state = getState();
	}

	if (m_isOverridingText)
	{
		state.text = QCoreApplication::translate("actions", m_overrideText.toUtf8().constData());
	}

	if (!definition.shortcuts.isEmpty())
	{
		state.text += QLatin1Char('\t') + definition.shortcuts.first().toString(QKeySequence::NativeText);
	}

	setState(state);
}

void Action::setOverrideText(const QString &text)
{
	m_overrideText = text;
	m_isOverridingText = true;

	update();
}

void Action::setState(const ActionsManager::ActionDefinition::State &state)
{
	setText(state.text);
	setIcon(state.icon);
	setEnabled(state.isEnabled);
	setChecked(state.isChecked);
}

void Action::setParameters(const QVariantMap &parameters)
{
	m_parameters = parameters;

	update();
}

QString Action::getText() const
{
	return QCoreApplication::translate("actions", (m_isOverridingText ? m_overrideText : ActionsManager::getActionDefinition(m_identifier).defaultState.text).toUtf8().constData());
}

ActionsManager::ActionDefinition::State Action::getState() const
{
	ActionsManager::ActionDefinition::State state;
	state.text = getText();
	state.icon = icon();
	state.isEnabled = isEnabled();
	state.isChecked = isChecked();

	return state;
}

QVariantMap Action::getParameters() const
{
	return m_parameters;
}

QVector<QKeySequence> Action::getShortcuts() const
{
	return ActionsManager::getActionDefinition(m_identifier).shortcuts;
}

int Action::getIdentifier() const
{
	return m_identifier;
}

bool Action::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::LanguageChange:
			update();

			break;
		case QEvent::LayoutDirectionChange:
			switch (m_identifier)
			{
				case ActionsManager::GoBackAction:
				case ActionsManager::GoForwardAction:
				case ActionsManager::RewindAction:
				case ActionsManager::FastForwardAction:
					update(true);

					break;
				default:
					break;
			}

			break;
		default:
			break;
	}

	return QAction::event(event);
}

bool Action::calculateCheckedState(const QVariantMap &parameters, Action *action)
{
	if (parameters.contains(QLatin1String("isChecked")))
	{
		return parameters.value(QLatin1String("isChecked")).toBool();
	}

	if (action)
	{
		action->toggle();

		return action->isChecked();
	}

	return true;
}

bool Action::isLocal(int identifier)
{
	switch (identifier)
	{
		case ActionsManager::SaveAction:
		case ActionsManager::CloneTabAction:
		case ActionsManager::ClearTabHistoryAction:
		case ActionsManager::PurgeTabHistoryAction:
		case ActionsManager::MuteTabMediaAction:
		case ActionsManager::OpenLinkAction:
		case ActionsManager::OpenLinkInCurrentTabAction:
		case ActionsManager::OpenLinkInNewTabAction:
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
		case ActionsManager::OpenLinkInNewWindowAction:
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
		case ActionsManager::OpenLinkInApplicationAction:
		case ActionsManager::CopyLinkToClipboardAction:
		case ActionsManager::BookmarkLinkAction:
		case ActionsManager::SaveLinkToDiskAction:
		case ActionsManager::SaveLinkToDownloadsAction:
		case ActionsManager::OpenSelectionAsLinkAction:
		case ActionsManager::OpenFrameInCurrentTabAction:
		case ActionsManager::OpenFrameInNewTabAction:
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
		case ActionsManager::OpenFrameInApplicationAction:
		case ActionsManager::CopyFrameLinkToClipboardAction:
		case ActionsManager::ReloadFrameAction:
		case ActionsManager::ViewFrameSourceAction:
		case ActionsManager::OpenImageInNewTabAction:
		case ActionsManager::OpenImageInNewTabBackgroundAction:
		case ActionsManager::SaveImageToDiskAction:
		case ActionsManager::CopyImageToClipboardAction:
		case ActionsManager::CopyImageUrlToClipboardAction:
		case ActionsManager::ReloadImageAction:
		case ActionsManager::ImagePropertiesAction:
		case ActionsManager::SaveMediaToDiskAction:
		case ActionsManager::CopyMediaUrlToClipboardAction:
		case ActionsManager::MediaControlsAction:
		case ActionsManager::MediaLoopAction:
		case ActionsManager::MediaPlayPauseAction:
		case ActionsManager::MediaMuteAction:
		case ActionsManager::FillPasswordAction:
		case ActionsManager::GoBackAction:
		case ActionsManager::GoForwardAction:
		case ActionsManager::RewindAction:
		case ActionsManager::FastForwardAction:
		case ActionsManager::StopAction:
		case ActionsManager::StopScheduledReloadAction:
		case ActionsManager::ReloadAction:
		case ActionsManager::ReloadOrStopAction:
		case ActionsManager::ReloadAndBypassCacheAction:
		case ActionsManager::ScheduleReloadAction:
		case ActionsManager::UndoAction:
		case ActionsManager::RedoAction:
		case ActionsManager::CutAction:
		case ActionsManager::CopyAction:
		case ActionsManager::CopyPlainTextAction:
		case ActionsManager::CopyAddressAction:
		case ActionsManager::CopyToNoteAction:
		case ActionsManager::PasteAction:
		case ActionsManager::PasteAndGoAction:
		case ActionsManager::PasteNoteAction:
		case ActionsManager::DeleteAction:
		case ActionsManager::SelectAllAction:
		case ActionsManager::UnselectAction:
		case ActionsManager::ClearAllAction:
		case ActionsManager::CheckSpellingAction:
		case ActionsManager::SelectDictionaryAction:
		case ActionsManager::FindAction:
		case ActionsManager::FindNextAction:
		case ActionsManager::FindPreviousAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::SearchAction:
		case ActionsManager::CreateSearchAction:
		case ActionsManager::ZoomInAction:
		case ActionsManager::ZoomOutAction:
		case ActionsManager::ZoomOriginalAction:
		case ActionsManager::ScrollToStartAction:
		case ActionsManager::ScrollToEndAction:
		case ActionsManager::ScrollPageUpAction:
		case ActionsManager::ScrollPageDownAction:
		case ActionsManager::ScrollPageLeftAction:
		case ActionsManager::ScrollPageRightAction:
		case ActionsManager::StartDragScrollAction:
		case ActionsManager::StartMoveScrollAction:
		case ActionsManager::EndScrollAction:
		case ActionsManager::ActivateContentAction:
		case ActionsManager::BookmarkPageAction:
		case ActionsManager::QuickBookmarkAccessAction:
		case ActionsManager::LoadPluginsAction:
		case ActionsManager::EnableJavaScriptAction:
		case ActionsManager::EnableReferrerAction:
		case ActionsManager::ViewSourceAction:
		case ActionsManager::OpenPageInApplicationAction:
		case ActionsManager::InspectPageAction:
		case ActionsManager::InspectElementAction:
		case ActionsManager::WebsitePreferencesAction:
		case ActionsManager::QuickPreferencesAction:
		case ActionsManager::ResetQuickPreferencesAction:
		case ActionsManager::WebsiteInformationAction:
		case ActionsManager::WebsiteCertificateInformationAction:
			return true;
		default:
			break;
	}

	return false;
}

Shortcut::Shortcut(int identifier, const QKeySequence &sequence, QWidget *parent) : QShortcut(sequence, parent),
	m_identifier(identifier)
{
}

void Shortcut::setParameters(const QVariantMap &parameters)
{
	m_parameters = parameters;
}

QVariantMap Shortcut::getParameters() const
{
	return m_parameters;
}

int Shortcut::getIdentifier() const
{
	return m_identifier;
}

}
