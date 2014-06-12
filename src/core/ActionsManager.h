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

#ifndef OTTER_ACTIONSMANAGER_H
#define OTTER_ACTIONSMANAGER_H

#include <QtCore/QObject>
#include <QtWidgets/QAction>
#include <QtWidgets/QShortcut>

namespace Otter
{

enum WindowAction
{
	NoAction = 0,
	OpenLinkAction,
	OpenLinkInThisTabAction,
	OpenLinkInNewTabAction,
	OpenLinkInNewTabBackgroundAction,
	OpenLinkInNewWindowAction,
	OpenLinkInNewWindowBackgroundAction,
	CopyLinkToClipboardAction,
	SaveLinkToDiskAction,
	SaveLinkToDownloadsAction,
	OpenFrameInThisTabAction,
	OpenFrameInNewTabAction,
	OpenFrameInNewTabBackgroundAction,
	CopyFrameLinkToClipboardAction,
	OpenImageInNewTabAction,
	SaveImageToDiskAction,
	CopyImageToClipboardAction,
	CopyImageUrlToClipboardAction,
	ImagePropertiesAction,
	GoBackAction,
	GoForwardAction,
	RewindBackAction,
	RewindForwardAction,
	StopAction,
	StopScheduledPageRefreshAction,
	ReloadAction,
	ReloadOrStopAction,
	ReloadFrameAction,
	ReloadImageAction,
	ReloadAndBypassCacheAction,
	ReloadTimeAction,
	CutAction,
	CopyAction,
	PasteAction,
	DeleteAction,
	SelectAllAction,
	ClearAllAction,
	SpellCheckAction,
	UndoAction,
	RedoAction,
	InspectPageAction,
	InspectElementAction,
	PrintAction,
	BookmarkAction,
	BookmarkLinkAction,
	CopyAddressAction,
	ViewSourceAction,
	ViewSourceFrameAction,
	ValidateAction,
	ContentBlockingAction,
	WebsitePreferencesAction,
	ZoomInAction,
	ZoomOutAction,
	ZoomOriginalAction,
	SearchAction,
	SearchMenuAction,
	OpenSelectionAsLinkAction,
	CreateSearchAction,
	FindAction,
	FindNextAction,
	FindPreviousAction,
	SaveMediaToDiskAction,
	CopyMediaUrlToClipboardAction,
	ToggleMediaControlsAction,
	ToggleMediaLoopAction,
	ToggleMediaPlayPauseAction,
	ToggleMediaMuteAction,
	ActivateAddressFieldAction,
	PasteAndGoAction
};

class ActionsManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void loadProfiles();
	static void registerAction(const QLatin1String &identifier, const QString &text, const QIcon &icon = QIcon(), WindowAction windowAction = NoAction);
	static void registerWindow(QWidget *window, QList<QAction*> actions);
	static void triggerAction(const QString &action);
	static void triggerAction(WindowAction action);
	static void setupLocalAction(QAction *localAction, const QLatin1String &globalAction, bool connectTrigger = false);
	static QAction* getAction(const QString &action);
	static QAction* getAction(WindowAction action);
	static QKeySequence getNativeShortcut(const QString &action);
	static QStringList getIdentifiers();

protected:
	explicit ActionsManager(QObject *parent = NULL);

	void timerEvent(QTimerEvent *event);
	static void setupWindowActions(QObject *window);

protected slots:
	void optionChanged(const QString &option);
	void removeWindow(QObject *object);
	void triggerMacro();

private:
	int m_reloadTimer;

	static ActionsManager *m_instance;
	static QHash<QAction*, QStringList> m_applicationMacros;
	static QHash<QObject*, QHash<QString, QAction*> > m_mainWindowActions;
	static QHash<QString, QAction*> m_applicationActions;
	static QHash<QString, QList<QKeySequence> > m_profileShortcuts;
	static QHash<QString, QKeySequence> m_nativeShortcuts;
	static QHash<WindowAction, QAction*> m_windowActions;
};

}

#endif
