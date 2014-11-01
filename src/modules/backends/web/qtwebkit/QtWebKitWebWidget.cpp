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
#include "QtWebKitWebPage.h"
#include "../../../windows/web/ImagePropertiesDialog.h"
#include "../../../../core/ActionsManager.h"
#include "../../../../core/BookmarksManager.h"
#include "../../../../core/Console.h"
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
#include <QtGui/QClipboard>
#include <QtGui/QImageWriter>
#include <QtGui/QMouseEvent>
#include <QtGui/QMovie>
#include <QtNetwork/QAbstractNetworkCache>
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

QtWebKitWebWidget::QtWebKitWebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent) : WebWidget(isPrivate, backend, parent),
	m_webView(new QWebView(this)),
	m_page(new QtWebKitWebPage(this)),
	m_inspector(NULL),
	m_inspectorCloseButton(NULL),
	m_networkManager(NULL),
	m_splitter(new QSplitter(Qt::Vertical, this)),
	m_historyEntry(-1),
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
	setNetworkManager(NetworkManagerFactory::createManager(isPrivate, false, parent));

	m_webView->setPage(m_page);
	m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_webView->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, isPrivate);
	m_webView->installEventFilter(this);

	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Cut"), this), getAction(CutAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Copy"), this), getAction(CopyAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Paste"), this), getAction(PasteAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Delete"), this), getAction(DeleteAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("SelectAll"), this), getAction(SelectAllAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Undo"), this), getAction(UndoAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Redo"), this), getAction(RedoAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("GoBack"), this), getAction(GoBackAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("GoForward"), this), getAction(GoForwardAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Reload"), this), getAction(ReloadAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Stop"), this), getAction(StopAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenLink"), this), getAction(OpenLinkAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenLinkInThisTab"), this), getAction(OpenLinkInThisTabAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenLinkInNewWindow"), this), getAction(OpenLinkInNewWindowAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenFrameInNewTab"), this), getAction(OpenFrameInNewTabAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("SaveLinkToDisk"), this), getAction(SaveLinkToDiskAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenImageInNewTab"), this), getAction(OpenImageInNewTabAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("SaveImageToDisk"), this), getAction(SaveImageToDiskAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("CopyImageToClipboard"), this), getAction(CopyImageToClipboardAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("CopyImageUrlToClipboard"), this), getAction(CopyImageUrlToClipboardAction));
#if QTWEBKIT_VERSION >= 0x050200
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("SaveMediaToDisk"), this), getAction(SaveMediaToDiskAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("CopyMediaUrlToClipboard"), this), getAction(CopyMediaUrlToClipboardAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ToggleMediaControls"), this), getAction(ToggleMediaControlsAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ToggleMediaLoop"), this), getAction(ToggleMediaLoopAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ToggleMediaPlayPause"), this), getAction(ToggleMediaPlayPauseAction));
	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ToggleMediaMute"), this), getAction(ToggleMediaMuteAction));
#endif

	getAction(ReloadAction)->setEnabled(true);
	getAction(OpenLinkAction)->setIcon(Utils::getIcon(QLatin1String("document-open")));
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
	connect(m_splitter, SIGNAL(splitterMoved(int,int)), this, SIGNAL(progressBarGeometryChanged()));
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

	m_isLoading = true;

	m_thumbnail = QPixmap();

	getAction(RewindAction)->setEnabled(getAction(GoBackAction)->isEnabled());
	getAction(FastForwardAction)->setEnabled(getAction(GoForwardAction)->isEnabled());

	QAction *action = getAction(ReloadOrStopAction);

	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Stop"), this), action);

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

	QAction *action = getAction(ReloadOrStopAction);

	ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Reload"), this), action);

	action->setEnabled(true);
	action->setShortcut(QKeySequence());

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
			const QString imageType = imageUrl.mid(11, imageUrl.indexOf(QLatin1Char(';')) - 11);
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
	triggerAction(InspectPageAction, false);
}

void QtWebKitWebWidget::linkHovered(const QString &link)
{
	setStatusMessage(link, true);
}

void QtWebKitWebWidget::markPageRealoded()
{
	m_isReloading = true;
}

void QtWebKitWebWidget::openUrl(const QUrl &url, OpenHints hints)
{
	WebWidget *widget = clone(false);
	widget->setRequestedUrl(url);

	emit requestedNewWindow(widget, hints);
}

void QtWebKitWebWidget::setNetworkManager(NetworkManager *manager)
{
	if (m_networkManager)
	{
		m_networkManager->deleteLater();
	}

	m_networkManager = manager;
	m_networkManager->setParent(m_page);

	m_page->setNetworkAccessManager(m_networkManager);
	m_page->setForwardUnsupportedContent(true);

	connect(m_networkManager, SIGNAL(statusChanged(int,int,qint64,qint64,qint64)), this, SIGNAL(loadStatusChanged(int,int,qint64,qint64,qint64)));
	connect(m_networkManager, SIGNAL(documentLoadProgressChanged(int)), this, SIGNAL(loadProgress(int)));
}

void QtWebKitWebWidget::notifyTitleChanged()
{
	emit titleChanged(getTitle());

	SessionsManager::markSessionModified();
}

void QtWebKitWebWidget::notifyUrlChanged(const QUrl &url)
{
	updateOptions(url);
	getAction(RewindAction)->setEnabled(getAction(GoBackAction)->isEnabled());
	getAction(FastForwardAction)->setEnabled(getAction(GoForwardAction)->isEnabled());

	emit urlChanged(url);

	SessionsManager::markSessionModified();
}

void QtWebKitWebWidget::notifyIconChanged()
{
	emit iconChanged(getIcon());
}

void QtWebKitWebWidget::updateQuickSearchAction()
{
	QAction *defaultSearchAction = getAction(SearchAction);
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

	getAction(SearchMenuAction)->setEnabled(SearchesManager::getSearchEngines().count() > 1);
}

void QtWebKitWebWidget::updateOptions(const QUrl &url)
{
	QWebSettings *settings = m_webView->page()->settings();
	settings->setAttribute(QWebSettings::AutoLoadImages, getOption(QLatin1String("Browser/EnableImages"), url).toBool());
	settings->setAttribute(QWebSettings::PluginsEnabled, getOption(QLatin1String("Browser/EnablePlugins"), url).toBool());
	settings->setAttribute(QWebSettings::JavaEnabled, getOption(QLatin1String("Browser/EnableJava"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptEnabled, getOption(QLatin1String("Browser/EnableJavaScript"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, getOption(QLatin1String("Browser/JavaSriptCanAccessClipboard"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanCloseWindows, getOption(QLatin1String("Browser/JavaSriptCanCloseWindows"), url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanOpenWindows, getOption(QLatin1String("Browser/JavaScriptCanOpenWindows"), url).toBool());
	settings->setAttribute(QWebSettings::LocalStorageEnabled, getOption(QLatin1String("Browser/EnableLocalStorage"), url).toBool());
	settings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, getOption(QLatin1String("Browser/EnableOfflineStorageDatabase"), url).toBool());
	settings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, getOption(QLatin1String("Browser/EnableOfflineWebApplicationCache"), url).toBool());
	settings->setDefaultTextEncoding(getOption(QLatin1String("Content/DefaultEncoding"), url).toString());

	disconnect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));

	if (getOption(QLatin1String("Browser/JavaScriptCanShowStatusMessages"), url).toBool())
	{
		connect(m_webView->page(), SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusMessage(QString)));
	}
	else
	{
		setStatusMessage(QString());
	}

	const QString userAgent(getOption(QLatin1String("Network/UserAgent"), url).toString());

	m_page->setUserAgent(userAgent, NetworkManagerFactory::getUserAgent(userAgent).value);
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
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		triggerAction(static_cast<ActionIdentifier>(action->data().toInt()));
	}
}

void QtWebKitWebWidget::triggerAction(ActionIdentifier action, bool checked)
{
	const QWebPage::WebAction webAction = mapAction(action);

	if (webAction != QWebPage::NoWebAction)
	{
		m_webView->triggerPageAction(webAction, checked);

		return;
	}

	switch (action)
	{
		case RewindAction:
			m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(0));

			break;
		case FastForwardAction:
			m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(m_webView->page()->history()->count() - 1));

			break;
		case ReloadAction:
			m_webView->page()->triggerAction(QWebPage::Stop);
			m_webView->page()->triggerAction(QWebPage::Reload);

			break;
		case ReloadImageAction:
			if (!m_hitResult.imageUrl().isEmpty() && !m_hitResult.element().isNull())
			{
				if (getUrl().matches(m_hitResult.imageUrl(), (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash)))
				{
					triggerAction(ReloadAndBypassCacheAction);
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
				openUrl(m_hitResult.linkUrl(), NewTabOpen);
			}

			break;
		case OpenLinkInNewTabBackgroundAction:
			if (m_hitResult.linkUrl().isValid())
			{
				openUrl(m_hitResult.linkUrl(), NewTabBackgroundOpen);
			}

			break;
		case OpenLinkInNewWindowAction:
			if (m_hitResult.linkUrl().isValid())
			{
				openUrl(m_hitResult.linkUrl(), NewWindowOpen);
			}

			break;
		case OpenLinkInNewWindowBackgroundAction:
			if (m_hitResult.linkUrl().isValid())
			{
				openUrl(m_hitResult.linkUrl(), NewWindowBackgroundOpen);
			}

			break;
		case BookmarkLinkAction:
			if (m_hitResult.linkUrl().isValid())
			{
				emit requestedAddBookmark(m_hitResult.linkUrl(), m_hitResult.element().attribute(QLatin1String("title")));
			}

			break;
		case OpenSelectionAsLinkAction:
			openUrl(QUrl(m_webView->selectedText()));

			break;
		case ImagePropertiesAction:
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
		case SaveImageToDiskAction:
			if (m_hitResult.imageUrl().isValid())
			{
				emit downloadFile(QNetworkRequest(m_hitResult.imageUrl()));
			}

			break;
#if QTWEBKIT_VERSION >= 0x050200
		case SaveMediaToDiskAction:
			if (m_hitResult.mediaUrl().isValid())
			{
				emit downloadFile(QNetworkRequest(m_hitResult.mediaUrl()));
			}

			break;
#endif
		case InspectPageAction:
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

			getAction(InspectPageAction)->setChecked(checked);

			emit actionsChanged();
			emit progressBarGeometryChanged();

			break;
		case InspectElementAction:
			triggerAction(InspectPageAction, true);

			m_webView->triggerPageAction(QWebPage::InspectElement);

			break;
		case SaveLinkToDownloadsAction:
			TransfersManager::startTransfer(m_hitResult.linkUrl().toString(), QString(), isPrivate(), true);

			break;
		case OpenFrameInThisTabAction:
			if (m_hitResult.frame())
			{
				setUrl(m_hitResult.frame()->url().isValid() ? m_hitResult.frame()->url() : m_hitResult.frame()->requestedUrl());
			}

			break;
		case OpenFrameInNewTabBackgroundAction:
			if (m_hitResult.frame())
			{
				openUrl((m_hitResult.frame()->url().isValid() ? m_hitResult.frame()->url() : m_hitResult.frame()->requestedUrl()), NewTabBackgroundOpen);
			}

			break;
		case CopyLinkToClipboardAction:
			if (!m_hitResult.linkUrl().isEmpty())
			{
				QGuiApplication::clipboard()->setText(m_hitResult.linkUrl().toString());
			}

			break;
		case CopyFrameLinkToClipboardAction:
			if (m_hitResult.frame())
			{
				QGuiApplication::clipboard()->setText((m_hitResult.frame()->url().isValid() ? m_hitResult.frame()->url() : m_hitResult.frame()->requestedUrl()).toString());
			}

			break;
		case ReloadFrameAction:
			if (m_hitResult.frame())
			{
				const QUrl url = (m_hitResult.frame()->url().isValid() ? m_hitResult.frame()->url() : m_hitResult.frame()->requestedUrl());

				m_hitResult.frame()->setUrl(QUrl());
				m_hitResult.frame()->setUrl(url);
			}

			break;
		case SearchAction:
			quickSearch(getAction(SearchAction));

			break;
		case CreateSearchAction:
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
					QStringList keywords;
					QList<SearchInformation*> engines;

					for (int i = 0; i < identifiers.count(); ++i)
					{
						SearchInformation *engine = SearchesManager::getSearchEngine(identifiers.at(i));

						if (!engine)
						{
							continue;
						}

						engines.append(engine);

						const QString keyword = engine->keyword;

						if (!keyword.isEmpty())
						{
							keywords.append(keyword);
						}
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
		case ClearAllAction:
			triggerAction(SelectAllAction);
			triggerAction(DeleteAction);

			break;
		case CopyAsPlainTextAction:
			{
				const QString text = getSelectedText();

				if (!text.isEmpty())
				{
					QApplication::clipboard()->setText(text);
				}
			}

			break;
		case ScrollToStartAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), 0));

			break;
		case ScrollToEndAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), m_webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical)));

			break;
		case ScrollPageUpAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), qMax(0, (m_webView->page()->mainFrame()->scrollPosition().y() - m_webView->height()))));

			break;
		case ScrollPageDownAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(m_webView->page()->mainFrame()->scrollPosition().x(), qMin(m_webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical), (m_webView->page()->mainFrame()->scrollPosition().y() + m_webView->height()))));

			break;
		case ScrollPageLeftAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(qMax(0, (m_webView->page()->mainFrame()->scrollPosition().x() - m_webView->width())), m_webView->page()->mainFrame()->scrollPosition().y()));

			break;
		case ScrollPageRightAction:
			m_webView->page()->mainFrame()->setScrollPosition(QPoint(qMin(m_webView->page()->mainFrame()->scrollBarMaximum(Qt::Horizontal), (m_webView->page()->mainFrame()->scrollPosition().x() + m_webView->width())), m_webView->page()->mainFrame()->scrollPosition().y()));

			break;
		case WebsitePreferencesAction:
			{
				WebsitePreferencesDialog dialog(getUrl(), this);

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

void QtWebKitWebWidget::setOption(const QString &key, const QVariant &value)
{
	WebWidget::setOption(key, value);

	updateOptions(getUrl());
}

void QtWebKitWebWidget::setDefaultCharacterEncoding(const QString &encoding)
{
	m_webView->settings()->setDefaultTextEncoding(encoding);
	m_webView->reload();
}

void QtWebKitWebWidget::setUserAgent(const QString &identifier, const QString &value)
{
	m_page->setUserAgent(identifier, value);

	SessionsManager::markSessionModified();
}

void QtWebKitWebWidget::setHistory(const WindowHistoryInformation &history)
{
	if (history.entries.count() == 0)
	{
		m_webView->page()->history()->clear();

		getAction(GoBackAction)->setEnabled(false);
		getAction(GoForwardAction)->setEnabled(false);
		getAction(RewindAction)->setEnabled(false);
		getAction(FastForwardAction)->setEnabled(false);

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

	updateOptions(m_webView->page()->history()->itemAt(history.index).url());

	m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(history.index));
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

	m_webView->load(targetUrl);

	notifyTitleChanged();
	notifyIconChanged();
}

void QtWebKitWebWidget::showContextMenu(const QPoint &position)
{
	if (m_ignoreContextMenu || (position.isNull() && (m_webView->selectedText().isEmpty() || m_hotclickPosition.isNull())))
	{
		m_hotclickPosition = QPoint();

		return;
	}

	const QPoint hitPosition = (position.isNull() ? m_hotclickPosition : position);
	QWebFrame *frame = m_webView->page()->frameAt(hitPosition);

	m_hotclickPosition = QPoint();

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

	if (!m_hitResult.pixmap().isNull())
	{
		flags |= ImageMenu;

		const bool isImageOpened = getUrl().matches(m_hitResult.imageUrl(), (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash));
		const QString fileName = fontMetrics().elidedText(m_hitResult.imageUrl().fileName(), Qt::ElideMiddle, 256);
		QAction *openImageAction = getAction(OpenImageInNewTabAction);
		openImageAction->setText((fileName.isEmpty() || m_hitResult.imageUrl().scheme() == QLatin1String("data")) ? tr("Open Image (Untitled)") : tr("Open Image (%1)").arg(fileName));
		openImageAction->setEnabled(!isImageOpened);

		getAction(InspectElementAction)->setEnabled(!isImageOpened);
	}

#if QTWEBKIT_VERSION >= 0x050200
	if (m_hitResult.mediaUrl().isValid())
	{
		flags |= MediaMenu;

		const bool isVideo = (tagName == QLatin1String("video"));
		const bool isPaused = m_hitResult.element().evaluateJavaScript(QLatin1String("this.paused")).toBool();
		const bool isMuted = m_hitResult.element().evaluateJavaScript(QLatin1String("this.muted")).toBool();

		getAction(SaveMediaToDiskAction)->setText(isVideo ? tr("Save Video...") : tr("Save Audio..."));
		getAction(CopyMediaUrlToClipboardAction)->setText(isVideo ? tr("Copy Video Link to Clipboard") : tr("Copy Audio Link to Clipboard"));
		getAction(ToggleMediaControlsAction)->setText(tr("Show Controls"));
		getAction(ToggleMediaLoopAction)->setText(tr("Looping"));
		getAction(ToggleMediaPlayPauseAction)->setIcon(Utils::getIcon(isPaused ? QLatin1String("media-playback-start") : QLatin1String("media-playback-pause")));
		getAction(ToggleMediaPlayPauseAction)->setText(isPaused ? tr("Play") : tr("Pause"));
		getAction(ToggleMediaMuteAction)->setIcon(Utils::getIcon(isMuted ? QLatin1String("audio-volume-medium") : QLatin1String("audio-volume-muted")));
		getAction(ToggleMediaMuteAction)->setText(isMuted ? tr("Unmute") : tr("Mute"));
	}
#endif

	if (m_hitResult.isContentEditable())
	{
		flags |= EditMenu;

		getAction(ClearAllAction)->setEnabled(getAction(SelectAllAction)->isEnabled());
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
		getAction(OpenImageInNewTabAction)->setText(tr("Open Image"));
	}
}

WebWidget* QtWebKitWebWidget::clone(bool cloneHistory)
{
	const QPair<QString, QString> userAgent = getUserAgent();
	QtWebKitWebWidget *widget = new QtWebKitWebWidget(isPrivate(), getBackend(), NULL);
	widget->setNetworkManager(m_networkManager->clone(NULL));
	widget->setDefaultCharacterEncoding(getDefaultCharacterEncoding());
	widget->setUserAgent(userAgent.first, userAgent.second);
	widget->setQuickSearchEngine(getQuickSearchEngine());
	widget->setReloadTime(getReloadTime());

	if (cloneHistory)
	{
		widget->setHistory(getHistory());
	}

	widget->setZoom(getZoom());

	return widget;
}

QAction* QtWebKitWebWidget::getAction(ActionIdentifier action)
{
	const QWebPage::WebAction webAction = mapAction(action);

	if (webAction != QWebPage::NoWebAction)
	{
		return m_webView->page()->action(webAction);
	}

	if (action == UnknownAction)
	{
		return NULL;
	}

	if (m_actions.contains(action))
	{
		return m_actions[action];
	}

	QAction *actionObject = new QAction(this);
	actionObject->setData(action);

	connect(actionObject, SIGNAL(triggered()), this, SLOT(triggerAction()));

	switch (action)
	{
		case OpenLinkInNewTabAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenLinkInNewTab"), this), actionObject, true);

			break;
		case OpenLinkInNewTabBackgroundAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenLinkInNewTabBackground"), this), actionObject, true);

			break;
		case OpenLinkInNewWindowAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenLinkInNewWindow"), this), actionObject, true);

			break;
		case OpenLinkInNewWindowBackgroundAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenLinkInNewWindowBackground"), this), actionObject, true);

			break;
		case OpenFrameInThisTabAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenFrameInThisTab"), this), actionObject, true);

			break;
		case OpenFrameInNewTabBackgroundAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenFrameInNewTabBackground"), this), actionObject, true);

			break;
		case CopyLinkToClipboardAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("CopyLinkToClipboard"), this), actionObject, true);

			break;
		case CopyFrameLinkToClipboardAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("CopyFrameLinkToClipboard"), this), actionObject, true);

			break;
		case ViewSourceFrameAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ViewSourceFrame"), this), actionObject, true);

			actionObject->setEnabled(false);

			break;
		case ReloadFrameAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ReloadFrame"), this), actionObject, true);

			break;
		case ReloadImageAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ReloadImage"), this), actionObject, true);

			break;
		case SaveLinkToDownloadsAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("SaveLinkToDownloads"), this), actionObject);

			break;

		case SaveImageToDiskAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("SaveImageToDisk"), this), actionObject);

			break;
		case SaveMediaToDiskAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("SaveMediaToDisk"), this), actionObject);

			break;
		case RewindAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Rewind"), this), actionObject, true);

			actionObject->setEnabled(getAction(GoBackAction)->isEnabled());

			break;
		case FastForwardAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("FastForward"), this), actionObject, true);

			actionObject->setEnabled(getAction(GoForwardAction)->isEnabled());

			break;
		case ReloadAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Reload"), this), actionObject, true);

			break;
		case ReloadTimeAction:
			{
				ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ReloadTime"), this), actionObject, true);

				actionObject->setMenu(getReloadTimeMenu());
			}

			break;
		case PrintAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Print"), this), actionObject, true);

			break;
		case BookmarkAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("AddBookmark"), this), actionObject, true);

			break;
		case BookmarkLinkAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("BookmarkLink"), this), actionObject, true);

			break;
		case CopyAddressAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("CopyAddress"), this), actionObject, true);

			break;
		case ViewSourceAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ViewSource"), this), actionObject, true);

			actionObject->setEnabled(false);

			break;
		case ValidateAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Validate"), this), actionObject, true);

			actionObject->setMenu(new QMenu(this));
			actionObject->setEnabled(false);

			break;
		case WebsitePreferencesAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("WebsitePreferences"), this), actionObject, true);

			break;
		case ZoomInAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ZoomIn"), this), actionObject, true);

			break;
		case ZoomOutAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ZoomOut"), this), actionObject, true);

			break;
		case ZoomOriginalAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ZoomOriginal"), this), actionObject, true);

			break;
		case SearchAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Search"), this), actionObject, true);

			break;
		case SearchMenuAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("SearchMenu"), this), actionObject, true);

			actionObject->setMenu(getQuickSearchMenu());

			break;
		case OpenSelectionAsLinkAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("OpenSelectionAsLink"), this), actionObject, true);

			break;
		case ClearAllAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ClearAll"), this), actionObject, true);

			break;
		case SpellCheckAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("SpellCheck"), this), actionObject, true);

			actionObject->setEnabled(false);

			break;
		case ImagePropertiesAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("ImageProperties"), this), actionObject, true);

			break;
		case CreateSearchAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("CreateSearch"), this), actionObject, true);

			break;
		case ReloadOrStopAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Reload"), this), actionObject);

			actionObject->setEnabled(true);
			actionObject->setShortcut(QKeySequence());

			break;
		case InspectPageAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("InspectPage"), this), actionObject);

			actionObject->setEnabled(true);
			actionObject->setShortcut(QKeySequence());

			break;
		case InspectElementAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("InspectElement"), this), actionObject);

			actionObject->setEnabled(true);
			actionObject->setShortcut(QKeySequence());

			break;
		case FindAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("Find"), this), actionObject, true);

			actionObject->setEnabled(true);

			break;
		case FindNextAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("FindNext"), this), actionObject, true);

			actionObject->setEnabled(true);

			break;
		case FindPreviousAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("FindPrevious"), this), actionObject, true);

			actionObject->setEnabled(true);

			break;
		case CopyAsPlainTextAction:
			ActionsManager::setupLocalAction(ActionsManager::getAction(QLatin1String("CopyAsPlainText"), this), actionObject, true);

			actionObject->setEnabled(true);

			break;
		default:
			actionObject->deleteLater();
			actionObject = NULL;

			break;
	}

	if (actionObject)
	{
		m_actions[action] = actionObject;
	}

	return actionObject;
}

QUndoStack* QtWebKitWebWidget::getUndoStack()
{
	return m_webView->page()->undoStack();
}

QWebPage* QtWebKitWebWidget::getPage()
{
	return m_webView->page();
}

QString QtWebKitWebWidget::getDefaultCharacterEncoding() const
{
	return m_webView->settings()->defaultTextEncoding();
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

QPair<QString, QString> QtWebKitWebWidget::getUserAgent() const
{
	return m_page->getUserAgent();
}

QVariant QtWebKitWebWidget::evaluateJavaScript(const QString &script)
{
	return m_webView->page()->mainFrame()->evaluateJavaScript(script);
}

QUrl QtWebKitWebWidget::getUrl() const
{
	return m_webView->url();
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

QWebPage::WebAction QtWebKitWebWidget::mapAction(ActionIdentifier action) const
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
		case OpenImageInNewTabAction:
			return QWebPage::OpenImageInNewWindow;
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
#if QTWEBKIT_VERSION >= 0x050200
		case CopyMediaUrlToClipboardAction:
			return QWebPage::CopyMediaUrlToClipboard;
		case ToggleMediaControlsAction:
			return QWebPage::ToggleMediaControls;
		case ToggleMediaLoopAction:
			return QWebPage::ToggleMediaLoop;
		case ToggleMediaPlayPauseAction:
			return QWebPage::ToggleMediaPlayPause;
		case ToggleMediaMuteAction:
			return QWebPage::ToggleMediaMute;
#endif
		default:
			return QWebPage::NoWebAction;
	}

	return QWebPage::NoWebAction;
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
			const QPoint position = QCursor::pos();
			const QWebHitTestResult result = m_webView->page()->mainFrame()->hitTestContent(m_webView->mapFromGlobal(position));
			QString link = result.linkUrl().toString();
			QString text;

			if (link.isEmpty() && result.element().tagName().toLower() == QLatin1String("input") && (result.element().attribute(QLatin1String("type")).toLower() == QLatin1String("submit") || result.element().attribute(QLatin1String("type")).toLower() == QLatin1String("image")))
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

			if (SettingsManager::getValue(QLatin1String("Browser/ShowAddressToolTips")).toBool())
			{
				if (!link.isEmpty())
				{
					text = (result.title().isEmpty() ? tr("Address: %1").arg(link) : tr("Title: %1\nAddress: %2").arg(result.title()).arg(link));
				}
				else if (!result.title().isEmpty())
				{
					text = result.title();
				}
			}
			else
			{
				text = result.title();
			}

			setStatusMessage(link, true);

			QToolTip::showText(position, text, m_webView);

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

					triggerAction(GoBackAction);

					event->ignore();

					return true;
				}

				m_hitResult = m_webView->page()->mainFrame()->hitTestContent(mouseEvent->pos());

				if (m_hitResult.linkUrl().isValid())
				{
					if (mouseEvent->modifiers() & Qt::ControlModifier)
					{
						triggerAction(OpenLinkInNewTabBackgroundAction);

						event->accept();

						return true;
					}

					if (mouseEvent->modifiers() & Qt::ShiftModifier)
					{
						triggerAction(OpenLinkInNewTabAction);

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
				if (mouseEvent->buttons() & Qt::LeftButton)
				{
					triggerAction(GoForwardAction);

					event->ignore();

					return true;
				}
			}
			else if (mouseEvent->button() == Qt::BackButton)
			{
				triggerAction(GoBackAction);

				event->accept();

				return true;
			}
			else if (mouseEvent->button() == Qt::ForwardButton)
			{
				triggerAction(GoForwardAction);

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
			else if (mouseEvent->button() == Qt::RightButton && !(mouseEvent->buttons() & Qt::LeftButton))
			{
				if (m_isUsingRockerNavigation)
				{
					m_isUsingRockerNavigation = false;
				}
				else
				{
					m_ignoreContextMenu = false;

					showContextMenu(mouseEvent->pos());
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
					m_hotclickPosition = mouseEvent->pos();

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

			if (keyEvent->key() == Qt::Key_Escape && isLoading())
			{
				triggerAction(StopAction);

				ActionsManager::triggerAction(ActivateAddressFieldAction, this);

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
