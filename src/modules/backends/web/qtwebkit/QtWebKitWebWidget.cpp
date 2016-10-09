/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "QtWebKitWebWidget.h"
#include "QtWebKitInspector.h"
#include "QtWebKitNetworkManager.h"
#include "QtWebKitPage.h"
#include "QtWebKitPluginFactory.h"
#include "QtWebKitPluginWidget.h"
#include "QtWebKitWebBackend.h"
#include "../../../../core/ActionsManager.h"
#include "../../../../core/AddonsManager.h"
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
#include "../../../../core/UserScript.h"
#include "../../../../core/Utils.h"
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
#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebElement>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

QtWebKitWebWidget::QtWebKitWebWidget(bool isPrivate, WebBackend *backend, QtWebKitNetworkManager *networkManager, ContentsWidget *parent) : WebWidget(isPrivate, backend, parent),
	m_webView(new QWebView(this)),
	m_page(NULL),
	m_pluginFactory(new QtWebKitPluginFactory(this)),
	m_inspector(NULL),
	m_networkManager(networkManager),
	m_splitter(new QSplitter(Qt::Vertical, this)),
	m_loadingState(WindowsManager::FinishedLoadingState),
	m_transfersTimer(0),
	m_canLoadPlugins(false),
	m_isAudioMuted(false),
	m_isTyped(false),
	m_isNavigating(false)
{
	m_splitter->addWidget(m_webView);
	m_splitter->setChildrenCollapsible(false);
	m_splitter->setContentsMargins(0, 0, 0, 0);

	QVBoxLayout *layout(new QVBoxLayout(this));
	layout->addWidget(m_splitter);
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);
	setFocusPolicy(Qt::StrongFocus);

	if (m_networkManager)
	{
		m_networkManager->setWidget(this);
	}
	else
	{
		m_networkManager = new QtWebKitNetworkManager(isPrivate, NULL, this);
	}

	m_page = new QtWebKitPage(m_networkManager, this);
	m_page->setParent(m_webView);
	m_page->setPluginFactory(m_pluginFactory);
	m_page->setVisibilityState(isVisible() ? QWebPage::VisibilityStateVisible : QWebPage::VisibilityStateHidden);

	m_webView->setPage(m_page);
	m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_webView->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, isPrivate);
	m_webView->installEventFilter(this);

#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	if (isPrivate)
	{
		m_webView->settings()->setAttribute(QWebSettings::LocalStorageEnabled, false);
		m_webView->settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, false);
		m_webView->settings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, false);
	}
#endif

	QShortcut *selectAllShortcut(new QShortcut(QKeySequence(QKeySequence::SelectAll), this, 0, 0, Qt::WidgetWithChildrenShortcut));

	optionChanged(SettingsManager::Browser_JavaScriptCanShowStatusMessagesOption, SettingsManager::getValue(SettingsManager::Browser_JavaScriptCanShowStatusMessagesOption));
	optionChanged(SettingsManager::Content_BackgroundColorOption, SettingsManager::getValue(SettingsManager::Content_BackgroundColorOption));
	optionChanged(SettingsManager::History_BrowsingLimitAmountWindowOption, SettingsManager::getValue(SettingsManager::History_BrowsingLimitAmountWindowOption));
	updateEditActions();
	setZoom(SettingsManager::getValue(SettingsManager::Content_DefaultZoomOption).toInt());

	connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(updateBookmarkActions()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int,QVariant)));
	connect(m_page, SIGNAL(aboutToNavigate(QUrl,QWebFrame*,QWebPage::NavigationType)), this, SLOT(navigating(QUrl,QWebFrame*,QWebPage::NavigationType)));
	connect(m_page, SIGNAL(requestedNewWindow(WebWidget*,WindowsManager::OpenHints)), this, SIGNAL(requestedNewWindow(WebWidget*,WindowsManager::OpenHints)));
	connect(m_page, SIGNAL(requestedPopupWindow(QUrl,QUrl)), this, SIGNAL(requestedPopupWindow(QUrl,QUrl)));
	connect(m_page, SIGNAL(saveFrameStateRequested(QWebFrame*,QWebHistoryItem*)), this, SLOT(saveState(QWebFrame*,QWebHistoryItem*)));
	connect(m_page, SIGNAL(restoreFrameStateRequested(QWebFrame*)), this, SLOT(restoreState(QWebFrame*)));
	connect(m_page, SIGNAL(downloadRequested(QNetworkRequest)), this, SLOT(downloadFile(QNetworkRequest)));
	connect(m_page, SIGNAL(unsupportedContent(QNetworkReply*)), this, SLOT(downloadFile(QNetworkReply*)));
	connect(m_page, SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString)));
	connect(m_page, SIGNAL(microFocusChanged()), this, SLOT(updateEditActions()));
	connect(m_page, SIGNAL(printRequested(QWebFrame*)), this, SLOT(handlePrintRequest(QWebFrame*)));
	connect(m_page, SIGNAL(windowCloseRequested()), this, SLOT(handleWindowCloseRequest()));
	connect(m_page, SIGNAL(featurePermissionRequested(QWebFrame*,QWebPage::Feature)), this, SLOT(handlePermissionRequest(QWebFrame*,QWebPage::Feature)));
	connect(m_page, SIGNAL(featurePermissionRequestCanceled(QWebFrame*,QWebPage::Feature)), this, SLOT(handlePermissionCancel(QWebFrame*,QWebPage::Feature)));
	connect(m_page, SIGNAL(loadStarted()), this, SLOT(pageLoadStarted()));
	connect(m_page, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	connect(m_page, SIGNAL(recentlyAudibleChanged(bool)), this, SLOT(handleAudibleStateChange(bool)));
#endif
	connect(m_page, SIGNAL(viewingMediaChanged(bool)), this, SLOT(updateNavigationActions()));
	connect(m_page->mainFrame(), SIGNAL(contentsSizeChanged(QSize)), this, SIGNAL(progressBarGeometryChanged()));
	connect(m_page->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SIGNAL(progressBarGeometryChanged()));
	connect(m_webView, SIGNAL(titleChanged(QString)), this, SLOT(notifyTitleChanged()));
	connect(m_webView, SIGNAL(urlChanged(QUrl)), this, SLOT(notifyUrlChanged(QUrl)));
	connect(m_webView, SIGNAL(iconChanged()), this, SLOT(notifyIconChanged()));
	connect(m_networkManager, SIGNAL(pageInformationChanged(WebWidget::PageInformation,QVariant)), this, SIGNAL(pageInformationChanged(WebWidget::PageInformation,QVariant)));
	connect(m_networkManager, SIGNAL(requestBlocked(NetworkManager::ResourceInformation)), this, SIGNAL(requestBlocked(NetworkManager::ResourceInformation)));
	connect(m_networkManager, SIGNAL(contentStateChanged(WindowsManager::ContentStates)), this, SLOT(notifyContentStateChanged()));
	connect(m_splitter, SIGNAL(splitterMoved(int,int)), this, SIGNAL(progressBarGeometryChanged()));
	connect(selectAllShortcut, SIGNAL(activated()), getAction(ActionsManager::SelectAllAction), SLOT(trigger()));
}

QtWebKitWebWidget::~QtWebKitWebWidget()
{
	m_webView->stop();
	m_webView->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
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

		m_webView->page()->mainFrame()->load(request, method, body);
	}
}

void QtWebKitWebWidget::print(QPrinter *printer)
{
	m_webView->print(printer);
}

void QtWebKitWebWidget::optionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::Browser_JavaScriptCanShowStatusMessagesOption)
	{
		disconnect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));

		if (value.toBool() || SettingsManager::getValue(identifier, getUrl()).toBool())
		{
			connect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));
		}
		else
		{
			setStatusMessage(QString());
		}
	}
	else if (identifier == SettingsManager::Content_BackgroundColorOption)
	{
		QPalette palette(m_page->palette());
		palette.setColor(QPalette::Base, QColor(value.toString()));

		m_page->setPalette(palette);
	}
	else if (identifier == SettingsManager::History_BrowsingLimitAmountWindowOption)
	{
		m_webView->page()->history()->setMaximumItemCount(value.toInt());
	}
}

void QtWebKitWebWidget::navigating(const QUrl &url, QWebFrame *frame, QWebPage::NavigationType type)
{
	if (frame == m_page->mainFrame())
	{
		if (type != QWebPage::NavigationTypeBackOrForward)
		{
			pageLoadStarted();
			handleHistory();
		}

		m_networkManager->resetStatistics();

		updateOptions(url);

		m_isNavigating = true;

		emit aboutToNavigate();
	}
}

void QtWebKitWebWidget::pageLoadStarted()
{
	if (m_loadingState == WindowsManager::OngoingLoadingState)
	{
		return;
	}

	m_canLoadPlugins = (getOption(SettingsManager::Browser_EnablePluginsOption, getUrl()).toString() == QLatin1String("enabled"));
	m_loadingState = WindowsManager::OngoingLoadingState;
	m_thumbnail = QPixmap();

	updateNavigationActions();
	setStatusMessage(QString());
	setStatusMessage(QString(), true);

	emit progressBarGeometryChanged();
	emit loadingStateChanged(WindowsManager::OngoingLoadingState);
}

void QtWebKitWebWidget::pageLoadFinished()
{
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	if (m_isAudioMuted)
	{
		muteAudio(m_page->mainFrame(), true);
	}
#endif

	if (m_loadingState != WindowsManager::OngoingLoadingState)
	{
		return;
	}

	m_networkManager->handleLoadingFinished();

	m_loadingState = WindowsManager::FinishedLoadingState;
	m_thumbnail = QPixmap();

	updateNavigationActions();
	handleHistory();
	startReloadTimer();

	emit contentStateChanged(getContentState());
	emit loadingStateChanged(WindowsManager::FinishedLoadingState);

	QList<QWebFrame*> frames;
	const QList<UserScript*> scripts(AddonsManager::getUserScriptsForUrl(getUrl()));

	for (int i = 0; i < scripts.count(); ++i)
	{
		if (scripts.at(i)->shouldRunOnSubFrames())
		{
			if (frames.isEmpty())
			{
				QList<QWebFrame*> temporaryFrames;
				temporaryFrames.append(m_page->mainFrame());

				while (!temporaryFrames.isEmpty())
				{
					QWebFrame *frame(temporaryFrames.takeFirst());

					frames.append(frame);

					temporaryFrames.append(frame->childFrames());
				}
			}

			for (int j = 0; j < frames.count(); ++j)
			{
				frames.at(j)->documentElement().evaluateJavaScript(scripts.at(i)->getSource());
			}
		}
		else
		{
			m_page->mainFrame()->documentElement().evaluateJavaScript(scripts.at(i)->getSource());
		}
	}

	if (!SettingsManager::getValue(SettingsManager::Browser_RememberPasswordsOption).toBool())
	{
		return;
	}

	m_passwordToken = QUuid::createUuid().toString();

	QFile file(QLatin1String(":/modules/backends/web/qtwebkit/resources/formExtractor.js"));
	file.open(QIODevice::ReadOnly);

	if (frames.isEmpty())
	{
		QList<QWebFrame*> temporaryFrames;
		temporaryFrames.append(m_page->mainFrame());

		while (!temporaryFrames.isEmpty())
		{
			QWebFrame *frame(temporaryFrames.takeFirst());

			frames.append(frame);

			temporaryFrames.append(frame->childFrames());
		}
	}

	const QString script(QString(file.readAll()).arg(m_passwordToken));

	file.close();

	for (int j = 0; j < frames.count(); ++j)
	{
		frames.at(j)->documentElement().evaluateJavaScript(script);
	}
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
					QMessageBox::critical(SessionsManager::getActiveWindow(), tr("Error"), tr("Failed to open file for writing."), QMessageBox::Close);
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
	if (frame == m_webView->page()->mainFrame())
	{
		QVariantList data(m_webView->history()->currentItem().userData().toList());

		if (data.isEmpty() || data.count() < 3)
		{
			data.clear();
			data.append(0);
			data.append(getZoom());
			data.append(m_webView->page()->mainFrame()->scrollPosition());
		}
		else
		{
			data[ZoomEntryData] = getZoom();
			data[PositionEntryData] = m_webView->page()->mainFrame()->scrollPosition();
		}

		item->setUserData(data);
	}
}

void QtWebKitWebWidget::restoreState(QWebFrame *frame)
{
	if (frame == m_webView->page()->mainFrame())
	{
		setZoom(m_webView->history()->currentItem().userData().toList().value(ZoomEntryData, getZoom()).toInt());

		if (m_webView->page()->mainFrame()->scrollPosition() == QPoint(0, 0))
		{
			m_webView->page()->mainFrame()->setScrollPosition(m_webView->history()->currentItem().userData().toList().value(PositionEntryData).toPoint());
		}
	}
}

void QtWebKitWebWidget::linkHovered(const QString &link)
{
	setStatusMessage(link, true);
}

void QtWebKitWebWidget::clearPluginToken()
{
	QList<QWebFrame*> frames;
	frames.append(m_page->mainFrame());

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

void QtWebKitWebWidget::openRequest(const QUrl &url, QNetworkAccessManager::Operation operation, QIODevice *outgoingData)
{
	m_formRequestUrl = url;
	m_formRequestOperation = operation;
	m_formRequestBody = (outgoingData ? outgoingData->readAll() : QByteArray());

	if (outgoingData)
	{
		outgoingData->close();
		outgoingData->deleteLater();
	}

	setRequestedUrl(m_formRequestUrl, false, true);
	updateOptions(m_formRequestUrl);

	QTimer::singleShot(50, this, SLOT(openFormRequest()));
}

void QtWebKitWebWidget::openFormRequest()
{
	m_webView->page()->mainFrame()->load(QNetworkRequest(m_formRequestUrl), m_formRequestOperation, m_formRequestBody);

	m_formRequestUrl = QUrl();
	m_formRequestBody = QByteArray();
}

void QtWebKitWebWidget::viewSourceReplyFinished(QNetworkReply::NetworkError error)
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
		data.append(Utils::isUrlEmpty(url) ? 0 : HistoryManager::addEntry(url, getTitle(), m_webView->icon(), m_isTyped));
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
		HistoryManager::updateEntry(identifier, url, getTitle(), m_webView->icon());
	}
}

void QtWebKitWebWidget::handleWindowCloseRequest()
{
	const QString mode(SettingsManager::getValue(SettingsManager::Browser_JavaScriptCanCloseWindowsOption, getUrl()).toString());

	if (mode != QLatin1String("ask"))
	{
		if (mode == QLatin1String("allow"))
		{
			emit requestedCloseWindow();
		}

		return;
	}

	ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-warning")), tr("JavaScript"), tr("Webpage wants to close this tab, do you want to allow to close it?"), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), NULL, this);
	dialog.setCheckBox(tr("Do not show this message again"), false);

	connect(this, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	if (dialog.getCheckBoxState())
	{
		SettingsManager::setValue(SettingsManager::Browser_JavaScriptCanCloseWindowsOption, (dialog.isAccepted() ? QLatin1String("allow") : QLatin1String("disallow")));
	}

	if (dialog.isAccepted())
	{
		emit requestedCloseWindow();
	}
}

void QtWebKitWebWidget::handlePermissionRequest(QWebFrame *frame, QWebPage::Feature feature)
{
	notifyPermissionRequested(frame, feature, false);
}

void QtWebKitWebWidget::handlePermissionCancel(QWebFrame *frame, QWebPage::Feature feature)
{
	notifyPermissionRequested(frame, feature, true);
}

void QtWebKitWebWidget::openFormRequest(const QUrl &url, QNetworkAccessManager::Operation operation, QIODevice *outgoingData)
{
	m_webView->stop();

	QtWebKitWebWidget *widget(new QtWebKitWebWidget(isPrivate(), getBackend(), m_networkManager->clone()));
	widget->setOptions(getOptions());
	widget->setZoom(getZoom());
	widget->openRequest(url, operation, outgoingData);

	emit requestedNewWindow(widget, WindowsManager::calculateOpenHints(WindowsManager::NewTabOpen));
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
				m_webView->page()->setFeaturePermission(frame, nativeFeature, QWebPage::PermissionGrantedByUser);

				break;
			case DeniedPermission:
				m_webView->page()->setFeaturePermission(frame, nativeFeature, QWebPage::PermissionDeniedByUser);

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
	getAction(ActionsManager::UndoAction)->setText(text.isEmpty() ? tr("Undo") : tr("Undo: %1").arg(text));
}

void QtWebKitWebWidget::updateRedoText(const QString &text)
{
	getAction(ActionsManager::RedoAction)->setText(text.isEmpty() ? tr("Redo") : tr("Redo: %1").arg(text));
}

void QtWebKitWebWidget::updateOptions(const QUrl &url)
{
	const QString encoding(getOption(SettingsManager::Content_DefaultCharacterEncodingOption, url).toString());
	QWebSettings *settings(m_webView->page()->settings());
	settings->setAttribute(QWebSettings::AutoLoadImages, (getOption(SettingsManager::Browser_EnableImagesOption, url).toString() != QLatin1String("onlyCached")));
	settings->setAttribute(QWebSettings::PluginsEnabled, getOption(SettingsManager::Browser_EnablePluginsOption, url).toString() != QLatin1String("disabled"));
	settings->setAttribute(QWebSettings::JavaEnabled, getOption(SettingsManager::Browser_EnableJavaOption, url).toBool());
	settings->setAttribute(QWebSettings::JavascriptEnabled, getOption(SettingsManager::Browser_EnableJavaScriptOption, url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, getOption(SettingsManager::Browser_JavaScriptCanAccessClipboardOption, url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanCloseWindows, getOption(SettingsManager::Browser_JavaScriptCanCloseWindowsOption, url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanOpenWindows, getOption(SettingsManager::Browser_JavaScriptCanOpenWindowsOption, url).toBool());
	settings->setAttribute(QWebSettings::WebGLEnabled, getOption(SettingsManager::Browser_EnableWebglOption, url).toBool());
	settings->setAttribute(QWebSettings::LocalStorageEnabled, getOption(SettingsManager::Browser_EnableLocalStorageOption, url).toBool());
	settings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, getOption(SettingsManager::Browser_EnableOfflineStorageDatabaseOption, url).toBool());
	settings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, getOption(SettingsManager::Browser_EnableOfflineWebApplicationCacheOption, url).toBool());
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

	disconnect(m_webView->page(), SIGNAL(geometryChangeRequested(QRect)), this, SIGNAL(requestedGeometryChange(QRect)));
	disconnect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));

	if (getOption(SettingsManager::Browser_JavaScriptCanChangeWindowGeometryOption, url).toBool())
	{
		connect(m_webView->page(), SIGNAL(geometryChangeRequested(QRect)), this, SIGNAL(requestedGeometryChange(QRect)));
	}

	if (getOption(SettingsManager::Browser_JavaScriptCanShowStatusMessagesOption, url).toBool())
	{
		connect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));
	}
	else
	{
		setStatusMessage(QString());
	}

	m_page->updateStyleSheets(url);

	m_networkManager->updateOptions(url);

	m_canLoadPlugins = (getOption(SettingsManager::Browser_EnablePluginsOption, url).toString() == QLatin1String("enabled"));
}

void QtWebKitWebWidget::clearOptions()
{
	WebWidget::clearOptions();

	updateOptions(getUrl());
}

void QtWebKitWebWidget::goToHistoryIndex(int index)
{
	m_page->history()->goToItem(m_webView->history()->itemAt(index));
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

	QList<QWebFrame*> frames;
	frames.append(m_page->mainFrame());

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
				m_webView->page()->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);

				const QWebHitTestResult nativeResult(m_webView->page()->mainFrame()->hitTestContent(getClickPosition()));
				nativeResult.element().evaluateJavaScript(QLatin1String("var event = document.createEvent('MouseEvents'); event.initEvent('click', true, true); this.dispatchEvent(event)"));

				m_webView->page()->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, getOption(SettingsManager::Browser_JavaScriptCanOpenWindowsOption, getUrl()).toBool());

				setClickPosition(QPoint());
			}

			return;
		case ActionsManager::OpenLinkInCurrentTabAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, WindowsManager::CurrentTabOpen);
			}

			return;
		case ActionsManager::OpenLinkInNewTabAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, WindowsManager::calculateOpenHints(WindowsManager::NewTabOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewWindowAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, WindowsManager::calculateOpenHints(WindowsManager::NewWindowOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (WindowsManager::NewWindowOpen | WindowsManager::BackgroundOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateTabAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (WindowsManager::NewTabOpen | WindowsManager::PrivateOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateTabBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen | WindowsManager::PrivateOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateWindowAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (WindowsManager::NewWindowOpen | WindowsManager::PrivateOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, (WindowsManager::NewWindowOpen | WindowsManager::BackgroundOpen | WindowsManager::PrivateOpen));
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
				if (BookmarksManager::hasBookmark(getCurrentHitTestResult().linkUrl))
				{
					emit requestedEditBookmark(getCurrentHitTestResult().linkUrl);
				}
				else
				{
					const QWebHitTestResult hitResult(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position));
					const QString title(getCurrentHitTestResult().title);

					emit requestedAddBookmark(getCurrentHitTestResult().linkUrl, (title.isEmpty() ? hitResult.element().toPlainText() : title), QString());
				}
			}

			return;
		case ActionsManager::SaveLinkToDiskAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				downloadFile(QNetworkRequest(getCurrentHitTestResult().linkUrl));
			}
			else
			{
				m_webView->page()->triggerAction(QWebPage::DownloadLinkToDisk);
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
				openUrl(getCurrentHitTestResult().frameUrl, WindowsManager::calculateOpenHints(WindowsManager::NewTabOpen));
			}

			return;
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
			if (getCurrentHitTestResult().frameUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().frameUrl, (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen));
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
				const QString defaultEncoding(m_webView->page()->settings()->defaultTextEncoding());
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

				connect(reply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));
				connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(viewSourceReplyFinished(QNetworkReply::NetworkError)));

				emit requestedNewWindow(sourceViewer, WindowsManager::DefaultOpen);
			}

			return;
		case ActionsManager::OpenImageInNewTabAction:
			if (!getCurrentHitTestResult().imageUrl.isEmpty())
			{
				openUrl(getCurrentHitTestResult().imageUrl, WindowsManager::calculateOpenHints(WindowsManager::NewTabOpen));
			}

			return;
		case ActionsManager::OpenImageInNewTabBackgroundAction:
			if (!getCurrentHitTestResult().imageUrl.isEmpty())
			{
				openUrl(getCurrentHitTestResult().imageUrl, (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen));
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
					m_webView->page()->triggerAction(QWebPage::CopyImageToClipboard);
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

					m_webView->page()->mainFrame()->documentElement().evaluateJavaScript(QStringLiteral("var images = document.querySelectorAll('img[src=\"%1\"]'); for (var i = 0; i < images.length; ++i) { images[i].src = ''; images[i].src = \'%1\'; }").arg(src));
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

				ContentsWidget *parent(qobject_cast<ContentsWidget*>(parentWidget()));

				if (parent)
				{
					ImagePropertiesDialog *imagePropertiesDialog(new ImagePropertiesDialog(getCurrentHitTestResult().imageUrl, properties, (m_networkManager->cache() ? m_networkManager->cache()->data(getCurrentHitTestResult().imageUrl) : NULL), this));
					imagePropertiesDialog->setButtonsVisible(false);

					ContentsDialog *dialog(new ContentsDialog(ThemesManager::getIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), QString(), QString(), QDialogButtonBox::Close, imagePropertiesDialog, this));

					connect(this, SIGNAL(aboutToReload()), dialog, SLOT(close()));
					connect(imagePropertiesDialog, SIGNAL(finished(int)), dialog, SLOT(close()));

					showDialog(dialog, false);
				}
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
			m_webView->page()->triggerAction(QWebPage::ToggleMediaControls, calculateCheckedState(ActionsManager::MediaControlsAction, parameters));

			return;
		case ActionsManager::MediaLoopAction:
			m_webView->page()->triggerAction(QWebPage::ToggleMediaLoop, calculateCheckedState(ActionsManager::MediaLoopAction, parameters));

			return;
		case ActionsManager::MediaPlayPauseAction:
			m_webView->page()->triggerAction(QWebPage::ToggleMediaPlayPause);

			return;
		case ActionsManager::MediaMuteAction:
			m_webView->page()->triggerAction(QWebPage::ToggleMediaMute);

			return;
		case ActionsManager::GoBackAction:
			m_webView->page()->triggerAction(QWebPage::Back);

			return;
		case ActionsManager::GoForwardAction:
			m_webView->page()->triggerAction(QWebPage::Forward);

			return;
		case ActionsManager::RewindAction:
			m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(0));

			return;
		case ActionsManager::FastForwardAction:
			m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(m_webView->page()->history()->count() - 1));

			return;
		case ActionsManager::StopAction:
			m_webView->page()->triggerAction(QWebPage::Stop);

			return;
		case ActionsManager::StopScheduledReloadAction:
			m_webView->page()->triggerAction(QWebPage::StopScheduledPageRefresh);

			return;
		case ActionsManager::ReloadAction:
			emit aboutToReload();

			m_webView->page()->triggerAction(QWebPage::Stop);
			m_webView->page()->triggerAction(QWebPage::Reload);

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
			m_webView->page()->triggerAction(QWebPage::ReloadAndBypassCache);

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
			m_webView->page()->triggerAction(QWebPage::Undo);

			return;
		case ActionsManager::RedoAction:
			m_webView->page()->triggerAction(QWebPage::Redo);

			return;
		case ActionsManager::CutAction:
			m_webView->page()->triggerAction(QWebPage::Cut);

			return;
		case ActionsManager::CopyAction:
			m_webView->page()->triggerAction(QWebPage::Copy);

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
			m_webView->page()->triggerAction(QWebPage::Paste);

			return;
		case ActionsManager::DeleteAction:
			m_webView->page()->triggerAction(QWebPage::DeleteEndOfWord);

			return;
		case ActionsManager::SelectAllAction:
			m_webView->page()->triggerAction(QWebPage::SelectAll);

			return;
		case ActionsManager::UnselectAction:
			{
				const QWebElement element(m_page->mainFrame()->findFirstElement(QLatin1String(":focus")));

				if (element.tagName().toLower() == QLatin1String("textarea") || element.tagName().toLower() == QLatin1String("input"))
				{
					m_page->triggerAction(QWebPage::MoveToPreviousChar);
				}
				else
				{
					m_page->mainFrame()->documentElement().evaluateJavaScript(QLatin1String("window.getSelection().empty()"));
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
		case ActionsManager::SearchAction:
			quickSearch(getAction(ActionsManager::SearchAction));

			return;
		case ActionsManager::CreateSearchAction:
			{
				const QWebHitTestResult hitResult(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position));
				QWebElement parentElement(hitResult.element().parent());

				while (!parentElement.isNull() && parentElement.tagName().toLower() != QLatin1String("form"))
				{
					parentElement = parentElement.parent();
				}

				const QWebElementCollection inputs(parentElement.findAll(QLatin1String("input:not([disabled])[name], select:not([disabled])[name], textarea:not([disabled])[name]")));

				if (!parentElement.isNull() && parentElement.hasAttribute(QLatin1String("action")) && inputs.count() > 0)
				{
					QUrlQuery parameters;

					for (int i = 0; i < inputs.count(); ++i)
					{
						QString value;

						if (inputs.at(i).tagName().toLower() == QLatin1String("textarea"))
						{
							value = inputs.at(i).toPlainText();
						}
						else if (inputs.at(i).tagName().toLower() == QLatin1String("select"))
						{
							const QWebElementCollection options(inputs.at(i).findAll(QLatin1String("option")));

							for (int j = 0; j < options.count(); ++j)
							{
								if (options.at(j).hasAttribute(QLatin1String("selected")))
								{
									value = options.at(j).attribute(QLatin1String("value"), options.at(j).toPlainText());

									return;
								}
							}
						}
						else
						{
							if (inputs.at(i).attribute(QLatin1String("type")) == QLatin1String("submit") || ((inputs.at(i).attribute(QLatin1String("type")) == QLatin1String("checkbox") || inputs.at(i).attribute(QLatin1String("type")) == QLatin1String("radio")) && !inputs.at(i).hasAttribute(QLatin1String("checked"))))
							{
								continue;
							}

							value = inputs.at(i).attribute(QLatin1String("value"));
						}

						parameters.addQueryItem(inputs.at(i).attribute(QLatin1String("name")), ((inputs.at(i) == hitResult.element()) ? QLatin1String("{searchTerms}") : value));
					}

					const QStringList identifiers(SearchEnginesManager::getSearchEngines());
					const QStringList keywords(SearchEnginesManager::getSearchKeywords());
					const QIcon icon(m_webView->icon());
					const QUrl url(parentElement.attribute(QLatin1String("action")));
					SearchEnginesManager::SearchEngineDefinition searchEngine;
					searchEngine.identifier = Utils::createIdentifier(getUrl().host(), identifiers);
					searchEngine.title = getTitle();
					searchEngine.formUrl = getUrl();
					searchEngine.icon = (icon.isNull() ? ThemesManager::getIcon(QLatin1String("edit-find")) : icon);
					searchEngine.resultsUrl.url = (url.isEmpty() ? getUrl() : resolveUrl(parentElement.webFrame(), url)).toString();
					searchEngine.resultsUrl.enctype = parentElement.attribute(QLatin1String("enctype"));
					searchEngine.resultsUrl.method = ((parentElement.attribute(QLatin1String("method"), QLatin1String("get")).toLower() == QLatin1String("post")) ? QLatin1String("post") : QLatin1String("get"));
					searchEngine.resultsUrl.parameters = parameters;

					SearchEnginePropertiesDialog dialog(searchEngine, keywords, false, this);

					if (dialog.exec() == QDialog::Rejected)
					{
						return;
					}

					SearchEnginesManager::addSearchEngine(dialog.getSearchEngine(), dialog.isDefault());
				}
			}

			return;
		case ActionsManager::ScrollToStartAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), 0));

			return;
		case ActionsManager::ScrollToEndAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), m_webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical)));

			return;
		case ActionsManager::ScrollPageUpAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), qMax(0, (m_webView->page()->mainFrame()->scrollPosition().y() - m_webView->height()))));

			return;
		case ActionsManager::ScrollPageDownAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), qMin(m_webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical), (m_webView->page()->mainFrame()->scrollPosition().y() + m_webView->height()))));

			return;
		case ActionsManager::ScrollPageLeftAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(qMax(0, (m_webView->page()->mainFrame()->scrollPosition().x() - m_webView->width())), m_webView->page()->mainFrame()->scrollPosition().y()));

			return;
		case ActionsManager::ScrollPageRightAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(qMin(m_webView->page()->mainFrame()->scrollBarMaximum(Qt::Horizontal), (m_webView->page()->mainFrame()->scrollPosition().x() + m_webView->width())), m_webView->page()->mainFrame()->scrollPosition().y()));

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
				const QUrl url(getUrl());

				if (BookmarksManager::hasBookmark(url))
				{
					emit requestedEditBookmark(url);
				}
				else
				{
					const QString description(m_page->mainFrame()->findFirstElement(QLatin1String("[name=\"description\"]")).attribute(QLatin1String("content")));

					emit requestedAddBookmark(url, getTitle(), (description.isEmpty() ? m_page->mainFrame()->findFirstElement(QLatin1String("[name=\"og:description\"]")).attribute(QLatin1String("property")) : description));
				}
			}

			return;
		case ActionsManager::LoadPluginsAction:
			{
				m_canLoadPlugins = true;

				QList<QWebFrame*> frames;
				frames.append(m_page->mainFrame());

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
				const QString defaultEncoding(m_webView->page()->settings()->defaultTextEncoding());
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

				connect(reply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));
				connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(viewSourceReplyFinished(QNetworkReply::NetworkError)));

				emit requestedNewWindow(sourceViewer, WindowsManager::DefaultOpen);
			}

			return;
		case ActionsManager::InspectPageAction:
			{
				if (!m_inspector)
				{
					m_page->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

					m_inspector = new QtWebKitInspector(this);
					m_inspector->setPage(m_webView->page());

					m_splitter->addWidget(m_inspector);
				}

				const bool result(calculateCheckedState(ActionsManager::InspectPageAction, parameters));

				m_inspector->setVisible(result);

				getAction(ActionsManager::InspectPageAction)->setChecked(result);

				emit progressBarGeometryChanged();

				return;
			}
		case ActionsManager::InspectElementAction:
			{
				QVariantMap inspectPageParameters;
				inspectPageParameters[QLatin1String("isChecked")] = true;

				triggerAction(ActionsManager::InspectPageAction, inspectPageParameters);

				m_webView->triggerPageAction(QWebPage::InspectElement);
			}

			return;
		case ActionsManager::WebsitePreferencesAction:
			{
				const QUrl url(getUrl());
				WebsitePreferencesDialog dialog(url, m_networkManager->getCookieJar()->getCookies(url.host()), this);

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
		m_webView->page()->history()->clear();

		updateNavigationActions();
		updateOptions(QUrl());
		updatePageActions(QUrl());

		return;
	}

	const int index(qMin(history.index, (m_webView->history()->maximumItemCount() - 1)));

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
	stream >> *(m_webView->page()->history());
#else
	QVariantList entries;

	for (int i = 0; i < history.entries.count(); ++i)
	{
		QVariantMap position;
		position[QLatin1String("x")] = history.entries.at(i).position.x();
		position[QLatin1String("y")] = history.entries.at(i).position.y();

		QVariantMap entry;
		entry[QLatin1String("pageScaleFactor")] = (history.entries.at(i).zoom / qreal(100));
		entry[QLatin1String("title")] = history.entries.at(i).title;
		entry[QLatin1String("urlString")] = QString(QUrl::fromUserInput(history.entries.at(i).url).toEncoded());
		entry[QLatin1String("scrollPosition")] = position;

		entries.append(entry);
	}

	QVariantMap map;
	map[QLatin1String("currentItemIndex")] = index;
	map[QLatin1String("history")] = entries;

	m_webView->page()->history()->loadFromMap(map);
#endif

	for (int i = 0; i < history.entries.count(); ++i)
	{
		QVariantList data;
		data.append(-1);
		data.append(history.entries.at(i).zoom);
		data.append(history.entries.at(i).position);

		m_webView->page()->history()->itemAt(i).setUserData(data);
	}

	m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(index));

	const QUrl url(m_webView->page()->history()->itemAt(index).url());

	setRequestedUrl(url, false, true);
	updateOptions(url);
	updatePageActions(url);

	m_webView->page()->triggerAction(QWebPage::Reload);
}

#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
void QtWebKitWebWidget::setHistory(QDataStream &stream)
{
	stream.device()->reset();
	stream >> *(m_webView->page()->history());

	const QUrl url(m_webView->page()->history()->currentItem().url());

	setRequestedUrl(url, false, true);
	updateOptions(url);
	updatePageActions(url);

	m_webView->page()->triggerAction(QWebPage::Reload);
}
#else
void QtWebKitWebWidget::setHistory(const QVariantMap &history)
{
	m_webView->page()->history()->loadFromMap(history);

	const QUrl url(m_webView->page()->history()->currentItem().url());

	setRequestedUrl(url, false, true);
	updateOptions(url);
	updatePageActions(url);

	m_webView->page()->triggerAction(QWebPage::Reload);
}
#endif

void QtWebKitWebWidget::setZoom(int zoom)
{
	if (zoom != getZoom())
	{
		m_webView->setZoomFactor(qBound(0.1, (static_cast<qreal>(zoom) / 100), static_cast<qreal>(100)));

		SessionsManager::markSessionModified();

		emit zoomChanged(zoom);
		emit progressBarGeometryChanged();
	}
}

void QtWebKitWebWidget::setUrl(const QUrl &url, bool isTyped)
{
	if (url.scheme() == QLatin1String("javascript"))
	{
		m_webView->page()->mainFrame()->documentElement().evaluateJavaScript(url.toDisplayString(QUrl::RemoveScheme | QUrl::DecodeReserved));

		return;
	}

	if (!url.fragment().isEmpty() && url.matches(getUrl(), (QUrl::RemoveFragment | QUrl::StripTrailingSlash | QUrl::NormalizePathSegments)))
	{
		m_webView->page()->mainFrame()->scrollToAnchor(url.fragment());

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

	m_webView->load(targetUrl);

	notifyTitleChanged();
	notifyIconChanged();
}

void QtWebKitWebWidget::setPermission(FeaturePermission feature, const QUrl &url, WebWidget::PermissionPolicies policies)
{
	WebWidget::setPermission(feature, url, policies);

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

	QList<QWebFrame*> frames;
	frames.append(m_page->mainFrame());

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
		m_webView->reload();
	}
	else if (identifier == SettingsManager::Browser_SpellCheckDictionaryOption)
	{
		emit widgetActivated(this);

		resetSpellCheck(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position).element());
	}
}

void QtWebKitWebWidget::setOptions(const QHash<int, QVariant> &options)
{
	WebWidget::setOptions(options);

	updateOptions(getUrl());
}

void QtWebKitWebWidget::setScrollPosition(const QPoint &position)
{
	m_webView->page()->mainFrame()->setScrollPosition(position);
}

WebWidget* QtWebKitWebWidget::clone(bool cloneHistory, bool isPrivate)
{
	QtWebKitWebWidget *widget(new QtWebKitWebWidget((this->isPrivate() || isPrivate), getBackend(), ((this->isPrivate() != isPrivate) ? NULL : m_networkManager->clone()), NULL));
	widget->setOptions(getOptions());

	if (cloneHistory)
	{
#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
		QByteArray data;
		QDataStream stream(&data, QIODevice::ReadWrite);
		stream << *(m_webView->page()->history());

		widget->setHistory(stream);
#else
		widget->setHistory(m_webView->page()->history()->toMap());
#endif
	}

	widget->setZoom(getZoom());

	return widget;
}

Action* QtWebKitWebWidget::getAction(int identifier)
{
	if (identifier == ActionsManager::UndoAction && !getExistingAction(ActionsManager::UndoAction))
	{
		Action *action(WebWidget::getAction(ActionsManager::UndoAction));
		action->setEnabled(m_page->undoStack()->canUndo());

		updateUndoText(m_page->undoStack()->undoText());

		connect(m_page->undoStack(), SIGNAL(canUndoChanged(bool)), action, SLOT(setEnabled(bool)));
		connect(m_page->undoStack(), SIGNAL(undoTextChanged(QString)), this, SLOT(updateUndoText(QString)));

		return action;
	}

	if (identifier == ActionsManager::RedoAction && !getExistingAction(ActionsManager::RedoAction))
	{
		Action *action(WebWidget::getAction(ActionsManager::RedoAction));
		action->setEnabled(m_page->undoStack()->canRedo());

		updateRedoText(m_page->undoStack()->redoText());

		connect(m_page->undoStack(), SIGNAL(canRedoChanged(bool)), action, SLOT(setEnabled(bool)));
		connect(m_page->undoStack(), SIGNAL(redoTextChanged(QString)), this, SLOT(updateRedoText(QString)));

		return action;
	}

	return WebWidget::getAction(identifier);
}

QWebPage* QtWebKitWebWidget::getPage()
{
	return m_webView->page();
}

QString QtWebKitWebWidget::getTitle() const
{
	const QString title(m_webView->title());

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
	return m_webView->selectedText();
}

QString QtWebKitWebWidget::getPasswordToken() const
{
	return m_passwordToken;
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
	const QUrl url(m_webView->url());

	return (url.isEmpty() ? m_webView->page()->mainFrame()->requestedUrl() : url);
}

QIcon QtWebKitWebWidget::getIcon() const
{
	if (isPrivate())
	{
		return ThemesManager::getIcon(QLatin1String("tab-private"));
	}

	const QIcon icon(m_webView->icon());

	return (icon.isNull() ? ThemesManager::getIcon(QLatin1String("tab")) : icon);
}

QPixmap QtWebKitWebWidget::getThumbnail()
{
	if ((!m_thumbnail.isNull() && m_thumbnail.devicePixelRatio() == devicePixelRatio()) || m_loadingState == WindowsManager::OngoingLoadingState)
	{
		return m_thumbnail;
	}

	const QSize thumbnailSize(QSize(260, 170));
	const QSize oldViewportSize(m_webView->page()->viewportSize());
	const QPoint position(m_webView->page()->mainFrame()->scrollPosition());
	const qreal zoom(m_webView->page()->mainFrame()->zoomFactor());
	QSize contentsSize(m_webView->page()->mainFrame()->contentsSize());
	QWidget *newView(new QWidget());
	QWidget *oldView(m_webView->page()->view());

	m_webView->page()->setView(newView);
	m_webView->page()->setViewportSize(contentsSize);
	m_webView->page()->mainFrame()->setZoomFactor(1);

	if (contentsSize.width() > 2000)
	{
		contentsSize.setWidth(2000);
	}

	contentsSize.setHeight(thumbnailSize.height() * (qreal(contentsSize.width()) / thumbnailSize.width()));

	QPixmap pixmap(contentsSize);
	pixmap.fill(Qt::white);

	QPainter painter(&pixmap);

	m_webView->page()->mainFrame()->render(&painter, QWebFrame::ContentsLayer, QRegion(QRect(QPoint(0, 0), contentsSize)));
	m_webView->page()->mainFrame()->setZoomFactor(zoom);
	m_webView->page()->setView(oldView);
	m_webView->page()->setViewportSize(oldViewportSize);
	m_webView->page()->mainFrame()->setScrollPosition(position);

	painter.end();

	pixmap = pixmap.scaled((thumbnailSize * devicePixelRatio()), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	pixmap.setDevicePixelRatio(devicePixelRatio());

	newView->deleteLater();

	m_thumbnail = pixmap;

	return pixmap;
}

QPoint QtWebKitWebWidget::getScrollPosition() const
{
	return m_webView->page()->mainFrame()->scrollPosition();
}

QRect QtWebKitWebWidget::getProgressBarGeometry() const
{
	QRect geometry(QPoint(0, (height() - ((m_inspector && m_inspector->isVisible()) ? m_inspector->height() : 0) - 30)), QSize(width(), 30));
	const QRect horizontalScrollBar(m_webView->page()->mainFrame()->scrollBarGeometry(Qt::Horizontal));
	const QRect verticalScrollBar(m_webView->page()->mainFrame()->scrollBarGeometry(Qt::Vertical));

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

WebWidget::SslInformation QtWebKitWebWidget::getSslInformation() const
{
	return m_networkManager->getSslInformation();
}

WindowHistoryInformation QtWebKitWebWidget::getHistory() const
{
	QVariantList data(m_webView->history()->currentItem().userData().toList());

	if (data.isEmpty() || data.count() < 3)
	{
		data.clear();
		data.append(0);
		data.append(getZoom());
		data.append(m_webView->page()->mainFrame()->scrollPosition());
	}
	else
	{
		data[ZoomEntryData] = getZoom();
		data[PositionEntryData] = m_webView->page()->mainFrame()->scrollPosition();
	}

	m_webView->history()->currentItem().setUserData(data);

	QWebHistory *history(m_webView->history());
	WindowHistoryInformation information;
	information.index = history->currentItemIndex();

	const QString requestedUrl(m_webView->page()->mainFrame()->requestedUrl().toString());
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

	if (m_loadingState == WindowsManager::OngoingLoadingState && requestedUrl != history->itemAt(history->currentItemIndex()).url().toString())
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
	QWebFrame *frame(m_webView->page()->frameAt(position));

	if (!frame)
	{
		return HitTestResult();
	}

	const QWebHitTestResult nativeResult(m_webView->page()->mainFrame()->hitTestContent(position));
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
	const bool isSubmit((result.tagName == QLatin1String("input") || result.tagName == QLatin1String("button")) && (nativeResult.element().attribute(QLatin1String("type")).toLower() == QLatin1String("submit") || nativeResult.element().attribute(QLatin1String("type")).toLower() == QLatin1String("image")));

	if (isSubmit || result.tagName == QLatin1String("input") || result.tagName == QLatin1String("select") || result.tagName == QLatin1String("textarea"))
	{
		while (!parentElement.isNull() && parentElement.tagName().toLower() != QLatin1String("form"))
		{
			parentElement = parentElement.parent();
		}

		if (!parentElement.isNull() && parentElement.hasAttribute(QLatin1String("action")))
		{
			if (isSubmit)
			{
				const QUrl url(parentElement.attribute(QLatin1String("action")));

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

QList<LinkUrl> QtWebKitWebWidget::getFeeds() const
{
	const QWebElementCollection elements(m_webView->page()->mainFrame()->findAllElements(QLatin1String("a[type=\"application/atom+xml\"], a[type=\"application/rss+xml\"], link[type=\"application/atom+xml\"], link[type=\"application/rss+xml\"]")));
	QList<LinkUrl> links;
	QSet<QUrl> urls;

	for (int i = 0; i < elements.count(); ++i)
	{
		QUrl url(resolveUrl(m_webView->page()->mainFrame(), QUrl(elements.at(i).attribute(QLatin1String("href")))));

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

QList<LinkUrl> QtWebKitWebWidget::getSearchEngines() const
{
	const QWebElementCollection elements(m_webView->page()->mainFrame()->findAllElements(QLatin1String("link[type=\"application/opensearchdescription+xml\"]")));
	QList<LinkUrl> links;
	QSet<QUrl> urls;

	for (int i = 0; i < elements.count(); ++i)
	{
		QUrl url(resolveUrl(m_webView->page()->mainFrame(), QUrl(elements.at(i).attribute(QLatin1String("href")))));

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

QList<NetworkManager::ResourceInformation> QtWebKitWebWidget::getBlockedRequests() const
{
	return m_networkManager->getBlockedRequests();
}

QHash<QByteArray, QByteArray> QtWebKitWebWidget::getHeaders() const
{
	return m_networkManager->getHeaders();
}

WindowsManager::ContentStates QtWebKitWebWidget::getContentState() const
{
	const QUrl url(getUrl());

	if (url.isEmpty() || url.scheme() == QLatin1String("about"))
	{
		return WindowsManager::ApplicationContentState;
	}

	if (url.scheme() == QLatin1String("file"))
	{
		return WindowsManager::LocalContentState;
	}

	return (m_networkManager->getContentState() | WindowsManager::RemoteContentState);
}

WindowsManager::LoadingState QtWebKitWebWidget::getLoadingState() const
{
	return m_loadingState;
}

int QtWebKitWebWidget::getZoom() const
{
	return (m_webView->zoomFactor() * 100);
}

int QtWebKitWebWidget::getAmountOfNotLoadedPlugins() const
{
	return findChildren<QtWebKitPluginWidget*>().count();
}

bool QtWebKitWebWidget::canLoadPlugins() const
{
	return m_canLoadPlugins;
}

bool QtWebKitWebWidget::canGoBack() const
{
	return m_webView->history()->canGoBack();
}

bool QtWebKitWebWidget::canGoForward() const
{
	return m_webView->history()->canGoForward();
}

bool QtWebKitWebWidget::canShowContextMenu(const QPoint &position) const
{
	QContextMenuEvent menuEvent(QContextMenuEvent::Other, position, m_webView->mapToGlobal(position), Qt::NoModifier);

	return !m_page->swallowContextMenuEvent(&menuEvent);
}

bool QtWebKitWebWidget::canViewSource() const
{
	return !m_page->isViewingMedia();
}

bool QtWebKitWebWidget::hasSelection() const
{
	return m_page->hasSelection();
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

bool QtWebKitWebWidget::isPrivate() const
{
	return m_webView->settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled);
}

bool QtWebKitWebWidget::isInspecting() const
{
	return (m_inspector && m_inspector->isVisible());
}

bool QtWebKitWebWidget::isNavigating() const
{
	return m_isNavigating;
}

bool QtWebKitWebWidget::isScrollBar(const QPoint &position) const
{
	return (m_page->mainFrame()->scrollBarGeometry(Qt::Horizontal).contains(position) || m_page->mainFrame()->scrollBarGeometry(Qt::Vertical).contains(position));
}

bool QtWebKitWebWidget::findInPage(const QString &text, FindFlags flags)
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
		m_webView->findText(QString(), QWebPage::HighlightAllOccurrences);
		m_webView->findText(text, (nativeFlags | QWebPage::HighlightAllOccurrences));
	}

	return m_webView->findText(text, nativeFlags);
}

bool QtWebKitWebWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_webView)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (event->type() == QEvent::MouseButtonPress && mouseEvent && mouseEvent->button() == Qt::LeftButton && SettingsManager::getValue(SettingsManager::Browser_EnablePluginsOption, getUrl()).toString() == QLatin1String("onDemand"))
		{
			QWidget *widget(childAt(mouseEvent->pos()));
			const QWebHitTestResult hitResult(m_webView->page()->mainFrame()->hitTestContent(mouseEvent->pos()));
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
				QVariantMap parameters;
				parameters[QLatin1String("context")] = contextMenuEvent->reason();

				triggerAction(ActionsManager::ContextMenuAction, parameters);
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
