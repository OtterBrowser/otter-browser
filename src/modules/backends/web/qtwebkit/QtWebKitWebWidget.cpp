/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include "QtWebKitNetworkManager.h"
#include "QtWebKitPluginFactory.h"
#include "QtWebKitPluginWidget.h"
#include "QtWebKitPage.h"
#include "../../../../core/ActionsManager.h"
#include "../../../../core/BookmarksManager.h"
#include "../../../../core/Console.h"
#include "../../../../core/CookieJar.h"
#include "../../../../core/ContentBlockingManager.h"
#include "../../../../core/HistoryManager.h"
#include "../../../../core/NetworkCache.h"
#include "../../../../core/NetworkManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/NotesManager.h"
#include "../../../../core/SearchesManager.h"
#include "../../../../core/SessionsManager.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/Transfer.h"
#include "../../../../core/TransfersManager.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/ContentsWidget.h"
#include "../../../../ui/ImagePropertiesDialog.h"
#include "../../../../ui/MainWindow.h"
#include "../../../../ui/SearchPropertiesDialog.h"
#include "../../../../ui/SourceViewerWebWidget.h"
#include "../../../../ui/WebsitePreferencesDialog.h"

#include <QtCore/QDataStream>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtGui/QClipboard>
#include <QtGui/QImageWriter>
#include <QtGui/QMouseEvent>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QtWebKitVersion>
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
	m_inspectorCloseButton(NULL),
	m_networkManager(networkManager),
	m_splitter(new QSplitter(Qt::Vertical, this)),
	m_canLoadPlugins(false),
	m_ignoreContextMenu(false),
	m_ignoreContextMenuNextTime(false),
	m_isUsingRockerNavigation(false),
	m_isLoading(false),
	m_isTyped(false),
	m_isNavigating(false)
{
	m_splitter->addWidget(m_webView);
	m_splitter->setChildrenCollapsible(false);
	m_splitter->setContentsMargins(0, 0, 0, 0);

	QVBoxLayout *layout = new QVBoxLayout(this);
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

	m_webView->setPage(m_page);
	m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_webView->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, isPrivate);
	m_webView->installEventFilter(this);

	if (isPrivate)
	{
		m_webView->settings()->setIconDatabasePath(QString());
	}

	QShortcut *selectAllShortcut = new QShortcut(QKeySequence(QKeySequence::SelectAll), this, 0, 0, Qt::WidgetWithChildrenShortcut);

	optionChanged(QLatin1String("Browser/JavaScriptCanShowStatusMessages"), SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanShowStatusMessages")));
	optionChanged(QLatin1String("Content/BackgroundColor"), SettingsManager::getValue(QLatin1String("Content/BackgroundColor")));
	optionChanged(QLatin1String("History/BrowsingLimitAmountWindow"), SettingsManager::getValue(QLatin1String("History/BrowsingLimitAmountWindow")));
	updateEditActions();
	setZoom(SettingsManager::getValue(QLatin1String("Content/DefaultZoom")).toInt());

	connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(updateBookmarkActions()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(m_page, SIGNAL(aboutToNavigate(QWebFrame*,QWebPage::NavigationType)), this, SLOT(navigating(QWebFrame*,QWebPage::NavigationType)));
	connect(m_page, SIGNAL(requestedNewWindow(WebWidget*,OpenHints)), this, SIGNAL(requestedNewWindow(WebWidget*,OpenHints)));
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
	connect(m_page, SIGNAL(viewingMediaChanged(bool)), this, SLOT(updateNavigationActions()));
	connect(m_page->mainFrame(), SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
	connect(m_page->mainFrame(), SIGNAL(contentsSizeChanged(QSize)), this, SIGNAL(progressBarGeometryChanged()));
	connect(m_page->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SIGNAL(progressBarGeometryChanged()));
	connect(m_webView, SIGNAL(titleChanged(QString)), this, SLOT(notifyTitleChanged()));
	connect(m_webView, SIGNAL(urlChanged(QUrl)), this, SLOT(notifyUrlChanged(QUrl)));
	connect(m_webView, SIGNAL(iconChanged()), this, SLOT(notifyIconChanged()));
	connect(m_networkManager, SIGNAL(messageChanged(QString)), this, SIGNAL(loadMessageChanged(QString)));
	connect(m_networkManager, SIGNAL(statusChanged(int,int,qint64,qint64,qint64)), this, SIGNAL(loadStatusChanged(int,int,qint64,qint64,qint64)));
	connect(m_networkManager, SIGNAL(documentLoadProgressChanged(int)), this, SIGNAL(loadProgress(int)));
	connect(m_splitter, SIGNAL(splitterMoved(int,int)), this, SIGNAL(progressBarGeometryChanged()));
	connect(selectAllShortcut, SIGNAL(activated()), getAction(ActionsManager::SelectAllAction), SLOT(trigger()));
}

QtWebKitWebWidget::~QtWebKitWebWidget()
{
	m_webView->stop();
	m_webView->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
}

void QtWebKitWebWidget::focusInEvent(QFocusEvent *event)
{
	WebWidget::focusInEvent(event);

	m_webView->setFocus();

	if (m_inspector && m_inspector->isVisible())
	{
		m_inspectorCloseButton->raise();
	}
}

void QtWebKitWebWidget::search(const QString &query, const QString &engine)
{
	QNetworkRequest request;
	QNetworkAccessManager::Operation method;
	QByteArray body;

	if (SearchesManager::setupSearchQuery(query, engine, &request, &method, &body))
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

void QtWebKitWebWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Browser/JavaScriptCanShowStatusMessages"))
	{
		disconnect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));

		if (value.toBool() || SettingsManager::getValue(option, getUrl()).toBool())
		{
			connect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));
		}
		else
		{
			setStatusMessage(QString());
		}
	}
	else if (option == QLatin1String("Content/BackgroundColor"))
	{
		QPalette palette = m_page->palette();
		palette.setColor(QPalette::Base, QColor(value.toString()));

		m_page->setPalette(palette);
	}
	else if (option == QLatin1String("History/BrowsingLimitAmountWindow"))
	{
		m_webView->page()->history()->setMaximumItemCount(value.toInt());
	}
}

void QtWebKitWebWidget::navigating(QWebFrame *frame, QWebPage::NavigationType type)
{
	if (frame == m_page->mainFrame())
	{
		if (type != QWebPage::NavigationTypeBackOrForward)
		{
			pageLoadStarted();
			handleHistory();
		}

		m_isNavigating = true;
	}
}

void QtWebKitWebWidget::pageLoadStarted()
{
	m_canLoadPlugins = (getOption(QLatin1String("Browser/EnablePlugins"), getUrl()).toString() == QLatin1String("enabled"));
	m_isLoading = true;
	m_thumbnail = QPixmap();

	updateNavigationActions();
	setStatusMessage(QString());
	setStatusMessage(QString(), true);

	emit progressBarGeometryChanged();
	emit loadingChanged(true);
}

void QtWebKitWebWidget::pageLoadFinished()
{
	if (!m_isLoading)
	{
		return;
	}

	m_isLoading = false;
	m_thumbnail = QPixmap();

	m_networkManager->resetStatistics();

	updateNavigationActions();
	handleHistory();
	startReloadTimer();

	emit loadingChanged(false);
}

void QtWebKitWebWidget::downloadFile(const QNetworkRequest &request)
{
	if ((!getCurrentHitTestResult().imageUrl.isEmpty() && request.url() == getCurrentHitTestResult().imageUrl) || (!getCurrentHitTestResult().mediaUrl.isEmpty() && request.url() == getCurrentHitTestResult().mediaUrl))
	{
		NetworkCache *cache = NetworkManagerFactory::getCache();

		if (cache && cache->metaData(request.url()).isValid())
		{
			QIODevice *device = cache->data(request.url());

			if (device && device->size() > 0)
			{
				const QString path = TransfersManager::getSavePath(request.url().fileName());

				if (path.isEmpty())
				{
					device->deleteLater();

					return;
				}

				QFile file(path);

				if (!file.open(QFile::WriteOnly))
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
			const QString imageUrl = getCurrentHitTestResult().imageUrl.url();
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

			return;
		}

		QNetworkRequest mutableRequest(request);
		mutableRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

		new Transfer(m_networkManager->get(mutableRequest), QString(), (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::IsPrivateOption));
	}
	else
	{
		startTransfer(new Transfer(request, QString(), (Transfer::CanNotifyOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));
	}
}

void QtWebKitWebWidget::downloadFile(QNetworkReply *reply)
{
	m_networkManager->registerTransfer(reply);

	startTransfer(new Transfer(reply, QString(), (Transfer::CanNotifyOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));
}

void QtWebKitWebWidget::saveState(QWebFrame *frame, QWebHistoryItem *item)
{
	if (frame == m_webView->page()->mainFrame())
	{
		QVariantList data = m_webView->history()->currentItem().userData().toList();

		if (data.isEmpty() || data.length() < 3)
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

void QtWebKitWebWidget::hideInspector()
{
	QVariantMap parameters;
	parameters[QLatin1String("isChecked")] = false;

	triggerAction(ActionsManager::InspectPageAction, parameters);
}

void QtWebKitWebWidget::linkHovered(const QString &link)
{
	setStatusMessage(link, true);
}

void QtWebKitWebWidget::clearPluginToken()
{
	m_pluginToken = QString();

	Action *loadPluginsAction = getExistingAction(ActionsManager::LoadPluginsAction);

	if (loadPluginsAction)
	{
		loadPluginsAction->setEnabled(true);
	}
}

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
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

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

	const QUrl url = getUrl();
	const qint64 identifier = m_page->history()->currentItem().userData().toList().value(IdentifierEntryData).toLongLong();

	if (identifier == 0)
	{
		QVariantList data;
		data.append(HistoryManager::addEntry(url, getTitle(), m_webView->icon(), m_isTyped));
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

	if (dialog.getCheckBoxState())
	{
		SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanCloseWindows"), (dialog.isAccepted() ? QLatin1String("allow") : QLatin1String("disallow")));
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

	QtWebKitWebWidget *widget = new QtWebKitWebWidget(isPrivate(), getBackend(), m_networkManager->clone());
	widget->setOptions(getOptions());
	widget->setZoom(getZoom());
	widget->openRequest(url, operation, outgoingData);

	emit requestedNewWindow(widget, NewTabOpen);
}

void QtWebKitWebWidget::pasteText(const QString &text)
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

void QtWebKitWebWidget::notifyPermissionRequested(QWebFrame *frame, QWebPage::Feature feature, bool cancel)
{
	QString option;

	if (feature == QWebPage::Geolocation)
	{
		option = QLatin1String("Browser/EnableGeolocation");
	}
	else if (feature == QWebPage::Notifications)
	{
		option = QLatin1String("Browser/EnableNotifications");
	}

	if (!option.isEmpty())
	{
		if (cancel)
		{
			emit requestedPermission(option, frame->url(), true);
		}
		else
		{
			const QString value = SettingsManager::getValue(option, frame->url()).toString();

			if (value == QLatin1String("allow"))
			{
				m_page->setFeaturePermission(frame, feature, QWebPage::PermissionGrantedByUser);
			}
			else if (value == QLatin1String("disallow"))
			{
				m_page->setFeaturePermission(frame, feature, QWebPage::PermissionDeniedByUser);
			}
			else
			{
				emit requestedPermission(option, frame->requestedUrl(), false);
			}
		}
	}
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
	QWebSettings *settings = m_webView->page()->settings();
	settings->setAttribute(QWebSettings::AutoLoadImages, getOption(QLatin1String("Browser/EnableImages"), url).toBool());
	settings->setAttribute(QWebSettings::PluginsEnabled, getOption(QLatin1String("Browser/EnablePlugins"), url).toString() != QLatin1String("disabled"));
	settings->setAttribute(QWebSettings::JavaEnabled, getOption(QLatin1String("Browser/EnableJava"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptEnabled, getOption(QLatin1String("Browser/EnableJavaScript"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, getOption(QLatin1String("Browser/JavaScriptCanAccessClipboard"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanCloseWindows, getOption(QLatin1String("Browser/JavaScriptCanCloseWindows"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanOpenWindows, getOption(QLatin1String("Browser/JavaScriptCanOpenWindows"), url).toBool());
	settings->setAttribute(QWebSettings::LocalStorageEnabled, getOption(QLatin1String("Browser/EnableLocalStorage"), url).toBool());
	settings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, getOption(QLatin1String("Browser/EnableOfflineStorageDatabase"), url).toBool());
	settings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, getOption(QLatin1String("Browser/EnableOfflineWebApplicationCache"), url).toBool());
	settings->setDefaultTextEncoding(getOption(QLatin1String("Content/DefaultCharacterEncoding"), url).toString());

	disconnect(m_webView->page(), SIGNAL(geometryChangeRequested(QRect)), this, SIGNAL(requestedGeometryChange(QRect)));
	disconnect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));

	if (getOption(QLatin1String("Browser/JavaScriptCanChangeWindowGeometry"), url).toBool())
	{
		connect(m_webView->page(), SIGNAL(geometryChangeRequested(QRect)), this, SIGNAL(requestedGeometryChange(QRect)));
	}

	if (getOption(QLatin1String("Browser/JavaScriptCanShowStatusMessages"), url).toBool())
	{
		connect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));
	}
	else
	{
		setStatusMessage(QString());
	}

	m_contentBlockingProfiles = ContentBlockingManager::getProfileList(getOption(QLatin1String("Content/BlockingProfiles"), url).toStringList());

	m_page->updateStyleSheets(url);

	m_networkManager->updateOptions(url);

	m_canLoadPlugins = (getOption(QLatin1String("Browser/EnablePlugins"), url).toString() == QLatin1String("enabled"));
}

void QtWebKitWebWidget::clearOptions()
{
	WebWidget::clearOptions();

	updateOptions(getUrl());
}

void QtWebKitWebWidget::clearSelection()
{
	const QWebElement element = m_page->mainFrame()->findFirstElement(QLatin1String(":focus"));

	if (element.tagName().toLower() == QLatin1String("textarea") || element.tagName().toLower() == QLatin1String("input"))
	{
		m_page->triggerAction(QWebPage::MoveToPreviousChar);
	}
	else
	{
		m_page->mainFrame()->evaluateJavaScript(QLatin1String("window.getSelection().empty()"));
	}
}

void QtWebKitWebWidget::goToHistoryIndex(int index)
{
	m_webView->history()->goToItem(m_webView->history()->itemAt(index));
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
				const QString path = TransfersManager::getSavePath(suggestSaveFileName());

				if (!path.isEmpty())
				{
					QNetworkRequest request(getUrl());
					request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

					new Transfer(m_networkManager->get(request), path, (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::CanOverwriteOption | Transfer::IsPrivateOption));
				}
			}

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
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, CurrentTabOpen);
			}

			return;
		case ActionsManager::OpenLinkInNewTabAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, WindowsManager::calculateOpenHints(QGuiApplication::keyboardModifiers(), Qt::LeftButton, NewTabOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, NewBackgroundTabOpen);
			}

			return;
		case ActionsManager::OpenLinkInNewWindowAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, WindowsManager::calculateOpenHints(QGuiApplication::keyboardModifiers(), Qt::LeftButton, NewWindowOpen));
			}

			return;
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, NewBackgroundWindowOpen);
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateTabAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, NewPrivateTabOpen);
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateTabBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, NewPrivateBackgroundTabOpen);
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateWindowAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, NewPrivateWindowOpen);
			}

			return;
		case ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction:
			if (getCurrentHitTestResult().linkUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().linkUrl, NewPrivateBackgroundWindowOpen);
			}

			return;
		case ActionsManager::CopyLinkToClipboardAction:
			if (!getCurrentHitTestResult().linkUrl.isEmpty())
			{
				QGuiApplication::clipboard()->setText(getCurrentHitTestResult().linkUrl.toString());
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
					const QWebHitTestResult hitResult = m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position);
					const QString title = getCurrentHitTestResult().title;

					emit requestedAddBookmark(getCurrentHitTestResult().linkUrl, (title.isEmpty() ? hitResult.element().toPlainText() : title), QString());
				}
			}

			return;
		case ActionsManager::SaveLinkToDiskAction:
			m_webView->page()->triggerAction(QWebPage::DownloadLinkToDisk);

			return;
		case ActionsManager::SaveLinkToDownloadsAction:
			startTransfer(new Transfer(getCurrentHitTestResult().linkUrl.toString(), QString(), (Transfer::CanNotifyOption | Transfer::IsQuickTransferOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));

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
				openUrl(getCurrentHitTestResult().frameUrl, WindowsManager::calculateOpenHints(QGuiApplication::keyboardModifiers(), Qt::LeftButton, NewTabOpen));
			}

			return;
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
			if (getCurrentHitTestResult().frameUrl.isValid())
			{
				openUrl(getCurrentHitTestResult().frameUrl, NewBackgroundTabOpen);
			}

			return;
		case ActionsManager::CopyFrameLinkToClipboardAction:
			if (getCurrentHitTestResult().frameUrl.isValid())
			{
				QGuiApplication::clipboard()->setText(getCurrentHitTestResult().frameUrl.toString());
			}

			return;
		case ActionsManager::ReloadFrameAction:
			if (getCurrentHitTestResult().frameUrl.isValid())
			{
				const QUrl url = getCurrentHitTestResult().frameUrl;
				QWebHitTestResult hitResult = m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position);

				if (hitResult.frame())
				{
					hitResult.frame()->setUrl(QUrl());
					hitResult.frame()->setUrl(url);
				}
			}

			return;
		case ActionsManager::ViewFrameSourceAction:
			if (getCurrentHitTestResult().frameUrl.isValid())
			{
				QNetworkRequest request(getCurrentHitTestResult().frameUrl);
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

				QNetworkReply *reply = m_networkManager->get(request);
				SourceViewerWebWidget *sourceViewer = new SourceViewerWebWidget(isPrivate());
				sourceViewer->setRequestedUrl(QUrl(QLatin1String("view-source:") + getCurrentHitTestResult().frameUrl.toString()));

				m_viewSourceReplies[reply] = sourceViewer;

				connect(reply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));
				connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(viewSourceReplyFinished(QNetworkReply::NetworkError)));

				emit requestedNewWindow(sourceViewer, DefaultOpen);
			}

			return;
		case ActionsManager::OpenImageInNewTabAction:
			if (!getCurrentHitTestResult().imageUrl.isEmpty())
			{
				openUrl(getCurrentHitTestResult().imageUrl, WindowsManager::calculateOpenHints(QGuiApplication::keyboardModifiers(), Qt::LeftButton, NewTabOpen));
			}

			return;
		case ActionsManager::OpenImageInNewTabBackgroundAction:
			if (!getCurrentHitTestResult().imageUrl.isEmpty())
			{
				openUrl(getCurrentHitTestResult().imageUrl, NewBackgroundTabOpen);
			}

			return;
		case ActionsManager::SaveImageToDiskAction:
			if (getCurrentHitTestResult().imageUrl.isValid())
			{
				downloadFile(QNetworkRequest(getCurrentHitTestResult().imageUrl));
			}

			return;
		case ActionsManager::CopyImageToClipboardAction:
			m_webView->page()->triggerAction(QWebPage::CopyImageToClipboard);

			return;
		case ActionsManager::CopyImageUrlToClipboardAction:
			if (!getCurrentHitTestResult().imageUrl.isEmpty())
			{
				QApplication::clipboard()->setText(getCurrentHitTestResult().imageUrl.toString());
			}

			return;
		case ActionsManager::ReloadImageAction:
			if (!getCurrentHitTestResult().imageUrl.isEmpty())
			{
				if (getUrl().matches(getCurrentHitTestResult().imageUrl, (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash)))
				{
					triggerAction(ActionsManager::ReloadAndBypassCacheAction);
				}
				else
				{
					const QWebHitTestResult hitResult = m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position);
					const QUrl url(getCurrentHitTestResult().imageUrl);
					const QString src = hitResult.element().attribute(QLatin1String("src"));
					NetworkCache *cache = NetworkManagerFactory::getCache();

					hitResult.element().setAttribute(QLatin1String("src"), QString());

					if (cache)
					{
						cache->remove(url);
					}

					hitResult.element().setAttribute(QLatin1String("src"), src);

					m_webView->page()->mainFrame()->evaluateJavaScript(QStringLiteral("var images = document.querySelectorAll('img[src=\"%1\"]'); for (var i = 0; i < images.length; ++i) { images[i].src = ''; images[i].src = \'%1\'; }").arg(src));
				}
			}

			return;
		case ActionsManager::ImagePropertiesAction:
			{
				QVariantMap properties;
				properties[QLatin1String("alternativeText")] = getCurrentHitTestResult().alternateText;
				properties[QLatin1String("longDescription")] = getCurrentHitTestResult().longDescription;

				const QWebHitTestResult hitResult = m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position);

				if (!hitResult.pixmap().isNull())
				{
					properties[QLatin1String("width")] = hitResult.pixmap().width();
					properties[QLatin1String("height")] = hitResult.pixmap().height();
					properties[QLatin1String("depth")] = hitResult.pixmap().depth();
				}

				ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());
				ImagePropertiesDialog *imagePropertiesDialog = new ImagePropertiesDialog(getCurrentHitTestResult().imageUrl, properties, (m_networkManager->cache() ? m_networkManager->cache()->data(getCurrentHitTestResult().imageUrl) : NULL), this);
				imagePropertiesDialog->setButtonsVisible(false);

				if (parent)
				{
					ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), QString(), QString(), QDialogButtonBox::Close, imagePropertiesDialog, this);

					connect(this, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

					showDialog(&dialog);
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
				QApplication::clipboard()->setText(getCurrentHitTestResult().mediaUrl.toString());
			}

			return;
#if QTWEBKIT_VERSION >= 0x050200
		case ActionsManager::MediaControlsAction:
			m_webView->page()->triggerAction(QWebPage::ToggleMediaControls, parameters.value(QLatin1String("isChecked")).toBool());

			return;
		case ActionsManager::MediaLoopAction:
			m_webView->page()->triggerAction(QWebPage::ToggleMediaLoop, parameters.value(QLatin1String("isChecked")).toBool());

			return;
		case ActionsManager::MediaPlayPauseAction:
			m_webView->page()->triggerAction(QWebPage::ToggleMediaPlayPause);

			return;
		case ActionsManager::MediaMuteAction:
			m_webView->page()->triggerAction(QWebPage::ToggleMediaMute);

			return;
#endif
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
			m_webView->page()->triggerAction(QWebPage::ReloadAndBypassCache);

			return;
		case ActionsManager::ContextMenuAction:
			{
				if (getContextMenuReason() != QContextMenuEvent::Mouse)
				{
					const QWebElement element = m_page->mainFrame()->findFirstElement(QLatin1String(":focus"));

					if (element.isNull())
					{
						setClickPosition(m_webView->mapFromGlobal(QCursor::pos()));
					}
					else
					{
						QPoint clickPosition = element.geometry().center();
						QWebFrame *frame = element.webFrame();

						while (frame)
						{
							clickPosition -= frame->scrollPosition();

							frame = frame->parentFrame();
						}

						setClickPosition(clickPosition);
					}
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
			m_webView->page()->triggerAction(QWebPage::Paste);

			return;
		case ActionsManager::DeleteAction:
			m_webView->page()->triggerAction(QWebPage::DeleteEndOfWord);

			return;
		case ActionsManager::SelectAllAction:
			m_webView->page()->triggerAction(QWebPage::SelectAll);

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
				const QWebHitTestResult hitResult = m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().position);
				QWebElement parentElement = hitResult.element().parent();

				while (!parentElement.isNull() && parentElement.tagName().toLower() != QLatin1String("form"))
				{
					parentElement = parentElement.parent();
				}

				const QWebElementCollection inputs = parentElement.findAll(QLatin1String("input:not([disabled])[name], select:not([disabled])[name], textarea:not([disabled])[name]"));

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
							const QWebElementCollection options = inputs.at(i).findAll(QLatin1String("option"));

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
							if ((inputs.at(i).attribute(QLatin1String("type")) == QLatin1String("checkbox") || inputs.at(i).attribute(QLatin1String("type")) == QLatin1String("radio")) && !inputs.at(i).hasAttribute(QLatin1String("checked")))
							{
								continue;
							}

							value = inputs.at(i).attribute(QLatin1String("value"));
						}

						parameters.addQueryItem(inputs.at(i).attribute(QLatin1String("name")), ((inputs.at(i) == hitResult.element()) ? QLatin1String("{searchTerms}") : value));
					}

					const QStringList identifiers = SearchesManager::getSearchEngines();
					const QStringList keywords = SearchesManager::getSearchKeywords();
					const QIcon icon = m_webView->icon();
					const QUrl url(parentElement.attribute(QLatin1String("action")));
					SearchInformation engine;
					engine.identifier = Utils::createIdentifier(getUrl().host(), identifiers);
					engine.title = getTitle();
					engine.icon = (icon.isNull() ? Utils::getIcon(QLatin1String("edit-find")) : icon);
					engine.resultsUrl.url = (url.isEmpty() ? getUrl() : (url.isRelative() ? getUrl().resolved(url) : url)).toString();
					engine.resultsUrl.enctype = parentElement.attribute(QLatin1String("enctype"));
					engine.resultsUrl.method = ((parentElement.attribute(QLatin1String("method"), QLatin1String("get")).toLower() == QLatin1String("post")) ? QLatin1String("post") : QLatin1String("get"));
					engine.resultsUrl.parameters = parameters;

					SearchPropertiesDialog dialog(engine, keywords, false, this);

					if (dialog.exec() == QDialog::Rejected)
					{
						return;
					}

					SearchesManager::addSearchEngine(dialog.getSearchEngine(), dialog.isDefault());
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

				QWebElement element = m_page->mainFrame()->findFirstElement(QLatin1String(":focus"));

				if (element.tagName().toLower() == QLatin1String("textarea") || element.tagName().toLower() == QLatin1String("input"))
				{
					m_page->mainFrame()->evaluateJavaScript(QLatin1String("document.activeElement.blur()"));
				}
			}

			return;
		case ActionsManager::AddBookmarkAction:
			{
				const QUrl url = getUrl();

				if (BookmarksManager::hasBookmark(url))
				{
					emit requestedEditBookmark(url);
				}
				else
				{
					const QString description = m_page->mainFrame()->findFirstElement(QLatin1String("[name=\"description\"]")).attribute(QLatin1String("content"));

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
					QWebFrame *frame = frames.takeFirst();
					const QWebElementCollection elements = frame->documentElement().findAll(QLatin1String("object, embed"));

					for (int i = 0; i < elements.count(); ++i)
					{
						elements.at(i).replace(elements.at(i).clone());
					}

					frames.append(frame->childFrames());
				}

				Action *loadPluginsAction = getExistingAction(ActionsManager::LoadPluginsAction);

				if (loadPluginsAction)
				{
					loadPluginsAction->setEnabled(findChildren<QtWebKitPluginWidget*>().count() > 0);
				}
			}

			return;
		case ActionsManager::ViewSourceAction:
			if (canViewSource())
			{
				QNetworkRequest request(getUrl());
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

				QNetworkReply *reply = m_networkManager->get(request);
				SourceViewerWebWidget *sourceViewer = new SourceViewerWebWidget(isPrivate());
				sourceViewer->setRequestedUrl(QUrl(QLatin1String("view-source:") + getUrl().toString()));

				m_viewSourceReplies[reply] = sourceViewer;

				connect(reply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));
				connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(viewSourceReplyFinished(QNetworkReply::NetworkError)));

				emit requestedNewWindow(sourceViewer, DefaultOpen);
			}

			return;
		case ActionsManager::InspectPageAction:
			if (!m_inspector)
			{
				m_inspector = new QWebInspector(this);
				m_inspector->setPage(m_webView->page());
				m_inspector->setContextMenuPolicy(Qt::NoContextMenu);
				m_inspector->setMinimumHeight(200);
				m_inspector->installEventFilter(this);

				m_splitter->addWidget(m_inspector);

				m_inspectorCloseButton = new QToolButton(m_inspector);
				m_inspectorCloseButton->setAutoFillBackground(false);
				m_inspectorCloseButton->setAutoRaise(true);
				m_inspectorCloseButton->setIcon(Utils::getIcon(QLatin1String("window-close")));
				m_inspectorCloseButton->setToolTip(tr("Close"));

				connect(m_inspectorCloseButton, SIGNAL(clicked()), this, SLOT(hideInspector()));
			}

			m_inspector->setVisible(parameters.value(QLatin1String("isChecked")).toBool());

			if (parameters.value(QLatin1String("isChecked")).toBool())
			{
				m_inspectorCloseButton->setFixedSize(16, 16);
				m_inspectorCloseButton->show();
				m_inspectorCloseButton->raise();
				m_inspectorCloseButton->move(QPoint((m_inspector->width() - 19), 3));
			}
			else
			{
				m_inspectorCloseButton->hide();
			}

			getAction(ActionsManager::InspectPageAction)->setChecked(parameters.value(QLatin1String("isChecked")).toBool());

			emit progressBarGeometryChanged();

			return;
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

	const int index = qMin(history.index, (m_webView->history()->maximumItemCount() - 1));
	qint64 documentSequence = 0;
	qint64 itemSequence = 0;
	QByteArray data;
	QDataStream stream(&data, QIODevice::ReadWrite);
	stream << int(2) << history.entries.count() << index;

	for (int i = 0; i < history.entries.count(); ++i)
	{
		stream << QString(QUrl::toPercentEncoding(history.entries.at(i).url, QByteArray(":/#?&+=@%*"))) << history.entries.at(i).title << history.entries.at(i).url << quint32(2) << quint64(0) << ++documentSequence << quint64(0) << QString() << false << ++itemSequence << QString() << qint32(history.entries.at(i).position.x()) << qint32(history.entries.at(i).position.y()) << qreal(1) << false << QString() << false;
	}

	stream.device()->reset();
	stream >> *(m_webView->page()->history());

	for (int i = 0; i < history.entries.count(); ++i)
	{
		QVariantList data;
		data.append(-1);
		data.append(history.entries.at(i).zoom);
		data.append(history.entries.at(i).position);

		m_webView->page()->history()->itemAt(i).setUserData(data);
	}

	m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(index));

	const QUrl url = m_webView->page()->history()->itemAt(index).url();

	setRequestedUrl(url, false, true);
	updateOptions(url);
	updatePageActions(url);

	m_webView->page()->triggerAction(QWebPage::Reload);
}

void QtWebKitWebWidget::setHistory(QDataStream &stream)
{
	stream.device()->reset();
	stream >> *(m_webView->page()->history());

	const QUrl url = m_webView->page()->history()->currentItem().url();

	setRequestedUrl(url, false, true);
	updateOptions(url);
	updatePageActions(url);

	m_webView->page()->triggerAction(QWebPage::Reload);
}

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

void QtWebKitWebWidget::setUrl(const QUrl &url, bool typed)
{
	if (url.scheme() == QLatin1String("javascript"))
	{
		m_webView->page()->mainFrame()->evaluateJavaScript(url.toDisplayString(QUrl::RemoveScheme | QUrl::DecodeReserved));

		return;
	}

	if (!url.fragment().isEmpty() && url.matches(getUrl(), (QUrl::RemoveFragment | QUrl::StripTrailingSlash | QUrl::NormalizePathSegments)))
	{
		m_webView->page()->mainFrame()->scrollToAnchor(url.fragment());

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

	m_networkManager->resetStatistics();

	m_webView->load(targetUrl);

	notifyTitleChanged();
	notifyIconChanged();
}

void QtWebKitWebWidget::setPermission(const QString &key, const QUrl &url, WebWidget::PermissionPolicies policies)
{
	WebWidget::setPermission(key, url, policies);

	QWebPage::Feature feature = QWebPage::Geolocation;

	if (key == QLatin1String("Browser/EnableGeolocation"))
	{
		feature = QWebPage::Geolocation;
	}
	else if (key == QLatin1String("Browser/EnableNotifications"))
	{
		feature = QWebPage::Notifications;
	}
	else
	{
		return;
	}

	QList<QWebFrame*> frames;
	frames.append(m_page->mainFrame());

	while (!frames.isEmpty())
	{
		QWebFrame *frame = frames.takeFirst();

		if (frame->requestedUrl() == url)
		{
			m_page->setFeaturePermission(frame, feature, (policies.testFlag(GrantedPermission) ? QWebPage::PermissionGrantedByUser : QWebPage::PermissionDeniedByUser));
		}

		frames.append(frame->childFrames());
	}
}

void QtWebKitWebWidget::setOption(const QString &key, const QVariant &value)
{
	WebWidget::setOption(key, value);

	updateOptions(getUrl());

	if (key == QLatin1String("Content/DefaultCharacterEncoding"))
	{
		m_webView->reload();
	}
}

void QtWebKitWebWidget::setOptions(const QVariantHash &options)
{
	WebWidget::setOptions(options);

	updateOptions(getUrl());
}

void QtWebKitWebWidget::setScrollPosition(const QPoint &position)
{
	m_webView->page()->mainFrame()->setScrollPosition(position);
}

WebWidget* QtWebKitWebWidget::clone(bool cloneHistory)
{
	QtWebKitWebWidget *widget = new QtWebKitWebWidget(isPrivate(), getBackend(), m_networkManager->clone(), NULL);
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

Action* QtWebKitWebWidget::getAction(int identifier)
{
	if (identifier == ActionsManager::UndoAction && !getExistingAction(ActionsManager::UndoAction))
	{
		Action *action = WebWidget::getAction(ActionsManager::UndoAction);
		action->setEnabled(m_page->undoStack()->canUndo());

		updateUndoText(m_page->undoStack()->undoText());

		connect(m_page->undoStack(), SIGNAL(canUndoChanged(bool)), action, SLOT(setEnabled(bool)));
		connect(m_page->undoStack(), SIGNAL(undoTextChanged(QString)), this, SLOT(updateUndoText(QString)));

		return action;
	}

	if (identifier == ActionsManager::RedoAction && !getExistingAction(ActionsManager::RedoAction))
	{
		Action *action = WebWidget::getAction(ActionsManager::RedoAction);
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

QString QtWebKitWebWidget::getSelectedText() const
{
	return m_webView->selectedText();
}

QString QtWebKitWebWidget::getPluginToken() const
{
	return m_pluginToken;
}

QUrl QtWebKitWebWidget::getUrl() const
{
	const QUrl url = m_webView->url();

	return (url.isEmpty() ? m_webView->page()->mainFrame()->requestedUrl() : url);
}

QIcon QtWebKitWebWidget::getIcon() const
{
	if (isPrivate())
	{
		return Utils::getIcon(QLatin1String("tab-private"));
	}

	const QIcon icon = m_webView->icon();

	return (icon.isNull() ? Utils::getIcon(QLatin1String("tab")) : icon);
}

QPixmap QtWebKitWebWidget::getThumbnail()
{
	if (!m_thumbnail.isNull() && !isLoading())
	{
		return m_thumbnail;
	}

	const QSize thumbnailSize = QSize(260, 170);
	const QSize oldViewportSize = m_webView->page()->viewportSize();
	const QPoint position = m_webView->page()->mainFrame()->scrollPosition();
	const qreal zoom = m_webView->page()->mainFrame()->zoomFactor();
	QSize contentsSize = m_webView->page()->mainFrame()->contentsSize();
	QWidget *newView = new QWidget();
	QWidget *oldView = m_webView->page()->view();

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

	pixmap = pixmap.scaled(thumbnailSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

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
	const QRect horizontalScrollBar = m_webView->page()->mainFrame()->scrollBarGeometry(Qt::Horizontal);
	const QRect verticalScrollBar = m_webView->page()->mainFrame()->scrollBarGeometry(Qt::Vertical);

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

WindowHistoryInformation QtWebKitWebWidget::getHistory() const
{
	QVariantList data = m_webView->history()->currentItem().userData().toList();

	if (data.isEmpty() || data.length() < 3)
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

	QWebHistory *history = m_webView->history();
	WindowHistoryInformation information;
	information.index = history->currentItemIndex();

	const QString requestedUrl = m_webView->page()->mainFrame()->requestedUrl().toString();
	const int historyCount = history->count();

	for (int i = 0; i < historyCount; ++i)
	{
		const QWebHistoryItem item = history->itemAt(i);
		WindowHistoryEntry entry;
		entry.url = item.url().toString();
		entry.title = item.title();
		entry.position = item.userData().toList().value(PositionEntryData, QPoint(0, 0)).toPoint();
		entry.zoom = item.userData().toList().value(ZoomEntryData).toInt();

		information.entries.append(entry);
	}

	if (isLoading() && requestedUrl != history->itemAt(history->currentItemIndex()).url().toString())
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
	QWebFrame *frame = m_webView->page()->frameAt(position);

	if (!frame)
	{
		return HitTestResult();
	}

	const QWebHitTestResult nativeResult = m_webView->page()->mainFrame()->hitTestContent(position);

	HitTestResult result;
	result.title = nativeResult.title();
	result.tagName = nativeResult.element().tagName().toLower();
	result.alternateText = nativeResult.element().attribute(QLatin1String("alt"));
	result.longDescription = nativeResult.element().attribute(QLatin1String("longDesc"));
	result.frameUrl = ((nativeResult.frame() && nativeResult.frame() != m_page->mainFrame()) ? (nativeResult.frame()->url().isValid() ? nativeResult.frame()->url() : nativeResult.frame()->requestedUrl()) : QUrl());
	result.imageUrl = nativeResult.imageUrl();
	result.linkUrl = nativeResult.linkUrl();
#if QTWEBKIT_VERSION >= 0x050200
	result.mediaUrl = nativeResult.mediaUrl();
#endif
	result.position = position;
	result.geometry = nativeResult.element().geometry();

	QWebElement parentElement = nativeResult.element().parent();
	const bool isSubmit = ((result.tagName == QLatin1String("input") || result.tagName == QLatin1String("button")) && (nativeResult.element().attribute(QLatin1String("type")).toLower() == QLatin1String("submit") || nativeResult.element().attribute(QLatin1String("type")).toLower() == QLatin1String("image")));

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

				result.formUrl = (url.isEmpty() ? getUrl() : (url.isRelative() ? getUrl().resolved(url) : url)).toString();
			}

			if (parentElement.findAll(QLatin1String("input:not([disabled])[name], select:not([disabled])[name], textarea:not([disabled])[name]")).count() > 0)
			{
				result.flags |= IsFormTest;
			}
		}
	}

	if (result.tagName == QLatin1String("textarea") || result.tagName == QLatin1String("input"))
	{
		const QString type = nativeResult.element().attribute(QLatin1String("type")).toLower();

		if (type.isEmpty() || result.tagName == QLatin1String("textarea") || type == QLatin1String("text") || type == QLatin1String("search"))
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

QList<LinkUrl> QtWebKitWebWidget::getFeeds() const
{
	const QWebElementCollection elements = m_webView->page()->mainFrame()->findAllElements(QLatin1String("a[type=\"application/atom+xml\"], a[type=\"application/rss+xml\"], link[type=\"application/atom+xml\"], link[type=\"application/rss+xml\"]"));
	QList<LinkUrl> links;
	QSet<QUrl> urls;

	for (int i = 0; i < elements.count(); ++i)
	{
		QUrl url(elements.at(i).attribute(QLatin1String("href")));

		if (url.isRelative())
		{
			url = getUrl().resolved(url);
		}

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
	const QWebElementCollection elements = m_webView->page()->mainFrame()->findAllElements(QLatin1String("link[type=\"application/opensearchdescription+xml\"]"));
	QList<LinkUrl> links;
	QSet<QUrl> urls;

	for (int i = 0; i < elements.count(); ++i)
	{
		QUrl url(elements.at(i).attribute(QLatin1String("href")));

		if (url.isRelative())
		{
			url = getUrl().resolved(url);
		}

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

QHash<QByteArray, QByteArray> QtWebKitWebWidget::getHeaders() const
{
	return m_networkManager->getHeaders();
}

QVariantHash QtWebKitWebWidget::getStatistics() const
{
	return m_networkManager->getStatistics();
}

QVector<int> QtWebKitWebWidget::getContentBlockingProfiles() const
{
	return m_contentBlockingProfiles;
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

bool QtWebKitWebWidget::isLoading() const
{
	return m_isLoading;
}

bool QtWebKitWebWidget::isPrivate() const
{
	return m_webView->settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled);
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
#if QTWEBKIT_VERSION >= 0x050200
	QWebPage::FindFlags nativeFlags = (QWebPage::FindWrapsAroundDocument | QWebPage::FindBeginsInSelection);
#else
	QWebPage::FindFlags nativeFlags = QWebPage::FindWrapsAroundDocument;
#endif

	if (flags & BackwardFind)
	{
		nativeFlags |= QWebPage::FindBackward;
	}

	if (flags & CaseSensitiveFind)
	{
		nativeFlags |= QWebPage::FindCaseSensitively;
	}

	if (flags & HighlightAllFind || text.isEmpty())
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
		if (event->type() == QEvent::ContextMenu)
		{
			QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent*>(event);

			if (contextMenuEvent)
			{
				handleContextMenuEvent(contextMenuEvent, false);
			}
		}
		else if (event->type() == QEvent::Move || event->type() == QEvent::Resize)
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
		else if (event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			if (mouseEvent)
			{
				return handleMousePressEvent(mouseEvent, true, m_webView);
			}
		}
		else if (event->type() == QEvent::MouseButtonRelease)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			if (mouseEvent)
			{
				if (mouseEvent->button() == Qt::LeftButton && SettingsManager::getValue(QLatin1String("Browser/EnablePlugins"), getUrl()).toString() == QLatin1String("onDemand"))
				{
					QWidget *widget = childAt(mouseEvent->pos());
					const QWebHitTestResult hitResult = m_webView->page()->mainFrame()->hitTestContent(mouseEvent->pos());
					const QString tagName = hitResult.element().tagName().toLower();

					if (widget && widget->metaObject()->className() == QLatin1String("Otter::QtWebKitPluginWidget") && (tagName == QLatin1String("object") || tagName == QLatin1String("embed")))
					{
						m_pluginToken = QUuid::createUuid().toString();

						hitResult.element().setAttribute(QLatin1String("data-otter-browser"), m_pluginToken);

						QWebElement element = hitResult.element().clone();

						hitResult.element().replace(element);

						element.removeAttribute(QLatin1String("data-otter-browser"));

						Action *loadPluginsAction = getExistingAction(ActionsManager::LoadPluginsAction);

						if (loadPluginsAction)
						{
							loadPluginsAction->setEnabled(findChildren<QtWebKitPluginWidget*>().count() > 0);
						}

						return true;
					}
				}

				return handleMouseReleaseEvent(mouseEvent, true, m_webView);
			}
		}
		else if (event->type() == QEvent::MouseButtonDblClick)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			if (mouseEvent)
			{
				return handleMouseDoubleClickEvent(mouseEvent, false);
			}
		}
		else if (event->type() == QEvent::MouseMove)
		{
			event->ignore();

			return QObject::eventFilter(object, event);
		}
		else if (event->type() == QEvent::Wheel)
		{
			QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

			if (wheelEvent)
			{
				return handleWheelEvent(wheelEvent, true);
			}
		}
		else if (event->type() == QEvent::ShortcutOverride)
		{
			const QString tagName = m_page->mainFrame()->findFirstElement(QLatin1String("*:focus")).tagName().toLower();

			if (tagName == QLatin1String("object") || tagName == QLatin1String("embed"))
			{
				event->accept();

				return true;
			}

			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

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
	else if (object == m_inspector && (event->type() == QEvent::Move || event->type() == QEvent::Resize) && m_inspectorCloseButton)
	{
		m_inspectorCloseButton->move(QPoint((m_inspector->width() - 19), 3));
	}

	return QObject::eventFilter(object, event);
}

}
