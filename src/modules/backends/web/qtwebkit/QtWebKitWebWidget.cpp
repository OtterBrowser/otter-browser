/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "QtWebKitWebPage.h"
#include "../../../windows/web/ImagePropertiesDialog.h"
#include "../../../../core/ActionsManager.h"
#include "../../../../core/BookmarksManager.h"
#include "../../../../core/Console.h"
#include "../../../../core/CookieJar.h"
#include "../../../../core/GesturesManager.h"
#include "../../../../core/HistoryManager.h"
#include "../../../../core/NetworkCache.h"
#include "../../../../core/NetworkManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/SearchesManager.h"
#include "../../../../core/SessionsManager.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/TransfersManager.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/ContentsWidget.h"
#include "../../../../ui/MainWindow.h"
#include "../../../../ui/SearchPropertiesDialog.h"
#include "../../../../ui/WebsitePreferencesDialog.h"

#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtGui/QClipboard>
#include <QtGui/QImageWriter>
#include <QtGui/QMouseEvent>
#include <QtGui/QMovie>
#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QtWebKitVersion>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWidgets/QApplication>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolTip>
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
	m_historyEntry(-1),
	m_canLoadPlugins(false),
	m_ignoreContextMenu(false),
	m_isUsingRockerNavigation(false),
	m_isLoading(false),
	m_isReloading(false),
	m_isTyped(false)
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
		m_networkManager = new QtWebKitNetworkManager(isPrivate, this);
	}

	m_page = new QtWebKitWebPage(m_networkManager, this);
	m_page->setParent(m_webView);
	m_page->setPluginFactory(m_pluginFactory);

	m_webView->setPage(m_page);
	m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_webView->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, isPrivate);
	m_webView->installEventFilter(this);

	getAction(Action::GoBackAction)->setEnabled(m_webView->history()->canGoBack());
	getAction(Action::GoForwardAction)->setEnabled(m_webView->history()->canGoForward());
	getAction(Action::RewindAction)->setEnabled(m_webView->history()->canGoBack());
	getAction(Action::FastForwardAction)->setEnabled(m_webView->history()->canGoForward());
	getAction(Action::ReloadAction)->setEnabled(true);
	optionChanged(QLatin1String("History/BrowsingLimitAmountWindow"), SettingsManager::getValue(QLatin1String("History/BrowsingLimitAmountWindow")));
	optionChanged(QLatin1String("Browser/JavaScriptCanShowStatusMessages"), SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanShowStatusMessages")));
	optionChanged(QLatin1String("Content/BackgroundColor"), SettingsManager::getValue(QLatin1String("Content/BackgroundColor")));
	setZoom(SettingsManager::getValue(QLatin1String("Content/DefaultZoom")).toInt());

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(this, SIGNAL(quickSearchEngineChanged()), this, SLOT(updateQuickSearchAction()));
	connect(m_page, SIGNAL(requestedNewWindow(WebWidget*,OpenHints)), this, SIGNAL(requestedNewWindow(WebWidget*,OpenHints)));
	connect(m_page, SIGNAL(microFocusChanged()), this, SIGNAL(actionsChanged()));
	connect(m_page, SIGNAL(selectionChanged()), this, SIGNAL(actionsChanged()));
	connect(m_page, SIGNAL(loadStarted()), this, SLOT(pageLoadStarted()));
	connect(m_page, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished(bool)));
	connect(m_page, SIGNAL(saveFrameStateRequested(QWebFrame*,QWebHistoryItem*)), this, SLOT(saveState(QWebFrame*,QWebHistoryItem*)));
	connect(m_page, SIGNAL(restoreFrameStateRequested(QWebFrame*)), this, SLOT(restoreState(QWebFrame*)));
	connect(m_page, SIGNAL(downloadRequested(QNetworkRequest)), this, SLOT(downloadFile(QNetworkRequest)));
	connect(m_page, SIGNAL(unsupportedContent(QNetworkReply*)), this, SLOT(downloadFile(QNetworkReply*)));
	connect(m_page, SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString)));
	connect(m_page->mainFrame(), SIGNAL(contentsSizeChanged(QSize)), this, SIGNAL(progressBarGeometryChanged()));
	connect(m_page->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SIGNAL(progressBarGeometryChanged()));
	connect(m_page->mainFrame(), SIGNAL(loadStarted()), this, SIGNAL(progressBarGeometryChanged()));
	connect(m_webView, SIGNAL(titleChanged(const QString)), this, SLOT(notifyTitleChanged()));
	connect(m_webView, SIGNAL(urlChanged(const QUrl)), this, SLOT(notifyUrlChanged(const QUrl)));
	connect(m_webView, SIGNAL(iconChanged()), this, SLOT(notifyIconChanged()));
	connect(m_webView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_networkManager, SIGNAL(statusChanged(int,int,qint64,qint64,qint64)), this, SIGNAL(loadStatusChanged(int,int,qint64,qint64,qint64)));
	connect(m_networkManager, SIGNAL(documentLoadProgressChanged(int)), this, SIGNAL(loadProgress(int)));
	connect(m_splitter, SIGNAL(splitterMoved(int,int)), this, SIGNAL(progressBarGeometryChanged()));
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
	if (option == QLatin1String("History/BrowsingLimitAmountWindow"))
	{
		m_webView->page()->history()->setMaximumItemCount(value.toInt());
	}
	else if (option == QLatin1String("Browser/JavaScriptCanShowStatusMessages"))
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
}

void QtWebKitWebWidget::pageLoadStarted()
{
	if (m_isLoading)
	{
		return;
	}

	m_canLoadPlugins = (getOption(QLatin1String("Browser/EnablePlugins"), getUrl()).toString() == QLatin1String("enabled"));
	m_isLoading = true;
	m_thumbnail = QPixmap();

	getAction(Action::RewindAction)->setEnabled(getAction(Action::GoBackAction)->isEnabled());
	getAction(Action::FastForwardAction)->setEnabled(getAction(Action::GoForwardAction)->isEnabled());
	getAction(Action::LoadPluginsAction)->setEnabled(findChildren<QtWebKitPluginWidget*>().count() > 0);

	Action *action = getAction(Action::ReloadOrStopAction);
	action->setup(ActionsManager::getAction(Action::StopAction, this));
	action->setEnabled(true);
	action->setShortcut(QKeySequence());

	if (!isPrivate())
	{
		SessionsManager::markSessionModified();

		if (m_isReloading)
		{
			m_isReloading = false;
		}
		else if (HistoryManager::getEntry(m_historyEntry).url != getUrl())
		{
			m_historyEntry = HistoryManager::addEntry(getUrl(), m_webView->title(), m_webView->icon(), m_isTyped);
		}

		m_isTyped = false;
	}

	setStatusMessage(QString());
	setStatusMessage(QString(), true);

	emit loadingChanged(true);
}

void QtWebKitWebWidget::pageLoadFinished(bool ok)
{
	if (!m_isLoading)
	{
		return;
	}

	m_isLoading = false;

	m_thumbnail = QPixmap();

	m_networkManager->resetStatistics();

	Action *action = getAction(Action::ReloadOrStopAction);
	action->setup(ActionsManager::getAction(Action::ReloadAction, this));
	action->setEnabled(true);
	action->setShortcut(QKeySequence());

	getAction(Action::LoadPluginsAction)->setEnabled(findChildren<QtWebKitPluginWidget*>().count() > 0);

	if (!isPrivate())
	{
		if (ok)
		{
			SessionsManager::markSessionModified();

			if (m_historyEntry >= 0)
			{
				HistoryManager::updateEntry(m_historyEntry, getUrl(), m_webView->title(), m_webView->icon());
				BookmarksManager::updateVisits(getUrl().toString());
			}
		}
		else if (m_historyEntry >= 0)
		{
			HistoryManager::removeEntry(m_historyEntry);
		}
	}

	startReloadTimer();

	emit loadingChanged(false);
}

void QtWebKitWebWidget::downloadFile(const QNetworkRequest &request)
{
#if QTWEBKIT_VERSION >= 0x050200
	if ((!m_hitResult.imageUrl().isEmpty() && request.url() == m_hitResult.imageUrl()) || (!m_hitResult.mediaUrl().isEmpty() && request.url() == m_hitResult.mediaUrl()))
#else
	if (!m_hitResult.imageUrl().isEmpty() && request.url() == m_hitResult.imageUrl())
#endif
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

			device->deleteLater();
		}
		else if (!m_hitResult.imageUrl().isEmpty() && m_hitResult.imageUrl().url().contains(QLatin1String(";base64,")))
		{
			const QString imageUrl = m_hitResult.imageUrl().url();
			const QString imageType = imageUrl.mid(11, (imageUrl.indexOf(QLatin1Char(';')) - 11));
			const QString path = TransfersManager::getSavePath(tr("file") + QLatin1Char('.') + imageType);

			if (path.isEmpty())
			{
				return;
			}

			QImageWriter writer(path);

			if (!writer.write(QImage::fromData(QByteArray::fromBase64(imageUrl.mid(imageUrl.indexOf(QLatin1String(";base64,")) + 7).toUtf8()), imageType.toStdString().c_str())))
			{
				Console::addMessage(tr("Failed to save image %0: %1").arg(path).arg(writer.errorString()), OtherMessageCategory, ErrorMessageLevel);
			}

			return;
		}

		QNetworkRequest mutableRequest(request);
		mutableRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

		TransfersManager::startTransfer(mutableRequest, QString(), isPrivate(), false, true);
	}
	else
	{
		TransfersManager::startTransfer(request, QString(), isPrivate());
	}
}

void QtWebKitWebWidget::downloadFile(QNetworkReply *reply)
{
	TransfersManager::startTransfer(reply, QString(), isPrivate());
}

void QtWebKitWebWidget::saveState(QWebFrame *frame, QWebHistoryItem *item)
{
	if (frame == m_webView->page()->mainFrame())
	{
		QVariantHash data;
		data[QLatin1String("position")] = m_webView->page()->mainFrame()->scrollPosition();
		data[QLatin1String("zoom")] = getZoom();

		item->setUserData(data);
	}
}

void QtWebKitWebWidget::restoreState(QWebFrame *frame)
{
	if (frame == m_webView->page()->mainFrame())
	{
		setZoom(m_webView->history()->currentItem().userData().toHash().value(QLatin1String("zoom"), getZoom()).toInt());

		if (m_webView->page()->mainFrame()->scrollPosition() == QPoint(0, 0))
		{
			m_webView->page()->mainFrame()->setScrollPosition(m_webView->history()->currentItem().userData().toHash().value(QLatin1String("position")).toPoint());
		}
	}
}

void QtWebKitWebWidget::hideInspector()
{
	triggerAction(Action::InspectPageAction, false);
}

void QtWebKitWebWidget::linkHovered(const QString &link)
{
	setStatusMessage(link, true);
}

void QtWebKitWebWidget::markPageRealoded()
{
	m_isReloading = true;
}

void QtWebKitWebWidget::clearPluginToken()
{
	m_pluginToken = QString();
}

void QtWebKitWebWidget::openUrl(const QUrl &url, OpenHints hints)
{
	WebWidget *widget = clone(false);
	widget->setRequestedUrl(url);

	emit requestedNewWindow(widget, hints);
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

void QtWebKitWebWidget::openFormRequest(const QUrl &url, QNetworkAccessManager::Operation operation, QIODevice *outgoingData)
{
	m_webView->stop();

	QtWebKitWebWidget *widget = new QtWebKitWebWidget(isPrivate(), getBackend(), m_networkManager->clone());
	widget->setOptions(getOptions());
	widget->setQuickSearchEngine(getQuickSearchEngine());
	widget->setZoom(getZoom());
	widget->openRequest(url, operation, outgoingData);

	emit requestedNewWindow(widget, NewTabOpen);
}

void QtWebKitWebWidget::notifyTitleChanged()
{
	emit titleChanged(getTitle());

	SessionsManager::markSessionModified();
}

void QtWebKitWebWidget::notifyUrlChanged(const QUrl &url)
{
	updateOptions(url);
	getAction(Action::RewindAction)->setEnabled(getAction(Action::GoBackAction)->isEnabled());
	getAction(Action::FastForwardAction)->setEnabled(getAction(Action::GoForwardAction)->isEnabled());

	emit urlChanged(url);

	SessionsManager::markSessionModified();
}

void QtWebKitWebWidget::notifyIconChanged()
{
	emit iconChanged(getIcon());
}

void QtWebKitWebWidget::updateQuickSearchAction()
{
	QAction *defaultSearchAction = getAction(Action::SearchAction);
	SearchInformation *engine = SearchesManager::getSearchEngine(getQuickSearchEngine());

	if (engine)
	{
		defaultSearchAction->setEnabled(true);
		defaultSearchAction->setIcon(engine->icon.isNull() ? Utils::getIcon(QLatin1String("edit-find")) : engine->icon);
		defaultSearchAction->setText(engine->title);
		defaultSearchAction->setToolTip(engine->description);
	}
	else
	{
		defaultSearchAction->setEnabled(false);
		defaultSearchAction->setIcon(QIcon());
		defaultSearchAction->setText(tr("Search"));
		defaultSearchAction->setToolTip(tr("No search engines defined"));
	}

	getAction(Action::SearchMenuAction)->setEnabled(SearchesManager::getSearchEngines().count() > 1);
}

void QtWebKitWebWidget::updateOptions(const QUrl &url)
{
	QWebSettings *settings = m_webView->page()->settings();
	settings->setAttribute(QWebSettings::AutoLoadImages, getOption(QLatin1String("Browser/EnableImages"), url).toBool());
	settings->setAttribute(QWebSettings::PluginsEnabled, getOption(QLatin1String("Browser/EnablePlugins"), url).toString() != QLatin1String("disabled"));
	settings->setAttribute(QWebSettings::JavaEnabled, getOption(QLatin1String("Browser/EnableJava"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptEnabled, getOption(QLatin1String("Browser/EnableJavaScript"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, getOption(QLatin1String("Browser/JavaSriptCanAccessClipboard"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanCloseWindows, getOption(QLatin1String("Browser/JavaSriptCanCloseWindows"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanOpenWindows, getOption(QLatin1String("Browser/JavaScriptCanOpenWindows"), url).toBool());
	settings->setAttribute(QWebSettings::LocalStorageEnabled, getOption(QLatin1String("Browser/EnableLocalStorage"), url).toBool());
	settings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, getOption(QLatin1String("Browser/EnableOfflineStorageDatabase"), url).toBool());
	settings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, getOption(QLatin1String("Browser/EnableOfflineWebApplicationCache"), url).toBool());
	settings->setDefaultTextEncoding(getOption(QLatin1String("Content/DefaultCharacterEncoding"), url).toString());

	const QString thirdPartyCookiesPolicy = SettingsManager::getValue(QLatin1String("Network/ThirdPartyCookiesPolicy"), url).toString();

	if (thirdPartyCookiesPolicy == QLatin1String("acceptExisting"))
	{
		m_webView->settings()->setThirdPartyCookiePolicy(QWebSettings::AllowThirdPartyWithExistingCookies);
	}
	else if (thirdPartyCookiesPolicy == QLatin1String("ignore"))
	{
		m_webView->settings()->setThirdPartyCookiePolicy(QWebSettings::AlwaysBlockThirdPartyCookies);
	}
	else
	{
		m_webView->settings()->setThirdPartyCookiePolicy(QWebSettings::AlwaysAllowThirdPartyCookies);
	}

	disconnect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));

	if (getOption(QLatin1String("Browser/JavaScriptCanShowStatusMessages"), url).toBool())
	{
		connect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));
	}
	else
	{
		setStatusMessage(QString());
	}

	m_page->updatePageStyleSheets();

	m_networkManager->updateOptions(url);

	m_canLoadPlugins = (getOption(QLatin1String("Browser/EnablePlugins"), url).toString() == QLatin1String("enabled"));
}

void QtWebKitWebWidget::clearOptions()
{
	WebWidget::clearOptions();

	updateOptions(getUrl());
}

void QtWebKitWebWidget::showDialog(ContentsDialog *dialog)
{
	ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());

	if (parent)
	{
		parent->showDialog(dialog);
	}
}

void QtWebKitWebWidget::hideDialog(ContentsDialog *dialog)
{
	ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());

	if (parent)
	{
		parent->hideDialog(dialog);
	}
}

void QtWebKitWebWidget::goToHistoryIndex(int index)
{
	m_webView->history()->goToItem(m_webView->history()->itemAt(index));
}

void QtWebKitWebWidget::triggerAction()
{
	Action *action = qobject_cast<Action*>(sender());

	if (action)
	{
		triggerAction(action->getIdentifier());
	}
}

void QtWebKitWebWidget::triggerAction(int identifier, bool checked)
{
	switch (identifier)
	{
		case Action::OpenLinkAction:
			{
				QMouseEvent mousePressEvent(QEvent::MouseButtonPress, QPointF(m_clickPosition), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
				QMouseEvent mouseReleaseEvent(QEvent::MouseButtonRelease, QPointF(m_clickPosition), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

				QCoreApplication::sendEvent(m_webView, &mousePressEvent);
				QCoreApplication::sendEvent(m_webView, &mouseReleaseEvent);

				m_clickPosition = QPoint();
			}

			break;
		case Action::OpenLinkInCurrentTabAction:
			if (m_hitResult.linkUrl().isValid())
			{
				openUrl(m_hitResult.linkUrl(), CurrentTabOpen);
			}

			break;
		case Action::OpenLinkInNewTabAction:
			if (m_hitResult.linkUrl().isValid())
			{
				openUrl(m_hitResult.linkUrl(), NewTabOpen);
			}

			break;
		case Action::OpenLinkInNewTabBackgroundAction:
			if (m_hitResult.linkUrl().isValid())
			{
				openUrl(m_hitResult.linkUrl(), NewTabBackgroundOpen);
			}

			break;
		case Action::OpenLinkInNewWindowAction:
			if (m_hitResult.linkUrl().isValid())
			{
				openUrl(m_hitResult.linkUrl(), NewWindowOpen);
			}

			break;
		case Action::OpenLinkInNewWindowBackgroundAction:
			if (m_hitResult.linkUrl().isValid())
			{
				openUrl(m_hitResult.linkUrl(), NewWindowBackgroundOpen);
			}

			break;
		case Action::CopyLinkToClipboardAction:
			if (!m_hitResult.linkUrl().isEmpty())
			{
				QGuiApplication::clipboard()->setText(m_hitResult.linkUrl().toString());
			}

			break;
		case Action::BookmarkLinkAction:
			if (m_hitResult.linkUrl().isValid())
			{
				emit requestedAddBookmark(m_hitResult.linkUrl(), m_hitResult.element().attribute(QLatin1String("title")));
			}

			break;
		case Action::SaveLinkToDiskAction:
			m_webView->page()->triggerAction(QWebPage::DownloadLinkToDisk);

			break;
		case Action::SaveLinkToDownloadsAction:
			TransfersManager::startTransfer(m_hitResult.linkUrl().toString(), QString(), isPrivate(), true);

			break;
		case Action::OpenSelectionAsLinkAction:
			openUrl(QUrl(m_webView->selectedText()));

			break;
		case Action::OpenFrameInCurrentTabAction:
			if (m_hitResult.frame())
			{
				setUrl(m_hitResult.frame()->url().isValid() ? m_hitResult.frame()->url() : m_hitResult.frame()->requestedUrl());
			}

			break;
		case Action::OpenFrameInNewTabAction:
			if (m_hitResult.frame())
			{
				openUrl((m_hitResult.frame()->url().isValid() ? m_hitResult.frame()->url() : m_hitResult.frame()->requestedUrl()), CurrentTabOpen);
			}

			break;
		case Action::OpenFrameInNewTabBackgroundAction:
			if (m_hitResult.frame())
			{
				openUrl((m_hitResult.frame()->url().isValid() ? m_hitResult.frame()->url() : m_hitResult.frame()->requestedUrl()), NewTabBackgroundOpen);
			}

			break;
		case Action::CopyFrameLinkToClipboardAction:
			if (m_hitResult.frame())
			{
				QGuiApplication::clipboard()->setText((m_hitResult.frame()->url().isValid() ? m_hitResult.frame()->url() : m_hitResult.frame()->requestedUrl()).toString());
			}

			break;
		case Action::ReloadFrameAction:
			if (m_hitResult.frame())
			{
				const QUrl url = (m_hitResult.frame()->url().isValid() ? m_hitResult.frame()->url() : m_hitResult.frame()->requestedUrl());

				m_hitResult.frame()->setUrl(QUrl());
				m_hitResult.frame()->setUrl(url);
			}

			break;
		case Action::OpenImageInNewTabAction:
			if (!m_hitResult.imageUrl().isEmpty())
			{
				openUrl(m_hitResult.imageUrl(), NewTabOpen);
			}

			break;
		case Action::SaveImageToDiskAction:
			if (m_hitResult.imageUrl().isValid())
			{
				emit downloadFile(QNetworkRequest(m_hitResult.imageUrl()));
			}

			break;
		case Action::CopyImageToClipboardAction:
			m_webView->page()->triggerAction(QWebPage::CopyImageToClipboard);

			break;
		case Action::CopyImageUrlToClipboardAction:
			if (!m_hitResult.imageUrl().isEmpty())
			{
				QApplication::clipboard()->setText(m_hitResult.imageUrl().toString());
			}

			break;
		case Action::ReloadImageAction:
			if ((!m_hitResult.imageUrl().isEmpty() || m_hitResult.element().tagName().toLower() == QLatin1String("img")) && !m_hitResult.element().isNull())
			{
				if (getUrl().matches(m_hitResult.imageUrl(), (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash)))
				{
					triggerAction(Action::ReloadAndBypassCacheAction);
				}
				else
				{
					const QUrl url(m_hitResult.imageUrl());
					const QString src = m_hitResult.element().attribute(QLatin1String("src"));
					NetworkCache *cache = NetworkManagerFactory::getCache();

					m_hitResult.element().setAttribute(QLatin1String("src"), QString());

					if (cache)
					{
						cache->remove(url);
					}

					m_hitResult.element().setAttribute(QLatin1String("src"), src);

					evaluateJavaScript(QStringLiteral("var images = document.querySelectorAll('img[src=\"%1\"]'); for (var i = 0; i < images.length; ++i) { images[i].src = ''; images[i].src = \'%1\'; }").arg(src));
				}
			}

			break;
		case Action::ImagePropertiesAction:
			{
				ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());
				ImagePropertiesDialog *imagePropertiesDialog = new ImagePropertiesDialog(m_hitResult.imageUrl(), m_hitResult.element().attribute(QLatin1String("alt")), m_hitResult.element().attribute(QLatin1String("longdesc")), m_hitResult.pixmap(), (m_networkManager->cache() ? m_networkManager->cache()->data(m_hitResult.imageUrl()) : NULL), this);
				imagePropertiesDialog->setButtonsVisible(false);

				if (parent)
				{
					ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Close), imagePropertiesDialog, this);
					QEventLoop eventLoop;

					parent->showDialog(&dialog);

					connect(&dialog, SIGNAL(closed(bool,QDialogButtonBox::StandardButton)), &eventLoop, SLOT(quit()));
					connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

					eventLoop.exec();

					parent->hideDialog(&dialog);
				}
			}

			break;
#if QTWEBKIT_VERSION >= 0x050200
		case Action::SaveMediaToDiskAction:
			if (m_hitResult.mediaUrl().isValid())
			{
				emit downloadFile(QNetworkRequest(m_hitResult.mediaUrl()));
			}

			break;
		case Action::CopyMediaUrlToClipboardAction:
			if (!m_hitResult.mediaUrl().isEmpty())
			{
				QApplication::clipboard()->setText(m_hitResult.mediaUrl().toString());
			}

			break;
		case Action::ToggleMediaControlsAction:
			m_webView->page()->triggerAction(QWebPage::ToggleMediaControls);

			break;
		case Action::ToggleMediaLoopAction:
			m_webView->page()->triggerAction(QWebPage::ToggleMediaLoop);

			break;
		case Action::ToggleMediaPlayPauseAction:
			m_webView->page()->triggerAction(QWebPage::ToggleMediaPlayPause);

			break;
		case Action::ToggleMediaMuteAction:
			m_webView->page()->triggerAction(QWebPage::ToggleMediaMute);
#endif
		case Action::GoBackAction:
			m_webView->page()->triggerAction(QWebPage::Back);

			break;
		case Action::GoForwardAction:
			m_webView->page()->triggerAction(QWebPage::Forward);

			break;
		case Action::RewindAction:
			m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(0));

			break;
		case Action::FastForwardAction:
			m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(m_webView->page()->history()->count() - 1));

			break;
		case Action::StopAction:
			m_webView->page()->triggerAction(QWebPage::Stop);

			break;
		case Action::StopScheduledReloadAction:
			m_webView->page()->triggerAction(QWebPage::StopScheduledPageRefresh);

			break;
		case Action::ReloadAction:
			emit aboutToReload();

			m_webView->page()->triggerAction(QWebPage::Stop);
			m_webView->page()->triggerAction(QWebPage::Reload);

			break;
		case Action::ReloadOrStopAction:
			if (isLoading())
			{
				triggerAction(Action::StopAction);
			}
			else
			{
				triggerAction(Action::ReloadAction);
			}

			break;
		case Action::ReloadAndBypassCacheAction:
			m_webView->page()->triggerAction(QWebPage::ReloadAndBypassCache);

			break;
		case Action::UndoAction:
			m_webView->page()->triggerAction(QWebPage::Undo);

			break;
		case Action::RedoAction:
			m_webView->page()->triggerAction(QWebPage::Redo);

			break;
		case Action::CutAction:
			m_webView->page()->triggerAction(QWebPage::Cut);

			break;
		case Action::CopyAction:
			m_webView->page()->triggerAction(QWebPage::Copy);

			break;
		case Action::CopyPlainTextAction:
			{
				const QString text = getSelectedText();

				if (!text.isEmpty())
				{
					QApplication::clipboard()->setText(text);
				}
			}

			break;
		case Action::CopyAddressAction:
			QApplication::clipboard()->setText(getUrl().toString());

			break;
		case Action::PasteAction:
			m_webView->page()->triggerAction(QWebPage::Paste);

			break;
		case Action::DeleteAction:
			m_webView->page()->triggerAction(QWebPage::DeleteEndOfWord);

			break;
		case Action::SelectAllAction:
			m_webView->page()->triggerAction(QWebPage::SelectAll);

			break;
		case Action::ClearAllAction:
			triggerAction(Action::SelectAllAction);
			triggerAction(Action::DeleteAction);

			break;
		case Action::SearchAction:
			quickSearch(getAction(Action::SearchAction));

			break;
		case Action::CreateSearchAction:
			{
				QWebElement parentElement = m_hitResult.element().parent();

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

									break;
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

						parameters.addQueryItem(inputs.at(i).attribute(QLatin1String("name")), ((inputs.at(i) == m_hitResult.element()) ? QLatin1String("{searchTerms}") : value));
					}

					const QStringList identifiers = SearchesManager::getSearchEngines();
					const QStringList keywords = SearchesManager::getSearchKeywords();
					QList<SearchInformation*> engines;

					for (int i = 0; i < identifiers.count(); ++i)
					{
						SearchInformation *engine = SearchesManager::getSearchEngine(identifiers.at(i));

						if (!engine)
						{
							continue;
						}

						engines.append(engine);
					}

					QString identifier = getUrl().host().toLower().remove(QRegularExpression(QLatin1String("[^a-z0-9]")));

					while (identifier.isEmpty() || identifiers.contains(identifier))
					{
						identifier = QInputDialog::getText(this, tr("Select Identifier"), tr("Enter Unique Search Engine Identifier:"));

						if (identifier.isEmpty())
						{
							return;
						}
					}

					const QIcon icon = m_webView->icon();
					const QUrl url(parentElement.attribute(QLatin1String("action")));
					QVariantHash engineData;
					engineData[QLatin1String("identifier")] = identifier;
					engineData[QLatin1String("isDefault")] = false;
					engineData[QLatin1String("encoding")] = QLatin1String("UTF-8");
					engineData[QLatin1String("selfUrl")] = QString();
					engineData[QLatin1String("resultsUrl")] = (url.isEmpty() ? getUrl() : (url.isRelative() ? getUrl().resolved(url) : url)).toString();
					engineData[QLatin1String("resultsEnctype")] = parentElement.attribute(QLatin1String("enctype"));
					engineData[QLatin1String("resultsMethod")] = ((parentElement.attribute(QLatin1String("method"), QLatin1String("get")).toLower() == QLatin1String("post")) ? QLatin1String("post") : QLatin1String("get"));
					engineData[QLatin1String("resultsParameters")] = parameters.toString(QUrl::FullyDecoded);
					engineData[QLatin1String("suggestionsUrl")] = QString();
					engineData[QLatin1String("suggestionsEnctype")] = QString();
					engineData[QLatin1String("suggestionsMethod")] = QLatin1String("get");
					engineData[QLatin1String("suggestionsParameters")] = QString();
					engineData[QLatin1String("keyword")] = QString();
					engineData[QLatin1String("title")] = getTitle();
					engineData[QLatin1String("description")] = QString();
					engineData[QLatin1String("icon")] = (icon.isNull() ? Utils::getIcon(QLatin1String("edit-find")) : icon);

					SearchPropertiesDialog dialog(engineData, keywords, this);

					if (dialog.exec() == QDialog::Rejected)
					{
						return;
					}

					engineData = dialog.getEngineData();

					if (keywords.contains(engineData[QLatin1String("keyword")].toString()))
					{
						engineData[QLatin1String("keyword")] = QString();
					}

					SearchInformation *engine = new SearchInformation();
					engine->identifier = engineData[QLatin1String("identifier")].toString();
					engine->title = engineData[QLatin1String("title")].toString();
					engine->description = engineData[QLatin1String("description")].toString();
					engine->keyword = engineData[QLatin1String("keyword")].toString();
					engine->encoding = engineData[QLatin1String("encoding")].toString();
					engine->selfUrl = engineData[QLatin1String("selfUrl")].toString();
					engine->resultsUrl.url = engineData[QLatin1String("resultsUrl")].toString();
					engine->resultsUrl.enctype = engineData[QLatin1String("resultsEnctype")].toString();
					engine->resultsUrl.method = engineData[QLatin1String("resultsMethod")].toString();
					engine->resultsUrl.parameters = QUrlQuery(engineData[QLatin1String("resultsParameters")].toString());
					engine->icon = engineData[QLatin1String("icon")].value<QIcon>();

					engines.append(engine);

					if (SearchesManager::setSearchEngines(engines) && engineData[QLatin1String("isDefault")].toBool())
					{
						SettingsManager::setValue(QLatin1String("Search/DefaultSearchEngine"), engineData[QLatin1String("identifier")].toString());
					}
				}
			}

			break;
		case Action::ScrollToStartAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), 0));

			break;
		case Action::ScrollToEndAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), m_webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical)));

			break;
		case Action::ScrollPageUpAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), qMax(0, (m_webView->page()->mainFrame()->scrollPosition().y() - m_webView->height()))));

			break;
		case Action::ScrollPageDownAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), qMin(m_webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical), (m_webView->page()->mainFrame()->scrollPosition().y() + m_webView->height()))));

			break;
		case Action::ScrollPageLeftAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(qMax(0, (m_webView->page()->mainFrame()->scrollPosition().x() - m_webView->width())), m_webView->page()->mainFrame()->scrollPosition().y()));

			break;
		case Action::ScrollPageRightAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(qMin(m_webView->page()->mainFrame()->scrollBarMaximum(Qt::Horizontal), (m_webView->page()->mainFrame()->scrollPosition().x() + m_webView->width())), m_webView->page()->mainFrame()->scrollPosition().y()));

			break;
		case Action::ActivateWebpageAction:
			{
				m_webView->setFocus();

				m_page->mainFrame()->setFocus();

				QWebElement element = m_page->mainFrame()->findFirstElement(QLatin1String(":focus"));

				if (element.tagName().toLower() == QLatin1String("textarea") || element.tagName().toLower() == QLatin1String("input"))
				{
					m_page->mainFrame()->evaluateJavaScript(QLatin1String("document.activeElement.blur()"));
				}
			}

			break;
		case Action::LoadPluginsAction:
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

				getAction(Action::LoadPluginsAction)->setEnabled(false);
			}

			break;
		case Action::InspectPageAction:
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

			m_inspector->setVisible(checked);

			if (checked)
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

			getAction(Action::InspectPageAction)->setChecked(checked);

			emit actionsChanged();
			emit progressBarGeometryChanged();

			break;
		case Action::InspectElementAction:
			triggerAction(Action::InspectPageAction, true);

			m_webView->triggerPageAction(QWebPage::InspectElement);

			break;
		case Action::WebsitePreferencesAction:
			{
				const QUrl url(getUrl());
				WebsitePreferencesDialog dialog(url, m_networkManager->getCookieJar()->getCookies(url.host()), this);

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

void QtWebKitWebWidget::showContextMenu(const QPoint &position)
{
	if (m_ignoreContextMenu || (position.isNull() && (m_webView->selectedText().isEmpty() || m_clickPosition.isNull())))
	{
		return;
	}

	const QPoint hitPosition = (position.isNull() ? m_clickPosition : position);
	QContextMenuEvent menuEvent(QContextMenuEvent::Other, hitPosition, m_webView->mapToGlobal(hitPosition), Qt::NoModifier);

	if (m_page->swallowContextMenuEvent(&menuEvent))
	{
		return;
	}

	QWebFrame *frame = m_webView->page()->frameAt(hitPosition);

	if (!frame)
	{
		return;
	}

	m_hitResult = frame->hitTestContent(hitPosition);

	MenuFlags flags = NoMenu;
	const QString tagName = m_hitResult.element().tagName().toLower();
	const QString tagType = m_hitResult.element().attribute(QLatin1String("type")).toLower();

	if (tagName == QLatin1String("textarea") || tagName == QLatin1String("select") || (tagName == QLatin1String("input") && (tagType.isEmpty() || tagType == QLatin1String("text") || tagType == QLatin1String("search"))))
	{
		QWebElement parentElement = m_hitResult.element().parent();

		while (!parentElement.isNull() && parentElement.tagName().toLower() != QLatin1String("form"))
		{
			parentElement = parentElement.parent();
		}

		if (!parentElement.isNull() && parentElement.hasAttribute(QLatin1String("action")) && !parentElement.findFirst(QLatin1String("input[name], select[name], textarea[name]")).isNull())
		{
			flags |= FormMenu;
		}
	}

	if (m_hitResult.pixmap().isNull() && m_hitResult.isContentSelected() && !m_webView->selectedText().isEmpty())
	{
		updateQuickSearchAction();

		flags |= SelectionMenu;
	}

	if (m_hitResult.linkUrl().isValid())
	{
		if (m_hitResult.linkUrl().scheme() == QLatin1String("mailto"))
		{
			flags |= MailMenu;
		}
		else
		{
			flags |= LinkMenu;
		}
	}

	if (!m_hitResult.pixmap().isNull() || !m_hitResult.imageUrl().isEmpty() || m_hitResult.element().tagName().toLower() == QLatin1String("img"))
	{
		flags |= ImageMenu;

		const bool isImageOpened = getUrl().matches(m_hitResult.imageUrl(), (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash));
		const QString fileName = fontMetrics().elidedText(m_hitResult.imageUrl().fileName(), Qt::ElideMiddle, 256);
		QAction *openImageAction = getAction(Action::OpenImageInNewTabAction);
		openImageAction->setText((fileName.isEmpty() || m_hitResult.imageUrl().scheme() == QLatin1String("data")) ? tr("Open Image (Untitled)") : tr("Open Image (%1)").arg(fileName));
		openImageAction->setEnabled(!isImageOpened);

		getAction(Action::InspectElementAction)->setEnabled(!isImageOpened);
	}

#if QTWEBKIT_VERSION >= 0x050200
	if (m_hitResult.mediaUrl().isValid())
	{
		flags |= MediaMenu;

		const bool isVideo = (tagName == QLatin1String("video"));
		const bool isPaused = m_hitResult.element().evaluateJavaScript(QLatin1String("this.paused")).toBool();
		const bool isMuted = m_hitResult.element().evaluateJavaScript(QLatin1String("this.muted")).toBool();

		getAction(Action::SaveMediaToDiskAction)->setText(isVideo ? tr("Save Video...") : tr("Save Audio..."));
		getAction(Action::CopyMediaUrlToClipboardAction)->setText(isVideo ? tr("Copy Video Link to Clipboard") : tr("Copy Audio Link to Clipboard"));
		getAction(Action::ToggleMediaControlsAction)->setText(tr("Show Controls"));
		getAction(Action::ToggleMediaLoopAction)->setText(tr("Looping"));
		getAction(Action::ToggleMediaPlayPauseAction)->setIcon(Utils::getIcon(isPaused ? QLatin1String("media-playback-start") : QLatin1String("media-playback-pause")));
		getAction(Action::ToggleMediaPlayPauseAction)->setText(isPaused ? tr("Play") : tr("Pause"));
		getAction(Action::ToggleMediaMuteAction)->setIcon(Utils::getIcon(isMuted ? QLatin1String("audio-volume-medium") : QLatin1String("audio-volume-muted")));
		getAction(Action::ToggleMediaMuteAction)->setText(isMuted ? tr("Unmute") : tr("Mute"));
	}
#endif

	if (m_hitResult.isContentEditable())
	{
		flags |= EditMenu;

		getAction(Action::ClearAllAction)->setEnabled(getAction(Action::SelectAllAction)->isEnabled());
	}

	if (flags == NoMenu || flags == FormMenu)
	{
		flags |= StandardMenu;

		if (m_hitResult.frame() != m_webView->page()->mainFrame())
		{
			flags |= FrameMenu;
		}
	}

	WebWidget::showContextMenu(hitPosition, flags);

	if (flags & ImageMenu)
	{
		getAction(Action::OpenImageInNewTabAction)->setText(tr("Open Image"));
	}
}

void QtWebKitWebWidget::setHistory(const WindowHistoryInformation &history)
{
	if (history.entries.count() == 0)
	{
		m_webView->page()->history()->clear();

		getAction(Action::GoBackAction)->setEnabled(false);
		getAction(Action::GoForwardAction)->setEnabled(false);
		getAction(Action::RewindAction)->setEnabled(false);
		getAction(Action::FastForwardAction)->setEnabled(false);

		emit actionsChanged();

		return;
	}

	qint64 documentSequence = 0;
	qint64 itemSequence = 0;
	QByteArray data;
	QDataStream stream(&data, QIODevice::ReadWrite);
	stream << int(2) << history.entries.count() << history.index;

	for (int i = 0; i < history.entries.count(); ++i)
	{
		stream << QString(QUrl::toPercentEncoding(history.entries.at(i).url, QByteArray(":/#?&+=@%*"))) << history.entries.at(i).title << history.entries.at(i).url << quint32(2) << quint64(0) << ++documentSequence << quint64(0) << QString() << false << ++itemSequence << QString() << qint32(history.entries.at(i).position.x()) << qint32(history.entries.at(i).position.y()) << qreal(1) << false << QString() << false;
	}

	stream.device()->reset();
	stream >> *(m_webView->page()->history());

	for (int i = 0; i < history.entries.count(); ++i)
	{
		QVariantHash data;
		data[QLatin1String("position")] = history.entries.at(i).position;
		data[QLatin1String("zoom")] = history.entries.at(i).zoom;

		m_webView->page()->history()->itemAt(i).setUserData(data);
	}

	setRequestedUrl(m_webView->page()->history()->currentItem().url(), false, true);
	updateOptions(m_webView->page()->history()->itemAt(history.index).url());

	m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(history.index));
}

void QtWebKitWebWidget::setHistory(QDataStream &stream)
{
	stream.device()->reset();
	stream >> *(m_webView->page()->history());

	setRequestedUrl(m_webView->page()->history()->currentItem().url(), false, true);
	updateOptions(m_webView->page()->history()->currentItem().url());
}

void QtWebKitWebWidget::setZoom(int zoom)
{
	if (zoom != getZoom())
	{
		m_webView->setZoomFactor(qBound(0.1, ((qreal) zoom / 100), (qreal) 100));

		SessionsManager::markSessionModified();

		emit zoomChanged(zoom);
		emit progressBarGeometryChanged();
	}
}

void QtWebKitWebWidget::setUrl(const QUrl &url, bool typed)
{
	if (url.scheme() == QLatin1String("javascript"))
	{
		evaluateJavaScript(url.toDisplayString(QUrl::RemoveScheme | QUrl::DecodeReserved));

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

QString QtWebKitWebWidget::getPluginToken() const
{
	return m_pluginToken;
}

WebWidget* QtWebKitWebWidget::clone(bool cloneHistory)
{
	QtWebKitWebWidget *widget = new QtWebKitWebWidget(isPrivate(), getBackend(), m_networkManager->clone());
	widget->setOptions(getOptions());
	widget->setQuickSearchEngine(getQuickSearchEngine());
	widget->setReloadTime(getReloadTime());

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
	if (identifier < 0)
	{
		return NULL;
	}

	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	Action *action = new Action(identifier, QIcon(), QString(), this);
	action->setup(ActionsManager::getAction(identifier, this));

	connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

	switch (identifier)
	{
		case Action::ReloadOrStopAction:
		case Action::InspectPageAction:
		case Action::InspectElementAction:
		case Action::FindAction:
		case Action::FindNextAction:
		case Action::FindPreviousAction:
		case Action::CopyPlainTextAction:
			action->setEnabled(true);

			break;
		case Action::ViewFrameSourceAction:
		case Action::ViewSourceAction:
		case Action::LoadPluginsAction:
			action->setEnabled(false);

			break;
		case Action::RewindAction:
			action->setEnabled(getAction(Action::GoBackAction)->isEnabled());

			break;
		case Action::FastForwardAction:
			action->setEnabled(getAction(Action::GoForwardAction)->isEnabled());

			break;
		case Action::ScheduleReloadAction:
			action->setMenu(getReloadTimeMenu());

			break;
		case Action::ValidateAction:
			action->setEnabled(false);
			action->setMenu(new QMenu(this));

			break;
		case Action::SearchMenuAction:
			action->setMenu(getQuickSearchMenu());

			break;
		case Action::CheckSpellingAction:
			action->setup(ActionsManager::getAction(Action::CheckSpellingAction, this));
			action->setEnabled(false);

			break;
		default:
			break;
	}

	if (action)
	{
		m_actions[identifier] = action;
	}

	return action;
}

QUndoStack* QtWebKitWebWidget::getUndoStack()
{
	return m_webView->page()->undoStack();
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

QHash<QByteArray, QByteArray> QtWebKitWebWidget::getHeaders() const
{
	return m_networkManager->getHeaders();
}

QVariantHash QtWebKitWebWidget::getStatistics() const
{
	return m_networkManager->getStatistics();
}

QVariant QtWebKitWebWidget::evaluateJavaScript(const QString &script)
{
	return m_webView->page()->mainFrame()->evaluateJavaScript(script);
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
	QVariantHash data;
	data[QLatin1String("position")] = m_webView->page()->mainFrame()->scrollPosition();
	data[QLatin1String("zoom")] = getZoom();

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
		entry.position = item.userData().toHash().value(QLatin1String("position"), QPoint(0, 0)).toPoint();
		entry.zoom = item.userData().toHash().value(QLatin1String("zoom")).toInt();

		information.entries.append(entry);
	}

	if (isLoading() && requestedUrl != history->itemAt(history->currentItemIndex()).url().toString())
	{
		WindowHistoryEntry entry;
		entry.url = requestedUrl;
		entry.title = getTitle();
		entry.position = data.value(QLatin1String("position"), QPoint(0, 0)).toPoint();
		entry.zoom = data.value(QLatin1String("zoom")).toInt();

		information.index = historyCount;
		information.entries.append(entry);
	}

	return information;
}

bool QtWebKitWebWidget::canLoadPlugins() const
{
	return m_canLoadPlugins;
}

int QtWebKitWebWidget::getZoom() const
{
	return (m_webView->zoomFactor() * 100);
}

bool QtWebKitWebWidget::isLoading() const
{
	return m_isLoading;
}

bool QtWebKitWebWidget::isPrivate() const
{
	return m_webView->settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled);
}

bool QtWebKitWebWidget::find(const QString &text, FindFlags flags)
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

	if (flags & HighlightAllFind)
	{
		nativeFlags |= QWebPage::HighlightAllOccurrences;
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

			m_ignoreContextMenu = (contextMenuEvent->reason() == QContextMenuEvent::Mouse);
		}
		else if (event->type() == QEvent::Resize)
		{
			emit progressBarGeometryChanged();
		}
		else if (event->type() == QEvent::ToolTip)
		{
			const QString toolTipsMode = SettingsManager::getValue(QLatin1String("Browser/ToolTipsMode")).toString();
			const QPoint position = QCursor::pos();
			const QWebHitTestResult result = m_webView->page()->mainFrame()->hitTestContent(m_webView->mapFromGlobal(position));
			QString link = result.linkUrl().toString();
			QString text;

			if (link.isEmpty() && (result.element().tagName().toLower() == QLatin1String("input") || result.element().tagName().toLower() == QLatin1String("button")) && (result.element().attribute(QLatin1String("type")).toLower() == QLatin1String("submit") || result.element().attribute(QLatin1String("type")).toLower() == QLatin1String("image")))
			{
				QWebElement parentElement = result.element().parent();

				while (!parentElement.isNull() && parentElement.tagName().toLower() != QLatin1String("form"))
				{
					parentElement = parentElement.parent();
				}

				if (!parentElement.isNull() && parentElement.hasAttribute(QLatin1String("action")))
				{
					const QUrl url(parentElement.attribute(QLatin1String("action")));

					link = (url.isEmpty() ? getUrl() : (url.isRelative() ? getUrl().resolved(url) : url)).toString();
				}
			}

			if (toolTipsMode != QLatin1String("disabled"))
			{
				const QString title = result.title().replace(QLatin1Char('&'), QLatin1String("&amp;")).replace(QLatin1Char('<'), QLatin1String("&lt;")).replace(QLatin1Char('>'), QLatin1String("&gt;"));

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

			setStatusMessage((link.isEmpty() ? result.title() : link), true);

			if (!text.isEmpty())
			{
				QToolTip::showText(position, QStringLiteral("<div style=\"white-space:pre-line;\">%1</div>").arg(text), m_webView);
			}

			event->accept();

			return true;
		}
		else if (event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			if (mouseEvent->button() == Qt::LeftButton)
			{
				if (mouseEvent->buttons() & Qt::RightButton)
				{
					m_isUsingRockerNavigation = true;

					triggerAction(Action::GoBackAction);

					event->ignore();

					return true;
				}

				m_hitResult = m_webView->page()->mainFrame()->hitTestContent(mouseEvent->pos());

				if (m_hitResult.linkUrl().isValid())
				{
					if (mouseEvent->modifiers() & Qt::ControlModifier)
					{
						triggerAction(Action::OpenLinkInNewTabBackgroundAction);

						event->accept();

						return true;
					}

					if (mouseEvent->modifiers() & Qt::ShiftModifier)
					{
						triggerAction(Action::OpenLinkInNewTabAction);

						event->accept();

						return true;
					}
				}
			}
			else if (mouseEvent->button() == Qt::MiddleButton)
			{
				const QWebHitTestResult result = m_webView->page()->mainFrame()->hitTestContent(mouseEvent->pos());

				if (result.linkUrl().isValid() && !m_webView->page()->mainFrame()->scrollBarGeometry(Qt::Horizontal).contains(mouseEvent->pos()) && !m_webView->page()->mainFrame()->scrollBarGeometry(Qt::Vertical).contains(mouseEvent->pos()))
				{
					openUrl(result.linkUrl(), ((mouseEvent->modifiers() & Qt::AltModifier) ? NewTabBackgroundEndOpen : NewTabBackgroundOpen));

					event->accept();

					return true;
				}
			}
			else if (mouseEvent->button() == Qt::RightButton)
			{
				m_hitResult = m_webView->page()->mainFrame()->hitTestContent(mouseEvent->pos());

				if (mouseEvent->buttons() & Qt::LeftButton)
				{
					triggerAction(Action::GoForwardAction);

					event->ignore();
				}
				else
				{
					event->accept();

					if (!m_hitResult.linkElement().isNull())
					{
						m_clickPosition = mouseEvent->pos();
					}

					GesturesManager::startGesture(m_webView, mouseEvent);
				}

				return true;
			}
			else if (mouseEvent->button() == Qt::BackButton)
			{
				triggerAction(Action::GoBackAction);

				event->accept();

				return true;
			}
			else if (mouseEvent->button() == Qt::ForwardButton)
			{
				triggerAction(Action::GoForwardAction);

				event->accept();

				return true;
			}
		}
		else if (event->type() == QEvent::MouseButtonRelease)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			if (mouseEvent->button() == Qt::MidButton)
			{
				m_hitResult = m_webView->page()->mainFrame()->hitTestContent(mouseEvent->pos());

				if (m_hitResult.linkUrl().isValid())
				{
					return true;
				}
			}
			else if (mouseEvent->button() == Qt::LeftButton && SettingsManager::getValue(QLatin1String("Browser/EnablePlugins"), getUrl()).toString() == QLatin1String("onDemand"))
			{
				QWidget *widget = childAt(mouseEvent->pos());

				m_hitResult = m_webView->page()->mainFrame()->hitTestContent(mouseEvent->pos());

				if (widget && widget->metaObject()->className() == QLatin1String("Otter::QtWebKitPluginWidget") && (m_hitResult.element().tagName().toLower() == QLatin1String("object") || m_hitResult.element().tagName().toLower() == QLatin1String("embed")))
				{
					m_pluginToken = QUuid::createUuid().toString();

					m_hitResult.element().setAttribute(QLatin1String("data-otter-browser"), m_pluginToken);

					QWebElement element = m_hitResult.element().clone();

					m_hitResult.element().replace(element);

					element.removeAttribute(QLatin1String("data-otter-browser"));

					getAction(Action::LoadPluginsAction)->setEnabled(findChildren<QtWebKitPluginWidget*>().count() > 0);
				}
			}
			else if (mouseEvent->button() == Qt::RightButton && !(mouseEvent->buttons() & Qt::LeftButton))
			{
				if (m_isUsingRockerNavigation)
				{
					m_isUsingRockerNavigation = false;
				}
				else
				{
					m_ignoreContextMenu = false;

					if (!GesturesManager::endGesture(m_webView, mouseEvent))
					{
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
				const QWebHitTestResult result = m_webView->page()->mainFrame()->hitTestContent(mouseEvent->pos());

				if (!result.isContentEditable() && result.element().tagName().toLower() != QLatin1String("textarea") && result.element().tagName().toLower() != QLatin1String("select") && result.element().tagName().toLower() != QLatin1String("input"))
				{
					m_clickPosition = mouseEvent->pos();

					QTimer::singleShot(250, this, SLOT(showContextMenu()));
				}
			}
		}
		else if (event->type() == QEvent::Wheel)
		{
			QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

			if (wheelEvent->modifiers() & Qt::CTRL)
			{
				setZoom(getZoom() + (wheelEvent->delta() / 16));

				event->accept();

				return true;
			}

		}
		else if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

			if (keyEvent->key() == Qt::Key_Escape)
			{
				if (isLoading())
				{
					triggerAction(Action::StopAction);

					ActionsManager::triggerAction(Action::ActivateAddressFieldAction, this);

					event->accept();

					return true;
				}

				if (m_webView->hasSelection())
				{
					QWebElement element = m_page->mainFrame()->findFirstElement(QLatin1String(":focus"));

					if (element.tagName().toLower() == QLatin1String("textarea") || element.tagName().toLower() == QLatin1String("input"))
					{
						m_page->triggerAction(QWebPage::MoveToPreviousChar);
					}
					else
					{
						m_page->mainFrame()->evaluateJavaScript(QLatin1String("window.getSelection().empty()"));
					}

					event->accept();

					return true;
				}
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
