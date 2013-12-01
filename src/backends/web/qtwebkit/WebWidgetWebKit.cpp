#include "WebWidgetWebKit.h"
#include "WebPageWebKit.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/NetworkAccessManager.h"
#include "../../../core/SessionsManager.h"

#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
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

WebWidgetWebKit::WebWidgetWebKit(bool privateWindow, QWidget *parent, WebPageWebKit *page) : WebWidget(parent),
	m_webView(new QWebView(this)),
	m_networkAccessManager(NULL),
	m_isLinkHovered(false),
	m_isLoading(false)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_webView);
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);

	m_networkAccessManager = new NetworkAccessManager(privateWindow, this);

	if (!privateWindow)
	{
		QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
		diskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

		m_networkAccessManager->setCache(diskCache);
	}

	if (page)
	{
		page->setParent(m_webView);
	}
	else
	{
		page = new WebPageWebKit(m_webView);
	}

	page->setNetworkAccessManager(m_networkAccessManager);

	m_webView->installEventFilter(this);
	m_webView->setPage(page);
	m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_webView->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, privateWindow);

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

	getAction(OpenLinkInThisTabAction)->setIcon(QIcon::fromTheme("document-open", QIcon(":/icons/document-open.png")));

	connect(page, SIGNAL(requestedNewWindow(WebWidget*)), this, SIGNAL(requestedNewWindow(WebWidget*)));
	connect(page, SIGNAL(microFocusChanged()), this, SIGNAL(actionsChanged()));
	connect(page, SIGNAL(selectionChanged()), this, SIGNAL(actionsChanged()));
	connect(page, SIGNAL(loadStarted()), this, SLOT(loadStarted()));
	connect(page, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
	connect(page, SIGNAL(statusBarMessage(QString)), this, SIGNAL(statusMessageChanged(QString)));
	connect(page, SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString,QString)));
	connect(page, SIGNAL(saveFrameStateRequested(QWebFrame*,QWebHistoryItem*)), this, SLOT(saveState(QWebFrame*,QWebHistoryItem*)));
	connect(page, SIGNAL(restoreFrameStateRequested(QWebFrame*)), this, SLOT(restoreState(QWebFrame*)));
	connect(m_webView, SIGNAL(titleChanged(const QString)), this, SLOT(notifyTitleChanged()));
	connect(m_webView, SIGNAL(urlChanged(const QUrl)), this, SLOT(notifyUrlChanged(const QUrl)));
	connect(m_webView, SIGNAL(iconChanged()), this, SLOT(notifyIconChanged()));
	connect(m_webView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
	connect(m_networkAccessManager, SIGNAL(statusChanged(int,int,qint64,qint64,qint64)), this, SIGNAL(loadStatusChanged(int,int,qint64,qint64,qint64)));
	connect(m_networkAccessManager, SIGNAL(documentLoadProgressChanged(int)), this, SIGNAL(loadProgress(int)));
}

void WebWidgetWebKit::print(QPrinter *printer)
{
	m_webView->print(printer);
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

	QToolTip::showText(QCursor::pos(), text, m_webView);

	emit statusMessageChanged(link, 0);
}

void WebWidgetWebKit::saveState(QWebFrame *frame, QWebHistoryItem *item)
{
	if (frame == m_webView->page()->mainFrame())
	{
		QVariantHash data;
		data["position"] = m_webView->page()->mainFrame()->scrollPosition();
		data["zoom"] = getZoom();

		item->setUserData(data);
	}
}

void WebWidgetWebKit::restoreState(QWebFrame *frame)
{
	if (frame == m_webView->page()->mainFrame())
	{
		setZoom(m_webView->history()->currentItem().userData().toHash().value("zoom", getZoom()).toInt());

		if (m_webView->page()->mainFrame()->scrollPosition() == QPoint(0, 0))
		{
			m_webView->page()->mainFrame()->setScrollPosition(m_webView->history()->currentItem().userData().toHash().value("position").toPoint());
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

void WebWidgetWebKit::triggerAction(WindowAction action, bool checked)
{
	const QWebPage::WebAction webAction = mapAction(action);

	if (webAction != QWebPage::NoWebAction)
	{
		m_webView->triggerPageAction(webAction, checked);

		return;
	}

	switch (action)
	{
		case RewindBackAction:
			m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(0));

			break;
		case RewindForwardAction:
			m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(m_webView->page()->history()->count() - 1));

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
		case OpenLinkInNewTabAction:
			if (m_hitResult.linkUrl().isValid())
			{
				emit requestedOpenUrl(m_hitResult.linkUrl(), false, false);
			}

			break;
		case OpenLinkInNewTabBackgroundAction:
			if (m_hitResult.linkUrl().isValid())
			{
				emit requestedOpenUrl(m_hitResult.linkUrl(), true, false);
			}

			break;
		case OpenLinkInNewWindowAction:
			if (m_hitResult.linkUrl().isValid())
			{
				emit requestedOpenUrl(m_hitResult.linkUrl(), false, true);
			}

			break;
		case OpenLinkInNewWindowBackgroundAction:
			if (m_hitResult.linkUrl().isValid())
			{
				emit requestedOpenUrl(m_hitResult.linkUrl(), true, true);
			}

			break;
		case BookmarkLinkAction:
			if (m_hitResult.linkUrl().isValid())
			{
				emit requestedAddBookmark(m_hitResult.linkUrl());
			}

			break;
		case OpenSelectionAsLinkAction:
			setUrl(m_webView->selectedText());

			break;
		default:
			break;
	}
}

void WebWidgetWebKit::setDefaultTextEncoding(const QString &encoding)
{
	m_webView->settings()->setDefaultTextEncoding(encoding);
	m_webView->reload();
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
		m_webView->setZoomFactor(qBound(0.1, ((qreal) zoom / 100), (qreal) 100));

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

	if (!url.fragment().isEmpty() && url.matches(getUrl(), (QUrl::RemoveFragment | QUrl::StripTrailingSlash | QUrl::NormalizePathSegments)))
	{
		m_webView->page()->mainFrame()->scrollToAnchor(url.fragment());

		return;
	}

	if (url.isValid() && url.scheme().isEmpty() && !url.path().startsWith('/'))
	{
		QUrl httpUrl = url;
		httpUrl.setScheme("http");

		m_webView->setUrl(httpUrl);
	}
	else if (url.isValid() && (url.scheme().isEmpty() || url.scheme() == "file"))
	{
		QUrl localUrl = url;
		localUrl.setScheme("file");

		m_webView->setUrl(localUrl);
	}
	else
	{
		m_webView->setUrl(url);
	}

	notifyTitleChanged();
	notifyIconChanged();
}

void WebWidgetWebKit::showMenu(const QPoint &position)
{
	m_hitResult = m_webView->page()->frameAt(position)->hitTestContent(position);
	MenuFlags flags = NoMenu;

	if (m_hitResult.element().tagName().toLower() == "textarea" || (m_hitResult.element().tagName().toLower() == "input" && (m_hitResult.element().attribute("type").isEmpty() || m_hitResult.element().attribute("type").toLower() == "text")))
	{
		flags |= FormMenu;
	}

	if (m_hitResult.pixmap().isNull() && m_hitResult.isContentSelected() && !m_webView->selectedText().isEmpty())
	{
		flags |= SelectionMenu;
	}

	if (m_hitResult.linkUrl().isValid())
	{
		flags |= LinkMenu;
	}

	if (!m_hitResult.pixmap().isNull())
	{
		flags |= ImageMenu;
	}

	if (m_hitResult.isContentEditable())
	{
		flags |= EditMenu;
	}

	if (flags == NoMenu)
	{
		flags = StandardMenu;

		if (m_hitResult.frame() != m_webView->page()->mainFrame())
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

QAction *WebWidgetWebKit::getAction(WindowAction action)
{
	const QWebPage::WebAction webAction = mapAction(action);

	if (webAction != QWebPage::NoWebAction)
	{
		return m_webView->page()->action(webAction);
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

			break;
		case OpenLinkInNewTabBackgroundAction:
			ActionsManager::setupLocalAction(actionObject, "OpenLinkInNewTabBackground", true);

			break;
		case OpenLinkInNewWindowAction:
			ActionsManager::setupLocalAction(actionObject, "OpenLinkInNewWindow", true);

			break;
		case OpenLinkInNewWindowBackgroundAction:
			ActionsManager::setupLocalAction(actionObject, "OpenLinkInNewWindowBackground", true);

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

			break;
		case BookmarkLinkAction:
			ActionsManager::setupLocalAction(actionObject, "BookmarkLink", true);

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
	return m_webView->page()->undoStack();
}

QString WebWidgetWebKit::getDefaultTextEncoding() const
{
	return m_webView->settings()->defaultTextEncoding();
}

QString WebWidgetWebKit::getTitle() const
{
	const QString title = m_webView->title();

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
	return m_webView->page()->mainFrame()->evaluateJavaScript(script);
}

QUrl WebWidgetWebKit::getUrl() const
{
	return m_webView->url();
}

QIcon WebWidgetWebKit::getIcon() const
{
	if (isPrivate())
	{
		return QIcon(":/icons/tab-private.png");
	}

	const QIcon icon = m_webView->icon();

	return (icon.isNull() ? QIcon(":/icons/tab.png") : icon);
}

HistoryInformation WebWidgetWebKit::getHistory() const
{
	QVariantHash data;
	data["position"] = m_webView->page()->mainFrame()->scrollPosition();
	data["zoom"] = getZoom();

	m_webView->history()->currentItem().setUserData(data);

	QWebHistory *history = m_webView->history();
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

QWebPage::WebAction WebWidgetWebKit::mapAction(WindowAction action) const
{
	switch (action)
	{
		case OpenLinkAction:
			return QWebPage::OpenLink;
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
		triggerAction(static_cast<WindowAction>(action->data().toInt()));
	}
}

int WebWidgetWebKit::getZoom() const
{
	return (m_webView->zoomFactor() * 100);
}

bool WebWidgetWebKit::isLoading() const
{
	return m_isLoading;
}

bool WebWidgetWebKit::isPrivate() const
{
	return m_webView->settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled);
}

bool WebWidgetWebKit::find(const QString &text, FindFlags flags)
{
	QWebPage::FindFlags nativeFlags = (QWebPage::FindWrapsAroundDocument | QWebPage::FindBeginsInSelection);

	if (flags & BackwardFind)
	{
		nativeFlags |= QWebPage::FindBackward;
	}

	if (flags & CaseSensitiveFind)
	{
		nativeFlags |= QWebPage::FindCaseSensitively;
	}

	if (flags & HighlightAllFind)
	{
		nativeFlags |= QWebPage::HighlightAllOccurrences;
	}

	return m_webView->findText(text, nativeFlags);
}

bool WebWidgetWebKit::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_webView)
	{
		if (event->type() == QEvent::ToolTip && m_isLinkHovered)
		{
			event->accept();

			return true;
		}
		else if (event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			if (mouseEvent->button() == Qt::MiddleButton)
			{
				QWebHitTestResult result = m_webView->page()->mainFrame()->hitTestContent(mouseEvent->pos());

				if (result.linkUrl().isValid())
				{
					emit requestedOpenUrl(result.linkUrl(), true, false);

					event->accept();

					return true;
				}
			}

			if (mouseEvent->button() == Qt::BackButton)
			{
				triggerAction(GoBackAction);

				event->accept();

				return true;
			}

			if (mouseEvent->button() == Qt::ForwardButton)
			{
				triggerAction(GoForwardAction);

				event->accept();

				return true;
			}
		}
		else if (event->type() == QEvent::Wheel)
		{
			QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

			if (wheelEvent->modifiers() & Qt::CTRL || wheelEvent->buttons() & Qt::LeftButton)
			{
				setZoom(getZoom() + (wheelEvent->delta() / 16));

				event->accept();

				return true;
			}

		}
	}

	return QObject::eventFilter(object, event);
}

}
