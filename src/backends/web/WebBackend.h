#ifndef OTTER_WEBBACKEND_H
#define OTTER_WEBBACKEND_H

#include <QtCore/QObject>

namespace Otter
{

enum WebAction
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
	ReloadFrameAction,
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
	FullScreenAction,
	ZoomInAction,
	ZoomOutAction,
	ZoomOriginalAction,
	SearchAction,
	SearchMenuAction,
	OpenSelectionAsLinkAction,
	CreateSearchAction
};

class WebWidget;

class WebBackend : public QObject
{
	Q_OBJECT

public:
	explicit WebBackend(QObject *parent = NULL);

	virtual WebWidget* createWidget(QWidget *parent = NULL);
	virtual QString getTitle() const;
	virtual QString getDescription() const;
};

}

#endif
