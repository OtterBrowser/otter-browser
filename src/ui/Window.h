#ifndef OTTER_WINDOW_H
#define OTTER_WINDOW_H

#include "../core/SessionsManager.h"

#include <QtCore/QUrl>
#include <QtGui/QIcon>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QWidget>
#include <QtWidgets/QUndoStack>

namespace Otter
{

namespace Ui
{
	class Window;
}

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
	CreateSearchAction,
	FindAction,
	FindNextAction,
	FindPreviousAction
};

class ProgressBarWidget;
class WebWidget;

class Window : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QString title READ getTitle NOTIFY titleChanged)
	Q_PROPERTY(QUrl url READ getUrl WRITE setUrl NOTIFY urlChanged)
	Q_PROPERTY(QIcon icon READ getIcon NOTIFY iconChanged)
	Q_PROPERTY(int zoom READ getZoom WRITE setZoom NOTIFY zoomChanged)
	Q_PROPERTY(bool isClonable READ isClonable)
	Q_PROPERTY(bool isEmpty READ isEmpty)
	Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
	Q_PROPERTY(bool isPinned READ isPinned WRITE setPinned NOTIFY isPinnedChanged)
	Q_PROPERTY(bool isPrivate READ isPrivate)

public:
	explicit Window(WebWidget *widget, QWidget *parent = NULL);
	~Window();

	virtual void print(QPrinter *printer);
	virtual Window* clone(QWidget *parent = NULL);
	virtual QAction* getAction(WindowAction action);
	virtual QUndoStack* getUndoStack();
	virtual QString getDefaultTextEncoding() const;
	virtual QString getTitle() const;
	virtual QUrl getUrl() const;
	virtual QIcon getIcon() const;
	virtual HistoryInformation getHistory() const;
	virtual int getZoom() const;
	virtual bool isClonable() const;
	virtual bool isEmpty() const;
	virtual bool isLoading() const;
	virtual bool isPinned() const;
	virtual bool isPrivate() const;

public slots:
	virtual void triggerAction(WindowAction action, bool checked = false);
	virtual void setDefaultTextEncoding(const QString &encoding);
	virtual void setHistory(const HistoryInformation &history);
	virtual void setZoom(int zoom);
	virtual void setUrl(const QUrl &url);
	virtual void setPinned(bool pinned);

protected:
	void changeEvent(QEvent *event);
	void resizeEvent(QResizeEvent *event);

protected slots:
	void loadUrl();
	void notifyRequestedOpenUrl(const QUrl &url, bool background, bool newWindow);
	void updateUrl(const QUrl &url);
	void updateFind(bool backwards = false);
	void updateFindHighlight();
	void updateProgressBarWidget();
	void setLoading(bool loading);

private:
	WebWidget *m_webWidget;
	ProgressBarWidget *m_progressBarWidget;
	bool m_isPinned;
	Ui::Window *m_ui;

signals:
	void requestedOpenUrl(QUrl url, bool privateWindow, bool background, bool newWindow);
	void requestedAddBookmark(QUrl url);
	void actionsChanged();
	void statusMessageChanged(const QString &message, int timeout);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void loadingChanged(bool loading);
	void zoomChanged(int zoom);
	void isPinnedChanged(bool pinned);
};

}

#endif
