/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "QtWebKitWebWidget.h"
#include "QtWebKitInspector.h"
#include "QtWebKitNetworkManager.h"
#include "QtWebKitPage.h"
#include "QtWebKitPluginFactory.h"
#include "QtWebKitPluginWidget.h"
#include "QtWebKitWebBackend.h"
#include "../../../../core/Application.h"
#include "../../../../core/BookmarksManager.h"
#include "../../../../core/Console.h"
#include "../../../../core/CookieJar.h"
#include "../../../../core/ContentBlockingManager.h"
#include "../../../../core/GesturesManager.h"
#include "../../../../core/HistoryManager.h"
#include "../../../../core/NetworkCache.h"
#include "../../../../core/NetworkManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/NotesManager.h"
#include "../../../../core/SearchEnginesManager.h"
#include "../../../../core/SessionsManager.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/ThemesManager.h"
#include "../../../../core/TransfersManager.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/Action.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/ContentsWidget.h"
#include "../../../../ui/ImagePropertiesDialog.h"
#include "../../../../ui/MainWindow.h"
#include "../../../../ui/SearchEnginePropertiesDialog.h"
#include "../../../../ui/SourceViewerWebWidget.h"
#include "../../../../ui/WebsitePreferencesDialog.h"

#include <QtCore/QDataStream>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMimeData>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtGui/QClipboard>
#include <QtGui/QImageWriter>
#include <QtGui/QMouseEvent>
#include <QtPrintSupport/QPrintPreviewDialog>
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
#include <QtWebKit/QWebFullScreenRequest>
#endif
#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebElement>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QUndoStack>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

QtWebKitWebWidget::QtWebKitWebWidget(bool isPrivate, WebBackend *backend, QtWebKitNetworkManager *networkManager, ContentsWidget *parent) : WebWidget(isPrivate, backend, parent),
	m_webView(new QWebView(this)),
	m_page(nullptr),
	m_pluginFactory(new QtWebKitPluginFactory(this)),
	m_inspector(nullptr),
	m_networkManager(networkManager),
	m_loadingState(WebWidget::FinishedLoadingState),
	m_transfersTimer(0),
	m_canLoadPlugins(false),
	m_isAudioMuted(false),
	m_isFullScreen(false),
	m_isTyped(false),
	m_isNavigating(false)
{
	QVBoxLayout *layout(new QVBoxLayout(this));
	layout->addWidget(m_webView);
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);
	setFocusPolicy(Qt::StrongFocus);

	if (m_networkManager)
	{
		m_networkManager->setWidget(this);
	}
	else
	{
		m_networkManager = new QtWebKitNetworkManager(isPrivate, nullptr, this);
	}

	m_page = new QtWebKitPage(m_networkManager, this);
	m_page->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, isPrivate);
	m_page->setParent(m_webView);
	m_page->setPluginFactory(m_pluginFactory);
	m_page->setVisibilityState(isVisible() ? QWebPage::VisibilityStateVisible : QWebPage::VisibilityStateHidden);

	m_webView->setPage(m_page);
	m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_webView->installEventFilter(this);

#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	if (isPrivate)
	{
		m_page->settings()->setAttribute(QWebSettings::LocalStorageEnabled, false);
		m_page->settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, false);
		m_page->settings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, false);
	}
#endif

	QShortcut *selectAllShortcut(new QShortcut(QKeySequence(QKeySequence::SelectAll), this, 0, 0, Qt::WidgetWithChildrenShortcut));

	handleOptionChanged(SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption, SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption));
	handleOptionChanged(SettingsManager::Content_BackgroundColorOption, SettingsManager::getOption(SettingsManager::Content_BackgroundColorOption));
	handleOptionChanged(SettingsManager::History_BrowsingLimitAmountWindowOption, SettingsManager::getOption(SettingsManager::History_BrowsingLimitAmountWindowOption));
	updateEditActions();
	setZoom(SettingsManager::getOption(SettingsManager::Content_DefaultZoomOption).toInt());

	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));
	connect(m_page, SIGNAL(requestedNewWindow(WebWidget*,SessionsManager::OpenHints)), this, SIGNAL(requestedNewWindow(WebWidget*,SessionsManager::OpenHints)));
	connect(m_page, SIGNAL(requestedPopupWindow(QUrl,QUrl)), this, SIGNAL(requestedPopupWindow(QUrl,QUrl)));
	connect(m_page, SIGNAL(saveFrameStateRequested(QWebFrame*,QWebHistoryItem*)), this, SLOT(saveState(QWebFrame*,QWebHistoryItem*)));
	connect(m_page, SIGNAL(restoreFrameStateRequested(QWebFrame*)), this, SLOT(restoreState(QWebFrame*)));
	connect(m_page, SIGNAL(downloadRequested(QNetworkRequest)), this, SLOT(downloadFile(QNetworkRequest)));
	connect(m_page, SIGNAL(unsupportedContent(QNetworkReply*)), this, SLOT(downloadFile(QNetworkReply*)));
	connect(m_page, SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(handleLinkHovered(QString)));
	connect(m_page, SIGNAL(microFocusChanged()), this, SLOT(updateEditActions()));
	connect(m_page, SIGNAL(printRequested(QWebFrame*)), this, SLOT(handlePrintRequest(QWebFrame*)));
	connect(m_page, SIGNAL(windowCloseRequested()), this, SLOT(handleWindowCloseRequest()));
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	connect(m_page, SIGNAL(fullScreenRequested(QWebFullScreenRequest)), this, SLOT(handleFullScreenRequest(QWebFullScreenRequest)));
#endif
	connect(m_page, SIGNAL(featurePermissionRequested(QWebFrame*,QWebPage::Feature)), this, SLOT(handlePermissionRequest(QWebFrame*,QWebPage::Feature)));
	connect(m_page, SIGNAL(featurePermissionRequestCanceled(QWebFrame*,QWebPage::Feature)), this, SLOT(handlePermissionCancel(QWebFrame*,QWebPage::Feature)));
	connect(m_page, SIGNAL(loadStarted()), this, SLOT(handleLoadStarted()));
	connect(m_page, SIGNAL(loadProgress(int)), this, SLOT(handleLoadProgress(int)));
	connect(m_page, SIGNAL(loadFinished(bool)), this, SLOT(handleLoadFinished(bool)));
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	connect(m_page, SIGNAL(recentlyAudibleChanged(bool)), this, SLOT(handleAudibleStateChange(bool)));
#endif
	connect(m_page, SIGNAL(viewingMediaChanged(bool)), this, SLOT(updateNavigationActions()));
	connect(m_page->mainFrame(), SIGNAL(titleChanged(QString)), this, SLOT(notifyTitleChanged()));
	connect(m_page->mainFrame(), SIGNAL(urlChanged(QUrl)), this, SLOT(notifyUrlChanged(QUrl)));
	connect(m_page->mainFrame(), SIGNAL(iconChanged()), this, SLOT(notifyIconChanged()));
	connect(m_page->mainFrame(), SIGNAL(contentsSizeChanged(QSize)), this, SIGNAL(progressBarGeometryChanged()));
	connect(m_page->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SIGNAL(progressBarGeometryChanged()));
	connect(m_networkManager, SIGNAL(pageInformationChanged(WebWidget::PageInformation,QVariant)), this, SIGNAL(pageInformationChanged(WebWidget::PageInformation,QVariant)));
	connect(m_networkManager, SIGNAL(requestBlocked(NetworkManager::ResourceInformation)), this, SIGNAL(requestBlocked(NetworkManager::ResourceInformation)));
	connect(m_networkManager, SIGNAL(contentStateChanged(WebWidget::ContentStates)), this, SLOT(notifyContentStateChanged()));
	connect(selectAllShortcut, SIGNAL(activated()), createAction(ActionsManager::SelectAllAction), SLOT(trigger()));
}

QtWebKitWebWidget::~QtWebKitWebWidget()
{
	m_page->triggerAction(QWebPage::Stop);
	m_page->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
}

void QtWebKitWebWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_transfersTimer)
	{
		killTimer(m_transfersTimer);

		Transfer *transfer(m_transfers.dequeue());

		if (transfer)
		{
			startTransfer(transfer);
		}

		if (m_transfers.isEmpty())
		{
			m_transfersTimer = 0;
		}
		else
		{
			m_transfersTimer = startTimer(250);
		}
	}
	else
	{
		WebWidget::timerEvent(event);
	}
}

void QtWebKitWebWidget::showEvent(QShowEvent *event)
{
	WebWidget::showEvent(event);

	m_page->setVisibilityState(QWebPage::VisibilityStateVisible);
}

void QtWebKitWebWidget::hideEvent(QHideEvent *event)
{
	WebWidget::hideEvent(event);

	m_page->setVisibilityState(QWebPage::VisibilityStateHidden);
}

void QtWebKitWebWidget::focusInEvent(QFocusEvent *event)
{
	WebWidget::focusInEvent(event);

	m_webView->setFocus();

	emit widgetActivated(this);
}

void QtWebKitWebWidget::search(const QString &query, const QString &searchEngine)
{
	QNetworkRequest request;
	QNetworkAccessManager::Operation method;
	QByteArray body;

	if (SearchEnginesManager::setupSearchQuery(query, searchEngine, &request, &method, &body))
	{
		setRequestedUrl(request.url(), false, true);
		updateOptions(request.url());

		m_page->mainFrame()->load(request, method, body);
	}
}

void QtWebKitWebWidget::print(QPrinter *printer)
{
	m_webView->print(printer);
}

void QtWebKitWebWidget::downloadFile(const QNetworkRequest &request)
{
	if ((!getCurrentHitTestResult().imageUrl.isEmpty() && request.url() == getCurrentHitTestResult().imageUrl) || (!getCurrentHitTestResult().mediaUrl.isEmpty() && request.url() == getCurrentHitTestResult().mediaUrl))
	{
		NetworkCache *cache(NetworkManagerFactory::getCache());

		if (cache && cache->metaData(request.url()).isValid())
		{
			QIODevice *device(cache->data(request.url()));

			if (device && device->size() > 0)
			{
				const QString path(Utils::getSavePath(request.url().fileName()).path);

				if (path.isEmpty())
				{
					device->deleteLater();

					return;
				}

				QFile file(path);

				if (!file.open(QIODevice::WriteOnly))
				{
					QMessageBox::critical(this, tr("Error"), tr("Failed to open file for writing."), QMessageBox::Close);
				}

				file.write(device->readAll());
				file.close();

				device->deleteLater();

				return;
			}

			if (device)
			{
				device->deleteLater();
			}
		}
		else if (!getCurrentHitTestResult().imageUrl.isEmpty() && getCurrentHitTestResult().imageUrl.url().contains(QLatin1String(";base64,")))
		{
			const QString imageUrl(getCurrentHitTestResult().imageUrl.url());
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

			return;
		}

		QNetworkRequest mutableRequest(request);
		mutableRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

		new Transfer(m_networkManager->get(mutableRequest), QString(), (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::IsPrivateOption));
	}
	else
	{
		startDelayedTransfer(new Transfer(request, QString(), (Transfer::CanNotifyOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));
	}
}

void QtWebKitWebWidget::downloadFile(QNetworkReply *reply)
{
	m_networkManager->registerTransfer(reply);

	startDelayedTransfer(new Transfer(reply, QString(), (Transfer::CanNotifyOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));
}

void QtWebKitWebWidget::saveState(QWebFrame *frame, QWebHistoryItem *item)
{
	if (frame == m_page->mainFrame())
	{
		QVariantList data(m_page->history()->currentItem().userData().toList());

		if (data.isEmpty() || data.count() < 3)
		{
			data = QVariantList({0, getZoom(), m_page->mainFrame()->scrollPosition()});
		}
		else
		{
			data[ZoomEntryData] = getZoom();
			data[PositionEntryData] = m_page->mainFrame()->scrollPosition();
		}

		item->setUserData(data);
	}
}

void QtWebKitWebWidget::restoreState(QWebFrame *frame)
{
	if (frame == m_page->mainFrame())
	{
		setZoom(m_page->history()->currentItem().userData().toList().value(ZoomEntryData, getZoom()).toInt());

		if (m_page->mainFrame()->scrollPosition() == QPoint(0, 0))
		{
			m_page->mainFrame()->setScrollPosition(m_page->history()->currentItem().userData().toList().value(PositionEntryData).toPoint());
		}
	}
}

void QtWebKitWebWidget::clearPluginToken()
{
	QList<QWebFrame*> frames({m_page->mainFrame()});

	while (!frames.isEmpty())
	{
		QWebFrame *frame(frames.takeFirst());
		QWebElement element(frame->documentElement().findFirst(QStringLiteral("object[data-otter-browser=\"%1\"], embed[data-otter-browser=\"%1\"]").arg(m_pluginToken)));

		if (!element.isNull())
		{
			element.removeAttribute(QLatin1String("data-otter-browser"));

			break;
		}

		frames.append(frame->childFrames());
	}

	Action *loadPluginsAction(getExistingAction(ActionsManager::LoadPluginsAction));

	if (loadPluginsAction)
	{
		loadPluginsAction->setEnabled(findChildren<QtWebKitPluginWidget*>().count() > 0);
	}

	m_pluginToken = QString();
}

void QtWebKitWebWidget::resetSpellCheck(QWebElement element)
{
	if (element.isNull())
	{
		element = m_page->mainFrame()->findFirstElement(QLatin1String("*:focus"));
	}

	if (!element.isNull())
	{
		m_page->runScript(QLatin1String("resetSpellCheck"), element);
	}
}

#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
void QtWebKitWebWidget::muteAudio(QWebFrame *frame, bool isMuted)
{
	if (!frame)
	{
		return;
	}

	const QWebElementCollection elements(frame->findAllElements(QLatin1String("audio, video")));

	for (int i = 0; i < elements.count(); ++i)
	{
		elements.at(i).evaluateJavaScript(QLatin1String("this.muted = ") + (isMuted ? QLatin1String("true") : QLatin1String("false")));
	}

	const QList<QWebFrame*> frames(frame->childFrames());

	for (int i = 0; i < frames.count(); ++i)
	{
		muteAudio(frames.at(i), isMuted);
	}
}
#endif

void QtWebKitWebWidget::openRequest(const QNetworkRequest &request, QNetworkAccessManager::Operation operation, QIODevice *outgoingData)
{
	m_formRequest = request;
	m_formRequestOperation = operation;
	m_formRequestBody = (outgoingData ? outgoingData->readAll() : QByteArray());

	if (outgoingData)
	{
		outgoingData->close();
		outgoingData->deleteLater();
	}

	setRequestedUrl(m_formRequest.url(), false, true);
	updateOptions(m_formRequest.url());

	QTimer::singleShot(50, [&]()
	{
		m_page->mainFrame()->load(m_formRequest, m_formRequestOperation, m_formRequestBody);

		m_formRequest = QNetworkRequest();
		m_formRequestBody = QByteArray();
	});
}

void QtWebKitWebWidget::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::Content_BackgroundColorOption:
			{
				QPalette palette(m_page->palette());
				palette.setColor(QPalette::Base, QColor(value.toString()));

				m_page->setPalette(palette);
			}

			break;
		case SettingsManager::History_BrowsingLimitAmountWindowOption:
			m_page->history()->setMaximumItemCount(value.toInt());

			break;
		case SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption:
			disconnect(m_page, SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));

			if (value.toBool() || SettingsManager::getOption(identifier, getUrl()).toBool())
			{
				connect(m_page, SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));
			}
			else
			{
				setStatusMessage(QString());
			}

			break;
		default:
			break;
	}
}

void QtWebKitWebWidget::handleLoadStarted()
{
	if (m_loadingState == WebWidget::OngoingLoadingState)
	{
		return;
	}

	m_thumbnail = QPixmap();
	m_messageToken = QUuid::createUuid().toString();
	m_canLoadPlugins = (getOption(SettingsManager::Permissions_EnablePluginsOption, getUrl()).toString() == QLatin1String("enabled"));
	m_loadingState = WebWidget::OngoingLoadingState;

	updateNavigationActions();
	setStatusMessage(QString());
	setStatusMessage(QString(), true);

	emit progressBarGeometryChanged();
	emit actionsStateChanged(ActionsManager::ActionDefinition::NavigationCategory);
	emit loadingStateChanged(WebWidget::OngoingLoadingState);
}

void QtWebKitWebWidget::handleLoadProgress(int progress)
{
	m_networkManager->setPageInformation(TotalLoadingProgressInformation, progress);
}

void QtWebKitWebWidget::handleLoadFinished(bool result)
{
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	if (m_isAudioMuted)
	{
		muteAudio(m_page->mainFrame(), true);
	}
#endif

	if (m_loadingState != WebWidget::OngoingLoadingState)
	{
		return;
	}

	m_networkManager->handleLoadFinished(result);

	m_thumbnail = QPixmap();
	m_loadingState = WebWidget::FinishedLoadingState;

	updateNavigationActions();
	handleHistory();
	startReloadTimer();

	emit actionsStateChanged(ActionsManager::ActionDefinition::NavigationCategory);
	emit contentStateChanged(getContentState());
	emit loadingStateChanged(WebWidget::FinishedLoadingState);
}

void QtWebKitWebWidget::handleLinkHovered(const QString &link)
{
	setStatusMessage(link, true);
}

void QtWebKitWebWidget::handleViewSourceReplyFinished(QNetworkReply::NetworkError error)
{
	QNetworkReply *reply(qobject_cast<QNetworkReply*>(sender()));

	if (error == QNetworkReply::NoError && m_viewSourceReplies.contains(reply) && m_viewSourceReplies[reply])
	{
		m_viewSourceReplies[reply]->setContents(reply->readAll(), reply->header(QNetworkRequest::ContentTypeHeader).toString());
	}

	m_viewSourceReplies.remove(reply);

	reply->deleteLater();
}

void QtWebKitWebWidget::handlePrintRequest(QWebFrame *frame)
{
	QPrintPreviewDialog printPreviewDialog(this);
	printPreviewDialog.setWindowFlags(printPreviewDialog.windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
	printPreviewDialog.setWindowTitle(tr("Print Preview"));

	if (QApplication::activeWindow())
	{
		printPreviewDialog.resize(QApplication::activeWindow()->size());
	}

	connect(&printPreviewDialog, SIGNAL(paintRequested(QPrinter*)), frame, SLOT(print(QPrinter*)));

	printPreviewDialog.exec();
}

void QtWebKitWebWidget::handleHistory()
{
	if (isPrivate() || m_page->history()->count() == 0)
	{
		return;
	}

	const QUrl url(getUrl());
	const quint64 identifier(m_page->history()->currentItem().userData().toList().value(IdentifierEntryData).toULongLong());

	if (identifier == 0)
	{
		QVariantList data;
		data.append(Utils::isUrlEmpty(url) ? 0 : HistoryManager::addEntry(url, getTitle(), m_page->mainFrame()->icon(), m_isTyped));
		data.append(getZoom());
		data.append(QPoint(0, 0));

		if (m_isTyped)
		{
			m_isTyped = false;
		}

		m_page->history()->currentItem().setUserData(data);

		SessionsManager::markSessionModified();
		BookmarksManager::updateVisits(url.toString());
	}
	else if (identifier > 0)
	{
		HistoryManager::updateEntry(identifier, url, getTitle(), m_page->mainFrame()->icon());
	}
}

void QtWebKitWebWidget::handleNavigationRequest(const QUrl &url, QWebPage::NavigationType type)
{
	if (type != QWebPage::NavigationTypeBackOrForward && (type != QWebPage::NavigationTypeLinkClicked || !getUrl().matches(url, QUrl::RemoveFragment)))
	{
		handleLoadStarted();
		handleHistory();

		m_networkManager->resetStatistics();
	}

	updateOptions(url);

	m_isNavigating = true;

	emit aboutToNavigate();
}

#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
void QtWebKitWebWidget::handleFullScreenRequest(QWebFullScreenRequest request)
{
	request.accept();

	if (request.toggleOn())
	{
		const QString value(SettingsManager::getOption(SettingsManager::Permissions_EnableFullScreenOption, request.origin()).toString());

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
#endif

void QtWebKitWebWidget::handlePermissionRequest(QWebFrame *frame, QWebPage::Feature feature)
{
	notifyPermissionRequested(frame, feature, false);
}

void QtWebKitWebWidget::handlePermissionCancel(QWebFrame *frame, QWebPage::Feature feature)
{
	notifyPermissionRequested(frame, feature, true);
}

void QtWebKitWebWidget::openFormRequest(const QNetworkRequest &request, QNetworkAccessManager::Operation operation, QIODevice *outgoingData)
{
	m_page->triggerAction(QWebPage::Stop);

	QtWebKitWebWidget *widget(new QtWebKitWebWidget(isPrivate(), getBackend(), m_networkManager->clone()));
	widget->setOptions(getOptions());
	widget->setZoom(getZoom());
	widget->openRequest(request, operation, outgoingData);

	emit requestedNewWindow(widget, SessionsManager::calculateOpenHints(SessionsManager::NewTabOpen));
}

void QtWebKitWebWidget::pasteText(const QString &text)
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

void QtWebKitWebWidget::startDelayedTransfer(Transfer *transfer)
{
	m_transfers.enqueue(transfer);

	if (m_transfersTimer == 0)
	{
		m_transfersTimer = startTimer(250);
	}
}

void QtWebKitWebWidget::notifyTitleChanged()
{
	emit titleChanged(getTitle());

	handleHistory();
}

void QtWebKitWebWidget::notifyUrlChanged(const QUrl &url)
{
	m_isNavigating = false;

	updateOptions(url);
	updatePageActions(url);
	updateNavigationActions();

	emit urlChanged(url);
	emit actionsStateChanged(ActionsManager::ActionDefinition::NavigationCategory | ActionsManager::ActionDefinition::PageCategory);

	SessionsManager::markSessionModified();
}

void QtWebKitWebWidget::notifyIconChanged()
{
	emit iconChanged(getIcon());
}

void QtWebKitWebWidget::notifyPermissionRequested(QWebFrame *frame, QWebPage::Feature nativeFeature, bool cancel)
{
	FeaturePermission feature(UnknownFeature);

	switch (nativeFeature)
	{
		case QWebPage::Geolocation:
			feature = GeolocationFeature;

			break;
		case QWebPage::Notifications:
			feature = NotificationsFeature;

			break;
		default:
			return;
	}

	const QUrl url(frame->url().isValid() ? frame->url() : frame->requestedUrl());

	if (cancel)
	{
		emit requestedPermission(feature, url, true);
	}
	else
	{
		switch (getPermission(feature, url))
		{
			case GrantedPermission:
				m_page->setFeaturePermission(frame, nativeFeature, QWebPage::PermissionGrantedByUser);

				break;
			case DeniedPermission:
				m_page->setFeaturePermission(frame, nativeFeature, QWebPage::PermissionDeniedByUser);

				break;
			default:
				emit requestedPermission(feature, url, false);

				break;
		}
	}
}

void QtWebKitWebWidget::notifySavePasswordRequested(const PasswordsManager::PasswordInformation &password, bool isUpdate)
{
	emit requestedSavePassword(password, isUpdate);
}

void QtWebKitWebWidget::notifyContentStateChanged()
{
	emit contentStateChanged(getContentState());
}

void QtWebKitWebWidget::updateUndoText(const QString &text)
{
	createAction(ActionsManager::UndoAction)->setText(text.isEmpty() ? tr("Undo") : tr("Undo: %1").arg(text));
}

void QtWebKitWebWidget::updateRedoText(const QString &text)
{
	createAction(ActionsManager::RedoAction)->setText(text.isEmpty() ? tr("Redo") : tr("Redo: %1").arg(text));
}

void QtWebKitWebWidget::updateOptions(const QUrl &url)
{
	const QString encoding(getOption(SettingsManager::Content_DefaultCharacterEncodingOption, url).toString());
	const bool arePluginsEnabled(getOption(SettingsManager::Permissions_EnablePluginsOption, url).toString() != QLatin1String("disabled"));
	QWebSettings *settings(m_page->settings());
	settings->setAttribute(QWebSettings::AutoLoadImages, (getOption(SettingsManager::Permissions_EnableImagesOption, url).toString() != QLatin1String("onlyCached")));
	settings->setAttribute(QWebSettings::PluginsEnabled, arePluginsEnabled);
	settings->setAttribute(QWebSettings::JavaEnabled, arePluginsEnabled);
	settings->setAttribute(QWebSettings::JavascriptEnabled, (m_page->isErrorPage() || m_page->isViewingMedia() || getOption(SettingsManager::Permissions_EnableJavaScriptOption, url).toBool()));
	settings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, getOption(SettingsManager::Permissions_ScriptsCanAccessClipboardOption, url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanCloseWindows, getOption(SettingsManager::Permissions_ScriptsCanCloseWindowsOption, url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanOpenWindows, (getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, url).toString() != QLatin1String("blockAll")));
	settings->setAttribute(QWebSettings::WebGLEnabled, getOption(SettingsManager::Permissions_EnableWebglOption, url).toBool());
	settings->setAttribute(QWebSettings::LocalStorageEnabled, getOption(SettingsManager::Permissions_EnableLocalStorageOption, url).toBool());
	settings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, getOption(SettingsManager::Permissions_EnableOfflineStorageDatabaseOption, url).toBool());
	settings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, getOption(SettingsManager::Permissions_EnableOfflineWebApplicationCacheOption, url).toBool());
	settings->setDefaultTextEncoding((encoding == QLatin1String("auto")) ? QString() : encoding);
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	settings->setAttribute(QWebSettings::MediaEnabled, getOption(QtWebKitWebBackend::getOptionIdentifier(QtWebKitWebBackend::QtWebKitBackend_EnableMediaOption), url).toBool());
	settings->setAttribute(QWebSettings::MediaSourceEnabled, getOption(QtWebKitWebBackend::getOptionIdentifier(QtWebKitWebBackend::QtWebKitBackend_EnableMediaSourceOption), url).toBool());

	if (settings->testAttribute(QWebSettings::PrivateBrowsingEnabled))
	{
		settings->setAttribute(QWebSettings::LocalStorageEnabled, false);
		settings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, false);
		settings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, false);
	}
#endif

	disconnect(m_page, SIGNAL(geometryChangeRequested(QRect)), this, SIGNAL(requestedGeometryChange(QRect)));
	disconnect(m_page, SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));

	if (getOption(SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption, url).toBool())
	{
		connect(m_page, SIGNAL(geometryChangeRequested(QRect)), this, SIGNAL(requestedGeometryChange(QRect)));
	}

	if (getOption(SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption, url).toBool())
	{
		connect(m_page, SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));
	}
	else
	{
		setStatusMessage(QString());
	}

	m_page->updateStyleSheets(url);

	m_networkManager->updateOptions(url);

	m_canLoadPlugins = (getOption(SettingsManager::Permissions_EnablePluginsOption, url).toString() == QLatin1String("enabled"));
}

void QtWebKitWebWidget::clearOptions()
{
	WebWidget::clearOptions();

	updateOptions(getUrl());
}

void QtWebKitWebWidget::goToHistoryIndex(int index)
{
	m_page->history()->goToItem(m_page->history()->itemAt(index));
}

void QtWebKitWebWidget::removeHistoryIndex(int index, bool purge)
{
	if (purge)
	{
		const quint64 identifier(m_page->history()->itemAt(index).userData().toList().value(IdentifierEntryData).toULongLong());

		if (identifier > 0)
		{
			HistoryManager::removeEntry(identifier);
		}
	}

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

void QtWebKitWebWidget::fillPassword(const PasswordsManager::PasswordInformation &password)
{
	QFile file(QLatin1String(":/modules/backends/web/qtwebkit/resources/formFiller.js"));
	file.open(QIODevice::ReadOnly);

	QJsonArray fieldsArray;

	for (int i = 0; i < password.fields.count(); ++i)
	{
		QJsonObject fieldObject;
		fieldObject.insert(QLatin1String("name"), password.fields.at(i).name);
		fieldObject.insert(QLatin1String("value"), password.fields.at(i).value);
		fieldObject.insert(QLatin1String("type"), ((password.fields.at(i).type == PasswordsManager::PasswordField) ? QLatin1String("password") : QLatin1String("text")));

		fieldsArray.append(fieldObject);
	}

	const QString script(QString(file.readAll()).arg(QString(QJsonDocument(fieldsArray).toJson(QJsonDocument::Indented))));

	file.close();

	QList<QWebFrame*> frames({m_page->mainFrame()});

	while (!frames.isEmpty())
	{
		QWebFrame *frame(frames.takeFirst());
		frame->documentElement().evaluateJavaScript(script);

		frames.append(frame->childFrames());
	}
}

void QtWebKitWebWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	if (parameters.contains(QLatin1String("isBounced")))
	{
		return;
	}

	switch (identifier)
	{
		case ActionsManager::SaveAction:
			{
				const QString path(Utils::getSavePath(suggestSaveFileName(SingleHtmlFileSaveFormat)).path);

				if (!path.isEmpty())
				{
					QNetworkRequest request(getUrl());
					request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

					new Transfer(m_networkManager->get(request), path, (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::CanOverwriteOption | Transfer::IsPrivateOption));
				}
			}

			return;
		case ActionsManager::PurgeTabHistoryAction:
			for (int i = 0; i < m_page->history()->count(); ++i)
			{
				const quint64 identifier(m_page->history()->itemAt(i).userData().toList().value(IdentifierEntryData).toULongLong());

				if (identifier > 0)
				{
					HistoryManager::removeEntry(identifier);
				}
			}

		case ActionsManager::ClearTabHistoryAction:
			setUrl(QUrl(QLatin1String("about:blank")));

			m_page->history()->clear();

			updateNavigationActions();

			emit actionsStateChanged(ActionsManager::ActionDefinition::NavigationCategory);

			return;
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
		case ActionsManager::MuteTabMediaAction:
			m_isAudioMuted = !m_isAudioMuted;

			muteAudio(m_page->mainFrame(), m_isAudioMuted);
			handleAudibleStateChange(m_page->recentlyAudible());

			return;
#endif
		case ActionsManager::OpenLinkAction:
			{
				m_page->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);

				const QWebHitTestResult nativeResult(m_page->mainFrame()->hitTestContent(getClickPosition()));
				nativeResult.element().evaluateJavaScript(QLatin1String("var event = document.createEvent('MouseEvents'); event.initEvent('click', true, true); this.dispatchEvent(event)"));

				m_page->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, (getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, getUrl()).toString() != QLatin1String("blockAll")));

				setClickPosition(QPoint());
			}

			return;
		case ActionsManager::OpenLinkInCurrentTabAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, SessionsManager::CurrentTabOpen);
			}

			return;
		case ActionsManager::OpenLinkInNewTabAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, SessionsManager::calculateOpenHints(SessionsManager::NewTabOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewWindowAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, SessionsManager::calculateOpenHints(SessionsManager::NewWindowOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateTabAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (SessionsManager::NewTabOpen | SessionsManager::PrivateOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateTabBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen | SessionsManager::PrivateOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateWindowAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (SessionsManager::NewWindowOpen | SessionsManager::PrivateOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen | SessionsManager::PrivateOpen));
			}

			return;
		case ActionsManager::CopyLinkToClipboardAction:
			if (!getCurrentHitTestResult().linkUrl.isEmpty())
			{
				QGuiApplication::clipboard()->setText(getCurrentHitTestResult().linkUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			return;
		case ActionsManager::BookmarkLinkAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				const QWebHitTestResult hitResult(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position));
				const QString title(getCurrentHitTestResult().title);

				Application::triggerAction(ActionsManager::BookmarkPageAction, {{QLatin1String("url"), getCurrentHitTestResult().linkUrl}, {QLatin1String("title"), (title.isEmpty() ? hitResult.element().toPlainText() : title)}}, parentWidget());
			}

			return;
		case ActionsManager::SaveLinkToDiskAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				downloadFile(QNetworkRequest(getCurrentHitTestResult().linkUrl));
			}
			else
			{
				m_page->triggerAction(QWebPage::DownloadLinkToDisk);
			}

			return;
		case ActionsManager::SaveLinkToDownloadsAction:
			TransfersManager::addTransfer(new Transfer(getCurrentHitTestResult().linkUrl.toString(), QString(), (Transfer::CanNotifyOption | Transfer::CanAskForPathOption | Transfer::IsQuickTransferOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));

			return;
		case ActionsManager::OpenFrameInCurrentTabAction:
			if (getCurrentHitTestResult().frameUrl.isValid())
			{
				setUrl(getCurrentHitTestResult().frameUrl, false);
			}

			return;
		case ActionsManager::OpenFrameInNewTabAction:
			if (getCurrentHitTestResult().frameUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().frameUrl, SessionsManager::calculateOpenHints(SessionsManager::NewTabOpen));
			}

			return;
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
			if (getCurrentHitTestResult().frameUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().frameUrl, (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen));
			}

			return;
		case ActionsManager::CopyFrameLinkToClipboardAction:
			if (getCurrentHitTestResult().frameUrl.isValid())
			{
				QGuiApplication::clipboard()->setText(getCurrentHitTestResult().frameUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			return;
		case ActionsManager::ReloadFrameAction:
			if (getCurrentHitTestResult().frameUrl.isValid())
			{
				const QUrl url(getCurrentHitTestResult().frameUrl);
				QWebHitTestResult hitResult(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position));

				if (hitResult.frame())
				{
					m_networkManager->addContentBlockingException(url, NetworkManager::SubFrameType);

					hitResult.frame()->setUrl(QUrl());
					hitResult.frame()->setUrl(url);
				}
			}

			return;
		case ActionsManager::ViewFrameSourceAction:
			if (getCurrentHitTestResult().frameUrl.isValid())
			{
				const QString defaultEncoding(m_page->settings()->defaultTextEncoding());
				QNetworkRequest request(getCurrentHitTestResult().frameUrl);
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

				QNetworkReply *reply(m_networkManager->get(request));
				SourceViewerWebWidget *sourceViewer(new SourceViewerWebWidget(isPrivate()));
				sourceViewer->setRequestedUrl(QUrl(QLatin1String("view-source:") + getCurrentHitTestResult().frameUrl.toString()), false);

				if (!defaultEncoding.isEmpty())
				{
					sourceViewer->setOption(SettingsManager::Content_DefaultCharacterEncodingOption, defaultEncoding);
				}

				m_viewSourceReplies[reply] = sourceViewer;

				connect(reply, SIGNAL(finished()), this, SLOT(handleViewSourceReplyFinished()));
				connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleViewSourceReplyFinished(QNetworkReply::NetworkError)));

				emit requestedNewWindow(sourceViewer, SessionsManager::DefaultOpen);
			}

			return;
		case ActionsManager::OpenImageInNewTabAction:
			if (!getCurrentHitTestResult().imageUrl.isEmpty())
			{
				openUrl(getCurrentHitTestResult().imageUrl, SessionsManager::calculateOpenHints(SessionsManager::NewTabOpen));
			}

			return;
		case ActionsManager::OpenImageInNewTabBackgroundAction:
			if (!getCurrentHitTestResult().imageUrl.isEmpty())
			{
				openUrl(getCurrentHitTestResult().imageUrl, (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen));
			}

			return;
		case ActionsManager::SaveImageToDiskAction:
			if (getCurrentHitTestResult().imageUrl.isValid())
			{
				downloadFile(QNetworkRequest(getCurrentHitTestResult().imageUrl));
			}

			return;
		case ActionsManager::CopyImageToClipboardAction:
			{
				const QWebHitTestResult hitResult(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position));

				if (!hitResult.pixmap().isNull())
				{
					QApplication::clipboard()->setPixmap(hitResult.pixmap());
				}
				else
				{
					m_page->triggerAction(QWebPage::CopyImageToClipboard);
				}
			}

			return;
		case ActionsManager::CopyImageUrlToClipboardAction:
			if (!getCurrentHitTestResult().imageUrl.isEmpty())
			{
				QApplication::clipboard()->setText(getCurrentHitTestResult().imageUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			return;
		case ActionsManager::ReloadImageAction:
			if (!getCurrentHitTestResult().imageUrl.isEmpty())
			{
				m_networkManager->addContentBlockingException(getCurrentHitTestResult().imageUrl, NetworkManager::ImageType);

				m_page->settings()->setAttribute(QWebSettings::AutoLoadImages, true);

				if (getUrl().matches(getCurrentHitTestResult().imageUrl, (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash)))
				{
					triggerAction(ActionsManager::ReloadAndBypassCacheAction);
				}
				else
				{
					const QWebHitTestResult hitResult(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position));
					const QUrl url(getCurrentHitTestResult().imageUrl);
					const QString src(hitResult.element().attribute(QLatin1String("src")));
					NetworkCache *cache(NetworkManagerFactory::getCache());

					hitResult.element().setAttribute(QLatin1String("src"), QString());

					if (cache)
					{
						cache->remove(url);
					}

					hitResult.element().setAttribute(QLatin1String("src"), src);

					m_page->mainFrame()->documentElement().evaluateJavaScript(QStringLiteral("var images = document.querySelectorAll('img[src=\"%1\"]'); for (var i = 0; i < images.length; ++i) { images[i].src = ''; images[i].src = \'%1\'; }").arg(src));
				}
			}

			return;
		case ActionsManager::ImagePropertiesAction:
			{
				QVariantMap properties;
				properties[QLatin1String("alternativeText")] = getCurrentHitTestResult().alternateText;
				properties[QLatin1String("longDescription")] = getCurrentHitTestResult().longDescription;

				const QWebHitTestResult hitResult(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position));

				if (!hitResult.pixmap().isNull())
				{
					properties[QLatin1String("width")] = hitResult.pixmap().width();
					properties[QLatin1String("height")] = hitResult.pixmap().height();
					properties[QLatin1String("depth")] = hitResult.pixmap().depth();
				}

				ImagePropertiesDialog *imagePropertiesDialog(new ImagePropertiesDialog(getCurrentHitTestResult().imageUrl, properties, (m_networkManager->cache() ? m_networkManager->cache()->data(getCurrentHitTestResult().imageUrl) : nullptr), this));
				imagePropertiesDialog->setButtonsVisible(false);

				ContentsDialog *dialog(new ContentsDialog(ThemesManager::getIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), QString(), QString(), QDialogButtonBox::Close, imagePropertiesDialog, this));

				connect(this, SIGNAL(aboutToReload()), dialog, SLOT(close()));
				connect(imagePropertiesDialog, SIGNAL(finished(int)), dialog, SLOT(close()));

				showDialog(dialog, false);
			}

			return;
		case ActionsManager::SaveMediaToDiskAction:
			if (getCurrentHitTestResult().mediaUrl.isValid())
			{
				downloadFile(QNetworkRequest(getCurrentHitTestResult().mediaUrl));
			}

			return;
		case ActionsManager::CopyMediaUrlToClipboardAction:
			if (!getCurrentHitTestResult().mediaUrl.isEmpty())
			{
				QApplication::clipboard()->setText(getCurrentHitTestResult().mediaUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			return;
		case ActionsManager::MediaControlsAction:
			m_page->triggerAction(QWebPage::ToggleMediaControls, calculateCheckedState(ActionsManager::MediaControlsAction, parameters));

			return;
		case ActionsManager::MediaLoopAction:
			m_page->triggerAction(QWebPage::ToggleMediaLoop, calculateCheckedState(ActionsManager::MediaLoopAction, parameters));

			return;
		case ActionsManager::MediaPlayPauseAction:
			m_page->triggerAction(QWebPage::ToggleMediaPlayPause);

			return;
		case ActionsManager::MediaMuteAction:
			m_page->triggerAction(QWebPage::ToggleMediaMute);

			return;
		case ActionsManager::GoBackAction:
			m_page->triggerAction(QWebPage::Back);

			return;
		case ActionsManager::GoForwardAction:
			m_page->triggerAction(QWebPage::Forward);

			return;
		case ActionsManager::RewindAction:
			m_page->history()->goToItem(m_page->history()->itemAt(0));

			return;
		case ActionsManager::FastForwardAction:
			{
				const QUrl url(m_page->mainFrame()->evaluateJavaScript(getFastForwardScript(true)).toUrl());

				if (url.isValid())
				{
					setUrl(url);
				}
				else if (canGoForward())
				{
					m_page->triggerAction(QWebPage::Forward);
				}
			}

			return;
		case ActionsManager::StopAction:
			m_page->triggerAction(QWebPage::Stop);

			return;
		case ActionsManager::StopScheduledReloadAction:
			m_page->triggerAction(QWebPage::StopScheduledPageRefresh);

			return;
		case ActionsManager::ReloadAction:
			emit aboutToReload();

			m_page->triggerAction(QWebPage::Stop);
			m_page->triggerAction(QWebPage::Reload);

			return;
		case ActionsManager::ReloadOrStopAction:
			if (m_loadingState == WebWidget::OngoingLoadingState)
			{
				triggerAction(ActionsManager::StopAction);
			}
			else
			{
				triggerAction(ActionsManager::ReloadAction);
			}

			return;
		case ActionsManager::ReloadAndBypassCacheAction:
			m_page->triggerAction(QWebPage::ReloadAndBypassCache);

			return;
		case ActionsManager::ContextMenuAction:
			{
				if (parameters.contains(QLatin1String("context")) && parameters[QLatin1String("context")].toInt() == QContextMenuEvent::Keyboard)
				{
					const QWebElement element(m_page->mainFrame()->findFirstElement(QLatin1String(":focus")));

					if (element.isNull())
					{
						setClickPosition(m_webView->mapFromGlobal(QCursor::pos()));
					}
					else
					{
						QPoint clickPosition(element.geometry().center());
						QWebFrame *frame(element.webFrame());

						while (frame)
						{
							clickPosition -= frame->scrollPosition();

							frame = frame->parentFrame();
						}

						setClickPosition(clickPosition);
					}
				}
				else
				{
					setClickPosition(m_webView->mapFromGlobal(QCursor::pos()));
				}

				showContextMenu(getClickPosition());
			}

			return;
		case ActionsManager::UndoAction:
			m_page->triggerAction(QWebPage::Undo);

			return;
		case ActionsManager::RedoAction:
			m_page->triggerAction(QWebPage::Redo);

			return;
		case ActionsManager::CutAction:
			m_page->triggerAction(QWebPage::Cut);

			return;
		case ActionsManager::CopyAction:
			m_page->triggerAction(QWebPage::Copy);

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
			m_page->triggerAction(QWebPage::Paste);

			return;
		case ActionsManager::DeleteAction:
			m_page->triggerAction(QWebPage::DeleteEndOfWord);

			return;
		case ActionsManager::SelectAllAction:
			m_page->triggerAction(QWebPage::SelectAll);

			return;
		case ActionsManager::UnselectAction:
			{
				QWebFrame *frame(m_page->currentFrame() ? m_page->currentFrame() : m_page->mainFrame());
				const QWebElement element(frame->findFirstElement(QLatin1String(":focus")));

				if (element.tagName().toLower() == QLatin1String("textarea") || element.tagName().toLower() == QLatin1String("input"))
				{
					m_page->triggerAction(QWebPage::MoveToPreviousChar);
				}
				else
				{
					frame->documentElement().evaluateJavaScript(QLatin1String("window.getSelection().empty()"));
				}
			}

			break;
		case ActionsManager::ClearAllAction:
			triggerAction(ActionsManager::SelectAllAction);
			triggerAction(ActionsManager::DeleteAction);

			return;
		case ActionsManager::CheckSpellingAction:
			{
				QWebElement element(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position).element());
				element.evaluateJavaScript(QStringLiteral("this.spellcheck = %1").arg(calculateCheckedState(ActionsManager::CheckSpellingAction, parameters) ? QLatin1String("true") : QLatin1String("false")));

				resetSpellCheck(element);
			}

			break;
		case ActionsManager::CreateSearchAction:
			{
				const QWebHitTestResult hitResult(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position));
				QWebElement parentElement(hitResult.element().parent());

				while (!parentElement.isNull() && parentElement.tagName().toLower() != QLatin1String("form"))
				{
					parentElement = parentElement.parent();
				}

				QList<QWebElement> inputElements(parentElement.findAll(QLatin1String("button:not([disabled])[name][type=\"submit\"], input:not([disabled])[name], select:not([disabled])[name], textarea:not([disabled])[name]")).toList());

				if (!parentElement.hasAttribute(QLatin1String("action")) || inputElements.count() == 0)
				{
					return;
				}

				QWebElement searchTermsElement;
				const QString tagName(hitResult.element().tagName().toLower());
				const QUrl url(parentElement.attribute(QLatin1String("action")));
				const QIcon icon(m_page->mainFrame()->icon());
				SearchEnginesManager::SearchEngineDefinition searchEngine;
				searchEngine.identifier = Utils::createIdentifier(getUrl().host(), SearchEnginesManager::getSearchEngines());
				searchEngine.title = getTitle();
				searchEngine.formUrl = getUrl();
				searchEngine.icon = (icon.isNull() ? ThemesManager::getIcon(QLatin1String("edit-find")) : icon);
				searchEngine.resultsUrl.url = (url.isEmpty() ? getUrl() : resolveUrl(parentElement.webFrame(), url)).toString();
				searchEngine.resultsUrl.enctype = parentElement.attribute(QLatin1String("enctype"));
				searchEngine.resultsUrl.method = ((parentElement.attribute(QLatin1String("method"), QLatin1String("get")).toLower() == QLatin1String("post")) ? QLatin1String("post") : QLatin1String("get"));

				if (tagName != QLatin1String("select"))
				{
					const QString type(hitResult.element().attribute(QLatin1String("type")));

					if (!inputElements.contains(hitResult.element()) && (type == QLatin1String("image") || type == QLatin1String("submit")))
					{
						inputElements.append(hitResult.element());
					}

					if (tagName == QLatin1String("textarea") || type == QLatin1String("email") || type == QLatin1String("password") || type == QLatin1String("search") || type == QLatin1String("tel") || type == QLatin1String("text") || type == QLatin1String("url"))
					{
						searchTermsElement = hitResult.element();
					}
				}

				if (searchTermsElement.isNull())
				{
					searchTermsElement = parentElement.findFirst(QLatin1String("input:not([disabled])[name][type=\"search\"]"));
				}

				for (int i = 0; i < inputElements.count(); ++i)
				{
					const QString name(inputElements.at(i).attribute(QLatin1String("name")));

					if (inputElements.at(i).tagName().toLower() != QLatin1String("select"))
					{
						const QString type(inputElements.at(i).attribute(QLatin1String("type")));
						const bool isSubmit(type == QLatin1String("image") || type == QLatin1String("submit"));

						if ((isSubmit && inputElements.at(i) != hitResult.element()) || ((type == QLatin1String("checkbox") || type == QLatin1String("radio")) && !inputElements[i].evaluateJavaScript(QLatin1String("this.checked")).toBool()))
						{
							continue;
						}

						if (isSubmit && inputElements.at(i) == hitResult.element())
						{
							if (inputElements.at(i).hasAttribute(QLatin1String("formaction")))
							{
								searchEngine.resultsUrl.url = resolveUrl(parentElement.webFrame(), inputElements.at(i).attribute(QLatin1String("formaction"))).toString();
							}

							if (inputElements.at(i).hasAttribute(QLatin1String("formenctype")))
							{
								searchEngine.resultsUrl.enctype = inputElements.at(i).attribute(QLatin1String("formenctype"));
							}

							if (inputElements.at(i).hasAttribute(QLatin1String("formmethod")))
							{
								searchEngine.resultsUrl.method = inputElements.at(i).attribute(QLatin1String("formmethod"));
							}
						}

						if (!isSubmit && searchTermsElement.isNull() && type != QLatin1String("hidden"))
						{
							searchTermsElement = inputElements.at(i);
						}

						if (!name.isEmpty())
						{
							searchEngine.resultsUrl.parameters.addQueryItem(name, ((inputElements.at(i) == searchTermsElement) ? QLatin1String("{searchTerms}") : inputElements[i].evaluateJavaScript((inputElements.at(i).tagName().toLower() == QLatin1String("button")) ? QLatin1String("this.innerHTML") : QLatin1String("this.value")).toString()));
						}
					}
					else if (!name.isEmpty())
					{
						const QWebElementCollection optionElements(inputElements.at(i).findAll(QLatin1String("option:checked")));

						for (int j = 0; j < optionElements.count(); ++j)
						{
							searchEngine.resultsUrl.parameters.addQueryItem(name, optionElements.at(j).evaluateJavaScript(QLatin1String("this.value")).toString());
						}
					}
				}

				SearchEnginePropertiesDialog dialog(searchEngine, SearchEnginesManager::getSearchKeywords(), this);

				if (dialog.exec() == QDialog::Accepted)
				{
					SearchEnginesManager::addSearchEngine(dialog.getSearchEngine());
				}
			}

			return;
		case ActionsManager::ScrollToStartAction:
			m_page->mainFrame()->setScrollPosition(QPoint(m_page->mainFrame()->scrollPosition().x(), 0));

			return;
		case ActionsManager::ScrollToEndAction:
			m_page->mainFrame()->setScrollPosition(QPoint(m_page->mainFrame()->scrollPosition().x(), m_page->mainFrame()->scrollBarMaximum(Qt::Vertical)));

			return;
		case ActionsManager::ScrollPageUpAction:
			m_page->mainFrame()->setScrollPosition(QPoint(m_page->mainFrame()->scrollPosition().x(), qMax(0, (m_page->mainFrame()->scrollPosition().y() - m_webView->height()))));

			return;
		case ActionsManager::ScrollPageDownAction:
			m_page->mainFrame()->setScrollPosition(QPoint(m_page->mainFrame()->scrollPosition().x(), qMin(m_page->mainFrame()->scrollBarMaximum(Qt::Vertical), (m_page->mainFrame()->scrollPosition().y() + m_webView->height()))));

			return;
		case ActionsManager::ScrollPageLeftAction:
			m_page->mainFrame()->setScrollPosition(QPoint(qMax(0, (m_page->mainFrame()->scrollPosition().x() - m_webView->width())), m_page->mainFrame()->scrollPosition().y()));

			return;
		case ActionsManager::ScrollPageRightAction:
			m_page->mainFrame()->setScrollPosition(QPoint(qMin(m_page->mainFrame()->scrollBarMaximum(Qt::Horizontal), (m_page->mainFrame()->scrollPosition().x() + m_webView->width())), m_page->mainFrame()->scrollPosition().y()));

			return;
		case ActionsManager::ActivateContentAction:
			{
				m_webView->setFocus();

				m_page->mainFrame()->setFocus();

				const QWebElement element(m_page->mainFrame()->findFirstElement(QLatin1String(":focus")));

				if (element.tagName().toLower() == QLatin1String("textarea") || element.tagName().toLower() == QLatin1String("input"))
				{
					m_page->mainFrame()->documentElement().evaluateJavaScript(QLatin1String("document.activeElement.blur()"));
				}
			}

			return;
		case ActionsManager::BookmarkPageAction:
			{
				const QString description(m_page->mainFrame()->findFirstElement(QLatin1String("[name=\"description\"]")).attribute(QLatin1String("content")));

				Application::triggerAction(ActionsManager::BookmarkPageAction, {{QLatin1String("url"), getUrl()}, {QLatin1String("title"), getTitle()}, {QLatin1String("description"), (description.isEmpty() ? m_page->mainFrame()->findFirstElement(QLatin1String("[name=\"og:description\"]")).attribute(QLatin1String("property")) : description)}}, parentWidget());
			}

			return;
		case ActionsManager::LoadPluginsAction:
			{
				m_canLoadPlugins = true;

				QList<QWebFrame*> frames({m_page->mainFrame()});

				while (!frames.isEmpty())
				{
					QWebFrame *frame(frames.takeFirst());
					const QWebElementCollection elements(frame->documentElement().findAll(QLatin1String("object, embed")));

					for (int i = 0; i < elements.count(); ++i)
					{
						elements.at(i).replace(elements.at(i).clone());
					}

					frames.append(frame->childFrames());
				}

				Action *loadPluginsAction(getExistingAction(ActionsManager::LoadPluginsAction));

				if (loadPluginsAction)
				{
					loadPluginsAction->setEnabled(findChildren<QtWebKitPluginWidget*>().count() > 0);
				}
			}

			return;
		case ActionsManager::ViewSourceAction:
			if (canViewSource())
			{
				const QString defaultEncoding(m_page->settings()->defaultTextEncoding());
				QNetworkRequest request(getUrl());
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

				QNetworkReply *reply(m_networkManager->get(request));
				SourceViewerWebWidget *sourceViewer(new SourceViewerWebWidget(isPrivate()));
				sourceViewer->setRequestedUrl(QUrl(QLatin1String("view-source:") + getUrl().toString()), false);

				if (!defaultEncoding.isEmpty())
				{
					sourceViewer->setOption(SettingsManager::Content_DefaultCharacterEncodingOption, defaultEncoding);
				}

				m_viewSourceReplies[reply] = sourceViewer;

				connect(reply, SIGNAL(finished()), this, SLOT(handleViewSourceReplyFinished()));
				connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleViewSourceReplyFinished(QNetworkReply::NetworkError)));

				emit requestedNewWindow(sourceViewer, SessionsManager::DefaultOpen);
			}

			return;
		case ActionsManager::InspectPageAction:
			{
				if (!m_inspector)
				{
					getInspector();
				}

				const bool result(calculateCheckedState(ActionsManager::InspectPageAction, parameters));

				createAction(ActionsManager::InspectPageAction)->setChecked(result);

				emit requestedInspectorVisibilityChange(result);

				return;
			}
		case ActionsManager::InspectElementAction:
			{
				QVariantMap inspectPageParameters;
				inspectPageParameters[QLatin1String("isChecked")] = true;

				triggerAction(ActionsManager::InspectPageAction, inspectPageParameters);

				m_page->triggerAction(QWebPage::InspectElement);
			}

			return;
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
		case ActionsManager::FullScreenAction:
			{
				MainWindow *mainWindow(MainWindow::findMainWindow(this));

				if (mainWindow && !mainWindow->isFullScreen())
				{
					m_page->mainFrame()->evaluateJavaScript(QLatin1String("document.webkitExitFullscreen()"));
				}
			}

			return;
#endif
		case ActionsManager::WebsitePreferencesAction:
			{
				const QUrl url(getUrl());
				WebsitePreferencesDialog dialog(url, (url.host().isEmpty() ? QVector<QNetworkCookie>() : m_networkManager->getCookieJar()->getCookies(url.host())), this);

				if (dialog.exec() == QDialog::Accepted)
				{
					updateOptions(getUrl());

					const QVector<QNetworkCookie> cookiesToDelete(dialog.getCookiesToDelete());

					for (int i = 0; i < cookiesToDelete.count(); ++i)
					{
						m_networkManager->getCookieJar()->forceDeleteCookie(cookiesToDelete.at(i));
					}

					const QVector<QNetworkCookie> cookiesToInsert(dialog.getCookiesToInsert());

					for (int i = 0; i < cookiesToInsert.count(); ++i)
					{
						m_networkManager->getCookieJar()->forceInsertCookie(cookiesToInsert.at(i));
					}
				}
			}

			return;
		default:
			break;
	}

	bounceAction(identifier, parameters);
}

void QtWebKitWebWidget::setActiveStyleSheet(const QString &styleSheet)
{
	QWebElementCollection styleSheets(m_page->mainFrame()->findAllElements(QLatin1String("link[rel=\"alternate stylesheet\"]")));

	for (int i = 0; i < styleSheets.count(); ++i)
	{
		styleSheets.at(i).evaluateJavaScript(QLatin1String("this.disabled = true"));

		if (styleSheets.at(i).attribute(QLatin1String("title")) == styleSheet)
		{
			styleSheets.at(i).evaluateJavaScript(QLatin1String("this.disabled = false"));
		}
	}
}

void QtWebKitWebWidget::setHistory(const WindowHistoryInformation &history)
{
	if (history.entries.count() == 0)
	{
		m_page->history()->clear();

		updateNavigationActions();
		updateOptions(QUrl());
		updatePageActions(QUrl());

		if (m_page->getMainFrame())
		{
			m_page->getMainFrame()->runUserScripts(QUrl(QLatin1String("about:blank")));
		}

		emit actionsStateChanged(ActionsManager::ActionDefinition::NavigationCategory | ActionsManager::ActionDefinition::PageCategory);

		return;
	}

	const int index(qMin(history.index, (m_page->history()->maximumItemCount() - 1)));

#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
	qint64 documentSequence(0);
	qint64 itemSequence(0);
	QByteArray data;
	QDataStream stream(&data, QIODevice::ReadWrite);
	stream << int(2) << history.entries.count() << index;

	for (int i = 0; i < history.entries.count(); ++i)
	{
		stream << QString(QUrl::fromUserInput(history.entries.at(i).url).toEncoded()) << history.entries.at(i).title << history.entries.at(i).url << quint32(2) << quint64(0) << ++documentSequence << quint64(0) << QString() << false << ++itemSequence << QString() << qint32(history.entries.at(i).position.x()) << qint32(history.entries.at(i).position.y()) << qreal(1) << false << QString() << false;
	}

	stream.device()->reset();
	stream >> *(m_page->history());
#else
	QVariantList entries;

	for (int i = 0; i < history.entries.count(); ++i)
	{
		QVariantMap position;
		position[QLatin1String("x")] = history.entries.at(i).position.x();
		position[QLatin1String("y")] = history.entries.at(i).position.y();

		QVariantMap entry;
		entry[QLatin1String("pageScaleFactor")] = 0;
		entry[QLatin1String("title")] = history.entries.at(i).title;
		entry[QLatin1String("urlString")] = QString(QUrl::fromUserInput(history.entries.at(i).url).toEncoded());
		entry[QLatin1String("scrollPosition")] = position;

		entries.append(entry);
	}

	QVariantMap map;
	map[QLatin1String("currentItemIndex")] = index;
	map[QLatin1String("history")] = entries;

	m_page->history()->loadFromMap(map);
#endif

	for (int i = 0; i < history.entries.count(); ++i)
	{
		QVariantList data;
		data.append(-1);
		data.append(history.entries.at(i).zoom);
		data.append(history.entries.at(i).position);

		m_page->history()->itemAt(i).setUserData(data);
	}

	m_page->history()->goToItem(m_page->history()->itemAt(index));

	const QUrl url(m_page->history()->itemAt(index).url());

	setRequestedUrl(url, false, true);
	updateOptions(url);
	updatePageActions(url);

	m_page->triggerAction(QWebPage::Reload);

	emit actionsStateChanged(ActionsManager::ActionDefinition::PageCategory);
}

#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
void QtWebKitWebWidget::setHistory(QDataStream &stream)
{
	stream.device()->reset();
	stream >> *(m_page->history());

	const QUrl url(m_page->history()->currentItem().url());

	setRequestedUrl(url, false, true);
	updateOptions(url);
	updatePageActions(url);

	m_page->triggerAction(QWebPage::Reload);
}
#else
void QtWebKitWebWidget::setHistory(const QVariantMap &history)
{
	m_page->history()->loadFromMap(history);

	const QUrl url(m_page->history()->currentItem().url());

	setRequestedUrl(url, false, true);
	updateOptions(url);
	updatePageActions(url);

	m_page->triggerAction(QWebPage::Reload);

	emit actionsStateChanged(ActionsManager::ActionDefinition::PageCategory);
}
#endif

void QtWebKitWebWidget::setZoom(int zoom)
{
	if (zoom != getZoom())
	{
		m_page->mainFrame()->setZoomFactor(qBound(0.1, (static_cast<qreal>(zoom) / 100), static_cast<qreal>(100)));

		SessionsManager::markSessionModified();

		emit zoomChanged(zoom);
		emit progressBarGeometryChanged();
	}
}

void QtWebKitWebWidget::setUrl(const QUrl &url, bool isTyped)
{
	if (url.scheme() == QLatin1String("javascript"))
	{
		m_page->mainFrame()->documentElement().evaluateJavaScript(url.toDisplayString(QUrl::RemoveScheme | QUrl::DecodeReserved));

		return;
	}

	if (!url.fragment().isEmpty() && url.matches(getUrl(), (QUrl::RemoveFragment | QUrl::StripTrailingSlash | QUrl::NormalizePathSegments)))
	{
		m_page->mainFrame()->scrollToAnchor(url.fragment());

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

	m_page->mainFrame()->load(targetUrl);

	notifyTitleChanged();
	notifyIconChanged();
}

void QtWebKitWebWidget::setPermission(FeaturePermission feature, const QUrl &url, WebWidget::PermissionPolicies policies)
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

	QWebPage::Feature nativeFeature(QWebPage::Geolocation);

	switch (feature)
	{
		case GeolocationFeature:
			nativeFeature = QWebPage::Geolocation;

			break;
		case NotificationsFeature:
			nativeFeature = QWebPage::Notifications;

			break;
		default:
			return;
	}

	QList<QWebFrame*> frames({m_page->mainFrame()});

	while (!frames.isEmpty())
	{
		QWebFrame *frame(frames.takeFirst());

		if (frame->requestedUrl() == url)
		{
			m_page->setFeaturePermission(frame, nativeFeature, (policies.testFlag(GrantedPermission) ? QWebPage::PermissionGrantedByUser : QWebPage::PermissionDeniedByUser));
		}

		frames.append(frame->childFrames());
	}
}

void QtWebKitWebWidget::setOption(int identifier, const QVariant &value)
{
	WebWidget::setOption(identifier, value);

	updateOptions(getUrl());

	if (identifier == SettingsManager::Content_DefaultCharacterEncodingOption)
	{
		m_page->triggerAction(QWebPage::Reload);
	}
	else if (identifier == SettingsManager::Browser_SpellCheckDictionaryOption)
	{
		emit widgetActivated(this);

		resetSpellCheck(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position).element());
	}
}

void QtWebKitWebWidget::setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions)
{
	WebWidget::setOptions(options, excludedOptions);

	updateOptions(getUrl());
}

void QtWebKitWebWidget::setScrollPosition(const QPoint &position)
{
	m_page->mainFrame()->setScrollPosition(position);
}

WebWidget* QtWebKitWebWidget::clone(bool cloneHistory, bool isPrivate, const QStringList &excludedOptions)
{
	QtWebKitWebWidget *widget(new QtWebKitWebWidget((this->isPrivate() || isPrivate), getBackend(), ((this->isPrivate() != isPrivate) ? nullptr : m_networkManager->clone()), nullptr));
	widget->setOptions(getOptions(), excludedOptions);

	if (cloneHistory)
	{
#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
		QByteArray data;
		QDataStream stream(&data, QIODevice::ReadWrite);
		stream << *(m_page->history());

		widget->setHistory(stream);
#else
		widget->setHistory(m_page->history()->toMap());
#endif
	}

	widget->setZoom(getZoom());

	return widget;
}

QWidget* QtWebKitWebWidget::getInspector()
{
	if (!m_inspector)
	{
		m_page->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

		m_inspector = new QtWebKitInspector(this);
		m_inspector->setPage(m_page);
	}

	return m_inspector;
}

QWidget* QtWebKitWebWidget::getViewport()
{
	return m_webView;
}

Action* QtWebKitWebWidget::createAction(int identifier, const QVariantMap parameters, bool followState)
{
	if (identifier == ActionsManager::UndoAction && !getExistingAction(ActionsManager::UndoAction))
	{
		Action *action(WebWidget::createAction(ActionsManager::UndoAction));
		action->setEnabled(m_page->undoStack()->canUndo());

		updateUndoText(m_page->undoStack()->undoText());

		connect(m_page->undoStack(), SIGNAL(canUndoChanged(bool)), action, SLOT(setEnabled(bool)));
		connect(m_page->undoStack(), SIGNAL(undoTextChanged(QString)), this, SLOT(updateUndoText(QString)));

		return action;
	}

	if (identifier == ActionsManager::RedoAction && !getExistingAction(ActionsManager::RedoAction))
	{
		Action *action(WebWidget::createAction(ActionsManager::RedoAction));
		action->setEnabled(m_page->undoStack()->canRedo());

		updateRedoText(m_page->undoStack()->redoText());

		connect(m_page->undoStack(), SIGNAL(canRedoChanged(bool)), action, SLOT(setEnabled(bool)));
		connect(m_page->undoStack(), SIGNAL(redoTextChanged(QString)), this, SLOT(updateRedoText(QString)));

		return action;
	}

	return WebWidget::createAction(identifier, parameters, followState);
}

QtWebKitPage* QtWebKitWebWidget::getPage()
{
	return m_page;
}

QString QtWebKitWebWidget::getTitle() const
{
	const QString title(m_page->mainFrame()->title());

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

QString QtWebKitWebWidget::getActiveStyleSheet() const
{
	if (m_page->mainFrame()->documentElement().evaluateJavaScript(QLatin1String("var isDefault = true; for (var i = 0; i < document.styleSheets.length; ++i) { if (document.styleSheets[i].ownerNode.rel.indexOf('alt') >= 0) { isDefault = false; break; } } isDefault")).toBool())
	{
		return QString();
	}

	return m_page->mainFrame()->findFirstElement(QLatin1String("link[rel=\"alternate stylesheet\"]:not([disabled])")).attribute(QLatin1String("title"));
}

QString QtWebKitWebWidget::getSelectedText() const
{
	return m_page->selectedText();
}

QString QtWebKitWebWidget::getMessageToken() const
{
	return m_messageToken;
}

QString QtWebKitWebWidget::getPluginToken() const
{
	return m_pluginToken;
}

QVariant QtWebKitWebWidget::getPageInformation(WebWidget::PageInformation key) const
{
	if (key == LoadingTimeInformation)
	{
		return WebWidget::getPageInformation(LoadingTimeInformation);
	}

	return m_networkManager->getPageInformation(key);
}

QUrl QtWebKitWebWidget::resolveUrl(QWebFrame *frame, const QUrl &url) const
{
	if (url.isRelative())
	{
		return ((frame ? frame->baseUrl() : getUrl()).resolved(url));
	}

	return url;
}

QUrl QtWebKitWebWidget::getUrl() const
{
	const QUrl url(m_page->mainFrame()->url());

	return (url.isEmpty() ? m_page->mainFrame()->requestedUrl() : url);
}

QIcon QtWebKitWebWidget::getIcon() const
{
	if (isPrivate())
	{
		return ThemesManager::getIcon(QLatin1String("tab-private"));
	}

	const QIcon icon(m_page->mainFrame()->icon());

	return (icon.isNull() ? ThemesManager::getIcon(QLatin1String("tab")) : icon);
}

QPixmap QtWebKitWebWidget::getThumbnail()
{
	if ((!m_thumbnail.isNull() && m_thumbnail.devicePixelRatio() == devicePixelRatio()) || m_loadingState == WebWidget::OngoingLoadingState)
	{
		return m_thumbnail;
	}

	const QSize thumbnailSize(QSize(260, 170));
	const QSize oldViewportSize(m_page->viewportSize());
	const QPoint position(m_page->mainFrame()->scrollPosition());
	const qreal zoom(m_page->mainFrame()->zoomFactor());
	QSize contentsSize(m_page->mainFrame()->contentsSize());
	QWidget *newView(new QWidget());
	QWidget *oldView(m_page->view());

	m_page->setView(newView);
	m_page->setViewportSize(contentsSize);
	m_page->mainFrame()->setZoomFactor(1);

	if (contentsSize.width() > 2000)
	{
		contentsSize.setWidth(2000);
	}

	contentsSize.setHeight(thumbnailSize.height() * (qreal(contentsSize.width()) / thumbnailSize.width()));

	m_page->setViewportSize(contentsSize);

	QPixmap pixmap(contentsSize);
	pixmap.fill(Qt::white);

	QPainter painter(&pixmap);

	m_page->mainFrame()->render(&painter, QWebFrame::ContentsLayer, QRegion(QRect(QPoint(0, 0), contentsSize)));
	m_page->mainFrame()->setZoomFactor(zoom);
	m_page->setView(oldView);
	m_page->setViewportSize(oldViewportSize);
	m_page->mainFrame()->setScrollPosition(position);

	painter.end();

	pixmap = pixmap.scaled((thumbnailSize * devicePixelRatio()), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	pixmap.setDevicePixelRatio(devicePixelRatio());

	newView->deleteLater();

	m_thumbnail = pixmap;

	return pixmap;
}

QPoint QtWebKitWebWidget::getScrollPosition() const
{
	return m_page->mainFrame()->scrollPosition();
}

QRect QtWebKitWebWidget::getProgressBarGeometry() const
{
	if (!isVisible())
	{
		return QRect();
	}

	QRect geometry(QPoint(0, (height() - 30)), QSize(width(), 30));
	const QRect horizontalScrollBar(m_page->mainFrame()->scrollBarGeometry(Qt::Horizontal));
	const QRect verticalScrollBar(m_page->mainFrame()->scrollBarGeometry(Qt::Vertical));

	if (horizontalScrollBar.height() > 0 && geometry.intersects(horizontalScrollBar))
	{
		geometry.moveTop(m_webView->height() - 30 - horizontalScrollBar.height());
	}

	if (verticalScrollBar.width() > 0 && geometry.intersects(verticalScrollBar))
	{
		if (verticalScrollBar.left() == 0)
		{
			geometry.setLeft(verticalScrollBar.right());
		}
		else
		{
			geometry.setRight(verticalScrollBar.left());
		}
	}

	return geometry;
}

WebWidget::LinkUrl QtWebKitWebWidget::getActiveFrame() const
{
	QWebFrame *frame(m_page->frameAt(getClickPosition()));
	LinkUrl link;

	if (frame && frame != m_page->mainFrame())
	{
		link.title = frame->title();
		link.url = (frame->url().isValid() ? frame->url() : frame->requestedUrl());
	}

	return link;
}

WebWidget::LinkUrl QtWebKitWebWidget::getActiveImage() const
{
	const QWebHitTestResult result(m_page->mainFrame()->hitTestContent(getClickPosition()));
	LinkUrl link;

	if (!result.imageUrl().isEmpty())
	{
		link.title = result.alternateText();
		link.url = result.imageUrl();
	}

	return link;
}

WebWidget::LinkUrl QtWebKitWebWidget::getActiveLink() const
{
	const QWebHitTestResult result(m_page->mainFrame()->hitTestContent(getClickPosition()));
	LinkUrl link;

	if (!result.linkElement().isNull())
	{
		link.title = result.linkElement().toPlainText();
		link.url = result.linkUrl();
	}

	return link;
}

WebWidget::SslInformation QtWebKitWebWidget::getSslInformation() const
{
	return m_networkManager->getSslInformation();
}

WindowHistoryInformation QtWebKitWebWidget::getHistory() const
{
	QVariantList data(m_page->history()->currentItem().userData().toList());

	if (data.isEmpty() || data.count() < 3)
	{
		data.clear();
		data.append(0);
		data.append(getZoom());
		data.append(m_page->mainFrame()->scrollPosition());
	}
	else
	{
		data[ZoomEntryData] = getZoom();
		data[PositionEntryData] = m_page->mainFrame()->scrollPosition();
	}

	m_page->history()->currentItem().setUserData(data);

	QWebHistory *history(m_page->history());
	WindowHistoryInformation information;
	information.index = history->currentItemIndex();

	const QString requestedUrl(m_page->mainFrame()->requestedUrl().toString());
	const int historyCount(history->count());

	for (int i = 0; i < historyCount; ++i)
	{
		const QWebHistoryItem item(history->itemAt(i));
		WindowHistoryEntry entry;
		entry.url = item.url().toString();
		entry.title = item.title();
		entry.position = item.userData().toList().value(PositionEntryData, QPoint(0, 0)).toPoint();
		entry.zoom = item.userData().toList().value(ZoomEntryData).toInt();

		information.entries.append(entry);
	}

	if (m_loadingState == WebWidget::OngoingLoadingState && requestedUrl != history->itemAt(history->currentItemIndex()).url().toString())
	{
		WindowHistoryEntry entry;
		entry.url = requestedUrl;
		entry.title = getTitle();
		entry.position = data.value(PositionEntryData, QPoint(0, 0)).toPoint();
		entry.zoom = data.value(ZoomEntryData).toInt();

		information.index = historyCount;
		information.entries.append(entry);
	}

	return information;
}

WebWidget::HitTestResult QtWebKitWebWidget::getHitTestResult(const QPoint &position)
{
	QWebFrame *frame(m_page->frameAt(position));

	if (!frame)
	{
		return HitTestResult();
	}

	const QWebHitTestResult nativeResult(m_page->mainFrame()->hitTestContent(position));
	HitTestResult result;
	result.title = nativeResult.title();
	result.tagName = nativeResult.element().tagName().toLower();
	result.alternateText = nativeResult.element().attribute(QLatin1String("alt"));
	result.longDescription = nativeResult.element().attribute(QLatin1String("longDesc"));
	result.frameUrl = ((nativeResult.frame() && nativeResult.frame() != m_page->mainFrame()) ? (nativeResult.frame()->url().isValid() ? nativeResult.frame()->url() : nativeResult.frame()->requestedUrl()) : QUrl());
	result.imageUrl = nativeResult.imageUrl();
	result.linkUrl = nativeResult.linkUrl();
	result.mediaUrl = nativeResult.mediaUrl();
	result.position = position;
	result.geometry = nativeResult.element().geometry();

	QWebElement parentElement(nativeResult.element().parent());
	const QString type(nativeResult.element().attribute(QLatin1String("type")).toLower());
	const bool isSubmit((result.tagName == QLatin1String("button") || result.tagName == QLatin1String("input")) && (type == QLatin1String("image") || type == QLatin1String("submit")));

	if (isSubmit || result.tagName == QLatin1String("button") || result.tagName == QLatin1String("input") || result.tagName == QLatin1String("select") || result.tagName == QLatin1String("textarea"))
	{
		while (!parentElement.isNull() && parentElement.tagName().toLower() != QLatin1String("form"))
		{
			parentElement = parentElement.parent();
		}

		if (!parentElement.isNull() && parentElement.hasAttribute(QLatin1String("action")))
		{
			if (isSubmit)
			{
				const QUrl url(nativeResult.element().hasAttribute(QLatin1String("formaction")) ? nativeResult.element().attribute(QLatin1String("formaction")) : parentElement.attribute(QLatin1String("action")));

				result.formUrl = (url.isEmpty() ? getUrl() : resolveUrl(parentElement.webFrame(), url)).toString();
			}

			if (parentElement.findAll(QLatin1String("input:not([disabled])[name], select:not([disabled])[name], textarea:not([disabled])[name]")).count() > 0)
			{
				result.flags |= IsFormTest;
			}
		}
	}

	if (result.tagName == QLatin1String("textarea") || result.tagName == QLatin1String("input"))
	{
		const QString type(nativeResult.element().attribute(QLatin1String("type")).toLower());

		if (type.isEmpty() || result.tagName == QLatin1String("textarea") || type == QLatin1String("text") || type == QLatin1String("password") || type == QLatin1String("search"))
		{
			if (!nativeResult.element().hasAttribute(QLatin1String("disabled")) && !nativeResult.element().hasAttribute(QLatin1String("readonly")))
			{
				result.flags |= IsContentEditableTest;
			}

			if (nativeResult.element().evaluateJavaScript(QLatin1String("this.value")).toString().isEmpty())
			{
				result.flags |= IsEmptyTest;
			}
		}
	}
	else if (nativeResult.isContentEditable())
	{
		result.flags |= IsContentEditableTest;
	}

	if (nativeResult.isContentSelected())
	{
		result.flags |= IsSelectedTest;
	}

	if (nativeResult.element().evaluateJavaScript(QLatin1String("this.spellcheck")).toBool())
	{
		result.flags |= IsSpellCheckEnabled;
	}

	if (result.mediaUrl.isValid())
	{
		if (nativeResult.element().evaluateJavaScript(QLatin1String("this.controls")).toBool())
		{
			result.flags |= MediaHasControlsTest;
		}

		if (nativeResult.element().evaluateJavaScript(QLatin1String("this.looped")).toBool())
		{
			result.flags |= MediaIsLoopedTest;
		}

		if (nativeResult.element().evaluateJavaScript(QLatin1String("this.muted")).toBool())
		{
			result.flags |= MediaIsMutedTest;
		}

		if (nativeResult.element().evaluateJavaScript(QLatin1String("this.paused")).toBool())
		{
			result.flags |= MediaIsPausedTest;
		}
	}

	return result;
}

QStringList QtWebKitWebWidget::getBlockedElements() const
{
	return m_networkManager->getBlockedElements();
}

QStringList QtWebKitWebWidget::getStyleSheets() const
{
	QStringList titles;
	const QWebElementCollection styleSheets(m_page->mainFrame()->findAllElements(QLatin1String("link[rel=\"alternate stylesheet\"]")));

	for (int i = 0; i < styleSheets.count(); ++i)
	{
		const QString title(styleSheets.at(i).attribute(QLatin1String("title")));

		if (!title.isEmpty() && !titles.contains(title))
		{
			titles.append(title);
		}
	}

	return titles;
}

QVector<WebWidget::LinkUrl> QtWebKitWebWidget::getFeeds() const
{
	const QWebElementCollection elements(m_page->mainFrame()->findAllElements(QLatin1String("a[type=\"application/atom+xml\"], a[type=\"application/rss+xml\"], link[type=\"application/atom+xml\"], link[type=\"application/rss+xml\"]")));
	QVector<LinkUrl> links;
	QSet<QUrl> urls;

	for (int i = 0; i < elements.count(); ++i)
	{
		QUrl url(resolveUrl(m_page->mainFrame(), QUrl(elements.at(i).attribute(QLatin1String("href")))));

		if (urls.contains(url))
		{
			continue;
		}

		urls.insert(url);

		LinkUrl link;
		link.title = elements.at(i).attribute(QLatin1String("title"));
		link.mimeType = elements.at(i).attribute(QLatin1String("type"));
		link.url = url;

		links.append(link);
	}

	return links;
}

QVector<WebWidget::LinkUrl> QtWebKitWebWidget::getSearchEngines() const
{
	const QWebElementCollection elements(m_page->mainFrame()->findAllElements(QLatin1String("link[type=\"application/opensearchdescription+xml\"]")));
	QVector<LinkUrl> links;
	QSet<QUrl> urls;

	for (int i = 0; i < elements.count(); ++i)
	{
		QUrl url(resolveUrl(m_page->mainFrame(), QUrl(elements.at(i).attribute(QLatin1String("href")))));

		if (urls.contains(url))
		{
			continue;
		}

		urls.insert(url);

		LinkUrl link;
		link.title = elements.at(i).attribute(QLatin1String("title"));
		link.mimeType = elements.at(i).attribute(QLatin1String("type"));
		link.url = url;

		links.append(link);
	}

	return links;
}

QVector<NetworkManager::ResourceInformation> QtWebKitWebWidget::getBlockedRequests() const
{
	return m_networkManager->getBlockedRequests();
}

QHash<QByteArray, QByteArray> QtWebKitWebWidget::getHeaders() const
{
	return m_networkManager->getHeaders();
}

WebWidget::ContentStates QtWebKitWebWidget::getContentState() const
{
	const QUrl url(getUrl());

	if (url.isEmpty() || url.scheme() == QLatin1String("about"))
	{
		return WebWidget::ApplicationContentState;
	}

	if (url.scheme() == QLatin1String("file"))
	{
		return WebWidget::LocalContentState;
	}

	return (m_networkManager->getContentState() | WebWidget::RemoteContentState);
}

WebWidget::LoadingState QtWebKitWebWidget::getLoadingState() const
{
	return m_loadingState;
}

int QtWebKitWebWidget::getZoom() const
{
	return (m_page->mainFrame()->zoomFactor() * 100);
}

int QtWebKitWebWidget::getAmountOfNotLoadedPlugins() const
{
	return findChildren<QtWebKitPluginWidget*>().count();
}

int QtWebKitWebWidget::findInPage(const QString &text, FindFlags flags)
{
	QWebPage::FindFlags nativeFlags(QWebPage::FindWrapsAroundDocument | QWebPage::FindBeginsInSelection);

	if (flags.testFlag(BackwardFind))
	{
		nativeFlags |= QWebPage::FindBackward;
	}

	if (flags.testFlag(CaseSensitiveFind))
	{
		nativeFlags |= QWebPage::FindCaseSensitively;
	}

	if (flags.testFlag(HighlightAllFind) || text.isEmpty())
	{
		m_page->findText(QString(), QWebPage::HighlightAllOccurrences);
		m_page->findText(text, (nativeFlags | QWebPage::HighlightAllOccurrences));
	}

	return (m_page->findText(text, nativeFlags) ? -1 : 0);
}

bool QtWebKitWebWidget::canLoadPlugins() const
{
	return m_canLoadPlugins;
}

bool QtWebKitWebWidget::canGoBack() const
{
	return m_page->history()->canGoBack();
}

bool QtWebKitWebWidget::canGoForward() const
{
	return m_page->history()->canGoForward();
}

bool QtWebKitWebWidget::canFastForward() const
{
	if (canGoForward())
	{
		return true;
	}

	return m_page->mainFrame()->evaluateJavaScript(getFastForwardScript(false)).toBool();
}

bool QtWebKitWebWidget::canRedo() const
{
	return m_page->undoStack()->canRedo();
}

bool QtWebKitWebWidget::canUndo() const
{
	return m_page->undoStack()->canUndo();
}

bool QtWebKitWebWidget::canShowContextMenu(const QPoint &position) const
{
	if (!getOption(SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption).toBool())
	{
		return true;
	}

	QContextMenuEvent menuEvent(QContextMenuEvent::Other, position, m_webView->mapToGlobal(position), Qt::NoModifier);

	return !m_page->swallowContextMenuEvent(&menuEvent);
}

bool QtWebKitWebWidget::canViewSource() const
{
	return !m_page->isViewingMedia();
}

bool QtWebKitWebWidget::hasSelection() const
{
	return (m_page->hasSelection() && !m_page->selectedText().isEmpty());
}

#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
bool QtWebKitWebWidget::isAudible() const
{
	return m_page->recentlyAudible();
}

bool QtWebKitWebWidget::isAudioMuted() const
{
	return m_isAudioMuted;
}
#endif

bool QtWebKitWebWidget::isFullScreen() const
{
	return m_isFullScreen;
}

bool QtWebKitWebWidget::isInspecting() const
{
	return (m_inspector && m_inspector->isVisible());
}

bool QtWebKitWebWidget::isNavigating() const
{
	return m_isNavigating;
}

bool QtWebKitWebWidget::isPopup() const
{
	return m_page->isPopup();
}

bool QtWebKitWebWidget::isPrivate() const
{
	return m_page->settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled);
}

bool QtWebKitWebWidget::isScrollBar(const QPoint &position) const
{
	return (m_page->mainFrame()->scrollBarGeometry(Qt::Horizontal).contains(position) || m_page->mainFrame()->scrollBarGeometry(Qt::Vertical).contains(position));
}

bool QtWebKitWebWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_webView)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (event->type() == QEvent::MouseButtonPress && mouseEvent && mouseEvent->button() == Qt::LeftButton && SettingsManager::getOption(SettingsManager::Permissions_EnablePluginsOption, getUrl()).toString() == QLatin1String("onDemand"))
		{
			QWidget *widget(childAt(mouseEvent->pos()));
			const QWebHitTestResult hitResult(m_page->mainFrame()->hitTestContent(mouseEvent->pos()));
			const QString tagName(hitResult.element().tagName().toLower());

			if (widget && widget->metaObject()->className() == QLatin1String("Otter::QtWebKitPluginWidget") && (tagName == QLatin1String("object") || tagName == QLatin1String("embed")))
			{
				m_pluginToken = QUuid::createUuid().toString();

				QWebElement element(hitResult.element().clone());
				element.setAttribute(QLatin1String("data-otter-browser"), m_pluginToken);

				hitResult.element().replace(element);

				return true;
			}
		}

		if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::Wheel)
		{
			if (mouseEvent)
			{
				setClickPosition(mouseEvent->pos());
				updateHitTestResult(mouseEvent->pos());
			}

			QVector<GesturesManager::GesturesContext> contexts;

			if (getCurrentHitTestResult().flags.testFlag(IsContentEditableTest))
			{
				contexts.append(GesturesManager::ContentEditableContext);
			}

			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				contexts.append(GesturesManager::LinkContext);
			}

			contexts.append(GesturesManager::GenericContext);

			if ((!mouseEvent || !isScrollBar(mouseEvent->pos())) && GesturesManager::startGesture(object, event, contexts))
			{
				return true;
			}

			if (event->type() == QEvent::MouseButtonDblClick && mouseEvent->button() == Qt::LeftButton && SettingsManager::getOption(SettingsManager::Browser_ShowSelectionContextMenuOnDoubleClickOption).toBool())
			{
				const WebWidget::HitTestResult hitResult(getHitTestResult(mouseEvent->pos()));

				if (!hitResult.flags.testFlag(IsContentEditableTest) && hitResult.tagName != QLatin1String("textarea") && hitResult.tagName!= QLatin1String("select") && hitResult.tagName != QLatin1String("input"))
				{
					setClickPosition(mouseEvent->pos());

					QTimer::singleShot(250, this, SLOT(showContextMenu()));
				}
			}
		}
		else if (event->type() == QEvent::MouseMove)
		{
			event->ignore();

			return QObject::eventFilter(object, event);
		}
		else if (event->type() == QEvent::MouseButtonRelease && mouseEvent && mouseEvent->button() == Qt::MiddleButton && !getCurrentHitTestResult().flags.testFlag(IsContentEditableTest))
		{
			return true;
		}
		else if (event->type() == QEvent::ContextMenu)
		{
			QContextMenuEvent *contextMenuEvent(static_cast<QContextMenuEvent*>(event));

			if (contextMenuEvent && contextMenuEvent->reason() != QContextMenuEvent::Mouse)
			{
				triggerAction(ActionsManager::ContextMenuAction, {{QLatin1String("context"), contextMenuEvent->reason()}});
			}

			if (!getOption(SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption).toBool())
			{
				return true;
			}
		}
		else if (event->type() == QEvent::Move || event->type() == QEvent::Resize)
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
			const QString tagName(m_page->mainFrame()->findFirstElement(QLatin1String("*:focus")).tagName().toLower());

			if (tagName == QLatin1String("object") || tagName == QLatin1String("embed"))
			{
				event->accept();

				return true;
			}

			QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

			if (keyEvent->modifiers() == Qt::ControlModifier)
			{
				if (keyEvent->key() == Qt::Key_Backspace && m_page->currentFrame()->hitTestContent(m_page->inputMethodQuery(Qt::ImCursorRectangle).toRect().center()).isContentEditable())
				{
					event->accept();
				}

				return true;
			}

			if ((keyEvent->modifiers().testFlag(Qt::AltModifier) || keyEvent->modifiers().testFlag(Qt::GroupSwitchModifier)) && m_page->currentFrame()->hitTestContent(m_page->inputMethodQuery(Qt::ImCursorRectangle).toRect().center()).isContentEditable())
			{
				event->accept();

				return true;
			}
		}
	}

	return QObject::eventFilter(object, event);
}

}
