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
#include "../../../../core/SearchEnginesManager.h"
#include "../../../../core/Transfer.h"
#include "../../../../core/TransfersManager.h"
#include "../../../../core/Utils.h"
#include "../../../../core/WebBackend.h"
#include "../../../../ui/AuthenticationDialog.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/ContentsWidget.h"
#include "../../../../ui/ImagePropertiesDialog.h"
#include "../../../../ui/SearchEnginePropertiesDialog.h"
#include "../../../../ui/SourceViewerWebWidget.h"
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
	m_page(new QtWebEnginePage(isPrivate, this)),
	m_childWidget(NULL),
	m_iconReply(NULL),
	m_scrollTimer(startTimer(1000)),
	m_isEditing(false),
	m_isLoading(false),
	m_isTyped(false)
{
	m_webView->setPage(m_page);
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
	connect(m_page, SIGNAL(loadProgress(int)), this, SIGNAL(loadProgress(int)));
	connect(m_page, SIGNAL(loadStarted()), this, SLOT(pageLoadStarted()));
	connect(m_page, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
	connect(m_page, SIGNAL(linkHovered(QString)), this, SLOT(linkHovered(QString)));
	connect(m_page, SIGNAL(iconUrlChanged(QUrl)), this, SLOT(handleIconChange(QUrl)));
	connect(m_page, SIGNAL(authenticationRequired(QUrl,QAuthenticator*)), this, SLOT(handleAuthenticationRequired(QUrl,QAuthenticator*)));
	connect(m_page, SIGNAL(proxyAuthenticationRequired(QUrl,QAuthenticator*,QString)), this, SLOT(handleProxyAuthenticationRequired(QUrl,QAuthenticator*,QString)));
	connect(m_page, SIGNAL(windowCloseRequested()), this, SLOT(handleWindowCloseRequest()));
	connect(m_page, SIGNAL(featurePermissionRequested(QUrl,QWebEnginePage::Feature)), this, SLOT(handlePermissionRequest(QUrl,QWebEnginePage::Feature)));
	connect(m_page, SIGNAL(featurePermissionRequestCanceled(QUrl,QWebEnginePage::Feature)), this, SLOT(handlePermissionCancel(QUrl,QWebEnginePage::Feature)));
	connect(m_page, SIGNAL(viewingMediaChanged(bool)), this, SLOT(updateNavigationActions()));
	connect(m_page->profile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)), this, SLOT(downloadFile(QWebEngineDownloadItem*)));
	connect(m_webView, SIGNAL(titleChanged(QString)), this, SLOT(notifyTitleChanged()));
	connect(m_webView, SIGNAL(urlChanged(QUrl)), this, SLOT(notifyUrlChanged(QUrl)));
	connect(m_webView, SIGNAL(iconUrlChanged(QUrl)), this, SLOT(notifyIconChanged()));
}

void QtWebEngineWebWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_scrollTimer)
	{
		m_webView->page()->runJavaScript(QLatin1String("[window.scrollX, window.scrollY]"), invoke(this, &QtWebEngineWebWidget::handleScroll));
	}
	else
	{
		WebWidget::timerEvent(event);
	}
}

void QtWebEngineWebWidget::focusInEvent(QFocusEvent *event)
{
	WebWidget::focusInEvent(event);

	m_webView->setFocus();
}

void QtWebEngineWebWidget::search(const QString &query, const QString &searchEngine)
{
	QNetworkRequest request;
	QNetworkAccessManager::Operation method;
	QByteArray body;

	if (SearchEnginesManager::setupSearchQuery(query, searchEngine, &request, &method, &body))
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

	emit contentStateChanged(getContentState());
	emit loadingChanged(false);
}

void QtWebEngineWebWidget::downloadFile(QWebEngineDownloadItem *item)
{
	startTransfer(new Transfer(item->url(), QString(), (Transfer::CanNotifyOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));

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

void QtWebEngineWebWidget::removeHistoryIndex(int index, bool purge)
{
	Q_UNUSED(purge)

	WindowHistoryInformation history = getHistory();

	if (index < 0 || index >= history.entries.count())
	{
		return;
	}

	history.entries.removeAt(index);

	if (history.index >= index)
	{
		history.index = (history.index - 1);
	}

	setHistory(history);
}

void QtWebEngineWebWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	if (parameters.contains(QLatin1String("isBounced")))
	{
		return;
	}

	switch (identifier)
	{
		case ActionsManager::SaveAction:
			{
				const QString path = TransfersManager::getSavePath(suggestSaveFileName());

				if (!path.isEmpty())
				{
					QNetworkRequest request(getUrl());
					request.setHeader(QNetworkRequest::UserAgentHeader, m_webView->page()->profile()->httpUserAgent());

					new Transfer(request, path, (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::CanOverwriteOption | Transfer::IsPrivateOption));
				}
			}

			return;
		case ActionsManager::PurgeTabHistoryAction:
///TODO
		case ActionsManager::ClearTabHistoryAction:
			setUrl(QUrl(QLatin1String("about:blank")));

			m_page->history()->clear();

			updateNavigationActions();

			return;
		case ActionsManager::OpenLinkAction:
			{
				QMouseEvent mousePressEvent(QEvent::MouseButtonPress, QPointF(getClickPosition()), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
				QMouseEvent mouseReleaseEvent(QEvent::MouseButtonRelease, QPointF(getClickPosition()), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

				QCoreApplication::sendEvent(m_webView, &mousePressEvent);
				QCoreApplication::sendEvent(m_webView, &mouseReleaseEvent);

				setClickPosition(QPoint());
			}

			return;
		case ActionsManager::OpenLinkInCurrentTabAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, WindowsManager::CurrentTabOpen);
			}

			return;
		case ActionsManager::OpenLinkInNewTabAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, WindowsManager::calculateOpenHints(QGuiApplication::keyboardModifiers(), Qt::LeftButton, WindowsManager::NewTabOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewWindowAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, WindowsManager::calculateOpenHints(QGuiApplication::keyboardModifiers(), Qt::LeftButton, WindowsManager::NewWindowOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, (WindowsManager::NewWindowOpen | WindowsManager::BackgroundOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateTabAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, (WindowsManager::NewTabOpen | WindowsManager::PrivateOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateTabBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen | WindowsManager::PrivateOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateWindowAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, (WindowsManager::NewWindowOpen | WindowsManager::PrivateOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, (WindowsManager::NewWindowOpen | WindowsManager::BackgroundOpen | WindowsManager::PrivateOpen));
			}

			return;
		case ActionsManager::CopyLinkToClipboardAction:
			if (!m_hitResult.linkUrl.isEmpty())
			{
				QGuiApplication::clipboard()->setText(m_hitResult.linkUrl.toString());
			}

			return;
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

			return;
		case ActionsManager::SaveLinkToDiskAction:
			startTransfer(new Transfer(m_hitResult.linkUrl.toString(), QString(), (Transfer::CanNotifyOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));

			return;
		case ActionsManager::SaveLinkToDownloadsAction:
			TransfersManager::addTransfer(new Transfer(m_hitResult.linkUrl.toString(), QString(), (Transfer::CanNotifyOption | Transfer::CanAskForPathOption | Transfer::IsQuickTransferOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));

			return;
		case ActionsManager::OpenFrameInCurrentTabAction:
			if (m_hitResult.frameUrl.isValid())
			{
				setUrl(m_hitResult.frameUrl, false);
			}

			return;
		case ActionsManager::OpenFrameInNewTabAction:
			if (m_hitResult.frameUrl.isValid())
			{
				openUrl(m_hitResult.frameUrl, WindowsManager::calculateOpenHints(QGuiApplication::keyboardModifiers(), Qt::LeftButton, WindowsManager::NewTabOpen));
			}

			return;
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
			if (m_hitResult.frameUrl.isValid())
			{
				openUrl(m_hitResult.frameUrl, (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen));
			}

			return;
		case ActionsManager::CopyFrameLinkToClipboardAction:
			if (m_hitResult.frameUrl.isValid())
			{
				QGuiApplication::clipboard()->setText(m_hitResult.frameUrl.toString());
			}

			return;
		case ActionsManager::ReloadFrameAction:
			if (m_hitResult.frameUrl.isValid())
			{
//TODO Improve
				m_webView->page()->runJavaScript(QStringLiteral("var frames = document.querySelectorAll('iframe[src=\"%1\"], frame[src=\"%1\"]'); for (var i = 0; i < frames.length; ++i) { frames[i].src = ''; frames[i].src = \'%1\'; }").arg(m_hitResult.frameUrl.toString()));
			}

			return;
		case ActionsManager::ViewFrameSourceAction:
			if (m_hitResult.frameUrl.isValid())
			{
				const QString defaultEncoding = m_webView->page()->settings()->defaultTextEncoding();
				QNetworkRequest request(m_hitResult.frameUrl);
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
				request.setHeader(QNetworkRequest::UserAgentHeader, m_webView->page()->profile()->httpUserAgent());

				QNetworkReply *reply = NetworkManagerFactory::getNetworkManager()->get(request);
				SourceViewerWebWidget *sourceViewer = new SourceViewerWebWidget(isPrivate());
				sourceViewer->setRequestedUrl(QUrl(QLatin1String("view-source:") + m_hitResult.frameUrl.toString()));

				if (!defaultEncoding.isEmpty())
				{
					sourceViewer->setOption(QLatin1String("Content/DefaultCharacterEncoding"), defaultEncoding);
				}

				m_viewSourceReplies[reply] = sourceViewer;

				connect(reply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));
				connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(viewSourceReplyFinished(QNetworkReply::NetworkError)));

				emit requestedNewWindow(sourceViewer, WindowsManager::DefaultOpen);
			}

			return;
		case ActionsManager::OpenImageInNewTabAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				openUrl(m_hitResult.imageUrl, WindowsManager::calculateOpenHints(QGuiApplication::keyboardModifiers(), Qt::LeftButton, WindowsManager::NewTabOpen));
			}

			return;
		case ActionsManager::OpenImageInNewTabBackgroundAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				openUrl(m_hitResult.imageUrl, (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen));
			}

			return;
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

					new Transfer(request, QString(), (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::IsPrivateOption));
				}
			}

			return;
		case ActionsManager::CopyImageUrlToClipboardAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				QApplication::clipboard()->setText(m_hitResult.imageUrl.toString());
			}

			return;
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

			return;
		case ActionsManager::ImagePropertiesAction:
			if (m_hitResult.imageUrl.scheme() == QLatin1String("data"))
			{
				QVariantMap properties;
				properties[QLatin1String("alternativeText")] = m_hitResult.alternateText;
				properties[QLatin1String("longDescription")] = m_hitResult.longDescription;

				ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());

				if (parent)
				{
					ImagePropertiesDialog *imagePropertiesDialog = new ImagePropertiesDialog(m_hitResult.imageUrl, properties, NULL, this);
					imagePropertiesDialog->setButtonsVisible(false);

					ContentsDialog *dialog = new ContentsDialog(Utils::getIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), QString(), QString(), QDialogButtonBox::Close, imagePropertiesDialog, this);

					connect(this, SIGNAL(aboutToReload()), dialog, SLOT(close()));
					connect(imagePropertiesDialog, SIGNAL(finished(int)), dialog, SLOT(close()));

					showDialog(dialog, false);
				}
			}
			else
			{
				m_webView->page()->runJavaScript(QStringLiteral("var element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement); if (element && element.tagName && element.tagName.toLowerCase() == 'img') { [element.naturalWidth, element.naturalHeight]; }").arg(getClickPosition().x() / m_webView->zoomFactor()).arg(getClickPosition().y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleImageProperties));
			}

			return;
		case ActionsManager::SaveMediaToDiskAction:
			if (m_hitResult.mediaUrl.isValid())
			{
				QNetworkRequest request(m_hitResult.mediaUrl);
				request.setHeader(QNetworkRequest::UserAgentHeader, m_webView->page()->profile()->httpUserAgent());

				new Transfer(request, QString(), (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::IsPrivateOption));
			}

			return;
		case ActionsManager::CopyMediaUrlToClipboardAction:
			if (!m_hitResult.mediaUrl.isEmpty())
			{
				QApplication::clipboard()->setText(m_hitResult.mediaUrl.toString());
			}

			return;
		case ActionsManager::MediaControlsAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { element.controls = %3; }").arg(getClickPosition().x() / m_webView->zoomFactor()).arg(getClickPosition().y() / m_webView->zoomFactor()).arg(parameters.value(QLatin1String("isChecked")).toBool() ? QLatin1String("true") : QLatin1String("false")));

			return;
		case ActionsManager::MediaLoopAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { element.loop = %3; }").arg(getClickPosition().x() / m_webView->zoomFactor()).arg(getClickPosition().y() / m_webView->zoomFactor()).arg(parameters.value(QLatin1String("isChecked")).toBool() ? QLatin1String("true") : QLatin1String("false")));

			return;
		case ActionsManager::MediaPlayPauseAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { if (element.paused) { element.play(); } else { element.pause(); } }").arg(getClickPosition().x() / m_webView->zoomFactor()).arg(getClickPosition().y() / m_webView->zoomFactor()));

			return;
		case ActionsManager::MediaMuteAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { element.muted = !element.muted; }").arg(getClickPosition().x() / m_webView->zoomFactor()).arg(getClickPosition().y() / m_webView->zoomFactor()));

			return;
		case ActionsManager::GoBackAction:
			m_webView->triggerPageAction(QWebEnginePage::Back);

			return;
		case ActionsManager::GoForwardAction:
			m_webView->triggerPageAction(QWebEnginePage::Forward);

			return;
		case ActionsManager::RewindAction:
			m_webView->history()->goToItem(m_webView->history()->itemAt(0));

			return;
		case ActionsManager::FastForwardAction:
			m_webView->history()->goToItem(m_webView->history()->itemAt(m_webView->history()->count() - 1));

			return;
		case ActionsManager::StopAction:
			m_webView->triggerPageAction(QWebEnginePage::Stop);

			return;
		case ActionsManager::ReloadAction:
			emit aboutToReload();

			m_webView->triggerPageAction(QWebEnginePage::Stop);
			m_webView->triggerPageAction(QWebEnginePage::Reload);

			return;
		case ActionsManager::ReloadOrStopAction:
			if (isLoading())
			{
				triggerAction(ActionsManager::StopAction);
			}
			else
			{
				triggerAction(ActionsManager::ReloadAction);
			}

			return;
		case ActionsManager::ReloadAndBypassCacheAction:
			m_webView->triggerPageAction(QWebEnginePage::ReloadAndBypassCache);

			return;
		case ActionsManager::ContextMenuAction:
			showContextMenu(getClickPosition());

			return;
		case ActionsManager::UndoAction:
			m_webView->triggerPageAction(QWebEnginePage::Undo);

			return;
		case ActionsManager::RedoAction:
			m_webView->triggerPageAction(QWebEnginePage::Redo);

			return;
		case ActionsManager::CutAction:
			m_webView->triggerPageAction(QWebEnginePage::Cut);

			return;
		case ActionsManager::CopyAction:
			m_webView->triggerPageAction(QWebEnginePage::Copy);

			return;
		case ActionsManager::CopyPlainTextAction:
			{
				const QString text = getSelectedText();

				if (!text.isEmpty())
				{
					QApplication::clipboard()->setText(text);
				}
			}

			return;
		case ActionsManager::CopyAddressAction:
			QApplication::clipboard()->setText(getUrl().toString());

			return;
		case ActionsManager::CopyToNoteAction:
			{
				BookmarksItem *note = NotesManager::addNote(BookmarksModel::UrlBookmark, getUrl());
				note->setData(getSelectedText(), BookmarksModel::DescriptionRole);
			}

			return;
		case ActionsManager::PasteAction:
			m_webView->triggerPageAction(QWebEnginePage::Paste);

			return;
		case ActionsManager::DeleteAction:
			m_webView->page()->runJavaScript(QLatin1String("window.getSelection().deleteFromDocument()"));

			return;
		case ActionsManager::SelectAllAction:
			m_webView->triggerPageAction(QWebEnginePage::SelectAll);

			return;
		case ActionsManager::ClearAllAction:
			triggerAction(ActionsManager::SelectAllAction);
			triggerAction(ActionsManager::DeleteAction);

			return;
		case ActionsManager::SearchAction:
			quickSearch(getAction(ActionsManager::SearchAction));

			return;
		case ActionsManager::CreateSearchAction:
			{
				QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/createSearch.js"));
				file.open(QIODevice::ReadOnly);

				m_webView->page()->runJavaScript(QString(file.readAll()).arg(getClickPosition().x() / m_webView->zoomFactor()).arg(getClickPosition().y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleCreateSearch));

				file.close();
			}

			return;
		case ActionsManager::ScrollToStartAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollTo(0, 0)"));

			return;
		case ActionsManager::ScrollToEndAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollTo(0, document.body.scrollHeigh)"));

			return;
		case ActionsManager::ScrollPageUpAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollByPages(1)"));

			return;
		case ActionsManager::ScrollPageDownAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollByPages(-1)"));

			return;
		case ActionsManager::ScrollPageLeftAction:
			m_webView->page()->runJavaScript(QStringLiteral("window.scrollBy(-%1, 0)").arg(m_webView->width()));

			return;
		case ActionsManager::ScrollPageRightAction:
			m_webView->page()->runJavaScript(QStringLiteral("window.scrollBy(%1, 0)").arg(m_webView->width()));

			return;
		case ActionsManager::ActivateContentAction:
			{
				m_webView->setFocus();
				m_webView->page()->runJavaScript(QLatin1String("var element = document.activeElement; if (element && element.tagName && (element.tagName.toLowerCase() == 'input' || element.tagName.toLowerCase() == 'textarea'))) { document.activeElement.blur(); }"));
			}

			return;
		case ActionsManager::BookmarkPageAction:
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

			return;
		case ActionsManager::ViewSourceAction:
			if (canViewSource())
			{
				const QString defaultEncoding = m_webView->page()->settings()->defaultTextEncoding();
				QNetworkRequest request(getUrl());
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
				request.setHeader(QNetworkRequest::UserAgentHeader, m_webView->page()->profile()->httpUserAgent());

				QNetworkReply *reply = NetworkManagerFactory::getNetworkManager()->get(request);
				SourceViewerWebWidget *sourceViewer = new SourceViewerWebWidget(isPrivate());
				sourceViewer->setRequestedUrl(QUrl(QLatin1String("view-source:") + getUrl().toString()));

				if (!defaultEncoding.isEmpty())
				{
					sourceViewer->setOption(QLatin1String("Content/DefaultCharacterEncoding"), defaultEncoding);
				}

				m_viewSourceReplies[reply] = sourceViewer;

				connect(reply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));
				connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(viewSourceReplyFinished(QNetworkReply::NetworkError)));

				emit requestedNewWindow(sourceViewer, WindowsManager::DefaultOpen);
			}

			return;
		case ActionsManager::WebsitePreferencesAction:
			{
				const QUrl url(getUrl());
				WebsitePreferencesDialog dialog(url, QList<QNetworkCookie>(), this);

				if (dialog.exec() == QDialog::Accepted)
				{
					updateOptions(getUrl());
				}
			}

			return;
		default:
			break;
	}

	bounceAction(identifier, parameters);
}

void QtWebEngineWebWidget::openUrl(const QUrl &url, WindowsManager::OpenHints hints)
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

void QtWebEngineWebWidget::viewSourceReplyFinished(QNetworkReply::NetworkError error)
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (error == QNetworkReply::NoError && m_viewSourceReplies.contains(reply) && m_viewSourceReplies[reply])
	{
		m_viewSourceReplies[reply]->setContents(reply->readAll(), reply->header(QNetworkRequest::ContentTypeHeader).toString());
	}

	m_viewSourceReplies.remove(reply);

	reply->deleteLater();
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

void QtWebEngineWebWidget::handleCreateSearch(const QVariant &result)
{
	if (result.isNull())
	{
		return;
	}

	const QUrlQuery parameters(result.toMap().value(QLatin1String("query")).toString());
	const QStringList identifiers = SearchEnginesManager::getSearchEngines();
	const QStringList keywords = SearchEnginesManager::getSearchKeywords();
	const QIcon icon = getIcon();
	const QUrl url(result.toMap().value(QLatin1String("url")).toString());
	SearchEnginesManager::SearchEngineDefinition searchEngine;
	searchEngine.identifier = Utils::createIdentifier(getUrl().host(), identifiers);
	searchEngine.title = getTitle();
	searchEngine.formUrl = getUrl();
	searchEngine.icon = (icon.isNull() ? Utils::getIcon(QLatin1String("edit-find")) : icon);
	searchEngine.resultsUrl.url = (url.isEmpty() ? getUrl() : (url.isRelative() ? getUrl().resolved(url) : url)).toString();
	searchEngine.resultsUrl.enctype = result.toMap().value(QLatin1String("enctype")).toString();
	searchEngine.resultsUrl.method = result.toMap().value(QLatin1String("method")).toString();
	searchEngine.resultsUrl.parameters = parameters;

	SearchEnginePropertiesDialog dialog(searchEngine, keywords, false, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	SearchEnginesManager::addSearchEngine(dialog.getSearchEngine(), dialog.isDefault());
}

void QtWebEngineWebWidget::handleEditingCheck(const QVariant &result)
{
	m_isEditing = result.toBool();

	emit unlockEventLoop();
}

void QtWebEngineWebWidget::handleHitTest(const QVariant &result)
{
	m_hitResult = HitTestResult(result);

	emit hitTestResultReady();
	emit unlockEventLoop();
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

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), QString(), QString(), QDialogButtonBox::Close, imagePropertiesDialog, this);

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
	emit urlChanged((url.toString() == QLatin1String("about:blank")) ? m_webView->page()->requestedUrl() : url);

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
	Action *action = getExistingAction(ActionsManager::UndoAction);

	if (action)
	{
		action->setEnabled(m_webView->pageAction(QWebEnginePage::Undo)->isEnabled());
		action->setText(m_webView->pageAction(QWebEnginePage::Undo)->text());
	}
}

void QtWebEngineWebWidget::updateRedo()
{
	Action *action = getExistingAction(ActionsManager::RedoAction);

	if (action)
	{
		action->setEnabled(m_webView->pageAction(QWebEnginePage::Redo)->isEnabled());
		action->setText(m_webView->pageAction(QWebEnginePage::Redo)->text());
	}
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
		m_webView->setZoomFactor(qBound(0.1, (static_cast<qreal>(zoom) / 100), static_cast<qreal>(100)));

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

WebWidget* QtWebEngineWebWidget::clone(bool cloneHistory, bool isPrivate)
{
	QtWebEngineWebWidget *widget = new QtWebEngineWebWidget((this->isPrivate() || isPrivate), getBackend());
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
	if (identifier == ActionsManager::UndoAction && !getExistingAction(ActionsManager::UndoAction))
	{
		Action *action = WebWidget::getAction(ActionsManager::UndoAction);

		updateUndo();

		connect(m_webView->pageAction(QWebEnginePage::Undo), SIGNAL(changed()), this, SLOT(updateUndo()));

		return action;
	}

	if (identifier == ActionsManager::RedoAction && !getExistingAction(ActionsManager::RedoAction))
	{
		Action *action = WebWidget::getAction(ActionsManager::RedoAction);

		updateRedo();

		connect(m_webView->pageAction(QWebEnginePage::Redo), SIGNAL(changed()), this, SLOT(updateRedo()));

		return action;
	}

	if (identifier == ActionsManager::InspectElementAction && !getExistingAction(ActionsManager::InspectElementAction))
	{
		Action *action = WebWidget::getAction(ActionsManager::InspectElementAction);
		action->setEnabled(false);

		return action;
	}

	return WebWidget::getAction(identifier);
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

	return ((url.isEmpty() || url.toString() == QLatin1String("about:blank")) ? m_webView->page()->requestedUrl() : url);
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

WebWidget::HitTestResult QtWebEngineWebWidget::getHitTestResult(const QPoint &position)
{
	QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
	file.open(QIODevice::ReadOnly);

	QEventLoop eventLoop;

	m_webView->page()->runJavaScript(QString(file.readAll()).arg(position.x() / m_webView->zoomFactor()).arg(position.y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleHitTest));

	file.close();

	connect(this, SIGNAL(unlockEventLoop()), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

	eventLoop.exec();

	return m_hitResult;
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

bool QtWebEngineWebWidget::canGoBack() const
{
	return m_webView->history()->canGoBack();
}

bool QtWebEngineWebWidget::canGoForward() const
{
	return m_webView->history()->canGoForward();
}

bool QtWebEngineWebWidget::canShowContextMenu(const QPoint &position) const
{
	Q_UNUSED(position)

	return true;
}

bool QtWebEngineWebWidget::canViewSource() const
{
	return !m_page->isViewingMedia();
}

bool QtWebEngineWebWidget::hasSelection() const
{
	return m_webView->hasSelection();
}

bool QtWebEngineWebWidget::isLoading() const
{
	return m_isLoading;
}

bool QtWebEngineWebWidget::isPrivate() const
{
	return m_webView->page()->profile()->isOffTheRecord();
}

bool QtWebEngineWebWidget::isScrollBar(const QPoint &position) const
{
	Q_UNUSED(position)

	return false;
}

bool QtWebEngineWebWidget::findInPage(const QString &text, FindFlags flags)
{
	QWebEnginePage::FindFlags nativeFlags = 0;

	if (flags.testFlag(BackwardFind))
	{
		nativeFlags |= QWebEnginePage::FindBackward;
	}

	if (flags.testFlag(CaseSensitiveFind))
	{
		nativeFlags |= QWebEnginePage::FindCaseSensitively;
	}

	m_webView->findText(text, nativeFlags);

	return false;
}

bool QtWebEngineWebWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_webView && event->type() == QEvent::ChildAdded)
	{
		QChildEvent *childEvent = static_cast<QChildEvent*>(event);

		if (childEvent->child())
		{
			childEvent->child()->installEventFilter(this);

			m_childWidget = qobject_cast<QWidget*>(childEvent->child());
		}
	}
	else if (object == m_webView && (event->type() == QEvent::Move || event->type() == QEvent::Resize))
	{
		emit progressBarGeometryChanged();
	}
	else if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);

		if (helpEvent)
		{
			handleToolTipEvent(helpEvent, m_webView);
		}

		return true;
	}
	else if (event->type() == QEvent::ShortcutOverride)
	{
		QEventLoop eventLoop;

		m_webView->page()->runJavaScript(QLatin1String("var element = document.body.querySelector(':focus'); var tagName = (element ? element.tagName.toLowerCase() : ''); var result = false; if (tagName == 'textarea' || tagName == 'input') { var type = (element.type ? element.type.toLowerCase() : ''); if ((type == '' || tagName == 'textarea' || type == 'text' || type == 'search') && !element.hasAttribute('readonly') && !element.hasAttribute('disabled')) { result = true; } } result;"), invoke(this, &QtWebEngineWebWidget::handleEditingCheck));

		connect(this, SIGNAL(unlockEventLoop()), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();

		if (m_isEditing)
		{
			event->accept();

			return true;
		}
	}

	return QObject::eventFilter(object, event);
}

}
