/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../../core/ThemesManager.h"
#include "../../../../core/TransfersManager.h"
#include "../../../../core/UserScript.h"
#include "../../../../core/Utils.h"
#include "../../../../core/WebBackend.h"
#include "../../../../ui/AuthenticationDialog.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/ContentsWidget.h"
#include "../../../../ui/ImagePropertiesDialog.h"
#include "../../../../ui/MainWindow.h"
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
#include <QtWebEngineCore/QWebEngineCookieStore>
#include <QtWebEngineWidgets/QWebEngineHistory>
#include <QtWebEngineWidgets/QWebEngineProfile>
#include <QtWebEngineWidgets/QWebEngineScript>
#include <QtWebEngineWidgets/QWebEngineSettings>
#include <QtWidgets/QApplication>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

QtWebEngineWebWidget::QtWebEngineWebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent) : WebWidget(isPrivate, backend, parent),
	m_webView(nullptr),
	m_page(new QtWebEnginePage(isPrivate, this)),
	m_iconReply(nullptr),
	m_loadingTime(nullptr),
	m_loadingState(WindowsManager::FinishedLoadingState),
	m_documentLoadingProgress(0),
	m_focusProxyTimer(0),
#if QT_VERSION < 0x050700
	m_scrollTimer(startTimer(1000)),
#else
	m_scrollTimer(0),
#endif
	m_updateNavigationActionsTimer(0),
	m_isEditing(false),
	m_isFullScreen(false),
	m_isTyped(false)
{
	setFocusPolicy(Qt::StrongFocus);

	connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(updateBookmarkActions()));
	connect(m_page, SIGNAL(loadProgress(int)), this, SLOT(notifyDocumentLoadingProgress(int)));
	connect(m_page, SIGNAL(loadStarted()), this, SLOT(pageLoadStarted()));
	connect(m_page, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
	connect(m_page, SIGNAL(linkHovered(QString)), this, SLOT(linkHovered(QString)));
#if QT_VERSION < 0x050700
	connect(m_page, SIGNAL(iconUrlChanged(QUrl)), this, SLOT(handleIconChange(QUrl)));
#else
	connect(m_page, SIGNAL(iconChanged(QIcon)), this, SIGNAL(iconChanged(QIcon)));
#endif
	connect(m_page, SIGNAL(requestedPopupWindow(QUrl,QUrl)), this, SIGNAL(requestedPopupWindow(QUrl,QUrl)));
	connect(m_page, SIGNAL(aboutToNavigate(QUrl,QWebEnginePage::NavigationType)), this, SIGNAL(aboutToNavigate()));
	connect(m_page, SIGNAL(requestedNewWindow(WebWidget*,WindowsManager::OpenHints)), this, SIGNAL(requestedNewWindow(WebWidget*,WindowsManager::OpenHints)));
	connect(m_page, SIGNAL(authenticationRequired(QUrl,QAuthenticator*)), this, SLOT(handleAuthenticationRequired(QUrl,QAuthenticator*)));
	connect(m_page, SIGNAL(proxyAuthenticationRequired(QUrl,QAuthenticator*,QString)), this, SLOT(handleProxyAuthenticationRequired(QUrl,QAuthenticator*,QString)));
	connect(m_page, SIGNAL(windowCloseRequested()), this, SLOT(handleWindowCloseRequest()));
	connect(m_page, SIGNAL(fullScreenRequested(QWebEngineFullScreenRequest)), this, SLOT(handleFullScreenRequest(QWebEngineFullScreenRequest)));
	connect(m_page, SIGNAL(featurePermissionRequested(QUrl,QWebEnginePage::Feature)), this, SLOT(handlePermissionRequest(QUrl,QWebEnginePage::Feature)));
	connect(m_page, SIGNAL(featurePermissionRequestCanceled(QUrl,QWebEnginePage::Feature)), this, SLOT(handlePermissionCancel(QUrl,QWebEnginePage::Feature)));
#if QT_VERSION >= 0x050700
	connect(m_page, SIGNAL(recentlyAudibleChanged(bool)), this, SLOT(handleAudibleStateChange(bool)));
#endif
	connect(m_page, SIGNAL(viewingMediaChanged(bool)), this, SLOT(updateNavigationActions()));
	connect(m_page, SIGNAL(titleChanged(QString)), this, SLOT(notifyTitleChanged()));
	connect(m_page, SIGNAL(urlChanged(QUrl)), this, SLOT(notifyUrlChanged(QUrl)));
#if QT_VERSION < 0x050700
	connect(m_page, SIGNAL(iconUrlChanged(QUrl)), this, SLOT(notifyIconChanged()));
#endif
}

void QtWebEngineWebWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_focusProxyTimer)
	{
		if (focusWidget())
		{
			focusWidget()->removeEventFilter(this);
			focusWidget()->installEventFilter(this);
		}
	}
	else if (event->timerId() == m_scrollTimer)
	{
		m_page->runJavaScript(QLatin1String("[window.scrollX, window.scrollY]"), [&](const QVariant &result)
		{
			if (result.isValid())
			{
				m_scrollPosition = QPoint(result.toList()[0].toInt(), result.toList()[1].toInt());
			}
		});
	}
	else if (event->timerId() == m_updateNavigationActionsTimer)
	{
		killTimer(m_updateNavigationActionsTimer);

		m_updateNavigationActionsTimer = 0;

		WebWidget::updateNavigationActions();
	}
	else
	{
		WebWidget::timerEvent(event);
	}
}

void QtWebEngineWebWidget::showEvent(QShowEvent *event)
{
	WebWidget::showEvent(event);

	ensureInitialized();

	if (m_focusProxyTimer == 0)
	{
		m_focusProxyTimer = startTimer(500);
	}
}

void QtWebEngineWebWidget::hideEvent(QHideEvent *event)
{
	WebWidget::hideEvent(event);

	killTimer(m_focusProxyTimer);

	m_focusProxyTimer = 0;
}

void QtWebEngineWebWidget::focusInEvent(QFocusEvent *event)
{
	WebWidget::focusInEvent(event);

	ensureInitialized();

	m_webView->setFocus();
}

void QtWebEngineWebWidget::ensureInitialized()
{
	if (!m_webView)
	{
		m_webView = new QWebEngineView(this);
		m_webView->setPage(m_page);
		m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
		m_webView->installEventFilter(this);

		QVBoxLayout *layout(new QVBoxLayout(this));
		layout->addWidget(m_webView);
		layout->setContentsMargins(0, 0, 0, 0);

		setLayout(layout);

		connect(m_webView, SIGNAL(renderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus,int)), this, SLOT(notifyRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus)));
	}
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

			m_page->runJavaScript(QString(file.readAll()).arg(request.url().toString()).arg(QString(body)));

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
#if QT_VERSION < 0x050800
	ensureInitialized();

	m_webView->render(printer);
#else
	QEventLoop eventLoop;

	m_page->print(printer, [&](bool)
	{
		eventLoop.quit();
	});

	eventLoop.exec();
#endif
}

void QtWebEngineWebWidget::pageLoadStarted()
{
	m_lastUrlClickTime = QDateTime();
	m_loadingState = WindowsManager::OngoingLoadingState;
	m_documentLoadingProgress = 0;

	if (!m_loadingTime)
	{
		m_loadingTime = new QTime();
		m_loadingTime->start();
	}

	setStatusMessage(QString());
	setStatusMessage(QString(), true);

	emit progressBarGeometryChanged();
	emit loadingStateChanged(WindowsManager::OngoingLoadingState);
	emit pageInformationChanged(DocumentLoadingProgressInformation, 0);
}

void QtWebEngineWebWidget::pageLoadFinished()
{
	m_loadingState = WindowsManager::FinishedLoadingState;

	updateNavigationActions();
	startReloadTimer();

	emit contentStateChanged(getContentState());
	emit loadingStateChanged(WindowsManager::FinishedLoadingState);
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

void QtWebEngineWebWidget::goToHistoryIndex(int index)
{
	m_page->history()->goToItem(m_page->history()->itemAt(index));
}

void QtWebEngineWebWidget::removeHistoryIndex(int index, bool purge)
{
	Q_UNUSED(purge)

	WindowHistoryInformation history(getHistory());

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
#if QT_VERSION >= 0x050700
			m_page->triggerAction(QWebEnginePage::SavePage);
#else

			{
				const QString path(Utils::getSavePath(suggestSaveFileName(SingleHtmlFileSaveFormat)).path);

				if (!path.isEmpty())
				{
					QNetworkRequest request(getUrl());
					request.setHeader(QNetworkRequest::UserAgentHeader, m_page->profile()->httpUserAgent());

					new Transfer(request, path, (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::CanOverwriteOption | Transfer::IsPrivateOption));
				}
			}
#endif

			return;
		case ActionsManager::PurgeTabHistoryAction:
///TODO
		case ActionsManager::ClearTabHistoryAction:
			setUrl(QUrl(QLatin1String("about:blank")));

			m_page->history()->clear();

			updateNavigationActions();

			return;
#if QT_VERSION >= 0x050700
		case ActionsManager::MuteTabMediaAction:
			m_page->setAudioMuted(!m_page->isAudioMuted());

			handleAudibleStateChange(m_page->recentlyAudible());

			return;
#endif
		case ActionsManager::OpenLinkAction:
			{
				ensureInitialized();

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
				openUrl(m_hitResult.linkUrl, WindowsManager::calculateOpenHints(WindowsManager::NewTabOpen));
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
				openUrl(m_hitResult.linkUrl, WindowsManager::calculateOpenHints(WindowsManager::NewWindowOpen));
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
				QGuiApplication::clipboard()->setText(m_hitResult.linkUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
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
				openUrl(m_hitResult.frameUrl, WindowsManager::calculateOpenHints(WindowsManager::NewTabOpen));
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
				QGuiApplication::clipboard()->setText(m_hitResult.frameUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			return;
		case ActionsManager::ReloadFrameAction:
			if (m_hitResult.frameUrl.isValid())
			{
//TODO Improve
				m_page->runJavaScript(QStringLiteral("var frames = document.querySelectorAll('iframe[src=\"%1\"], frame[src=\"%1\"]'); for (var i = 0; i < frames.length; ++i) { frames[i].src = ''; frames[i].src = \'%1\'; }").arg(m_hitResult.frameUrl.toString()));
			}

			return;
		case ActionsManager::ViewFrameSourceAction:
			if (m_hitResult.frameUrl.isValid())
			{
				const QString defaultEncoding(m_page->settings()->defaultTextEncoding());
				QNetworkRequest request(m_hitResult.frameUrl);
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
				request.setHeader(QNetworkRequest::UserAgentHeader, m_page->profile()->httpUserAgent());

				QNetworkReply *reply(NetworkManagerFactory::getNetworkManager()->get(request));
				SourceViewerWebWidget *sourceViewer(new SourceViewerWebWidget(isPrivate()));
				sourceViewer->setRequestedUrl(QUrl(QLatin1String("view-source:") + m_hitResult.frameUrl.toString()), false);

				if (!defaultEncoding.isEmpty())
				{
					sourceViewer->setOption(SettingsManager::Content_DefaultCharacterEncodingOption, defaultEncoding);
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
				openUrl(m_hitResult.imageUrl, WindowsManager::calculateOpenHints(WindowsManager::NewTabOpen));
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
					const QString imageUrl(m_hitResult.imageUrl.url());
					const QString imageType(imageUrl.mid(11, (imageUrl.indexOf(QLatin1Char(';')) - 11)));
					const QString path(Utils::getSavePath(tr("file") + QLatin1Char('.') + imageType).path);

					if (path.isEmpty())
					{
						return;
					}

					QImageWriter writer(path);

					if (!writer.write(QImage::fromData(QByteArray::fromBase64(imageUrl.mid(imageUrl.indexOf(QLatin1String(";base64,")) + 7).toUtf8()), imageType.toStdString().c_str())))
					{
						Console::addMessage(tr("Failed to save image: %1").arg(writer.errorString()), Console::OtherCategory, Console::ErrorLevel, path, -1, getWindowIdentifier());
					}
				}
				else
				{
					QNetworkRequest request(m_hitResult.imageUrl);
					request.setHeader(QNetworkRequest::UserAgentHeader, m_page->profile()->httpUserAgent());

					new Transfer(request, QString(), (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::IsPrivateOption));
				}
			}

			return;
		case ActionsManager::CopyImageToClipboardAction:
			m_page->triggerAction(QWebEnginePage::CopyImageToClipboard);

			return;
		case ActionsManager::CopyImageUrlToClipboardAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				QApplication::clipboard()->setText(m_hitResult.imageUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
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
					m_page->runJavaScript(QStringLiteral("var images = document.querySelectorAll('img[src=\"%1\"]'); for (var i = 0; i < images.length; ++i) { images[i].src = ''; images[i].src = \'%1\'; }").arg(m_hitResult.imageUrl.toString()));
				}
			}

			return;
		case ActionsManager::ImagePropertiesAction:
			if (m_hitResult.imageUrl.scheme() == QLatin1String("data"))
			{
				QVariantMap properties;
				properties[QLatin1String("alternativeText")] = m_hitResult.alternateText;
				properties[QLatin1String("longDescription")] = m_hitResult.longDescription;

				ImagePropertiesDialog *imagePropertiesDialog(new ImagePropertiesDialog(m_hitResult.imageUrl, properties, nullptr, this));
				imagePropertiesDialog->setButtonsVisible(false);

				ContentsDialog *dialog(new ContentsDialog(ThemesManager::getIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), QString(), QString(), QDialogButtonBox::Close, imagePropertiesDialog, this));

				connect(this, SIGNAL(aboutToReload()), dialog, SLOT(close()));
				connect(imagePropertiesDialog, SIGNAL(finished(int)), dialog, SLOT(close()));

				showDialog(dialog, false);
			}
			else
			{
				m_page->runJavaScript(QStringLiteral("var element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement); if (element && element.tagName && element.tagName.toLowerCase() == 'img') { [element.naturalWidth, element.naturalHeight]; }").arg(getClickPosition().x() / m_page->zoomFactor()).arg(getClickPosition().y() / m_page->zoomFactor()), [&](const QVariant &result)
				{
					QVariantMap properties;
					properties[QLatin1String("alternativeText")] = m_hitResult.alternateText;
					properties[QLatin1String("longDescription")] = m_hitResult.longDescription;

					if (result.isValid())
					{
						properties[QLatin1String("width")] = result.toList()[0].toInt();
						properties[QLatin1String("height")] = result.toList()[1].toInt();
					}

					ImagePropertiesDialog *imagePropertiesDialog(new ImagePropertiesDialog(m_hitResult.imageUrl, properties, nullptr, this));
					imagePropertiesDialog->setButtonsVisible(false);

					ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), QString(), QString(), QDialogButtonBox::Close, imagePropertiesDialog, this);

					connect(this, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

					showDialog(&dialog);
				});
			}

			return;
		case ActionsManager::SaveMediaToDiskAction:
			if (m_hitResult.mediaUrl.isValid())
			{
				QNetworkRequest request(m_hitResult.mediaUrl);
				request.setHeader(QNetworkRequest::UserAgentHeader, m_page->profile()->httpUserAgent());

				new Transfer(request, QString(), (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::IsPrivateOption));
			}

			return;
		case ActionsManager::CopyMediaUrlToClipboardAction:
			if (!m_hitResult.mediaUrl.isEmpty())
			{
				QApplication::clipboard()->setText(m_hitResult.mediaUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			return;
		case ActionsManager::MediaControlsAction:
			m_page->triggerAction(QWebEnginePage::ToggleMediaControls, calculateCheckedState(ActionsManager::MediaControlsAction, parameters));

			return;
		case ActionsManager::MediaLoopAction:
			m_page->triggerAction(QWebEnginePage::ToggleMediaLoop, calculateCheckedState(ActionsManager::MediaLoopAction, parameters));

			return;
		case ActionsManager::MediaPlayPauseAction:
			m_page->triggerAction(QWebEnginePage::ToggleMediaPlayPause);

			return;
		case ActionsManager::MediaMuteAction:
			m_page->triggerAction(QWebEnginePage::ToggleMediaMute);

			return;
		case ActionsManager::GoBackAction:
			m_page->triggerAction(QWebEnginePage::Back);

			return;
		case ActionsManager::GoForwardAction:
			m_page->triggerAction(QWebEnginePage::Forward);

			return;
		case ActionsManager::RewindAction:
			m_page->history()->goToItem(m_page->history()->itemAt(0));

			return;
		case ActionsManager::FastForwardAction:
			m_page->runJavaScript(getFastForwardScript(true), [&](const QVariant &result)
			{
				const QUrl url(result.toUrl());

				if (url.isValid())
				{
					setUrl(url);
				}
				else if (canGoForward())
				{
					m_page->triggerAction(QWebEnginePage::Forward);
				}
			});

			return;
		case ActionsManager::StopAction:
			m_page->triggerAction(QWebEnginePage::Stop);

			return;
		case ActionsManager::ReloadAction:
			emit aboutToReload();

			m_page->triggerAction(QWebEnginePage::Stop);
			m_page->triggerAction(QWebEnginePage::Reload);

			return;
		case ActionsManager::ReloadOrStopAction:
			if (m_loadingState == WindowsManager::OngoingLoadingState)
			{
				triggerAction(ActionsManager::StopAction);
			}
			else
			{
				triggerAction(ActionsManager::ReloadAction);
			}

			return;
		case ActionsManager::ReloadAndBypassCacheAction:
			m_page->triggerAction(QWebEnginePage::ReloadAndBypassCache);

			return;
		case ActionsManager::ContextMenuAction:
			showContextMenu(getClickPosition());

			return;
		case ActionsManager::UndoAction:
			m_page->triggerAction(QWebEnginePage::Undo);

			return;
		case ActionsManager::RedoAction:
			m_page->triggerAction(QWebEnginePage::Redo);

			return;
		case ActionsManager::CutAction:
			m_page->triggerAction(QWebEnginePage::Cut);

			return;
		case ActionsManager::CopyAction:
			m_page->triggerAction(QWebEnginePage::Copy);

			return;
		case ActionsManager::CopyPlainTextAction:
			{
				const QString text(getSelectedText());

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
				BookmarksItem *note(NotesManager::addNote(BookmarksModel::UrlBookmark, getUrl()));
				note->setData(getSelectedText(), BookmarksModel::DescriptionRole);
			}

			return;
		case ActionsManager::PasteAction:
			m_page->triggerAction(QWebEnginePage::Paste);

			return;
		case ActionsManager::DeleteAction:
			m_page->runJavaScript(QLatin1String("window.getSelection().deleteFromDocument()"));

			return;
		case ActionsManager::SelectAllAction:
			m_page->triggerAction(QWebEnginePage::SelectAll);

			return;
		case ActionsManager::UnselectAction:
#if QT_VERSION >= 0x050700
			m_page->triggerAction(QWebEnginePage::Unselect);
#else
			m_page->runJavaScript(QLatin1String("window.getSelection().empty()"));
#endif
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

				m_page->runJavaScript(QString(file.readAll()).arg(getClickPosition().x() / m_page->zoomFactor()).arg(getClickPosition().y() / m_page->zoomFactor()), [&](const QVariant &result)
				{
					if (result.isNull())
					{
						return;
					}

					const QUrlQuery parameters(result.toMap().value(QLatin1String("query")).toString());
					const QStringList identifiers(SearchEnginesManager::getSearchEngines());
					const QStringList keywords(SearchEnginesManager::getSearchKeywords());
					const QIcon icon(getIcon());
					const QUrl url(result.toMap().value(QLatin1String("url")).toString());
					SearchEnginesManager::SearchEngineDefinition searchEngine;
					searchEngine.identifier = Utils::createIdentifier(getUrl().host(), identifiers);
					searchEngine.title = getTitle();
					searchEngine.formUrl = getUrl();
					searchEngine.icon = (icon.isNull() ? ThemesManager::getIcon(QLatin1String("edit-find")) : icon);
					searchEngine.resultsUrl.url = (url.isEmpty() ? getUrl() : (url.isRelative() ? getUrl().resolved(url) : url)).toString();
					searchEngine.resultsUrl.enctype = result.toMap().value(QLatin1String("enctype")).toString();
					searchEngine.resultsUrl.method = result.toMap().value(QLatin1String("method")).toString();
					searchEngine.resultsUrl.parameters = parameters;

					SearchEnginePropertiesDialog dialog(searchEngine, keywords, this);

					if (dialog.exec() == QDialog::Rejected)
					{
						return;
					}

					SearchEnginesManager::addSearchEngine(dialog.getSearchEngine());
				});

				file.close();
			}

			return;
		case ActionsManager::ScrollToStartAction:
			m_page->runJavaScript(QLatin1String("window.scrollTo(0, 0)"));

			return;
		case ActionsManager::ScrollToEndAction:
			m_page->runJavaScript(QLatin1String("window.scrollTo(0, document.body.scrollHeigh)"));

			return;
		case ActionsManager::ScrollPageUpAction:
			m_page->runJavaScript(QLatin1String("window.scrollByPages(1)"));

			return;
		case ActionsManager::ScrollPageDownAction:
			m_page->runJavaScript(QLatin1String("window.scrollByPages(-1)"));

			return;
		case ActionsManager::ScrollPageLeftAction:
			ensureInitialized();

			m_page->runJavaScript(QStringLiteral("window.scrollBy(-%1, 0)").arg(m_webView->width()));

			return;
		case ActionsManager::ScrollPageRightAction:
			ensureInitialized();

			m_page->runJavaScript(QStringLiteral("window.scrollBy(%1, 0)").arg(m_webView->width()));

			return;
		case ActionsManager::ActivateContentAction:
			{
				ensureInitialized();

				m_webView->setFocus();

				m_page->runJavaScript(QLatin1String("var element = document.activeElement; if (element && element.tagName && (element.tagName.toLowerCase() == 'input' || element.tagName.toLowerCase() == 'textarea'))) { document.activeElement.blur(); }"));
			}

			return;
		case ActionsManager::BookmarkPageAction:
			{
				const QUrl url(getUrl());

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
				const QString defaultEncoding(m_page->settings()->defaultTextEncoding());
				QNetworkRequest request(getUrl());
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
				request.setHeader(QNetworkRequest::UserAgentHeader, m_page->profile()->httpUserAgent());

				QNetworkReply *reply(NetworkManagerFactory::getNetworkManager()->get(request));
				SourceViewerWebWidget *sourceViewer(new SourceViewerWebWidget(isPrivate()));
				sourceViewer->setRequestedUrl(QUrl(QLatin1String("view-source:") + getUrl().toString()), false);

				if (!defaultEncoding.isEmpty())
				{
					sourceViewer->setOption(SettingsManager::Content_DefaultCharacterEncodingOption, defaultEncoding);
				}

				m_viewSourceReplies[reply] = sourceViewer;

				connect(reply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));
				connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(viewSourceReplyFinished(QNetworkReply::NetworkError)));

				emit requestedNewWindow(sourceViewer, WindowsManager::DefaultOpen);
			}

			return;
		case ActionsManager::FullScreenAction:
			{
				MainWindow *mainWindow(MainWindow::findMainWindow(this));

				if (mainWindow && !mainWindow->isFullScreen())
				{
					m_page->triggerAction(QWebEnginePage::ExitFullScreen);
				}
			}

			return;
		case ActionsManager::WebsitePreferencesAction:
			{
				const QUrl url(getUrl());
				WebsitePreferencesDialog dialog(url, QList<QNetworkCookie>(), this);

				if (dialog.exec() == QDialog::Accepted)
				{
					updateOptions(getUrl());

					const QList<QNetworkCookie> cookiesToDelete(dialog.getCookiesToDelete());

					for (int i = 0; i < cookiesToDelete.count(); ++i)
					{
						m_page->profile()->cookieStore()->deleteCookie(cookiesToDelete.at(i));
					}

					const QList<QNetworkCookie> cookiesToInsert(dialog.getCookiesToInsert());

					for (int i = 0; i < cookiesToInsert.count(); ++i)
					{
						m_page->profile()->cookieStore()->setCookie(cookiesToInsert.at(i));
					}
				}
			}

			return;
		default:
			break;
	}

	bounceAction(identifier, parameters);
}

void QtWebEngineWebWidget::pasteText(const QString &text)
{
	QMimeData *mimeData(new QMimeData());
	const QStringList mimeTypes(QGuiApplication::clipboard()->mimeData()->formats());

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
	m_iconReply = nullptr;
}

void QtWebEngineWebWidget::viewSourceReplyFinished(QNetworkReply::NetworkError error)
{
	QNetworkReply *reply(qobject_cast<QNetworkReply*>(sender()));

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
	request.setHeader(QNetworkRequest::UserAgentHeader, m_page->profile()->httpUserAgent());

	m_iconReply = NetworkManagerFactory::getNetworkManager()->get(request);
	m_iconReply->setParent(this);

	connect(m_iconReply, SIGNAL(finished()), this, SLOT(iconReplyFinished()));
}

void QtWebEngineWebWidget::handleAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator)
{
	AuthenticationDialog *authenticationDialog(new AuthenticationDialog(url, authenticator, AuthenticationDialog::HttpAuthentication, this));
	authenticationDialog->setButtonsVisible(false);

	ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, this);

	connect(&dialog, SIGNAL(accepted(bool)), authenticationDialog, SLOT(accept()));
	connect(this, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	showDialog(&dialog);
}

void QtWebEngineWebWidget::handleProxyAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator, const QString &proxy)
{
	Q_UNUSED(url)

	AuthenticationDialog *authenticationDialog(new AuthenticationDialog(proxy, authenticator, AuthenticationDialog::ProxyAuthentication, this));
	authenticationDialog->setButtonsVisible(false);

	ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, this);

	connect(&dialog, SIGNAL(accepted(bool)), authenticationDialog, SLOT(accept()));
	connect(this, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	showDialog(&dialog);
}

void QtWebEngineWebWidget::handleFullScreenRequest(QWebEngineFullScreenRequest request)
{
	request.accept();

	if (request.toggleOn())
	{
		const QString value(SettingsManager::getValue(SettingsManager::Permissions_EnableFullScreenOption, request.origin()).toString());

		if (value == QLatin1String("allow"))
		{
			MainWindow *mainWindow(MainWindow::findMainWindow(this));

			if (mainWindow && !mainWindow->isFullScreen())
			{
				mainWindow->triggerAction(ActionsManager::FullScreenAction);
			}
		}
		else if (value == QLatin1String("ask"))
		{
			emit requestedPermission(FullScreenFeature, request.origin(), false);
		}
	}
	else
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(this));

		if (mainWindow && mainWindow->isFullScreen())
		{
			mainWindow->triggerAction(ActionsManager::FullScreenAction);
		}

		emit requestedPermission(FullScreenFeature, request.origin(), true);
	}

	m_isFullScreen = request.toggleOn();

	emit isFullScreenChanged(m_isFullScreen);
}

void QtWebEngineWebWidget::handlePermissionRequest(const QUrl &url, QWebEnginePage::Feature feature)
{
	notifyPermissionRequested(url, feature, false);
}

void QtWebEngineWebWidget::handlePermissionCancel(const QUrl &url, QWebEnginePage::Feature feature)
{
	notifyPermissionRequested(url, feature, true);
}

void QtWebEngineWebWidget::handleWindowCloseRequest()
{
	const QString mode(SettingsManager::getValue(SettingsManager::Permissions_ScriptsCanCloseWindowsOption, getUrl()).toString());

	if (mode != QLatin1String("ask"))
	{
		if (mode == QLatin1String("allow"))
		{
			emit requestedCloseWindow();
		}

		return;
	}

	ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-warning")), tr("JavaScript"), tr("Webpage wants to close this tab, do you want to allow to close it?"), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), nullptr, this);
	dialog.setCheckBox(tr("Do not show this message again"), false);

	connect(this, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	showDialog(&dialog);

	if (dialog.getCheckBoxState())
	{
		SettingsManager::setValue(SettingsManager::Permissions_ScriptsCanCloseWindowsOption, (dialog.isAccepted() ? QLatin1String("allow") : QLatin1String("disallow")));
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
	emit urlChanged((url.toString() == QLatin1String("about:blank")) ? m_page->requestedUrl() : url);

	SessionsManager::markSessionModified();
}

void QtWebEngineWebWidget::notifyIconChanged()
{
	emit iconChanged(getIcon());
}

void QtWebEngineWebWidget::notifyPermissionRequested(const QUrl &url, QWebEnginePage::Feature nativeFeature, bool cancel)
{
	FeaturePermission feature(UnknownFeature);

	switch (nativeFeature)
	{
		case QWebEnginePage::Geolocation:
			feature = GeolocationFeature;

			break;
		case QWebEnginePage::MediaAudioCapture:
			feature = CaptureAudioFeature;

			break;
		case QWebEnginePage::MediaVideoCapture:
			feature = CaptureVideoFeature;

			break;
		case QWebEnginePage::MediaAudioVideoCapture:
			feature = CaptureAudioVideoFeature;

			break;
		case QWebEnginePage::Notifications:
			feature = NotificationsFeature;

			break;
		case QWebEnginePage::MouseLock:
			feature = PointerLockFeature;

			break;
		default:
			return;
	}

	if (cancel)
	{
		emit requestedPermission(feature, url, true);
	}
	else
	{
		switch (getPermission(feature, url))
		{
			case GrantedPermission:
				m_page->setFeaturePermission(url, nativeFeature, QWebEnginePage::PermissionGrantedByUser);

				break;
			case DeniedPermission:
				m_page->setFeaturePermission(url, nativeFeature, QWebEnginePage::PermissionDeniedByUser);

				break;
			default:
				emit requestedPermission(feature, url, false);

				break;
		}
	}
}

void QtWebEngineWebWidget::notifyRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus status)
{
	if (status != QWebEnginePage::NormalTerminationStatus)
	{
		m_loadingState = WindowsManager::CrashedLoadingState;

		emit loadingStateChanged(WindowsManager::CrashedLoadingState);
	}
}

void QtWebEngineWebWidget::notifyDocumentLoadingProgress(int progress)
{
	m_documentLoadingProgress = progress;

	emit pageInformationChanged(DocumentLoadingProgressInformation, progress);
}

void QtWebEngineWebWidget::updateNavigationActions()
{
	if (m_updateNavigationActionsTimer == 0)
	{
		m_updateNavigationActionsTimer = startTimer(0);
	}
}

void QtWebEngineWebWidget::updateUndo()
{
	Action *action(getExistingAction(ActionsManager::UndoAction));

	if (action)
	{
		action->setEnabled(m_page->action(QWebEnginePage::Undo)->isEnabled());
		action->setText(m_page->action(QWebEnginePage::Undo)->text());
	}
}

void QtWebEngineWebWidget::updateRedo()
{
	Action *action(getExistingAction(ActionsManager::RedoAction));

	if (action)
	{
		action->setEnabled(m_page->action(QWebEnginePage::Redo)->isEnabled());
		action->setText(m_page->action(QWebEnginePage::Redo)->text());
	}
}

void QtWebEngineWebWidget::updateOptions(const QUrl &url)
{
	const QString encoding(getOption(SettingsManager::Content_DefaultCharacterEncodingOption, url).toString());
	QWebEngineSettings *settings(m_page->settings());
	settings->setAttribute(QWebEngineSettings::AutoLoadImages, (getOption(SettingsManager::Permissions_EnableImagesOption, url).toString() != QLatin1String("onlyCached")));
	settings->setAttribute(QWebEngineSettings::JavascriptEnabled, getOption(SettingsManager::Permissions_EnableJavaScriptOption, url).toBool());
	settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, getOption(SettingsManager::Permissions_ScriptsCanAccessClipboardOption, url).toBool());
	settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, (getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, url).toString() != QLatin1String("blockAll")));
#if QT_VERSION >= 0x050700
	settings->setAttribute(QWebEngineSettings::WebGLEnabled, getOption(SettingsManager::Permissions_EnableWebglOption, url).toBool());
#endif
	settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, getOption(SettingsManager::Permissions_EnableLocalStorageOption, url).toBool());
	settings->setDefaultTextEncoding((encoding == QLatin1String("auto")) ? QString() : encoding);

	m_page->profile()->setHttpUserAgent(getBackend()->getUserAgent(NetworkManagerFactory::getUserAgent(getOption(SettingsManager::Network_UserAgentOption, url).toString()).value));

	disconnect(m_page, SIGNAL(geometryChangeRequested(QRect)), this, SIGNAL(requestedGeometryChange(QRect)));

	if (getOption(SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption, url).toBool())
	{
		connect(m_page, SIGNAL(geometryChangeRequested(QRect)), this, SIGNAL(requestedGeometryChange(QRect)));
	}
}

void QtWebEngineWebWidget::setScrollPosition(const QPoint &position)
{
	m_page->runJavaScript(QStringLiteral("window.scrollTo(%1, %2); [window.scrollX, window.scrollY];").arg(position.x()).arg(position.y()), [&](const QVariant &result)
	{
		if (result.isValid())
		{
			m_scrollPosition = QPoint(result.toList()[0].toInt(), result.toList()[1].toInt());
		}
	});
}

void QtWebEngineWebWidget::setHistory(const WindowHistoryInformation &history)
{
	if (history.entries.count() == 0)
	{
		m_page->history()->clear();

		updateNavigationActions();
		updateOptions(QUrl());
		updatePageActions(QUrl());

		const QList<UserScript*> scripts(UserScript::getUserScriptsForUrl(QUrl(QLatin1String("about:blank"))));

		for (int i = 0; i < scripts.count(); ++i)
		{
#if QT_VERSION >= 0x050700
			m_page->runJavaScript(scripts.at(i)->getSource(), QWebEngineScript::UserWorld);
#else
			m_page->runJavaScript(scripts.at(i)->getSource());
#endif
		}

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
	stream >> *(m_page->history());

	m_page->history()->goToItem(m_page->history()->itemAt(history.index));

	const QUrl url(m_page->history()->itemAt(history.index).url());

	setRequestedUrl(url, false, true);
	updateOptions(url);
	updatePageActions(url);

	m_page->triggerAction(QWebEnginePage::Reload);
}

void QtWebEngineWebWidget::setHistory(QDataStream &stream)
{
	stream.device()->reset();
	stream >> *(m_page->history());

	setRequestedUrl(m_page->history()->currentItem().url(), false, true);
	updateOptions(m_page->history()->currentItem().url());
}

void QtWebEngineWebWidget::setZoom(int zoom)
{
	if (zoom != getZoom())
	{
		m_page->setZoomFactor(qBound(0.1, (static_cast<qreal>(zoom) / 100), static_cast<qreal>(100)));

		SessionsManager::markSessionModified();

		emit zoomChanged(zoom);
		emit progressBarGeometryChanged();
	}
}

void QtWebEngineWebWidget::setUrl(const QUrl &url, bool isTyped)
{
	if (url.scheme() == QLatin1String("javascript"))
	{
		m_page->runJavaScript(url.toDisplayString(QUrl::RemoveScheme | QUrl::DecodeReserved));

		return;
	}

	if (!url.fragment().isEmpty() && url.matches(getUrl(), (QUrl::RemoveFragment | QUrl::StripTrailingSlash | QUrl::NormalizePathSegments)))
	{
		m_page->runJavaScript(QStringLiteral("var element = document.querySelector('a[name=%1], [id=%1]'); if (element) { var geometry = element.getBoundingClientRect(); [(geometry.left + window.scrollX), (geometry.top + window.scrollY)]; }").arg(url.fragment()), [&](const QVariant &result)
		{
			if (result.isValid())
			{
				setScrollPosition(QPoint(result.toList()[0].toInt(), result.toList()[1].toInt()));
			}
		});

		return;
	}

	m_isTyped = isTyped;

	QUrl targetUrl(url);

	if (url.isValid() && url.scheme().isEmpty() && !url.path().startsWith('/'))
	{
		QUrl httpUrl(url);
		httpUrl.setScheme(QLatin1String("http"));

		targetUrl = httpUrl;
	}
	else if (url.isValid() && (url.scheme().isEmpty() || url.scheme() == "file"))
	{
		QUrl localUrl(url);
		localUrl.setScheme(QLatin1String("file"));

		targetUrl = localUrl;
	}

	updateOptions(targetUrl);

	m_page->load(targetUrl);

	notifyTitleChanged();
	notifyIconChanged();
}

void QtWebEngineWebWidget::setPermission(FeaturePermission feature, const QUrl &url, WebWidget::PermissionPolicies policies)
{
	WebWidget::setPermission(feature, url, policies);

	if (feature == FullScreenFeature)
	{
		if (policies.testFlag(GrantedPermission))
		{
			MainWindow *mainWindow(MainWindow::findMainWindow(this));

			if (mainWindow && !mainWindow->isFullScreen())
			{
				mainWindow->triggerAction(ActionsManager::FullScreenAction);
			}
		}

		return;
	}

	QWebEnginePage::Feature nativeFeature(QWebEnginePage::Geolocation);

	switch (feature)
	{
		case GeolocationFeature:
			nativeFeature = QWebEnginePage::Geolocation;

			break;
		case NotificationsFeature:
			nativeFeature = QWebEnginePage::Notifications;

			break;
		case PointerLockFeature:
			nativeFeature = QWebEnginePage::MouseLock;

			break;
		case CaptureAudioFeature:
			nativeFeature = QWebEnginePage::MediaAudioCapture;

			break;
		case CaptureVideoFeature:
			nativeFeature = QWebEnginePage::MediaVideoCapture;

			break;
		case CaptureAudioVideoFeature:
			nativeFeature = QWebEnginePage::MediaAudioVideoCapture;

			break;
		default:
			return;
	}

	m_page->setFeaturePermission(url, nativeFeature, (policies.testFlag(GrantedPermission) ? QWebEnginePage::PermissionGrantedByUser : QWebEnginePage::PermissionDeniedByUser));
}

void QtWebEngineWebWidget::setOption(int identifier, const QVariant &value)
{
	WebWidget::setOption(identifier, value);

	updateOptions(getUrl());

	if (identifier == SettingsManager::Content_DefaultCharacterEncodingOption)
	{
		m_page->triggerAction(QWebEnginePage::Reload);
	}
}

void QtWebEngineWebWidget::setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions)
{
	WebWidget::setOptions(options, excludedOptions);

	updateOptions(getUrl());
}

WebWidget* QtWebEngineWebWidget::clone(bool cloneHistory, bool isPrivate, const QStringList &excludedOptions)
{
	QtWebEngineWebWidget *widget(new QtWebEngineWebWidget((this->isPrivate() || isPrivate), getBackend()));
	widget->setOptions(getOptions(), excludedOptions);

	if (cloneHistory)
	{
		QByteArray data;
		QDataStream stream(&data, QIODevice::ReadWrite);
		stream << *(m_page->history());

		widget->setHistory(stream);
	}

	widget->setZoom(getZoom());

	return widget;
}

QWidget* QtWebEngineWebWidget::getViewport()
{
	return (focusWidget() ? focusWidget() : m_webView);
}

Action* QtWebEngineWebWidget::getAction(int identifier)
{
	if (identifier == ActionsManager::UndoAction && !getExistingAction(ActionsManager::UndoAction))
	{
		Action *action(WebWidget::getAction(ActionsManager::UndoAction));

		updateUndo();

		connect(m_page->action(QWebEnginePage::Undo), SIGNAL(changed()), this, SLOT(updateUndo()));

		return action;
	}

	if (identifier == ActionsManager::RedoAction && !getExistingAction(ActionsManager::RedoAction))
	{
		Action *action(WebWidget::getAction(ActionsManager::RedoAction));

		updateRedo();

		connect(m_page->action(QWebEnginePage::Redo), SIGNAL(changed()), this, SLOT(updateRedo()));

		return action;
	}

	if (identifier == ActionsManager::InspectElementAction && !getExistingAction(ActionsManager::InspectElementAction))
	{
		Action *action(WebWidget::getAction(ActionsManager::InspectElementAction));
		action->setEnabled(false);

		return action;
	}

	return WebWidget::getAction(identifier);
}

QWebEnginePage* QtWebEngineWebWidget::getPage()
{
	return m_page;
}

QString QtWebEngineWebWidget::getTitle() const
{
	const QString title(m_page->title());

	if (title.isEmpty())
	{
		const QUrl url(getUrl());

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
	return m_page->selectedText();
}

QVariant QtWebEngineWebWidget::getPageInformation(WebWidget::PageInformation key) const
{
	if (key == DocumentLoadingProgressInformation || key == TotalLoadingProgressInformation)
	{
		return m_documentLoadingProgress;
	}

	return WebWidget::getPageInformation(key);
}

QUrl QtWebEngineWebWidget::getUrl() const
{
	const QUrl url(m_page->url());

	return ((url.isEmpty() || url.toString() == QLatin1String("about:blank")) ? m_page->requestedUrl() : url);
}

QIcon QtWebEngineWebWidget::getIcon() const
{
	if (isPrivate())
	{
		return ThemesManager::getIcon(QLatin1String("tab-private"));
	}

#if QT_VERSION < 0x050700
	return (m_icon.isNull() ? ThemesManager::getIcon(QLatin1String("tab")) : m_icon);
#else
	const QIcon icon(m_page->icon());

	return (icon.isNull() ? ThemesManager::getIcon(QLatin1String("tab")) : icon);
#endif
}

QDateTime QtWebEngineWebWidget::getLastUrlClickTime() const
{
	return m_lastUrlClickTime;
}

QPixmap QtWebEngineWebWidget::getThumbnail()
{
	return QPixmap();
}

QPoint QtWebEngineWebWidget::getScrollPosition() const
{
#if QT_VERSION < 0x050700
	return m_scrollPosition;
#else
	return m_page->scrollPosition().toPoint();
#endif
}

WindowHistoryInformation QtWebEngineWebWidget::getHistory() const
{
	QWebEngineHistory *history(m_page->history());
	WindowHistoryInformation information;
	information.index = history->currentItemIndex();

	const QString requestedUrl(m_page->requestedUrl().toString());
	const int historyCount(history->count());

	for (int i = 0; i < historyCount; ++i)
	{
		const QWebEngineHistoryItem item(history->itemAt(i));
		WindowHistoryEntry entry;
		entry.url = item.url().toString();
		entry.title = item.title();

		information.entries.append(entry);
	}

	if (m_loadingState == WindowsManager::OngoingLoadingState && requestedUrl != history->itemAt(history->currentItemIndex()).url().toString())
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

	m_page->runJavaScript(QString(file.readAll()).arg(position.x() / m_page->zoomFactor()).arg(position.y() / m_page->zoomFactor()), [&](const QVariant &result)
	{
		m_hitResult = HitTestResult(result);

		eventLoop.quit();
	});

	file.close();

	connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

	eventLoop.exec();

	return m_hitResult;
}

QList<SpellCheckManager::DictionaryInformation> QtWebEngineWebWidget::getDictionaries() const
{
	return QList<SpellCheckManager::DictionaryInformation>();
}

QHash<QByteArray, QByteArray> QtWebEngineWebWidget::getHeaders() const
{
	return QHash<QByteArray, QByteArray>();
}

WindowsManager::LoadingState QtWebEngineWebWidget::getLoadingState() const
{
	return m_loadingState;
}

int QtWebEngineWebWidget::getZoom() const
{
	return (m_page->zoomFactor() * 100);
}

bool QtWebEngineWebWidget::canGoBack() const
{
	return m_page->history()->canGoBack();
}

bool QtWebEngineWebWidget::canGoForward() const
{
	return m_page->history()->canGoForward();
}

bool QtWebEngineWebWidget::canFastForward() const
{
	if (canGoForward())
	{
		return true;
	}

	QEventLoop eventLoop;
	bool canFastFoward(false);

	m_page->runJavaScript(getFastForwardScript(false), [&](const QVariant &result)
	{
		canFastFoward = result.toBool();

		eventLoop.quit();
	});

	connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

	eventLoop.exec();

	return canFastFoward;
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
	return m_page->hasSelection();
}

#if QT_VERSION >= 0x050700
bool QtWebEngineWebWidget::isAudible() const
{
	return m_page->recentlyAudible();
}

bool QtWebEngineWebWidget::isAudioMuted() const
{
	return m_page->isAudioMuted();
}
#endif

bool QtWebEngineWebWidget::isFullScreen() const
{
	return m_isFullScreen;
}

bool QtWebEngineWebWidget::isPrivate() const
{
	return m_page->profile()->isOffTheRecord();
}

bool QtWebEngineWebWidget::isScrollBar(const QPoint &position) const
{
	Q_UNUSED(position)

	return false;
}

bool QtWebEngineWebWidget::findInPage(const QString &text, FindFlags flags)
{
	QWebEnginePage::FindFlags nativeFlags(0);

	if (flags.testFlag(BackwardFind))
	{
		nativeFlags |= QWebEnginePage::FindBackward;
	}

	if (flags.testFlag(CaseSensitiveFind))
	{
		nativeFlags |= QWebEnginePage::FindCaseSensitively;
	}

	m_page->findText(text, nativeFlags);

	return false;
}

bool QtWebEngineWebWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_webView && event->type() == QEvent::ChildAdded)
	{
		QChildEvent *childEvent(static_cast<QChildEvent*>(event));

		if (childEvent->child())
		{
			childEvent->child()->installEventFilter(this);
		}
	}
	else if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::Wheel)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent)
		{
			setClickPosition(mouseEvent->pos());
			updateHitTestResult(mouseEvent->pos());

			if (mouseEvent->button() == Qt::LeftButton && !getCurrentHitTestResult().linkUrl.isEmpty())
			{
				m_lastUrlClickTime = QDateTime::currentDateTime();
			}
		}

		QList<GesturesManager::GesturesContext> contexts;

		if (getCurrentHitTestResult().flags.testFlag(IsContentEditableTest))
		{
			contexts.append(GesturesManager::ContentEditableGesturesContext);
		}

		if (getCurrentHitTestResult().linkUrl.isValid())
		{
			contexts.append(GesturesManager::LinkGesturesContext);
		}

		contexts.append(GesturesManager::GenericGesturesContext);

		if ((!mouseEvent || !isScrollBar(mouseEvent->pos())) && GesturesManager::startGesture(object, event, contexts))
		{
			return true;
		}

		if (event->type() == QEvent::MouseButtonDblClick && mouseEvent->button() == Qt::LeftButton && SettingsManager::getValue(SettingsManager::Browser_ShowSelectionContextMenuOnDoubleClickOption).toBool())
		{
			const WebWidget::HitTestResult hitResult(getHitTestResult(mouseEvent->pos()));

			if (!hitResult.flags.testFlag(IsContentEditableTest) && hitResult.tagName != QLatin1String("textarea") && hitResult.tagName!= QLatin1String("select") && hitResult.tagName != QLatin1String("input"))
			{
				setClickPosition(mouseEvent->pos());

				QTimer::singleShot(250, this, SLOT(showContextMenu()));
			}
		}
	}
	else if (object == m_webView && event->type() == QEvent::ContextMenu)
	{
		QContextMenuEvent *contextMenuEvent(static_cast<QContextMenuEvent*>(event));

		if (contextMenuEvent && contextMenuEvent->reason() != QContextMenuEvent::Mouse)
		{
			QVariantMap parameters;
			parameters[QLatin1String("context")] = contextMenuEvent->reason();

			triggerAction(ActionsManager::ContextMenuAction, parameters);
		}
	}
	else if (object == m_webView && (event->type() == QEvent::Move || event->type() == QEvent::Resize))
	{
		emit progressBarGeometryChanged();
	}
	else if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));

		if (helpEvent)
		{
			handleToolTipEvent(helpEvent, m_webView);
		}

		return true;
	}
	else if (event->type() == QEvent::ShortcutOverride)
	{
		QEventLoop eventLoop;

		m_page->runJavaScript(QLatin1String("var element = document.body.querySelector(':focus'); var tagName = (element ? element.tagName.toLowerCase() : ''); var result = false; if (tagName == 'textarea' || tagName == 'input') { var type = (element.type ? element.type.toLowerCase() : ''); if ((type == '' || tagName == 'textarea' || type == 'text' || type == 'search') && !element.hasAttribute('readonly') && !element.hasAttribute('disabled')) { result = true; } } result;"), [&](const QVariant &result)
		{
			m_isEditing = result.toBool();

			eventLoop.quit();
		});

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
