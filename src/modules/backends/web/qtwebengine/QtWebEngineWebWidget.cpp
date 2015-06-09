/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QtWebEngineWebWidget.h"
#include "QtWebEnginePage.h"
#include "../../../../core/BookmarksManager.h"
#include "../../../../core/Console.h"
#include "../../../../core/GesturesManager.h"
#include "../../../../core/HistoryManager.h"
#include "../../../../core/NetworkManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/NotesManager.h"
#include "../../../../core/SearchesManager.h"
#include "../../../../core/Transfer.h"
#include "../../../../core/TransfersManager.h"
#include "../../../../core/Utils.h"
#include "../../../../core/WebBackend.h"
#include "../../../../ui/AuthenticationDialog.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/ContentsWidget.h"
#include "../../../../ui/ImagePropertiesDialog.h"
#include "../../../../ui/SearchPropertiesDialog.h"
#include "../../../../ui/WebsitePreferencesDialog.h"

#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QImageWriter>
#include <QtWebEngineWidgets/QWebEngineHistory>
#include <QtWebEngineWidgets/QWebEngineProfile>
#include <QtWebEngineWidgets/QWebEngineSettings>
#include <QtWidgets/QApplication>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QVBoxLayout>

template<typename Arg, typename R, typename C>

struct InvokeWrapper
{
	R *receiver;

	void (C::*memberFunction)(Arg);

	void operator()(Arg result)
	{
		(receiver->*memberFunction)(result);
	}
};

template<typename Arg, typename R, typename C>

InvokeWrapper<Arg, R, C> invoke(R *receiver, void (C::*memberFunction)(Arg))
{
	InvokeWrapper<Arg, R, C> wrapper = {receiver, memberFunction};

	return wrapper;
}

namespace Otter
{

QtWebEngineWebWidget::QtWebEngineWebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent) : WebWidget(isPrivate, backend, parent),
	m_webView(new QWebEngineView(this)),
	m_childWidget(NULL),
	m_iconReply(NULL),
	m_ignoreContextMenu(false),
	m_ignoreContextMenuNextTime(false),
	m_isUsingRockerNavigation(false),
	m_isLoading(false),
	m_isTyped(false)
{
	m_webView->setPage(new QtWebEnginePage(isPrivate, this));
	m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_webView->installEventFilter(this);

	const QList<QWidget*> children = m_webView->findChildren<QWidget*>();

	for (int i = 0; i < children.count(); ++i)
	{
		children.at(i)->installEventFilter(this);

		m_childWidget = children.at(i);
	}

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_webView);
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);
	setFocusPolicy(Qt::StrongFocus);

	connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(updateBookmarkActions()));
	connect(m_webView, SIGNAL(titleChanged(QString)), this, SLOT(notifyTitleChanged()));
	connect(m_webView, SIGNAL(urlChanged(QUrl)), this, SLOT(notifyUrlChanged(QUrl)));
	connect(m_webView, SIGNAL(iconUrlChanged(QUrl)), this, SLOT(notifyIconChanged()));
	connect(m_webView->page(), SIGNAL(loadProgress(int)), this, SIGNAL(loadProgress(int)));
	connect(m_webView->page(), SIGNAL(loadStarted()), this, SLOT(pageLoadStarted()));
	connect(m_webView->page(), SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
	connect(m_webView->page(), SIGNAL(linkHovered(QString)), this, SLOT(linkHovered(QString)));
	connect(m_webView->page(), SIGNAL(iconUrlChanged(QUrl)), this, SLOT(handleIconChange(QUrl)));
	connect(m_webView->page(), SIGNAL(authenticationRequired(QUrl,QAuthenticator*)), this, SLOT(handleAuthenticationRequired(QUrl,QAuthenticator*)));
	connect(m_webView->page(), SIGNAL(proxyAuthenticationRequired(QUrl,QAuthenticator*,QString)), this, SLOT(handleProxyAuthenticationRequired(QUrl,QAuthenticator*,QString)));
	connect(m_webView->page(), SIGNAL(windowCloseRequested()), this, SLOT(handleWindowCloseRequest()));
	connect(m_webView->page(), SIGNAL(featurePermissionRequested(QUrl,QWebEnginePage::Feature)), this, SLOT(handlePermissionRequest(QUrl,QWebEnginePage::Feature)));
	connect(m_webView->page(), SIGNAL(featurePermissionRequestCanceled(QUrl,QWebEnginePage::Feature)), this, SLOT(handlePermissionCancel(QUrl,QWebEnginePage::Feature)));
	connect(m_webView->page()->profile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)), this, SLOT(downloadFile(QWebEngineDownloadItem*)));
}

void QtWebEngineWebWidget::focusInEvent(QFocusEvent *event)
{
	WebWidget::focusInEvent(event);

	m_webView->setFocus();
}

void QtWebEngineWebWidget::mousePressEvent(QMouseEvent *event)
{
	WebWidget::mousePressEvent(event);

	if (getScrollMode() == MoveScroll)
	{
		triggerAction(ActionsManager::EndScrollAction);

		m_ignoreContextMenuNextTime = true;
	}
}

void QtWebEngineWebWidget::search(const QString &query, const QString &engine)
{
	QNetworkRequest request;
	QNetworkAccessManager::Operation method;
	QByteArray body;

	if (SearchesManager::setupSearchQuery(query, engine, &request, &method, &body))
	{
		setRequestedUrl(request.url(), false, true);
		updateOptions(request.url());

		if (method == QNetworkAccessManager::PostOperation)
		{
			QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/sendPost.js"));
			file.open(QIODevice::ReadOnly);

			m_webView->page()->runJavaScript(QString(file.readAll()).arg(request.url().toString()).arg(QString(body)));

			file.close();
		}
		else
		{
			setUrl(request.url(), false);
		}
	}
}

void QtWebEngineWebWidget::print(QPrinter *printer)
{
	m_webView->render(printer);
}

void QtWebEngineWebWidget::pageLoadStarted()
{
	m_isLoading = true;

	setStatusMessage(QString());
	setStatusMessage(QString(), true);

	emit progressBarGeometryChanged();
	emit loadingChanged(true);
}

void QtWebEngineWebWidget::pageLoadFinished()
{
	m_isLoading = false;

	updateNavigationActions();
	startReloadTimer();

	emit loadingChanged(false);
}

void QtWebEngineWebWidget::downloadFile(QWebEngineDownloadItem *item)
{
	TransfersManager::startTransfer(item->url(), QString(), false, isPrivate());

	item->cancel();
	item->deleteLater();
}

void QtWebEngineWebWidget::linkHovered(const QString &link)
{
	setStatusMessage(link, true);
}

void QtWebEngineWebWidget::clearOptions()
{
	WebWidget::clearOptions();

	updateOptions(getUrl());
}

void QtWebEngineWebWidget::clearSelection()
{
	m_webView->page()->runJavaScript(QLatin1String("window.getSelection().empty()"));
}

void QtWebEngineWebWidget::goToHistoryIndex(int index)
{
	m_webView->history()->goToItem(m_webView->history()->itemAt(index));
}

void QtWebEngineWebWidget::triggerAction(int identifier, bool checked)
{
	switch (identifier)
	{
		case ActionsManager::SaveAction:
			{
				const QString path = TransfersManager::getSavePath(suggestSaveFileName());

				if (!path.isEmpty())
				{
					QNetworkRequest request(getUrl());
					request.setHeader(QNetworkRequest::UserAgentHeader, m_webView->page()->profile()->httpUserAgent());

					Transfer *transfer = new Transfer(request, path, false, true, this);
					transfer->setAutoDelete(true);
				}
			}

			break;
		case ActionsManager::OpenLinkAction:
			{
				QMouseEvent mousePressEvent(QEvent::MouseButtonPress, QPointF(m_clickPosition), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
				QMouseEvent mouseReleaseEvent(QEvent::MouseButtonRelease, QPointF(m_clickPosition), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

				QCoreApplication::sendEvent(m_webView, &mousePressEvent);
				QCoreApplication::sendEvent(m_webView, &mouseReleaseEvent);

				m_clickPosition = QPoint();
			}

			break;
		case ActionsManager::OpenLinkInCurrentTabAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, CurrentTabOpen);
			}

			break;
		case ActionsManager::OpenLinkInNewTabAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewTabOpen);
			}

			break;
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewBackgroundTabOpen);
			}

			break;
		case ActionsManager::OpenLinkInNewWindowAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewWindowOpen);
			}

			break;
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewBackgroundWindowOpen);
			}

			break;
		case ActionsManager::OpenLinkInNewPrivateTabAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewPrivateTabOpen);
			}

			break;
		case ActionsManager::OpenLinkInNewPrivateTabBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewPrivateBackgroundTabOpen);
			}

			break;
		case ActionsManager::OpenLinkInNewPrivateWindowAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewPrivateWindowOpen);
			}

			break;
		case ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewPrivateBackgroundWindowOpen);
			}

			break;
		case ActionsManager::CopyLinkToClipboardAction:
			if (!m_hitResult.linkUrl.isEmpty())
			{
				QGuiApplication::clipboard()->setText(m_hitResult.linkUrl.toString());
			}

			break;
		case ActionsManager::BookmarkLinkAction:
			if (m_hitResult.linkUrl.isValid())
			{
				if (BookmarksManager::hasBookmark(m_hitResult.linkUrl))
				{
					emit requestedEditBookmark(m_hitResult.linkUrl);
				}
				else
				{
					emit requestedAddBookmark(m_hitResult.linkUrl, m_hitResult.title, QString());
				}
			}

			break;
		case ActionsManager::SaveLinkToDiskAction:
			TransfersManager::startTransfer(m_hitResult.linkUrl.toString(), QString(), false, isPrivate());

			break;
		case ActionsManager::SaveLinkToDownloadsAction:
			TransfersManager::startTransfer(m_hitResult.linkUrl.toString(), QString(), true, isPrivate());

			break;
		case ActionsManager::OpenFrameInCurrentTabAction:
			if (m_hitResult.frameUrl.isValid())
			{
				setUrl(m_hitResult.frameUrl);
			}

			break;
		case ActionsManager::OpenFrameInNewTabAction:
			if (m_hitResult.frameUrl.isValid())
			{
				openUrl(m_hitResult.frameUrl, CurrentTabOpen);
			}

			break;
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
			if (m_hitResult.frameUrl.isValid())
			{
				openUrl(m_hitResult.frameUrl, NewBackgroundTabOpen);
			}

			break;
		case ActionsManager::CopyFrameLinkToClipboardAction:
			if (m_hitResult.frameUrl.isValid())
			{
				QGuiApplication::clipboard()->setText(m_hitResult.frameUrl.toString());
			}

			break;
		case ActionsManager::ReloadFrameAction:
			if (m_hitResult.frameUrl.isValid())
			{
//TODO Improve
				m_webView->page()->runJavaScript(QStringLiteral("var frames = document.querySelectorAll('iframe[src=\"%1\"], frame[src=\"%1\"]'); for (var i = 0; i < frames.length; ++i) { frames[i].src = ''; frames[i].src = \'%1\'; }").arg(m_hitResult.frameUrl.toString()));
			}

			break;
		case ActionsManager::OpenImageInNewTabAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				openUrl(m_hitResult.imageUrl, NewTabOpen);
			}

			break;
		case ActionsManager::OpenImageInNewTabBackgroundAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				openUrl(m_hitResult.imageUrl, NewBackgroundTabOpen);
			}

			break;
		case ActionsManager::SaveImageToDiskAction:
			if (m_hitResult.imageUrl.isValid())
			{
				if (m_hitResult.imageUrl.url().contains(QLatin1String(";base64,")))
				{
					const QString imageUrl = m_hitResult.imageUrl.url();
					const QString imageType = imageUrl.mid(11, (imageUrl.indexOf(QLatin1Char(';')) - 11));
					const QString path = TransfersManager::getSavePath(tr("file") + QLatin1Char('.') + imageType);

					if (path.isEmpty())
					{
						return;
					}

					QImageWriter writer(path);

					if (!writer.write(QImage::fromData(QByteArray::fromBase64(imageUrl.mid(imageUrl.indexOf(QLatin1String(";base64,")) + 7).toUtf8()), imageType.toStdString().c_str())))
					{
						Console::addMessage(tr("Failed to save image: %1").arg(writer.errorString()), OtherMessageCategory, ErrorMessageLevel, path);
					}
				}
				else
				{
					QNetworkRequest request(m_hitResult.imageUrl);
					request.setHeader(QNetworkRequest::UserAgentHeader, m_webView->page()->profile()->httpUserAgent());

					Transfer *transfer = new Transfer(request, QString(), false, false, this);
					transfer->setAutoDelete(true);
				}
			}

			break;
		case ActionsManager::CopyImageUrlToClipboardAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				QApplication::clipboard()->setText(m_hitResult.imageUrl.toString());
			}

			break;
		case ActionsManager::ReloadImageAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				if (getUrl().matches(m_hitResult.imageUrl, (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash)))
				{
					triggerAction(ActionsManager::ReloadAndBypassCacheAction);
				}
				else
				{
//TODO Improve
					m_webView->page()->runJavaScript(QStringLiteral("var images = document.querySelectorAll('img[src=\"%1\"]'); for (var i = 0; i < images.length; ++i) { images[i].src = ''; images[i].src = \'%1\'; }").arg(m_hitResult.imageUrl.toString()));
				}
			}

			break;
		case ActionsManager::ImagePropertiesAction:
			if (m_hitResult.imageUrl.scheme() == QLatin1String("data"))
			{
				QVariantMap properties;
				properties[QLatin1String("alternativeText")] = m_hitResult.alternateText;
				properties[QLatin1String("longDescription")] = m_hitResult.longDescription;

				ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());
				ImagePropertiesDialog *imagePropertiesDialog = new ImagePropertiesDialog(m_hitResult.imageUrl, properties, NULL, this);
				imagePropertiesDialog->setButtonsVisible(false);

				if (parent)
				{
					ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Close), imagePropertiesDialog, this);

					connect(this, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

					showDialog(&dialog);
				}
			}
			else
			{
				m_webView->page()->runJavaScript(QStringLiteral("var element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement); if (element && element.tagName && element.tagName.toLowerCase() == 'img') { [element.naturalWidth, element.naturalHeight]; }").arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleImageProperties));
			}

			break;
		case ActionsManager::SaveMediaToDiskAction:
			if (m_hitResult.mediaUrl.isValid())
			{
				QNetworkRequest request(m_hitResult.mediaUrl);
				request.setHeader(QNetworkRequest::UserAgentHeader, m_webView->page()->profile()->httpUserAgent());

				Transfer *transfer = new Transfer(request, QString(), false, false, this);
				transfer->setAutoDelete(true);
			}

			break;
		case ActionsManager::CopyMediaUrlToClipboardAction:
			if (!m_hitResult.mediaUrl.isEmpty())
			{
				QApplication::clipboard()->setText(m_hitResult.mediaUrl.toString());
			}

			break;
		case ActionsManager::MediaControlsAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { element.controls = %3; }").arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()).arg(checked ? QLatin1String("true") : QLatin1String("false")));

			break;
		case ActionsManager::MediaLoopAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { element.loop = %3; }").arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()).arg(checked ? QLatin1String("true") : QLatin1String("false")));

			break;
		case ActionsManager::MediaPlayPauseAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { if (element.paused) { element.play(); } else { element.pause(); } }").arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()));

			break;
		case ActionsManager::MediaMuteAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { element.muted = !element.muted; }").arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()));

			break;
		case ActionsManager::GoBackAction:
			m_webView->triggerPageAction(QWebEnginePage::Back);

			break;
		case ActionsManager::GoForwardAction:
			m_webView->triggerPageAction(QWebEnginePage::Forward);

			break;
		case ActionsManager::RewindAction:
			m_webView->history()->goToItem(m_webView->history()->itemAt(0));

			break;
		case ActionsManager::FastForwardAction:
			m_webView->history()->goToItem(m_webView->history()->itemAt(m_webView->history()->count() - 1));

			break;
		case ActionsManager::StopAction:
			m_webView->triggerPageAction(QWebEnginePage::Stop);

			break;
		case ActionsManager::ReloadAction:
			emit aboutToReload();

			m_webView->triggerPageAction(QWebEnginePage::Stop);
			m_webView->triggerPageAction(QWebEnginePage::Reload);

			break;
		case ActionsManager::ReloadOrStopAction:
			if (isLoading())
			{
				triggerAction(ActionsManager::StopAction);
			}
			else
			{
				triggerAction(ActionsManager::ReloadAction);
			}

			break;
		case ActionsManager::ReloadAndBypassCacheAction:
			m_webView->triggerPageAction(QWebEnginePage::ReloadAndBypassCache);

			break;
		case ActionsManager::ContextMenuAction:
			showContextMenu();

			break;
		case ActionsManager::UndoAction:
			m_webView->triggerPageAction(QWebEnginePage::Undo);

			break;
		case ActionsManager::RedoAction:
			m_webView->triggerPageAction(QWebEnginePage::Redo);

			break;
		case ActionsManager::CutAction:
			m_webView->triggerPageAction(QWebEnginePage::Cut);

			break;
		case ActionsManager::CopyAction:
			m_webView->triggerPageAction(QWebEnginePage::Copy);

			break;
		case ActionsManager::CopyPlainTextAction:
			{
				const QString text = getSelectedText();

				if (!text.isEmpty())
				{
					QApplication::clipboard()->setText(text);
				}
			}

			break;
		case ActionsManager::CopyAddressAction:
			QApplication::clipboard()->setText(getUrl().toString());

			break;
		case ActionsManager::CopyToNoteAction:
			{
				BookmarksItem *note = NotesManager::addNote(BookmarksModel::UrlBookmark, getUrl());
				note->setData(getSelectedText(), BookmarksModel::DescriptionRole);
			}

			break;
		case ActionsManager::PasteAction:
			m_webView->triggerPageAction(QWebEnginePage::Paste);

			break;
		case ActionsManager::DeleteAction:
			m_webView->page()->runJavaScript(QLatin1String("window.getSelection().deleteFromDocument()"));

			break;
		case ActionsManager::SelectAllAction:
			m_webView->triggerPageAction(QWebEnginePage::SelectAll);

			break;
		case ActionsManager::ClearAllAction:
			triggerAction(ActionsManager::SelectAllAction);
			triggerAction(ActionsManager::DeleteAction);

			break;
		case ActionsManager::SearchAction:
			quickSearch(getAction(ActionsManager::SearchAction));

			break;
		case ActionsManager::CreateSearchAction:
			{
				QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/createSearch.js"));
				file.open(QIODevice::ReadOnly);

				m_webView->page()->runJavaScript(QString(file.readAll()).arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleCreateSearch));

				file.close();
			}

			break;
		case ActionsManager::ScrollToStartAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollTo(0, 0)"));

			break;
		case ActionsManager::ScrollToEndAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollTo(0, document.body.scrollHeigh)"));

			break;
		case ActionsManager::ScrollPageUpAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollByPages(1)"));

			break;
		case ActionsManager::ScrollPageDownAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollByPages(-1)"));

			break;
		case ActionsManager::ScrollPageLeftAction:
			m_webView->page()->runJavaScript(QStringLiteral("window.scrollBy(-%1, 0)").arg(m_webView->width()));

			break;
		case ActionsManager::ScrollPageRightAction:
			m_webView->page()->runJavaScript(QStringLiteral("window.scrollBy(%1, 0)").arg(m_webView->width()));

			break;
		case ActionsManager::StartDragScrollAction:
			setScrollMode(DragScroll);

			break;
		case ActionsManager::StartMoveScrollAction:
			setScrollMode(MoveScroll);

			break;
		case ActionsManager::EndScrollAction:
			setScrollMode(NoScroll);

			break;
		case ActionsManager::ActivateContentAction:
			{
				m_webView->setFocus();
				m_webView->page()->runJavaScript(QLatin1String("var element = document.activeElement; if (element && element.tagName && (element.tagName.toLowerCase() == 'input' || element.tagName.toLowerCase() == 'textarea'))) { document.activeElement.blur(); }"));
			}

			break;
		case ActionsManager::AddBookmarkAction:
			{
				const QUrl url = getUrl();

				if (BookmarksManager::hasBookmark(url))
				{
					emit requestedEditBookmark(url);
				}
				else
				{
					emit requestedAddBookmark(url, getTitle(), QString());
				}
			}

			break;
		case ActionsManager::WebsitePreferencesAction:
			{
				const QUrl url(getUrl());
				WebsitePreferencesDialog dialog(url, QList<QNetworkCookie>(), this);

				if (dialog.exec() == QDialog::Accepted)
				{
					updateOptions(getUrl());
				}
			}

			break;
		default:
			break;
	}
}

void QtWebEngineWebWidget::openUrl(const QUrl &url, OpenHints hints)
{
	WebWidget *widget = clone(false);
	widget->setRequestedUrl(url);

	emit requestedNewWindow(widget, hints);
}

void QtWebEngineWebWidget::pasteText(const QString &text)
{
	QMimeData *mimeData = new QMimeData();
	const QStringList mimeTypes = QGuiApplication::clipboard()->mimeData()->formats();

	for (int i = 0; i < mimeTypes.count(); ++i)
	{
		mimeData->setData(mimeTypes.at(i), QGuiApplication::clipboard()->mimeData()->data(mimeTypes.at(i)));
	}

	QGuiApplication::clipboard()->setText(text);

	triggerAction(ActionsManager::PasteAction);

	QGuiApplication::clipboard()->setMimeData(mimeData);
}

void QtWebEngineWebWidget::iconReplyFinished()
{
	if (!m_iconReply)
	{
		return;
	}

	m_icon = QIcon(QPixmap::fromImage(QImage::fromData(m_iconReply->readAll())));

	emit iconChanged(getIcon());

	m_iconReply->deleteLater();
	m_iconReply = NULL;
}

void QtWebEngineWebWidget::handleIconChange(const QUrl &url)
{
	if (m_iconReply && m_iconReply->url() != url)
	{
		m_iconReply->abort();
		m_iconReply->deleteLater();
	}

	m_icon = QIcon();

	emit iconChanged(getIcon());

	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::UserAgentHeader, m_webView->page()->profile()->httpUserAgent());

	m_iconReply = NetworkManagerFactory::getNetworkManager()->get(request);
	m_iconReply->setParent(this);

	connect(m_iconReply, SIGNAL(finished()), this, SLOT(iconReplyFinished()));
}

void QtWebEngineWebWidget::handleAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator)
{
	AuthenticationDialog *authenticationDialog = new AuthenticationDialog(url, authenticator, this);
	authenticationDialog->setButtonsVisible(false);

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, this);

	connect(&dialog, SIGNAL(accepted()), authenticationDialog, SLOT(accept()));
	connect(this, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	showDialog(&dialog);
}

void QtWebEngineWebWidget::handleProxyAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator, const QString &proxy)
{
	Q_UNUSED(url)

	AuthenticationDialog *authenticationDialog = new AuthenticationDialog(proxy, authenticator, this);
	authenticationDialog->setButtonsVisible(false);

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, this);

	connect(&dialog, SIGNAL(accepted()), authenticationDialog, SLOT(accept()));
	connect(this, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	showDialog(&dialog);
}

void QtWebEngineWebWidget::handleContextMenu(const QVariant &result)
{
	m_hitResult = HitTestResult(result);

	emit hitTestResultReady();

	if (m_ignoreContextMenu || (!m_hitResult.geometry.isValid() && m_clickPosition.isNull()))
	{
		return;
	}

	updateEditActions();

	MenuFlags flags = NoMenu;

	if (m_hitResult.flags.testFlag(IsFormTest))
	{
		flags |= FormMenu;
	}

	if (!m_hitResult.imageUrl.isValid() && m_hitResult.flags.testFlag(IsSelectedTest) && !m_webView->selectedText().isEmpty())
	{
		flags |= SelectionMenu;
	}

	if (m_hitResult.linkUrl.isValid())
	{
		if (m_hitResult.linkUrl.scheme() == QLatin1String("mailto"))
		{
			flags |= MailMenu;
		}
		else
		{
			flags |= LinkMenu;
		}
	}

	if (!m_hitResult.imageUrl.isEmpty())
	{
		flags |= ImageMenu;
	}

	if (m_hitResult.mediaUrl.isValid())
	{
		flags |= MediaMenu;
	}

	if (m_hitResult.flags.testFlag(IsContentEditableTest))
	{
		flags |= EditMenu;
	}

	if (flags == NoMenu || flags == FormMenu)
	{
		flags |= StandardMenu;

		if (m_hitResult.frameUrl.isValid())
		{
			flags |= FrameMenu;
		}
	}

	WebWidget::showContextMenu((m_clickPosition.isNull() ? m_hitResult.geometry.center() : m_clickPosition), flags);
}

void QtWebEngineWebWidget::handleCreateSearch(const QVariant &result)
{
	if (result.isNull())
	{
		return;
	}

	const QUrlQuery parameters(result.toMap().value(QLatin1String("query")).toString());
	const QStringList identifiers = SearchesManager::getSearchEngines();
	const QStringList keywords = SearchesManager::getSearchKeywords();
	const QIcon icon = getIcon();
	const QUrl url(result.toMap().value(QLatin1String("url")).toString());
	SearchInformation engine;
	engine.identifier = Utils::createIdentifier(getUrl().host(), identifiers);
	engine.title = getTitle();
	engine.icon = (icon.isNull() ? Utils::getIcon(QLatin1String("edit-find")) : icon);
	engine.resultsUrl.url = (url.isEmpty() ? getUrl() : (url.isRelative() ? getUrl().resolved(url) : url)).toString();
	engine.resultsUrl.enctype = result.toMap().value(QLatin1String("enctype")).toString();
	engine.resultsUrl.method = result.toMap().value(QLatin1String("method")).toString();
	engine.resultsUrl.parameters = parameters;

	SearchPropertiesDialog dialog(engine, keywords, false, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	SearchesManager::addSearchEngine(dialog.getSearchEngine(), dialog.isDefault());
}

void QtWebEngineWebWidget::handleHitTest(const QVariant &result)
{
	m_hitResult = HitTestResult(result);

	emit hitTestResultReady();
	emit unlockEventLoop();
}

void QtWebEngineWebWidget::handleHotClick(const QVariant &result)
{
	m_hitResult = HitTestResult(result);

	emit hitTestResultReady();

	if (!m_hitResult.flags.testFlag(IsContentEditableTest) && m_hitResult.tagName != QLatin1String("textarea") && m_hitResult.tagName != QLatin1String("select") && m_hitResult.tagName != QLatin1String("input"))
	{
		QTimer::singleShot(250, this, SLOT(showHotClickMenu()));
	}
}

void QtWebEngineWebWidget::handleImageProperties(const QVariant &result)
{
	QVariantMap properties;
	properties[QLatin1String("alternativeText")] = m_hitResult.alternateText;
	properties[QLatin1String("longDescription")] = m_hitResult.longDescription;

	if (result.isValid())
	{
		properties[QLatin1String("width")] = result.toList()[0].toInt();
		properties[QLatin1String("height")] = result.toList()[1].toInt();
	}

	ImagePropertiesDialog *imagePropertiesDialog = new ImagePropertiesDialog(m_hitResult.imageUrl, properties, NULL, this);
	imagePropertiesDialog->setButtonsVisible(false);

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Close), imagePropertiesDialog, this);

	connect(this, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	showDialog(&dialog);
}

void QtWebEngineWebWidget::handlePermissionRequest(const QUrl &url, QWebEnginePage::Feature feature)
{
	notifyPermissionRequested(url, feature, false);
}

void QtWebEngineWebWidget::handlePermissionCancel(const QUrl &url, QWebEnginePage::Feature feature)
{
	notifyPermissionRequested(url, feature, true);
}

void QtWebEngineWebWidget::handleScroll(const QVariant &result)
{
	if (result.isValid())
	{
		m_scrollPosition = QPoint(result.toList()[0].toInt(), result.toList()[1].toInt());
	}
}

void QtWebEngineWebWidget::handleScrollToAnchor(const QVariant &result)
{
	if (result.isValid())
	{
		setScrollPosition(QPoint(result.toList()[0].toInt(), result.toList()[1].toInt()));
	}
}

void QtWebEngineWebWidget::handleToolTip(const QVariant &result)
{
	const HitTestResult hitResult(result);
	const QString toolTipsMode = SettingsManager::getValue(QLatin1String("Browser/ToolTipsMode")).toString();
	const QString link = (hitResult.linkUrl.isValid() ? hitResult.linkUrl : hitResult.formUrl).toString();
	QString text;

	if (toolTipsMode != QLatin1String("disabled"))
	{
		const QString title = QString(hitResult.title).replace(QLatin1Char('&'), QLatin1String("&amp;")).replace(QLatin1Char('<'), QLatin1String("&lt;")).replace(QLatin1Char('>'), QLatin1String("&gt;"));

		if (toolTipsMode == QLatin1String("extended"))
		{
			if (!link.isEmpty())
			{
				text = (title.isEmpty() ? QString() : tr("Title: %1").arg(title) + QLatin1String("<br>")) + tr("Address: %1").arg(link);
			}
			else if (!title.isEmpty())
			{
				text = title;
			}
		}
		else
		{
			text = title;
		}
	}

	setStatusMessage((link.isEmpty() ? hitResult.title : link), true);

	if (!text.isEmpty())
	{
		QToolTip::showText(m_webView->mapToGlobal(m_clickPosition), QStringLiteral("<div style=\"white-space:pre-line;\">%1</div>").arg(text), m_webView);
	}
}

void QtWebEngineWebWidget::handleWindowCloseRequest()
{
	const QString mode = SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanCloseWindows"), getUrl()).toString();

	if (mode != QLatin1String("ask"))
	{
		if (mode == QLatin1String("allow"))
		{
			emit requestedCloseWindow();
		}

		return;
	}

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-warning")), tr("JavaScript"), tr("Webpage wants to close this tab, do you want to allow to close it?"), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), NULL, this);
	dialog.setCheckBox(tr("Do not show this message again"), false);

	connect(this, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	showDialog(&dialog);

	if (dialog.getCheckBoxState())
	{
		SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanCloseWindows"), (dialog.isAccepted() ? QLatin1String("allow") : QLatin1String("disallow")));
	}

	if (dialog.isAccepted())
	{
		emit requestedCloseWindow();
	}
}

void QtWebEngineWebWidget::notifyTitleChanged()
{
	emit titleChanged(getTitle());
}

void QtWebEngineWebWidget::notifyUrlChanged(const QUrl &url)
{
	updateOptions(url);
	updatePageActions(url);
	updateNavigationActions();

	m_icon = QIcon();

	emit iconChanged(getIcon());
	emit urlChanged(url);

	SessionsManager::markSessionModified();
}

void QtWebEngineWebWidget::notifyIconChanged()
{
	emit iconChanged(getIcon());
}

void QtWebEngineWebWidget::notifyPermissionRequested(const QUrl &url, QWebEnginePage::Feature feature, bool cancel)
{
	QString option;

	if (feature == QWebEnginePage::Geolocation)
	{
		option = QLatin1String("Browser/EnableGeolocation");
	}
	else if (feature == QWebEnginePage::MediaAudioCapture)
	{
		option = QLatin1String("Browser/EnableMediaCaptureAudio");
	}
	else if (feature == QWebEnginePage::MediaVideoCapture)
	{
		option = QLatin1String("Browser/EnableMediaCaptureVideo");
	}
	else if (feature == QWebEnginePage::MediaAudioVideoCapture)
	{
		option = QLatin1String("Browser/EnableMediaCaptureAudioVideo");
	}
	else if (feature == QWebEnginePage::Notifications)
	{
		option = QLatin1String("Browser/EnableNotifications");
	}
	else if (feature == QWebEnginePage::MouseLock)
	{
		option = QLatin1String("Browser/EnablePointerLock");
	}

	if (!option.isEmpty())
	{
		if (cancel)
		{
			emit requestedPermission(option, url, true);
		}
		else
		{
			const QString value = SettingsManager::getValue(option, url).toString();

			if (value == QLatin1String("allow"))
			{
				m_webView->page()->setFeaturePermission(url, feature, QWebEnginePage::PermissionGrantedByUser);
			}
			else if (value == QLatin1String("disallow"))
			{
				m_webView->page()->setFeaturePermission(url, feature, QWebEnginePage::PermissionDeniedByUser);
			}
			else
			{
				emit requestedPermission(option, url, false);
			}
		}
	}
}

void QtWebEngineWebWidget::updateUndo()
{
	if (m_actions.contains(ActionsManager::UndoAction))
	{
		m_actions[ActionsManager::UndoAction]->setText(m_webView->pageAction(QWebEnginePage::Undo)->text());
		m_actions[ActionsManager::UndoAction]->setEnabled(m_webView->pageAction(QWebEnginePage::Undo)->isEnabled());
	}
}

void QtWebEngineWebWidget::updateRedo()
{
	if (m_actions.contains(ActionsManager::RedoAction))
	{
		m_actions[ActionsManager::RedoAction]->setText(m_webView->pageAction(QWebEnginePage::Redo)->text());
		m_actions[ActionsManager::RedoAction]->setEnabled(m_webView->pageAction(QWebEnginePage::Redo)->isEnabled());
	}
}

void QtWebEngineWebWidget::updatePageActions(const QUrl &url)
{
	if (m_actions.contains(ActionsManager::AddBookmarkAction))
	{
		m_actions[ActionsManager::AddBookmarkAction]->setOverrideText(BookmarksManager::hasBookmark(url.toString()) ? QT_TRANSLATE_NOOP("actions", "Edit Bookmark…") : QT_TRANSLATE_NOOP("actions", "Add Bookmark…"));
	}

	if (m_actions.contains(ActionsManager::WebsitePreferencesAction))
	{
		m_actions[ActionsManager::WebsitePreferencesAction]->setEnabled(!url.isEmpty() && url.scheme() != QLatin1String("about"));
	}
}

void QtWebEngineWebWidget::updateNavigationActions()
{
	if (m_actions.contains(ActionsManager::GoBackAction))
	{
		m_actions[ActionsManager::GoBackAction]->setEnabled(m_webView->history()->canGoBack());
	}

	if (m_actions.contains(ActionsManager::GoForwardAction))
	{
		m_actions[ActionsManager::GoForwardAction]->setEnabled(m_webView->history()->canGoForward());
	}

	if (m_actions.contains(ActionsManager::RewindAction))
	{
		m_actions[ActionsManager::RewindAction]->setEnabled(m_webView->history()->canGoBack());
	}

	if (m_actions.contains(ActionsManager::FastForwardAction))
	{
		m_actions[ActionsManager::FastForwardAction]->setEnabled(m_webView->history()->canGoForward());
	}

	if (m_actions.contains(ActionsManager::StopAction))
	{
		m_actions[ActionsManager::StopAction]->setEnabled(m_isLoading);
	}

	if (m_actions.contains(ActionsManager::ReloadAction))
	{
		m_actions[ActionsManager::ReloadAction]->setEnabled(!m_isLoading);
	}

	if (m_actions.contains(ActionsManager::ReloadOrStopAction))
	{
		m_actions[ActionsManager::ReloadOrStopAction]->setup(m_isLoading ? getAction(ActionsManager::StopAction) : getAction(ActionsManager::ReloadAction));
	}

	if (m_actions.contains(ActionsManager::LoadPluginsAction))
	{
		m_actions[ActionsManager::LoadPluginsAction]->setEnabled(false);
	}
}

void QtWebEngineWebWidget::updateEditActions()
{
	if (m_actions.contains(ActionsManager::CutAction))
	{
		m_actions[ActionsManager::CutAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Cut)->isEnabled());
	}

	if (m_actions.contains(ActionsManager::CopyAction))
	{
		m_actions[ActionsManager::CopyAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Copy)->isEnabled());
	}

	if (m_actions.contains(ActionsManager::CopyPlainTextAction))
	{
		m_actions[ActionsManager::CopyPlainTextAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Copy)->isEnabled());
	}

	if (m_actions.contains(ActionsManager::CopyToNoteAction))
	{
		m_actions[ActionsManager::CopyToNoteAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Copy)->isEnabled());
	}

	if (m_actions.contains(ActionsManager::PasteAction))
	{
		m_actions[ActionsManager::PasteAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Paste)->isEnabled());
	}

	if (m_actions.contains(ActionsManager::PasteAndGoAction))
	{
		m_actions[ActionsManager::PasteAndGoAction]->setEnabled(!QApplication::clipboard()->text().isEmpty());
	}

	if (m_actions.contains(ActionsManager::PasteNoteAction))
	{
		m_actions[ActionsManager::PasteNoteAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Paste)->isEnabled() && NotesManager::getModel()->getRootItem()->hasChildren());
	}

	if (m_actions.contains(ActionsManager::DeleteAction))
	{
		m_actions[ActionsManager::DeleteAction]->setEnabled(m_webView->hasSelection() && m_hitResult.flags.testFlag(IsContentEditableTest));
	}

	if (m_actions.contains(ActionsManager::SelectAllAction))
	{
		m_actions[ActionsManager::SelectAllAction]->setEnabled(!m_hitResult.flags.testFlag(IsContentEditableTest) || !m_hitResult.flags.testFlag(IsEmptyTest));
	}

	if (m_actions.contains(ActionsManager::ClearAllAction))
	{
		m_actions[ActionsManager::ClearAllAction]->setEnabled(m_hitResult.flags.testFlag(IsContentEditableTest) && !m_hitResult.flags.testFlag(IsEmptyTest));
	}

	if (m_actions.contains(ActionsManager::SearchAction))
	{
		const SearchInformation engine = SearchesManager::getSearchEngine(getOption(QLatin1String("Search/DefaultQuickSearchEngine")).toString());
		const bool isValid = !engine.identifier.isEmpty();

		m_actions[ActionsManager::SearchAction]->setEnabled(isValid);
		m_actions[ActionsManager::SearchAction]->setData(isValid ? engine.identifier : QVariant());
		m_actions[ActionsManager::SearchAction]->setIcon((!isValid || engine.icon.isNull()) ? Utils::getIcon(QLatin1String("edit-find")) : engine.icon);
		m_actions[ActionsManager::SearchAction]->setOverrideText(isValid ? engine.title : QT_TRANSLATE_NOOP("actions", "Search"));
		m_actions[ActionsManager::SearchAction]->setToolTip(isValid ? engine.description : tr("No search engines defined"));
	}

	if (m_actions.contains(ActionsManager::SearchMenuAction))
	{
		m_actions[ActionsManager::SearchMenuAction]->setEnabled(SearchesManager::getSearchEngines().count() > 1);
	}

	updateLinkActions();
	updateFrameActions();
	updateImageActions();
	updateMediaActions();
}

void QtWebEngineWebWidget::updateLinkActions()
{
	const bool isLink = m_hitResult.linkUrl.isValid();

	if (m_actions.contains(ActionsManager::OpenLinkAction))
	{
		m_actions[ActionsManager::OpenLinkAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInCurrentTabAction))
	{
		m_actions[ActionsManager::OpenLinkInCurrentTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewTabAction))
	{
		m_actions[ActionsManager::OpenLinkInNewTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewTabBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewWindowAction))
	{
		m_actions[ActionsManager::OpenLinkInNewWindowAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewWindowBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewWindowBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateTabAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateTabBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateWindowAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateWindowAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::CopyLinkToClipboardAction))
	{
		m_actions[ActionsManager::CopyLinkToClipboardAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::BookmarkLinkAction))
	{
		m_actions[ActionsManager::BookmarkLinkAction]->setOverrideText(BookmarksManager::hasBookmark(m_hitResult.linkUrl.toString()) ? QT_TRANSLATE_NOOP("actions", "Edit Link Bookmark…") : QT_TRANSLATE_NOOP("actions", "Bookmark Link…"));
		m_actions[ActionsManager::BookmarkLinkAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::SaveLinkToDiskAction))
	{
		m_actions[ActionsManager::SaveLinkToDiskAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::SaveLinkToDownloadsAction))
	{
		m_actions[ActionsManager::SaveLinkToDownloadsAction]->setEnabled(isLink);
	}
}

void QtWebEngineWebWidget::updateFrameActions()
{
	const bool isFrame = m_hitResult.frameUrl.isValid();

	if (m_actions.contains(ActionsManager::OpenFrameInCurrentTabAction))
	{
		m_actions[ActionsManager::OpenFrameInCurrentTabAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::OpenFrameInNewTabAction))
	{
		m_actions[ActionsManager::OpenFrameInNewTabAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::OpenFrameInNewTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenFrameInNewTabBackgroundAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::CopyFrameLinkToClipboardAction))
	{
		m_actions[ActionsManager::CopyFrameLinkToClipboardAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::ReloadFrameAction))
	{
		m_actions[ActionsManager::ReloadFrameAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::ViewFrameSourceAction))
	{
		m_actions[ActionsManager::ViewFrameSourceAction]->setEnabled(false);
	}
}

void QtWebEngineWebWidget::updateImageActions()
{
	const bool isImage = !m_hitResult.imageUrl.isEmpty();
	const bool isOpened = getUrl().matches(m_hitResult.imageUrl, (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash));
	const QString fileName = fontMetrics().elidedText(m_hitResult.imageUrl.fileName(), Qt::ElideMiddle, 256);

	if (m_actions.contains(ActionsManager::OpenImageInNewTabAction))
	{
		m_actions[ActionsManager::OpenImageInNewTabAction]->setOverrideText(isImage ? (fileName.isEmpty() || m_hitResult.imageUrl.scheme() == QLatin1String("data")) ? tr("Open Image in New Tab (Untitled)") : tr("Open Image in New Tab (%1)").arg(fileName) : QT_TRANSLATE_NOOP("actions", "Open Image in New Tab"));
		m_actions[ActionsManager::OpenImageInNewTabAction]->setEnabled(isImage && !isOpened);
	}

	if (m_actions.contains(ActionsManager::OpenImageInNewTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenImageInNewTabBackgroundAction]->setOverrideText(isImage ? (fileName.isEmpty() || m_hitResult.imageUrl.scheme() == QLatin1String("data")) ? tr("Open Image in New Background Tab (Untitled)") : tr("Open Image in New Background Tab (%1)").arg(fileName) : QT_TRANSLATE_NOOP("actions", "Open Image in New Background Tab"));
		m_actions[ActionsManager::OpenImageInNewTabBackgroundAction]->setEnabled(isImage && !isOpened);
	}

	if (m_actions.contains(ActionsManager::SaveImageToDiskAction))
	{
		m_actions[ActionsManager::SaveImageToDiskAction]->setEnabled(isImage);
	}

	if (m_actions.contains(ActionsManager::CopyImageUrlToClipboardAction))
	{
		m_actions[ActionsManager::CopyImageUrlToClipboardAction]->setEnabled(isImage);
	}

	if (m_actions.contains(ActionsManager::ReloadImageAction))
	{
		m_actions[ActionsManager::ReloadImageAction]->setEnabled(isImage);
	}

	if (m_actions.contains(ActionsManager::ImagePropertiesAction))
	{
		m_actions[ActionsManager::ImagePropertiesAction]->setEnabled(isImage);
	}
}

void QtWebEngineWebWidget::updateMediaActions()
{
	const bool isMedia = m_hitResult.mediaUrl.isValid();
	const bool isVideo = (m_hitResult.tagName == QLatin1String("video"));

	if (m_actions.contains(ActionsManager::SaveMediaToDiskAction))
	{
		m_actions[ActionsManager::SaveMediaToDiskAction]->setOverrideText(isVideo ? QT_TRANSLATE_NOOP("actions", "Save Video…") : QT_TRANSLATE_NOOP("actions", "Save Audio…"));
		m_actions[ActionsManager::SaveMediaToDiskAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::CopyMediaUrlToClipboardAction))
	{
		m_actions[ActionsManager::CopyMediaUrlToClipboardAction]->setOverrideText(isVideo ? QT_TRANSLATE_NOOP("actions", "Copy Video Link to Clipboard") : QT_TRANSLATE_NOOP("actions", "Copy Audio Link to Clipboard"));
		m_actions[ActionsManager::CopyMediaUrlToClipboardAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaControlsAction))
	{
		m_actions[ActionsManager::MediaControlsAction]->setChecked(m_hitResult.flags.testFlag(MediaHasControlsTest));
		m_actions[ActionsManager::MediaControlsAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaLoopAction))
	{
		m_actions[ActionsManager::MediaLoopAction]->setChecked(m_hitResult.flags.testFlag(MediaIsLoopedTest));
		m_actions[ActionsManager::MediaLoopAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaPlayPauseAction))
	{
		m_actions[ActionsManager::MediaPlayPauseAction]->setOverrideText(m_hitResult.flags.testFlag(MediaIsPausedTest) ? QT_TRANSLATE_NOOP("actions", "Play") : QT_TRANSLATE_NOOP("actions", "Pause"));
		m_actions[ActionsManager::MediaPlayPauseAction]->setIcon(Utils::getIcon(m_hitResult.flags.testFlag(MediaIsPausedTest) ? QLatin1String("media-playback-start") : QLatin1String("media-playback-pause")));
		m_actions[ActionsManager::MediaPlayPauseAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaMuteAction))
	{
		m_actions[ActionsManager::MediaMuteAction]->setOverrideText(m_hitResult.flags.testFlag(MediaIsMutedTest) ? QT_TRANSLATE_NOOP("actions", "Unmute") : QT_TRANSLATE_NOOP("actions", "Mute"));
		m_actions[ActionsManager::MediaMuteAction]->setIcon(Utils::getIcon(m_hitResult.flags.testFlag(MediaIsMutedTest) ? QLatin1String("audio-volume-medium") : QLatin1String("audio-volume-muted")));
		m_actions[ActionsManager::MediaMuteAction]->setEnabled(isMedia);
	}
}

void QtWebEngineWebWidget::updateBookmarkActions()
{
	updatePageActions(getUrl());
	updateLinkActions();
}

void QtWebEngineWebWidget::updateOptions(const QUrl &url)
{
	QWebEngineSettings *settings = m_webView->page()->settings();
	settings->setAttribute(QWebEngineSettings::AutoLoadImages, getOption(QLatin1String("Browser/EnableImages"), url).toBool());
	settings->setAttribute(QWebEngineSettings::JavascriptEnabled, getOption(QLatin1String("Browser/EnableJavaScript"), url).toBool());
	settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, getOption(QLatin1String("Browser/JavaScriptCanAccessClipboard"), url).toBool());
	settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, getOption(QLatin1String("Browser/JavaScriptCanOpenWindows"), url).toBool());
	settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, getOption(QLatin1String("Browser/EnableLocalStorage"), url).toBool());
	settings->setDefaultTextEncoding(getOption(QLatin1String("Content/DefaultCharacterEncoding"), url).toString());

	m_webView->page()->profile()->setHttpUserAgent(getBackend()->getUserAgent(NetworkManagerFactory::getUserAgent(getOption(QLatin1String("Network/UserAgent"), url).toString()).value));

	disconnect(m_webView->page(), SIGNAL(geometryChangeRequested(QRect)), this, SIGNAL(requestedGeometryChange(QRect)));

	if (getOption(QLatin1String("Browser/JavaScriptCanChangeWindowGeometry"), url).toBool())
	{
		connect(m_webView->page(), SIGNAL(geometryChangeRequested(QRect)), this, SIGNAL(requestedGeometryChange(QRect)));
	}
}

void QtWebEngineWebWidget::showContextMenu(const QPoint &position)
{
	QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
	file.open(QIODevice::ReadOnly);

	m_webView->page()->runJavaScript(QString(file.readAll()).arg(position.x()).arg(position.y()), invoke(this, &QtWebEngineWebWidget::handleContextMenu));

	file.close();
}

void QtWebEngineWebWidget::showHotClickMenu()
{
	if (!m_webView->selectedText().trimmed().isEmpty())
	{
		showContextMenu(m_clickPosition);
	}
}

void QtWebEngineWebWidget::setScrollPosition(const QPoint &position)
{
	m_webView->page()->runJavaScript(QStringLiteral("window.scrollTo(%1, %2); [window.scrollX, window.scrollY];").arg(position.x()).arg(position.y()), invoke(this, &QtWebEngineWebWidget::handleScroll));
}

void QtWebEngineWebWidget::setHistory(const WindowHistoryInformation &history)
{
	if (history.entries.count() == 0)
	{
		m_webView->page()->history()->clear();

		updateNavigationActions();
		updateOptions(QUrl());
		updatePageActions(QUrl());

		return;
	}

	QByteArray data;
	QDataStream stream(&data, QIODevice::ReadWrite);
	stream << int(3) << history.entries.count() << history.index;

	for (int i = 0; i < history.entries.count(); ++i)
	{
		stream << QUrl(history.entries.at(i).url) << history.entries.at(i).title << QByteArray() << qint32(0) << false << QUrl() << qint32(0) << QUrl(history.entries.at(i).url) << false << qint64(QDateTime::currentDateTime().toTime_t()) << int(200);
	}

	stream.device()->reset();
	stream >> *(m_webView->page()->history());

	m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(history.index));

	const QUrl url = m_webView->page()->history()->itemAt(history.index).url();

	setRequestedUrl(url, false, true);
	updateOptions(url);
	updatePageActions(url);

	m_webView->reload();
}

void QtWebEngineWebWidget::setHistory(QDataStream &stream)
{
	stream.device()->reset();
	stream >> *(m_webView->page()->history());

	setRequestedUrl(m_webView->page()->history()->currentItem().url(), false, true);
	updateOptions(m_webView->page()->history()->currentItem().url());
}

void QtWebEngineWebWidget::setZoom(int zoom)
{
	if (zoom != getZoom())
	{
		m_webView->setZoomFactor(qBound(0.1, ((qreal) zoom / 100), (qreal) 100));

		SessionsManager::markSessionModified();

		emit zoomChanged(zoom);
		emit progressBarGeometryChanged();
	}
}

void QtWebEngineWebWidget::setUrl(const QUrl &url, bool typed)
{
	if (url.scheme() == QLatin1String("javascript"))
	{
		m_webView->page()->runJavaScript(url.toDisplayString(QUrl::RemoveScheme | QUrl::DecodeReserved));

		return;
	}

	if (!url.fragment().isEmpty() && url.matches(getUrl(), (QUrl::RemoveFragment | QUrl::StripTrailingSlash | QUrl::NormalizePathSegments)))
	{
		m_webView->page()->runJavaScript(QStringLiteral("var element = document.querySelector('a[name=%1], [id=%1]'); if (element) { var geometry = element.getBoundingClientRect(); [(geometry.left + window.scrollX), (geometry.top + window.scrollY)]; }").arg(url.fragment()), invoke(this, &QtWebEngineWebWidget::handleScrollToAnchor));

		return;
	}

	m_isTyped = typed;

	QUrl targetUrl(url);

	if (url.isValid() && url.scheme().isEmpty() && !url.path().startsWith('/'))
	{
		QUrl httpUrl = url;
		httpUrl.setScheme(QLatin1String("http"));

		targetUrl = httpUrl;
	}
	else if (url.isValid() && (url.scheme().isEmpty() || url.scheme() == "file"))
	{
		QUrl localUrl = url;
		localUrl.setScheme(QLatin1String("file"));

		targetUrl = localUrl;
	}

	updateOptions(targetUrl);

	m_webView->load(targetUrl);

	notifyTitleChanged();
	notifyIconChanged();
}

void QtWebEngineWebWidget::setPermission(const QString &key, const QUrl &url, WebWidget::PermissionPolicies policies)
{
	WebWidget::setPermission(key, url, policies);

	QWebEnginePage::Feature feature = QWebEnginePage::Geolocation;

	if (key == QLatin1String("Browser/EnableGeolocation"))
	{
		feature = QWebEnginePage::Geolocation;
	}
	else if (key == QLatin1String("Browser/EnableMediaCaptureAudio"))
	{
		feature = QWebEnginePage::MediaAudioCapture;
	}
	else if (key == QLatin1String("Browser/EnableMediaCaptureVideo"))
	{
		feature = QWebEnginePage::MediaVideoCapture;
	}
	else if (key == QLatin1String("Browser/EnableMediaCaptureAudioVideo"))
	{
		feature = QWebEnginePage::MediaAudioVideoCapture;
	}
	else if (key == QLatin1String("Browser/EnableNotifications"))
	{
		feature = QWebEnginePage::Notifications;
	}
	else if (key == QLatin1String("Browser/EnablePointerLock"))
	{
		feature = QWebEnginePage::MouseLock;
	}
	else
	{
		return;
	}

	m_webView->page()->setFeaturePermission(url, feature, (policies.testFlag(GrantedPermission) ? QWebEnginePage::PermissionGrantedByUser : QWebEnginePage::PermissionDeniedByUser));
}

void QtWebEngineWebWidget::setOption(const QString &key, const QVariant &value)
{
	WebWidget::setOption(key, value);

	updateOptions(getUrl());

	if (key == QLatin1String("Content/DefaultCharacterEncoding"))
	{
		m_webView->reload();
	}
}

void QtWebEngineWebWidget::setOptions(const QVariantHash &options)
{
	WebWidget::setOptions(options);

	updateOptions(getUrl());
}

WebWidget* QtWebEngineWebWidget::clone(bool cloneHistory)
{
	QtWebEngineWebWidget *widget = new QtWebEngineWebWidget(isPrivate(), getBackend());
	widget->setOptions(getOptions());

	if (cloneHistory)
	{
		QByteArray data;
		QDataStream stream(&data, QIODevice::ReadWrite);
		stream << *(m_webView->page()->history());

		widget->setHistory(stream);
	}

	widget->setZoom(getZoom());

	return widget;
}

Action* QtWebEngineWebWidget::getAction(int identifier)
{
	if (identifier < 0)
	{
		return NULL;
	}

	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	Action *action = new Action(identifier, this);

	m_actions[identifier] = action;

	connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

	switch (identifier)
	{
		case ActionsManager::StopScheduledReloadAction:
		case ActionsManager::CopyImageToClipboardAction:
		case ActionsManager::CheckSpellingAction:
		case ActionsManager::ViewSourceAction:
		case ActionsManager::InspectPageAction:
		case ActionsManager::InspectElementAction:
		case ActionsManager::LoadPluginsAction:
			action->setEnabled(false);

			break;
		case ActionsManager::AddBookmarkAction:
		case ActionsManager::WebsitePreferencesAction:
			updatePageActions(getUrl());

			break;
		case ActionsManager::GoBackAction:
		case ActionsManager::RewindAction:
			action->setEnabled(m_webView->history()->canGoBack());

			break;
		case ActionsManager::GoForwardAction:
		case ActionsManager::FastForwardAction:
			action->setEnabled(m_webView->history()->canGoForward());

			break;
		case ActionsManager::PasteNoteAction:
			action->setMenu(getPasteNoteMenu());

			updateEditActions();

			break;
		case ActionsManager::StopAction:
			action->setEnabled(m_isLoading);

			break;

		case ActionsManager::ReloadAction:
			action->setEnabled(!m_isLoading);

			break;
		case ActionsManager::ReloadOrStopAction:
			action->setup(m_isLoading ? getAction(ActionsManager::StopAction) : getAction(ActionsManager::ReloadAction));

			break;
		case ActionsManager::ScheduleReloadAction:
			action->setMenu(getReloadTimeMenu());

			break;
		case ActionsManager::ValidateAction:
			action->setEnabled(false);
			action->setMenu(new QMenu(this));

			break;
		case ActionsManager::UndoAction:
			action->setEnabled(m_webView->pageAction(QWebEnginePage::Undo));

			updateUndo();

			connect(m_webView->pageAction(QWebEnginePage::Undo), SIGNAL(changed()), this, SLOT(updateUndo()));

			break;
		case ActionsManager::RedoAction:
			action->setEnabled(m_webView->pageAction(QWebEnginePage::Redo));

			updateRedo();

			connect(m_webView->pageAction(QWebEnginePage::Redo), SIGNAL(changed()), this, SLOT(updateRedo()));

			break;
		case ActionsManager::SearchMenuAction:
			action->setMenu(getQuickSearchMenu());

		case ActionsManager::CutAction:
		case ActionsManager::CopyAction:
		case ActionsManager::CopyPlainTextAction:
		case ActionsManager::CopyToNoteAction:
		case ActionsManager::PasteAction:
		case ActionsManager::PasteAndGoAction:
		case ActionsManager::DeleteAction:
		case ActionsManager::ClearAllAction:
		case ActionsManager::SearchAction:
			updateEditActions();

			break;
		case ActionsManager::OpenLinkAction:
		case ActionsManager::OpenLinkInCurrentTabAction:
		case ActionsManager::OpenLinkInNewTabAction:
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
		case ActionsManager::OpenLinkInNewWindowAction:
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
		case ActionsManager::OpenLinkInNewPrivateTabAction:
		case ActionsManager::OpenLinkInNewPrivateTabBackgroundAction:
		case ActionsManager::OpenLinkInNewPrivateWindowAction:
		case ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction:
		case ActionsManager::CopyLinkToClipboardAction:
		case ActionsManager::BookmarkLinkAction:
		case ActionsManager::SaveLinkToDiskAction:
		case ActionsManager::SaveLinkToDownloadsAction:
			updateLinkActions();

			break;
		case ActionsManager::OpenFrameInCurrentTabAction:
		case ActionsManager::OpenFrameInNewTabAction:
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
		case ActionsManager::CopyFrameLinkToClipboardAction:
		case ActionsManager::ReloadFrameAction:
		case ActionsManager::ViewFrameSourceAction:
			updateFrameActions();

			break;
		case ActionsManager::OpenImageInNewTabAction:
		case ActionsManager::OpenImageInNewTabBackgroundAction:
		case ActionsManager::SaveImageToDiskAction:
		case ActionsManager::CopyImageUrlToClipboardAction:
		case ActionsManager::ReloadImageAction:
		case ActionsManager::ImagePropertiesAction:
			updateImageActions();

			break;
		case ActionsManager::SaveMediaToDiskAction:
		case ActionsManager::CopyMediaUrlToClipboardAction:
		case ActionsManager::MediaControlsAction:
		case ActionsManager::MediaLoopAction:
		case ActionsManager::MediaPlayPauseAction:
		case ActionsManager::MediaMuteAction:
			updateMediaActions();

			break;
		default:
			break;
	}

	return action;
}

QWebEnginePage* QtWebEngineWebWidget::getPage()
{
	return m_webView->page();
}

QString QtWebEngineWebWidget::getTitle() const
{
	const QString title = m_webView->title();

	if (title.isEmpty())
	{
		const QUrl url = getUrl();

		if (url.scheme() == QLatin1String("about") && (url.path().isEmpty() || url.path() == QLatin1String("blank") || url.path() == QLatin1String("start")))
		{
			return tr("Blank Page");
		}

		if (url.isLocalFile())
		{
			return QFileInfo(url.toLocalFile()).canonicalFilePath();
		}

		if (!url.isEmpty())
		{
			return url.toString();
		}

		return tr("(Untitled)");
	}

	return title;
}

QString QtWebEngineWebWidget::getSelectedText() const
{
	return m_webView->selectedText();
}

QUrl QtWebEngineWebWidget::getUrl() const
{
	const QUrl url = m_webView->url();

	return (url.isEmpty() ? m_webView->page()->requestedUrl() : url);
}

QIcon QtWebEngineWebWidget::getIcon() const
{
	if (isPrivate())
	{
		return Utils::getIcon(QLatin1String("tab-private"));
	}

	return (m_icon.isNull() ? Utils::getIcon(QLatin1String("tab")) : m_icon);
}

QPixmap QtWebEngineWebWidget::getThumbnail()
{
	return QPixmap();
}

QPoint QtWebEngineWebWidget::getScrollPosition() const
{
	return m_scrollPosition;
}

QRect QtWebEngineWebWidget::getProgressBarGeometry() const
{
	return QRect(QPoint(0, (height() - 30)), QSize(width(), 30));
}

WindowHistoryInformation QtWebEngineWebWidget::getHistory() const
{
	QWebEngineHistory *history = m_webView->history();
	WindowHistoryInformation information;
	information.index = history->currentItemIndex();

	const QString requestedUrl = m_webView->page()->requestedUrl().toString();
	const int historyCount = history->count();

	for (int i = 0; i < historyCount; ++i)
	{
		const QWebEngineHistoryItem item = history->itemAt(i);
		WindowHistoryEntry entry;
		entry.url = item.url().toString();
		entry.title = item.title();

		information.entries.append(entry);
	}

	if (isLoading() && requestedUrl != history->itemAt(history->currentItemIndex()).url().toString())
	{
		WindowHistoryEntry entry;
		entry.url = requestedUrl;
		entry.title = getTitle();

		information.index = historyCount;
		information.entries.append(entry);
	}

	return information;
}

QHash<QByteArray, QByteArray> QtWebEngineWebWidget::getHeaders() const
{
	return QHash<QByteArray, QByteArray>();
}

QVariantHash QtWebEngineWebWidget::getStatistics() const
{
	return QVariantHash();
}

int QtWebEngineWebWidget::getZoom() const
{
	return (m_webView->zoomFactor() * 100);
}

bool QtWebEngineWebWidget::isLoading() const
{
	return m_isLoading;
}

bool QtWebEngineWebWidget::isPrivate() const
{
	return m_webView->page()->profile()->isOffTheRecord();
}

bool QtWebEngineWebWidget::findInPage(const QString &text, FindFlags flags)
{
	QWebEnginePage::FindFlags nativeFlags = 0;

	if (flags & BackwardFind)
	{
		nativeFlags |= QWebEnginePage::FindBackward;
	}

	if (flags & CaseSensitiveFind)
	{
		nativeFlags |= QWebEnginePage::FindCaseSensitively;
	}

	m_webView->findText(text, nativeFlags);

	return false;
}

bool QtWebEngineWebWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_webView)
	{
		if (event->type() == QEvent::ChildAdded)
		{
			QChildEvent *childEvent = static_cast<QChildEvent*>(event);

			if (childEvent->child())
			{
				childEvent->child()->installEventFilter(this);

				m_childWidget = qobject_cast<QWidget*>(childEvent->child());
			}
		}
		else if (event->type() == QEvent::ContextMenu)
		{
			QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent*>(event);

			m_ignoreContextMenu = (contextMenuEvent->reason() == QContextMenuEvent::Mouse);

			if (contextMenuEvent->reason() == QContextMenuEvent::Keyboard)
			{
				m_clickPosition = contextMenuEvent->pos();

				showContextMenu();
			}

			return true;
		}
		else if (event->type() == QEvent::Move || event->type() == QEvent::Resize)
		{
			emit progressBarGeometryChanged();
		}
		else if (event->type() == QEvent::ToolTip)
		{
			QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
			file.open(QIODevice::ReadOnly);

			m_clickPosition = m_webView->mapFromGlobal(QCursor::pos());

			m_webView->page()->runJavaScript(QString(file.readAll()).arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleToolTip));

			file.close();

			event->accept();

			return true;
		}
	}
	else
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			if (mouseEvent->button() == Qt::BackButton)
			{
				triggerAction(ActionsManager::GoBackAction);

				event->accept();

				return true;
			}

			if (mouseEvent->button() == Qt::ForwardButton)
			{
				triggerAction(ActionsManager::GoForwardAction);

				event->accept();

				return true;
			}

			if (mouseEvent->button() == Qt::LeftButton || mouseEvent->button() == Qt::MiddleButton)
			{
				m_webView->page()->runJavaScript(QStringLiteral("[window.scrollX, window.scrollY]"), invoke(this, &QtWebEngineWebWidget::handleScroll));

				if (mouseEvent->button() == Qt::LeftButton && mouseEvent->buttons().testFlag(Qt::RightButton))
				{
					m_isUsingRockerNavigation = true;

					triggerAction(ActionsManager::GoBackAction);

					return true;
				}

				if (mouseEvent->modifiers() != Qt::NoModifier || mouseEvent->button() == Qt::MiddleButton)
				{
					QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
					file.open(QIODevice::ReadOnly);

					QEventLoop eventLoop;

					m_webView->page()->runJavaScript(QString(file.readAll()).arg(mouseEvent->pos().x() / m_webView->zoomFactor()).arg(mouseEvent->pos().y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleHitTest));

					file.close();

					connect(this, SIGNAL(unlockEventLoop()), &eventLoop, SLOT(quit()));
					connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
					connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

					eventLoop.exec();

					if (m_hitResult.linkUrl.isValid())
					{
						openUrl(m_hitResult.linkUrl, WindowsManager::calculateOpenHints(mouseEvent->modifiers(), mouseEvent->button(), CurrentTabOpen));

						event->accept();

						return true;
					}

					if (mouseEvent->button() == Qt::MiddleButton)
					{
						if (!m_hitResult.linkUrl.isValid() && m_hitResult.tagName != QLatin1String("textarea") && m_hitResult.tagName != QLatin1String("input"))
						{
							triggerAction(ActionsManager::StartMoveScrollAction);

							return true;
						}
					}
				}
			}
			else if (mouseEvent->button() == Qt::RightButton)
			{
				if (mouseEvent->buttons().testFlag(Qt::LeftButton))
				{
					triggerAction(ActionsManager::GoForwardAction);

					event->ignore();
				}
				else
				{
					event->accept();

					QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
					file.open(QIODevice::ReadOnly);

					QEventLoop eventLoop;

					m_webView->page()->runJavaScript(QString(file.readAll()).arg(mouseEvent->pos().x() / m_webView->zoomFactor()).arg(mouseEvent->pos().y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleHitTest));

					file.close();

					connect(this, SIGNAL(unlockEventLoop()), &eventLoop, SLOT(quit()));
					connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
					connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

					eventLoop.exec();

					if (m_hitResult.linkUrl.isValid())
					{
						m_clickPosition = mouseEvent->pos();
					}

					GesturesManager::startGesture((m_hitResult.linkUrl.isValid() ? GesturesManager::LinkGesturesContext : GesturesManager::GenericGesturesContext), m_childWidget, mouseEvent);
				}

				return true;
			}
		}
		else if (event->type() == QEvent::MouseButtonRelease)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			if (getScrollMode() == MoveScroll)
			{
				return true;
			}

			if (mouseEvent->button() == Qt::MiddleButton)
			{
				QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
				file.open(QIODevice::ReadOnly);

				QEventLoop eventLoop;

				m_webView->page()->runJavaScript(QString(file.readAll()).arg(mouseEvent->pos().x() / m_webView->zoomFactor()).arg(mouseEvent->pos().y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleHitTest));

				file.close();

				connect(this, SIGNAL(unlockEventLoop()), &eventLoop, SLOT(quit()));
				connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
				connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

				eventLoop.exec();

				if (getScrollMode() == DragScroll)
				{
					triggerAction(ActionsManager::EndScrollAction);
				}
				else if (m_hitResult.linkUrl.isValid())
				{
					return true;
				}
			}
			else if (mouseEvent->button() == Qt::RightButton && !mouseEvent->buttons().testFlag(Qt::LeftButton))
			{
				if (m_isUsingRockerNavigation)
				{
					m_isUsingRockerNavigation = false;
					m_ignoreContextMenuNextTime = true;

					QMouseEvent mousePressEvent(QEvent::MouseButtonPress, QPointF(mouseEvent->pos()), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
					QMouseEvent mouseReleaseEvent(QEvent::MouseButtonRelease, QPointF(mouseEvent->pos()), Qt::RightButton, Qt::RightButton, Qt::NoModifier);

					QCoreApplication::sendEvent(m_webView, &mousePressEvent);
					QCoreApplication::sendEvent(m_webView, &mouseReleaseEvent);
				}
				else
				{
					m_ignoreContextMenu = false;

					if (!GesturesManager::endGesture(m_childWidget, mouseEvent))
					{
						if (m_ignoreContextMenuNextTime)
						{
							m_ignoreContextMenuNextTime = false;

							event->ignore();

							return false;
						}

						m_clickPosition = mouseEvent->pos();

						showContextMenu(mouseEvent->pos());
					}
				}

				return true;
			}
		}
		else if (event->type() == QEvent::MouseButtonDblClick && SettingsManager::getValue(QLatin1String("Browser/ShowSelectionContextMenuOnDoubleClick")).toBool())
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
			{
				m_clickPosition = mouseEvent->pos();

				QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
				file.open(QIODevice::ReadOnly);

				m_webView->page()->runJavaScript(QString(file.readAll()).arg(mouseEvent->pos().x() / m_webView->zoomFactor()).arg(mouseEvent->pos().y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleHotClick));

				file.close();
			}
		}
		else if (event->type() == QEvent::Wheel)
		{
			m_webView->page()->runJavaScript(QStringLiteral("[window.scrollX, window.scrollY]"), invoke(this, &QtWebEngineWebWidget::handleScroll));

			if (getScrollMode() == MoveScroll)
			{
				triggerAction(ActionsManager::EndScrollAction);

				return true;
			}
			else
			{
				QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

				if (wheelEvent->buttons() == Qt::RightButton)
				{
					m_ignoreContextMenuNextTime = true;

					event->ignore();

					return false;
				}

				if (wheelEvent->modifiers().testFlag(Qt::ControlModifier))
				{
					setZoom(getZoom() + (wheelEvent->delta() / 16));

					event->accept();

					return true;
				}
			}
		}
	}

	return QObject::eventFilter(object, event);
}

}
