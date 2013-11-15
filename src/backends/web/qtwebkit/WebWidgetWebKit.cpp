#include "WebWidgetWebKit.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/NetworkAccessManager.h"

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtGui/QClipboard>
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

WebWidgetWebKit::WebWidgetWebKit(QWidget *parent) : WebWidget(parent),
	m_webWidget(new QWebView(this)),
	m_isLoading(false)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_webWidget);
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);

	m_webWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	m_webWidget->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	m_webWidget->page()->setNetworkAccessManager(new NetworkAccessManager(this));

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
	connect(m_webWidget->page(), SIGNAL(linkClicked(QUrl)), this, SLOT(setUrl(QUrl)));
	connect(m_webWidget->page(), SIGNAL(loadStarted()), this, SLOT(loadStarted()));
	connect(m_webWidget->page(), SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
	connect(m_webWidget->page(), SIGNAL(statusBarMessage(QString)), this, SIGNAL(statusMessageChanged(QString)));
	connect(m_webWidget->page(), SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString,QString)));
}

void WebWidgetWebKit::print(QPrinter *printer)
{
	m_webWidget->print(printer);
}

WebWidget* WebWidgetWebKit::clone(QWidget *parent)
{
	WebWidget *widget = new WebWidgetWebKit(parent);
	widget->setPrivate(isPrivate());
	widget->setUrl(getUrl());
	widget->setZoom(getZoom());

///TODO clone history

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
			ActionsManager::setupLocalAction(actionObject, "OpenLinkInNewTab");

			actionObject->setEnabled(false);

			break;
		case OpenLinkInNewTabBackgroundAction:
			ActionsManager::setupLocalAction(actionObject, "OpenLinkInNewTabBackground");

			actionObject->setEnabled(false);

			break;
		case OpenLinkInNewWindowBackgroundAction:
			ActionsManager::setupLocalAction(actionObject, "OpenLinkInNewWindowBackground");

			actionObject->setEnabled(false);

			break;
		case OpenFrameInThisTabAction:
			ActionsManager::setupLocalAction(actionObject, "OpenFrameInThisTab");

			actionObject->setEnabled(false);

			break;
		case OpenFrameInNewTabBackgroundAction:
			ActionsManager::setupLocalAction(actionObject, "OpenFrameInNewTabBackground");

			actionObject->setEnabled(false);

			break;
		case CopyFrameLinkToClipboardAction:
			ActionsManager::setupLocalAction(actionObject, "CopyFrameLinkToClipboard");

			actionObject->setEnabled(false);

			break;
		case ViewSourceFrameAction:
			ActionsManager::setupLocalAction(actionObject, "ViewSourceFrame");

			actionObject->setEnabled(false);

			break;
		case ReloadFrameAction:
			ActionsManager::setupLocalAction(actionObject, "ReloadFrame");

			actionObject->setEnabled(false);

			break;
		case SaveLinkToDownloadsAction:
			ActionsManager::setupLocalAction(actionObject, "SaveLinkToDownloads");

			actionObject->setEnabled(false);

			break;
		case RewindBackAction:
			ActionsManager::setupLocalAction(actionObject, "RewindBack");

			actionObject->setEnabled(getAction(GoBackAction)->isEnabled());

			break;
		case RewindForwardAction:
			ActionsManager::setupLocalAction(actionObject, "RewindForward");

			actionObject->setEnabled(getAction(GoForwardAction)->isEnabled());

			break;
		case ReloadTimeAction:
			ActionsManager::setupLocalAction(actionObject, "ReloadTime");

			actionObject->setMenu(new QMenu(this));
			actionObject->setEnabled(false);

			break;
		case PrintAction:
			ActionsManager::setupLocalAction(actionObject, "Print");

			break;
		case BookmarkAction:
			ActionsManager::setupLocalAction(actionObject, "AddBookmark");

			actionObject->setEnabled(false);

			break;
		case BookmarkLinkAction:
			ActionsManager::setupLocalAction(actionObject, "BookmarkLink");

			actionObject->setEnabled(false);

			break;
		case CopyAddressAction:
			ActionsManager::setupLocalAction(actionObject, "CopyAddress");

			break;
		case ViewSourceAction:
			ActionsManager::setupLocalAction(actionObject, "ViewSource");

			actionObject->setEnabled(false);

			break;
		case ValidateAction:
			ActionsManager::setupLocalAction(actionObject, "Validate");

			actionObject->setMenu(new QMenu(this));
			actionObject->setEnabled(false);

			break;
		case ContentBlockingAction:
			ActionsManager::setupLocalAction(actionObject, "ContentBlocking");

			actionObject->setEnabled(false);

			break;
		case WebsitePreferencesAction:
			ActionsManager::setupLocalAction(actionObject, "WebsitePreferences");

			actionObject->setEnabled(false);

			break;
		case FullScreenAction:
			ActionsManager::setupLocalAction(actionObject, "FullScreen");

			actionObject->setEnabled(false);

			break;
		case ZoomInAction:
			ActionsManager::setupLocalAction(actionObject, "ZoomIn");

			break;
		case ZoomOutAction:
			ActionsManager::setupLocalAction(actionObject, "ZoomOut");

			break;
		case ZoomOriginalAction:
			ActionsManager::setupLocalAction(actionObject, "ZoomOriginal");

			break;
		case SearchAction:
			ActionsManager::setupLocalAction(actionObject, "Search");

			actionObject->setEnabled(false);

			break;
		case SearchMenuAction:
			ActionsManager::setupLocalAction(actionObject, "SearchMenu");

			actionObject->setMenu(new QMenu(this));
			actionObject->setEnabled(false);

			break;
		case OpenSelectionAsLinkAction:
			ActionsManager::setupLocalAction(actionObject, "OpenSelectionAsLink");

			actionObject->setEnabled(false);

			break;
		case ClearAllAction:
			ActionsManager::setupLocalAction(actionObject, "ClearAll");

			actionObject->setEnabled(false);

			break;
		case SpellCheckAction:
			ActionsManager::setupLocalAction(actionObject, "SpellCheck");

			actionObject->setEnabled(false);

			break;
		case ImagePropertiesAction:
			ActionsManager::setupLocalAction(actionObject, "ImageProperties");

			actionObject->setEnabled(false);

			break;
		case CreateSearchAction:
			ActionsManager::setupLocalAction(actionObject, "CreateSearch");

			actionObject->setEnabled(false);

			break;
		default:
			break;
	}

	m_customActions[action] = actionObject;

	return actionObject;
}

void WebWidgetWebKit::setDefaultTextEncoding(const QString &encoding)
{
	m_webWidget->settings()->setDefaultTextEncoding(encoding);
	m_webWidget->reload();
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

		if (localUrl.isLocalFile() && QFileInfo(localUrl.toLocalFile()).isDir())
		{
			QFile file(":/files/listing.html");
			file.open(QIODevice::ReadOnly | QIODevice::Text);

			QTextStream stream(&file);
			stream.setCodec("UTF-8");

			QDir directory(localUrl.toLocalFile());
			const QFileInfoList entries = directory.entryInfoList((QDir::AllEntries | QDir::Hidden), (QDir::Name | QDir::DirsFirst));
			QStringList navigation;

			do
			{
				navigation.prepend(QString("<a href=\"%1\">%2</a>%3").arg(directory.canonicalPath()).arg(directory.dirName().isEmpty() ? QString('/') : directory.dirName()).arg(directory.dirName().isEmpty() ? QString() : QString('/')));
			}
			while (directory.cdUp());

			QHash<QString, QString> variables;
			variables["title"] = QFileInfo(localUrl.toLocalFile()).canonicalFilePath();
			variables["description"] = tr("Directory Contents");
			variables["dir"] = (QGuiApplication::isLeftToRight() ? "ltr" : "rtl");
			variables["navigation"] = navigation.join(QString());
			variables["header_name"] = tr("Name");
			variables["header_type"] = tr("Type");
			variables["header_size"] = tr("Size");
			variables["header_date"] = tr("Date");
			variables["body"] = QString();

			QMimeDatabase database;

			for (int i = 0; i < entries.count(); ++i)
			{
				const QMimeType mimeType = database.mimeTypeForFile(entries.at(i).canonicalFilePath());
				QString size;

				if (!entries.at(i).isDir())
				{
					if (entries.at(i).size() > 1024)
					{
						if (entries.at(i).size() > 1048576)
						{
							if (entries.at(i).size() > 1073741824)
							{
								size = QString("%1 GB").arg((entries.at(i).size() / 1073741824.0), 0, 'f', 2);
							}
							else
							{
								size = QString("%1 MB").arg((entries.at(i).size() / 1048576.0), 0, 'f', 2);
							}
						}
						else
						{
							size = QString("%1 KB").arg((entries.at(i).size() / 1024.0), 0, 'f', 2);
						}
					}
					else
					{
						size = QString("%1 B").arg(entries.at(i).size());
					}
				}

				QByteArray byteArray;
				QBuffer buffer(&byteArray);
				QIcon::fromTheme(mimeType.iconName(), QIcon(entries.at(i).isDir() ? ":icons/inode-directory.png" : ":/icons/unknown.png")).pixmap(16, 16).save(&buffer, "PNG");

				variables["body"].append(QString("<tr>\n<td><a href=\"file://%1\"><img src=\"data:image/png;base64,%2\" alt=\"\"> %3</a></td>\n<td>%4</td>\n<td>%5</td>\n<td>%6</td>\n</tr>\n").arg(entries.at(i).filePath()).arg(QString(byteArray.toBase64())).arg(entries.at(i).fileName()).arg(mimeType.comment()).arg(size).arg(entries.at(i).lastModified().toString()));
			}

			QString html = stream.readAll();
			QHash<QString, QString>::iterator iterator;

			for (iterator = variables.begin(); iterator != variables.end(); ++iterator)
			{
				html.replace(QString("{%1}").arg(iterator.key()), iterator.value());
			}

			m_webWidget->setHtml(html, localUrl);
		}
		else
		{
			m_webWidget->setUrl(localUrl);
		}
	}
	else
	{
		m_webWidget->setUrl(url);
	}

	notifyTitleChanged();
	notifyIconChanged();
}

void WebWidgetWebKit::setPrivate(bool enabled)
{
	if (enabled != m_webWidget->settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled))
	{
		m_webWidget->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, enabled);

		notifyIconChanged();

		emit isPrivateChanged(enabled);
	}
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

	emit loadingChanged(true);
}

void WebWidgetWebKit::loadFinished(bool ok)
{
	Q_UNUSED(ok)

	m_isLoading = false;

	emit loadingChanged(false);
}

void WebWidgetWebKit::linkHovered(const QString &link, const QString &title)
{
	QString text;

	if (!link.isEmpty())
	{
		text = (title.isEmpty() ? tr("Address: %1").arg(link) : tr("Title: %1\nAddress: %2").arg(title).arg(link));
	}

	QToolTip::showText(QCursor::pos(), text, m_webWidget);

	emit statusMessageChanged(link, 0);
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

		return tr("Empty");
	}

	return title;
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
		default:
			break;
	}
}

}
