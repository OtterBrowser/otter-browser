#include "WebWidgetWebKit.h"
#include "WebPageWebKit.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/NetworkAccessManager.h"
#include "../../../core/SessionsManager.h"

#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtGui/QClipboard>
#include <QtNetwork/QNetworkDiskCache>
#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebElement>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebPage>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

WebWidgetWebKit::WebWidgetWebKit(bool privateWindow, QWidget *parent) : WebWidget(parent),
	m_webWidget(new WebViewWebKit(this)),
	m_networkAccessManager(NULL),
	m_isLinkHovered(false),
	m_isLoading(false)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_webWidget);
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);

	m_networkAccessManager = new NetworkAccessManager(this);

	if (!privateWindow)
	{
		QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
		diskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

		m_networkAccessManager->setCache(diskCache);
	}

	m_webWidget->installEventFilter(this);
	m_webWidget->setPage(new WebPageWebKit(m_webWidget));
	m_webWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	m_webWidget->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	m_webWidget->page()->setNetworkAccessManager(m_networkAccessManager);
	m_webWidget->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, privateWindow);

	ActionsManager::setupLocalAction(getAction(CutAction), "Cut");
	ActionsManager::setupLocalAction(getAction(CopyAction), "Copy");
	ActionsManager::setupLocalAction(getAction(PasteAction), "Paste");
	ActionsManager::setupLocalAction(getAction(DeleteAction), "Delete");
	ActionsManager::setupLocalAction(getAction(SelectAllAction), "SelectAll");
	ActionsManager::setupLocalAction(getAction(UndoAction), "Undo");
	ActionsManager::setupLocalAction(getAction(RedoAction), "Redo");
	ActionsManager::setupLocalAction(getAction(GoBackAction), "GoBack");
	ActionsManager::setupLocalAction(getAction(GoForwardAction), "GoForward");
	ActionsManager::setupLocalAction(getAction(ReloadAction), "Reload");
	ActionsManager::setupLocalAction(getAction(StopAction), "Stop");
	ActionsManager::setupLocalAction(getAction(OpenLinkInThisTabAction), "OpenLinkInThisTab");
	ActionsManager::setupLocalAction(getAction(OpenLinkInNewWindowAction), "OpenLinkInNewWindow");
	ActionsManager::setupLocalAction(getAction(OpenFrameInNewTabAction), "OpenFrameInNewTab");
	ActionsManager::setupLocalAction(getAction(SaveLinkToDiskAction), "SaveLinkToDisk");
	ActionsManager::setupLocalAction(getAction(CopyLinkToClipboardAction), "CopyLinkToClipboard");
	ActionsManager::setupLocalAction(getAction(OpenImageInNewTabAction), "OpenImageInNewTab");
	ActionsManager::setupLocalAction(getAction(SaveImageToDiskAction), "SaveImageToDisk");
	ActionsManager::setupLocalAction(getAction(CopyImageToClipboardAction), "CopyImageToClipboard");
	ActionsManager::setupLocalAction(getAction(CopyImageUrlToClipboardAction), "CopyImageUrlToClipboard");

	connect(m_webWidget, SIGNAL(titleChanged(const QString)), this, SLOT(notifyTitleChanged()));
	connect(m_webWidget, SIGNAL(urlChanged(const QUrl)), this, SLOT(notifyUrlChanged(const QUrl)));
	connect(m_webWidget, SIGNAL(iconChanged()), this, SLOT(notifyIconChanged()));
	connect(m_webWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
	connect(m_webWidget, SIGNAL(requestedZoomChange(int)), this, SLOT(setZoom(int)));
	connect(m_webWidget, SIGNAL(requestedTriggerAction(WebAction)), this, SLOT(triggerAction(WebAction)));
	connect(m_webWidget->page(), SIGNAL(microFocusChanged()), this, SIGNAL(actionsChanged()));
	connect(m_webWidget->page(), SIGNAL(selectionChanged()), this, SIGNAL(actionsChanged()));
	connect(m_webWidget->page(), SIGNAL(linkClicked(QUrl)), this, SLOT(setUrl(QUrl)));
	connect(m_webWidget->page(), SIGNAL(loadStarted()), this, SLOT(loadStarted()));
	connect(m_webWidget->page(), SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
	connect(m_webWidget->page(), SIGNAL(statusBarMessage(QString)), this, SIGNAL(statusMessageChanged(QString)));
	connect(m_webWidget->page(), SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString,QString)));
	connect(m_webWidget->page(), SIGNAL(restoreFrameStateRequested(QWebFrame*)), this, SLOT(restoreState(QWebFrame*)));
	connect(m_networkAccessManager, SIGNAL(statusChanged(int,int,qint64,qint64,qint64)), this, SIGNAL(loadStatusChanged(int,int,qint64,qint64,qint64)));
	connect(m_networkAccessManager, SIGNAL(documentLoadProgressChanged(int)), this, SIGNAL(loadProgress(int)));
}

void WebWidgetWebKit::print(QPrinter *printer)
{
	m_webWidget->print(printer);
}

void WebWidgetWebKit::loadStarted()
{
	m_isLoading = true;

	if (m_customActions.contains(RewindBackAction))
	{
		getAction(RewindBackAction)->setEnabled(getAction(GoBackAction)->isEnabled());
	}

	if (m_customActions.contains(RewindForwardAction))
	{
		getAction(RewindForwardAction)->setEnabled(getAction(GoForwardAction)->isEnabled());
	}

	if (m_customActions.contains(ReloadOrStopAction))
	{
		QAction *action = getAction(ReloadOrStopAction);

		ActionsManager::setupLocalAction(action, "Stop");

		action->setShortcut(QKeySequence());
	}

	if (!isPrivate())
	{
		SessionsManager::markSessionModified();
	}

	emit loadingChanged(true);
	emit statusMessageChanged(QString());
}

void WebWidgetWebKit::loadFinished(bool ok)
{
	m_isLoading = false;

	m_networkAccessManager->resetStatistics();

	if (m_customActions.contains(ReloadOrStopAction))
	{
		QAction *action = getAction(ReloadOrStopAction);

		ActionsManager::setupLocalAction(action, "Reload");

		action->setShortcut(QKeySequence());
	}

	if (ok && !isPrivate())
	{
		SessionsManager::markSessionModified();
	}

	emit loadingChanged(false);
}

void WebWidgetWebKit::linkHovered(const QString &link, const QString &title)
{
	QString text;

	if (!link.isEmpty())
	{
		text = (title.isEmpty() ? tr("Address: %1").arg(link) : tr("Title: %1\nAddress: %2").arg(title).arg(link));
	}

	m_isLinkHovered = !text.isEmpty();

	QToolTip::showText(QCursor::pos(), text, m_webWidget);

	emit statusMessageChanged(link, 0);
}

void WebWidgetWebKit::restoreState(QWebFrame *frame)
{
	if (frame == m_webWidget->page()->mainFrame())
	{
		setZoom(m_webWidget->history()->currentItem().userData().toHash().value("zoom", getZoom()).toInt());

		if (m_webWidget->page()->mainFrame()->scrollPosition() == QPoint(0, 0))
		{
			m_webWidget->page()->mainFrame()->setScrollPosition(m_webWidget->history()->currentItem().userData().toHash().value("position").toPoint());
		}
	}
}

void WebWidgetWebKit::notifyTitleChanged()
{
	emit titleChanged(getTitle());
}

void WebWidgetWebKit::notifyUrlChanged(const QUrl &url)
{
	if (m_customActions.contains(RewindBackAction))
	{
		getAction(RewindBackAction)->setEnabled(getAction(GoBackAction)->isEnabled());
	}

	if (m_customActions.contains(RewindForwardAction))
	{
		getAction(RewindForwardAction)->setEnabled(getAction(GoForwardAction)->isEnabled());
	}

	emit urlChanged(url);
}

void WebWidgetWebKit::notifyIconChanged()
{
	emit iconChanged(getIcon());
}

void WebWidgetWebKit::triggerAction(WebAction action, bool checked)
{
	const QWebPage::WebAction webAction = mapAction(action);

	if (webAction != QWebPage::NoWebAction)
	{
		m_webWidget->triggerPageAction(webAction, checked);

		return;
	}

	switch (action)
	{
		case RewindBackAction:
			m_webWidget->page()->history()->goToItem(m_webWidget->page()->history()->itemAt(0));

			break;
		case RewindForwardAction:
			m_webWidget->page()->history()->goToItem(m_webWidget->page()->history()->itemAt(m_webWidget->page()->history()->count() - 1));

			break;
		case CopyAddressAction:
			QApplication::clipboard()->setText(getUrl().toString());

			break;
		case ZoomInAction:
			setZoom(qMin((getZoom() + 10), 10000));

			break;
		case ZoomOutAction:
			setZoom(qMax((getZoom() - 10), 10));

			break;
		case ZoomOriginalAction:
			setZoom(100);

			break;
		case ReloadOrStopAction:
			if (isLoading())
			{
				triggerAction(StopAction);
			}
			else
			{
				triggerAction(ReloadAction);
			}

			break;
		default:
			break;
	}
}

void WebWidgetWebKit::setDefaultTextEncoding(const QString &encoding)
{
	m_webWidget->settings()->setDefaultTextEncoding(encoding);
	m_webWidget->reload();
}

void WebWidgetWebKit::setHistory(const HistoryInformation &history)
{
	Q_UNUSED(history)
///TODO
}

void WebWidgetWebKit::setZoom(int zoom)
{
	if (zoom != getZoom())
	{
		m_webWidget->setZoomFactor(qBound(0.1, ((qreal) zoom / 100), (qreal) 100));

		emit zoomChanged(zoom);
	}
}

void WebWidgetWebKit::setUrl(const QUrl &url)
{
	if (url.scheme() == "javascript")
	{
		evaluateJavaScript(url.path());

		return;
	}

	if (url.matches(getUrl(), (QUrl::RemoveFragment | QUrl::StripTrailingSlash | QUrl::NormalizePathSegments)))
	{
		m_webWidget->page()->mainFrame()->scrollToAnchor(url.fragment());

		return;
	}

	QVariantHash data;
	data["position"] = m_webWidget->page()->mainFrame()->scrollPosition();
	data["zoom"] = getZoom();

	m_webWidget->page()->history()->currentItem().setUserData(data);

	if (url.isValid() && url.scheme().isEmpty() && !url.path().startsWith('/'))
	{
		QUrl httpUrl = url;
		httpUrl.setScheme("http");

		m_webWidget->setUrl(httpUrl);
	}
	else if (url.isValid() && (url.scheme().isEmpty() || url.scheme() == "file"))
	{
		QUrl localUrl = url;
		localUrl.setScheme("file");

		m_webWidget->setUrl(localUrl);
	}
	else
	{
		m_webWidget->setUrl(url);
	}

	notifyTitleChanged();
	notifyIconChanged();
}

void WebWidgetWebKit::showMenu(const QPoint &position)
{
	const QWebHitTestResult result = m_webWidget->page()->frameAt(position)->hitTestContent(position);
	MenuFlags flags = NoMenu;

	if (result.element().tagName().toLower() == "textarea" || (result.element().tagName().toLower() == "input" && (result.element().attribute("type").isEmpty() || result.element().attribute("type").toLower() == "text")))
	{
		flags |= FormMenu;
	}

	if (result.pixmap().isNull() && result.isContentSelected() && !m_webWidget->selectedText().isEmpty())
	{
		flags |= SelectionMenu;
	}

	if (result.linkUrl().isValid())
	{
		flags |= LinkMenu;
	}

	if (!result.pixmap().isNull())
	{
		flags |= ImageMenu;
	}

	if (result.isContentEditable())
	{
		flags |= EditMenu;
	}

	if (flags == NoMenu)
	{
		flags = StandardMenu;

		if (result.frame() != m_webWidget->page()->mainFrame())
		{
			flags |= FrameMenu;
		}
	}

	WebWidget::showMenu(position, flags);
}

WebWidget* WebWidgetWebKit::clone(QWidget *parent)
{
	WebWidget *widget = new WebWidgetWebKit(isPrivate(), parent);
	widget->setDefaultTextEncoding(getDefaultTextEncoding());
	widget->setUrl(getUrl());
	widget->setZoom(getZoom());
	widget->setHistory(getHistory());

	return widget;
}

QAction *WebWidgetWebKit::getAction(WebAction action)
{
	const QWebPage::WebAction webAction = mapAction(action);

	if (webAction != QWebPage::NoWebAction)
	{
		return m_webWidget->page()->action(webAction);
	}

	if (action == NoAction)
	{
		return NULL;
	}

	if (m_customActions.contains(action))
	{
		return m_customActions[action];
	}

	QAction *actionObject = new QAction(this);
	actionObject->setData(action);

	connect(actionObject, SIGNAL(triggered()), this, SLOT(triggerAction()));

	switch (action)
	{
		case OpenLinkInNewTabAction:
			ActionsManager::setupLocalAction(actionObject, "OpenLinkInNewTab", true);

			actionObject->setEnabled(false);

			break;
		case OpenLinkInNewTabBackgroundAction:
			ActionsManager::setupLocalAction(actionObject, "OpenLinkInNewTabBackground", true);

			actionObject->setEnabled(false);

			break;
		case OpenLinkInNewWindowBackgroundAction:
			ActionsManager::setupLocalAction(actionObject, "OpenLinkInNewWindowBackground", true);

			actionObject->setEnabled(false);

			break;
		case OpenFrameInThisTabAction:
			ActionsManager::setupLocalAction(actionObject, "OpenFrameInThisTab", true);

			actionObject->setEnabled(false);

			break;
		case OpenFrameInNewTabBackgroundAction:
			ActionsManager::setupLocalAction(actionObject, "OpenFrameInNewTabBackground", true);

			actionObject->setEnabled(false);

			break;
		case CopyFrameLinkToClipboardAction:
			ActionsManager::setupLocalAction(actionObject, "CopyFrameLinkToClipboard", true);

			actionObject->setEnabled(false);

			break;
		case ViewSourceFrameAction:
			ActionsManager::setupLocalAction(actionObject, "ViewSourceFrame", true);

			actionObject->setEnabled(false);

			break;
		case ReloadFrameAction:
			ActionsManager::setupLocalAction(actionObject, "ReloadFrame", true);

			actionObject->setEnabled(false);

			break;
		case SaveLinkToDownloadsAction:
			ActionsManager::setupLocalAction(actionObject, "SaveLinkToDownloads");

			actionObject->setEnabled(false);

			break;
		case RewindBackAction:
			ActionsManager::setupLocalAction(actionObject, "RewindBack", true);

			actionObject->setEnabled(getAction(GoBackAction)->isEnabled());

			break;
		case RewindForwardAction:
			ActionsManager::setupLocalAction(actionObject, "RewindForward", true);

			actionObject->setEnabled(getAction(GoForwardAction)->isEnabled());

			break;
		case ReloadTimeAction:
			ActionsManager::setupLocalAction(actionObject, "ReloadTime", true);

			actionObject->setMenu(new QMenu(this));
			actionObject->setEnabled(false);

			break;
		case PrintAction:
			ActionsManager::setupLocalAction(actionObject, "Print", true);

			break;
		case BookmarkAction:
			ActionsManager::setupLocalAction(actionObject, "AddBookmark", true);

			actionObject->setEnabled(false);

			break;
		case BookmarkLinkAction:
			ActionsManager::setupLocalAction(actionObject, "BookmarkLink", true);

			actionObject->setEnabled(false);

			break;
		case CopyAddressAction:
			ActionsManager::setupLocalAction(actionObject, "CopyAddress", true);

			break;
		case ViewSourceAction:
			ActionsManager::setupLocalAction(actionObject, "ViewSource", true);

			actionObject->setEnabled(false);

			break;
		case ValidateAction:
			ActionsManager::setupLocalAction(actionObject, "Validate", true);

			actionObject->setMenu(new QMenu(this));
			actionObject->setEnabled(false);

			break;
		case ContentBlockingAction:
			ActionsManager::setupLocalAction(actionObject, "ContentBlocking", true);

			actionObject->setEnabled(false);

			break;
		case WebsitePreferencesAction:
			ActionsManager::setupLocalAction(actionObject, "WebsitePreferences", true);

			actionObject->setEnabled(false);

			break;
		case FullScreenAction:
			ActionsManager::setupLocalAction(actionObject, "FullScreen", true);

			actionObject->setEnabled(false);

			break;
		case ZoomInAction:
			ActionsManager::setupLocalAction(actionObject, "ZoomIn");

			break;
		case ZoomOutAction:
			ActionsManager::setupLocalAction(actionObject, "ZoomOut", true);

			break;
		case ZoomOriginalAction:
			ActionsManager::setupLocalAction(actionObject, "ZoomOriginal", true);

			break;
		case SearchAction:
			ActionsManager::setupLocalAction(actionObject, "Search", true);

			actionObject->setEnabled(false);

			break;
		case SearchMenuAction:
			ActionsManager::setupLocalAction(actionObject, "SearchMenu", true);

			actionObject->setMenu(new QMenu(this));
			actionObject->setEnabled(false);

			break;
		case OpenSelectionAsLinkAction:
			ActionsManager::setupLocalAction(actionObject, "OpenSelectionAsLink", true);

			actionObject->setEnabled(false);

			break;
		case ClearAllAction:
			ActionsManager::setupLocalAction(actionObject, "ClearAll", true);

			actionObject->setEnabled(false);

			break;
		case SpellCheckAction:
			ActionsManager::setupLocalAction(actionObject, "SpellCheck", true);

			actionObject->setEnabled(false);

			break;
		case ImagePropertiesAction:
			ActionsManager::setupLocalAction(actionObject, "ImageProperties", true);

			actionObject->setEnabled(false);

			break;
		case CreateSearchAction:
			ActionsManager::setupLocalAction(actionObject, "CreateSearch", true);

			actionObject->setEnabled(false);

			break;
		case ReloadOrStopAction:
			ActionsManager::setupLocalAction(actionObject, "Reload");

			actionObject->setShortcut(QKeySequence());

			break;
		default:
			break;
	}

	m_customActions[action] = actionObject;

	return actionObject;
}

QUndoStack *WebWidgetWebKit::getUndoStack()
{
	return m_webWidget->page()->undoStack();
}

QString WebWidgetWebKit::getDefaultTextEncoding() const
{
	return m_webWidget->settings()->defaultTextEncoding();
}

QString WebWidgetWebKit::getTitle() const
{
	const QString title = m_webWidget->title();

	if (title.isEmpty())
	{
		const QUrl url = getUrl();

		if (url.scheme() == "about" && (url.path().isEmpty() || url.path() == "blank"))
		{
			return tr("New Tab");
		}

		if (url.isLocalFile())
		{
			return QFileInfo(url.toLocalFile()).canonicalFilePath();
		}

		return tr("(Untitled)");
	}

	return title;
}

QVariant WebWidgetWebKit::evaluateJavaScript(const QString &script)
{
	return m_webWidget->page()->mainFrame()->evaluateJavaScript(script);
}

QUrl WebWidgetWebKit::getUrl() const
{
	return m_webWidget->url();
}

QIcon WebWidgetWebKit::getIcon() const
{
	if (isPrivate())
	{
		return QIcon(":/icons/tab-private.png");
	}

	const QIcon icon = m_webWidget->icon();

	return (icon.isNull() ? QIcon(":/icons/tab.png") : icon);
}

HistoryInformation WebWidgetWebKit::getHistory() const
{
	QVariantHash data;
	data["position"] = m_webWidget->page()->mainFrame()->scrollPosition();
	data["zoom"] = getZoom();

	m_webWidget->history()->currentItem().setUserData(data);

	QWebHistory *history = m_webWidget->history();
	HistoryInformation information;
	information.index = history->currentItemIndex();

	for (int i = 0; i < history->count(); ++i)
	{
		const QWebHistoryItem item = history->itemAt(i);
		HistoryEntry entry;
		entry.url = item.url().toString();
		entry.title = item.title();
		entry.position = item.userData().toHash().value("position", QPoint(0, 0)).toPoint();
		entry.zoom = item.userData().toHash().value("zoom").toInt();

		information.entries.append(entry);
	}

	return information;
}

QWebPage::WebAction WebWidgetWebKit::mapAction(WebAction action) const
{
	switch (action)
	{
		case OpenLinkAction:
			return QWebPage::OpenLink;
		case OpenLinkInNewWindowAction:
			return QWebPage::OpenLinkInNewWindow;
		case OpenLinkInThisTabAction:
			return QWebPage::OpenLinkInThisWindow;
		case OpenFrameInNewTabAction:
			return QWebPage::OpenFrameInNewWindow;
		case SaveLinkToDiskAction:
			return QWebPage::DownloadLinkToDisk;
		case CopyLinkToClipboardAction:
			return QWebPage::CopyLinkToClipboard;
		case OpenImageInNewTabAction:
			return QWebPage::OpenImageInNewWindow;
		case SaveImageToDiskAction:
			return QWebPage::DownloadImageToDisk;
		case CopyImageToClipboardAction:
			return QWebPage::CopyImageToClipboard;
		case CopyImageUrlToClipboardAction:
			return QWebPage::CopyImageUrlToClipboard;
		case GoBackAction:
			return QWebPage::Back;
		case GoForwardAction:
			return QWebPage::Forward;
		case StopAction:
			return QWebPage::Stop;
		case StopScheduledPageRefreshAction:
			return QWebPage::StopScheduledPageRefresh;
		case ReloadAction:
			return QWebPage::Reload;
		case ReloadAndBypassCacheAction:
			return QWebPage::ReloadAndBypassCache;
		case CutAction:
			return QWebPage::Cut;
		case CopyAction:
			return QWebPage::Copy;
		case PasteAction:
			return QWebPage::Paste;
		case DeleteAction:
			return QWebPage::DeleteEndOfWord;
		case SelectAllAction:
			return QWebPage::SelectAll;
		case UndoAction:
			return QWebPage::Undo;
		case RedoAction:
			return QWebPage::Redo;
		case InspectElementAction:
			return QWebPage::InspectElement;
		default:
			return QWebPage::NoWebAction;
	}

	return QWebPage::NoWebAction;
}

void WebWidgetWebKit::triggerAction()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		triggerAction(static_cast<WebAction>(action->data().toInt()));
	}
}

int WebWidgetWebKit::getZoom() const
{
	return (m_webWidget->zoomFactor() * 100);
}

bool WebWidgetWebKit::isLoading() const
{
	return m_isLoading;
}

bool WebWidgetWebKit::isPrivate() const
{
	return m_webWidget->settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled);
}

bool WebWidgetWebKit::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_webWidget && event->type() == QEvent::ToolTip && m_isLinkHovered)
	{
		return true;
	}

	return QObject::eventFilter(object, event);
}

}
