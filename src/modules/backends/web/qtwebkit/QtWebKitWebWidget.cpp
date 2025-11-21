/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "QtWebKitNetworkManager.h"
#include "QtWebKitPage.h"
#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
#include "QtWebKitPluginFactory.h"
#endif
#include "QtWebKitWebBackend.h"
#include "../../../../core/Application.h"
#include "../../../../core/BookmarksManager.h"
#include "../../../../core/Console.h"
#include "../../../../core/CookieJar.h"
#include "../../../../core/ContentFiltersManager.h"
#include "../../../../core/GesturesManager.h"
#include "../../../../core/HistoryManager.h"
#include "../../../../core/JsonSettings.h"
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
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/ContentsWidget.h"
#include "../../../../ui/ImagePropertiesDialog.h"
#include "../../../../ui/MainWindow.h"
#include "../../../../ui/SearchEnginePropertiesDialog.h"
#include "../../../../ui/SourceViewerWebWidget.h"
#include "../../../../ui/WebsitePreferencesDialog.h"

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
#if QT_VERSION >= 0x060000
#include <QtGui/QShortcut>
#endif
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtWebKit/QWebFullScreenRequest>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QWebHistory>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#if QT_VERSION < 0x060000
#include <QtWidgets/QShortcut>
#endif
#include <QtWidgets/QUndoStack>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

QtWebKitInspectorWidget::QtWebKitInspectorWidget(QWebPage *inspectedPage, QWidget *parent) : QWebInspector(parent)
{
	inspectedPage->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

	setMinimumHeight(200);
	setPage(inspectedPage);
}

void QtWebKitInspectorWidget::childEvent(QChildEvent *event)
{
	QWebInspector::childEvent(event);

	if (event->type() == QEvent::ChildAdded && event->child()->inherits("QWebView"))
	{
		QWebView *webView(qobject_cast<QWebView*>(event->child()));

		if (webView)
		{
			webView->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
		}
	}
}

QtWebKitWebWidget::QtWebKitWebWidget(const QVariantMap &parameters, WebBackend *backend, QtWebKitNetworkManager *networkManager, ContentsWidget *parent) : WebWidget(backend, parent),
	m_webView(new QWebView(this)),
	m_page(nullptr),
	m_inspectorWidget(nullptr),
	m_networkManager(networkManager),
	m_formRequestOperation(QNetworkAccessManager::GetOperation),
	m_loadingState(FinishedLoadingState),
#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
	m_amountOfDeferredPlugins(0),
#endif
	m_transfersTimer(0),
#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
	m_canLoadPlugins(false),
#endif
	m_isAudioMuted(false),
	m_isFullScreen(false),
	m_isTypedIn(false),
	m_isNavigating(false)
{
	const bool isPrivate(SessionsManager::calculateOpenHints(parameters).testFlag(SessionsManager::PrivateOpen));
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
	m_page->setVisibilityState(isVisible() ? QWebPage::VisibilityStateVisible : QWebPage::VisibilityStateHidden);

	m_webView->setPage(m_page);
	m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_webView->installEventFilter(this);

	if (parameters.contains(QLatin1String("size")))
	{
		m_page->setViewportSize(parameters[QLatin1String("size")].toSize());
	}

	handleOptionChanged(SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption, SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption));
	handleOptionChanged(SettingsManager::Content_BackgroundColorOption, SettingsManager::getOption(SettingsManager::Content_BackgroundColorOption));
	handleOptionChanged(SettingsManager::History_BrowsingLimitAmountWindowOption, SettingsManager::getOption(SettingsManager::History_BrowsingLimitAmountWindowOption));
	setZoom(SettingsManager::getOption(SettingsManager::Content_DefaultZoomOption).toInt());

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &QtWebKitWebWidget::handleOptionChanged);
	connect(m_page, &QtWebKitPage::requestedNewWindow, this, &QtWebKitWebWidget::requestedNewWindow);
	connect(m_page, &QtWebKitPage::requestedPopupWindow, this, &QtWebKitWebWidget::requestedPopupWindow);
	connect(m_page, &QtWebKitPage::downloadRequested, this, &QtWebKitWebWidget::handleDownloadRequested);
	connect(m_page, &QtWebKitPage::unsupportedContent, this, &QtWebKitWebWidget::handleUnsupportedContent);
	connect(m_page, &QtWebKitPage::linkHovered, this, &QtWebKitWebWidget::setStatusMessageOverride);
	connect(m_page, &QtWebKitPage::microFocusChanged, this, [&]()
	{
		emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::EditingCategory});
	});
	connect(m_page, &QtWebKitPage::printRequested, this, &QtWebKitWebWidget::handlePrintRequest);
	connect(m_page, &QtWebKitPage::windowCloseRequested, this, &QtWebKitWebWidget::handleWindowCloseRequest);
	connect(m_page, &QtWebKitPage::fullScreenRequested, this, &QtWebKitWebWidget::handleFullScreenRequest);
	connect(m_page, &QtWebKitPage::featurePermissionRequested, this, &QtWebKitWebWidget::handlePermissionRequest);
	connect(m_page, &QtWebKitPage::featurePermissionRequestCanceled, this, &QtWebKitWebWidget::handlePermissionCancel);
	connect(m_page, &QtWebKitPage::loadStarted, this, &QtWebKitWebWidget::handleLoadStarted);
	connect(m_page, &QtWebKitPage::loadProgress, this, &QtWebKitWebWidget::handleLoadProgress);
	connect(m_page, &QtWebKitPage::loadFinished, this, &QtWebKitWebWidget::handleLoadFinished);
	connect(m_page, &QtWebKitPage::recentlyAudibleChanged, this, &QtWebKitWebWidget::isAudibleChanged);
	connect(m_page->mainFrame(), &QWebFrame::titleChanged, this, &QtWebKitWebWidget::notifyTitleChanged);
	connect(m_page->mainFrame(), &QWebFrame::urlChanged, this, &QtWebKitWebWidget::notifyUrlChanged);
	connect(m_page->mainFrame(), &QWebFrame::iconChanged, this, &QtWebKitWebWidget::notifyIconChanged);
	connect(m_page->mainFrame(), &QWebFrame::contentsSizeChanged, this, &QtWebKitWebWidget::geometryChanged);
	connect(m_page->mainFrame(), &QWebFrame::initialLayoutCompleted, this, &QtWebKitWebWidget::geometryChanged);
	connect(m_page->undoStack(), &QUndoStack::canRedoChanged, this, &QtWebKitWebWidget::notifyRedoActionStateChanged);
	connect(m_page->undoStack(), &QUndoStack::redoTextChanged, this, &QtWebKitWebWidget::notifyRedoActionStateChanged);
	connect(m_page->undoStack(), &QUndoStack::canUndoChanged, this, &QtWebKitWebWidget::notifyUndoActionStateChanged);
	connect(m_page->undoStack(), &QUndoStack::undoTextChanged, this, &QtWebKitWebWidget::notifyUndoActionStateChanged);
	connect(m_networkManager, &QtWebKitNetworkManager::contentStateChanged, this, [&]()
	{
		emit contentStateChanged(getContentState());
	});
	connect(new QShortcut(QKeySequence(QKeySequence::SelectAll), this, nullptr, nullptr, Qt::WidgetWithChildrenShortcut), &QShortcut::activated, this, [&]()
	{
		triggerAction(ActionsManager::SelectAllAction);
	});
}

QtWebKitWebWidget::~QtWebKitWebWidget()
{
	m_networkManager->blockSignals(true);

	m_page->undoStack()->blockSignals(true);
	m_page->blockSignals(true);
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

void QtWebKitWebWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	const HitTestResult hitResult(getCurrentHitTestResult());
	const QUrl url(extractUrl(parameters));

	switch (identifier)
	{
		case ActionsManager::SaveAction:
			if (m_page->isViewingMedia())
			{
				const SaveInformation information(Utils::getSavePath(suggestSaveFileName(SingleFileSaveFormat)));

				if (information.canSave)
				{
					QNetworkRequest request(getUrl());
					request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

					TransfersManager::startTransfer(m_networkManager->get(request), information.path, (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::CanOverwriteOption | Transfer::IsPrivateOption));
				}
			}
			else
			{
				SaveFormat format(UnknownSaveFormat);
				const QString path(getSavePath({SingleFileSaveFormat, PdfSaveFormat}, &format));

				if (!path.isEmpty())
				{
					switch (format)
					{
						case PdfSaveFormat:
							{
								QPrinter printer;
								printer.setOutputFormat(QPrinter::PdfFormat);
								printer.setOutputFileName(path);
								printer.setCreator(QStringLiteral("Otter Browser %1").arg(Application::getFullVersion()));
								printer.setDocName(getTitle());

								m_page->mainFrame()->print(&printer);
							}

							break;
						default:
							{
								QNetworkRequest request(getUrl());
								request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

								TransfersManager::startTransfer(m_networkManager->get(request), path, (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::CanOverwriteOption | Transfer::IsPrivateOption));
							}

							break;
					}
				}
			}

			break;
		case ActionsManager::ClearTabHistoryAction:
			if (parameters.value(QLatin1String("clearGlobalHistory")).toBool())
			{
				for (int i = 0; i < m_page->history()->count(); ++i)
				{
					const quint64 historyIdentifier(getGlobalHistoryEntryIdentifier(i));

					if (historyIdentifier > 0)
					{
						HistoryManager::removeEntry(historyIdentifier);
					}
				}
			}

			setUrl(QUrl(QLatin1String("about:blank")));

			m_page->history()->clear();

			emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::NavigationCategory});

			break;
		case ActionsManager::PurgeTabHistoryAction:
			triggerAction(ActionsManager::ClearTabHistoryAction, {{QLatin1String("clearGlobalHistory"), true}}, trigger);

			break;
		case ActionsManager::MuteTabMediaAction:
			m_isAudioMuted = !m_isAudioMuted;

			muteAudio(m_page->mainFrame(), m_isAudioMuted);

			emit arbitraryActionsStateChanged({ActionsManager::MuteTabMediaAction});

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
			{
				const SessionsManager::OpenHints hints((identifier == ActionsManager::OpenLinkAction) ? SessionsManager::calculateOpenHints(parameters) : mapOpenActionToOpenHints(identifier));

				if (hints == SessionsManager::DefaultOpen && !hitResult.flags.testFlag(HitTestResult::IsLinkFromSelectionTest))
				{
					m_page->triggerAction(QWebPage::OpenLink);

					setClickPosition({});
				}
				else if (url.isValid())
				{
					openUrl(url, hints);
				}
			}

			break;
		case ActionsManager::CopyLinkToClipboardAction:
			if (!url.isEmpty())
			{
				Application::clipboard()->setText(url.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			break;
		case ActionsManager::BookmarkLinkAction:
			if (url.isValid())
			{
				const QString title(hitResult.title);

				Application::triggerAction(ActionsManager::BookmarkPageAction, {{QLatin1String("url"), url}, {QLatin1String("title"), (title.isEmpty() ? m_page->mainFrame()->hitTestContent(hitResult.hitPosition).element().toPlainText() : title)}}, parentWidget(), trigger);
			}

			break;
		case ActionsManager::SaveLinkToDiskAction:
			if (url.isValid())
			{
				handleDownloadRequested(QNetworkRequest(url));
			}
			else
			{
				m_page->triggerAction(QWebPage::DownloadLinkToDisk);
			}

			break;
		case ActionsManager::SaveLinkToDownloadsAction:
			TransfersManager::addTransfer(TransfersManager::startTransfer(url.toString(), {}, (Transfer::CanNotifyOption | Transfer::CanAskForPathOption | Transfer::IsQuickTransferOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));

			break;
		case ActionsManager::OpenFrameAction:
		case ActionsManager::OpenFrameInCurrentTabAction:
		case ActionsManager::OpenFrameInNewTabAction:
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
			if (hitResult.frameUrl.isValid())
			{
				openUrl(hitResult.frameUrl, ((identifier == ActionsManager::OpenFrameAction) ? SessionsManager::calculateOpenHints(parameters) : mapOpenActionToOpenHints(identifier)));
			}

			break;
		case ActionsManager::CopyFrameLinkToClipboardAction:
			if (hitResult.frameUrl.isValid())
			{
				Application::clipboard()->setText(hitResult.frameUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			break;
		case ActionsManager::ReloadFrameAction:
			if (hitResult.frameUrl.isValid())
			{
				const QUrl frameUrl(hitResult.frameUrl);
				QWebFrame *frame(m_page->mainFrame()->hitTestContent(hitResult.hitPosition).frame());

				if (frame)
				{
					m_networkManager->addContentBlockingException(frameUrl, NetworkManager::SubFrameType);

					frame->setUrl({});
					frame->setUrl(frameUrl);
				}
			}

			break;
		case ActionsManager::ViewFrameSourceAction:
			if (hitResult.frameUrl.isValid())
			{
				const QString defaultEncoding(m_page->settings()->defaultTextEncoding());
				QNetworkRequest request(hitResult.frameUrl);
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

				QNetworkReply *reply(m_networkManager->get(request));
				SourceViewerWebWidget *sourceViewer(new SourceViewerWebWidget(isPrivate()));
				sourceViewer->setRequestedUrl(QUrl(QLatin1String("view-source:") + hitResult.frameUrl.toString()), false);

				if (!defaultEncoding.isEmpty())
				{
					sourceViewer->setOption(SettingsManager::Content_DefaultCharacterEncodingOption, defaultEncoding);
				}

				m_viewSourceReplies[reply] = sourceViewer;

				connect(reply, &QNetworkReply::finished, this, &QtWebKitWebWidget::handleViewSourceReplyFinished);

				emit requestedNewWindow(sourceViewer, SessionsManager::DefaultOpen, {});
			}

			break;
		case ActionsManager::OpenImageAction:
		case ActionsManager::OpenImageInNewTabAction:
		case ActionsManager::OpenImageInNewTabBackgroundAction:
			if (hitResult.imageUrl.isValid())
			{
				openUrl(hitResult.imageUrl, ((identifier == ActionsManager::OpenImageAction) ? SessionsManager::calculateOpenHints(parameters) : mapOpenActionToOpenHints(identifier)));
			}

			break;
		case ActionsManager::SaveImageToDiskAction:
			if (hitResult.imageUrl.isValid())
			{
				handleDownloadRequested(QNetworkRequest(hitResult.imageUrl));
			}

			break;
		case ActionsManager::CopyImageToClipboardAction:
			{
				const QPixmap pixmap(m_page->mainFrame()->hitTestContent(hitResult.hitPosition).pixmap());

				if (!pixmap.isNull())
				{
					Application::clipboard()->setPixmap(pixmap);
				}
				else
				{
					m_page->triggerAction(QWebPage::CopyImageToClipboard);
				}
			}

			break;
		case ActionsManager::CopyImageUrlToClipboardAction:
			if (!hitResult.imageUrl.isEmpty())
			{
				Application::clipboard()->setText(hitResult.imageUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			break;
		case ActionsManager::ReloadImageAction:
			if (!hitResult.imageUrl.isEmpty())
			{
				m_networkManager->addContentBlockingException(hitResult.imageUrl, NetworkManager::ImageType);

				m_page->settings()->setAttribute(QWebSettings::AutoLoadImages, true);

				if (getUrl().matches(hitResult.imageUrl, (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash)))
				{
					triggerAction(ActionsManager::ReloadAndBypassCacheAction, {}, trigger);
				}
				else
				{
					QWebElement element(m_page->mainFrame()->hitTestContent(hitResult.hitPosition).element());
					const QUrl imageUrl(hitResult.imageUrl);
					const QString src(element.attribute(QLatin1String("src")));
					NetworkCache *cache(NetworkManagerFactory::getCache());

					element.setAttribute(QLatin1String("src"), {});

					if (cache)
					{
						cache->remove(imageUrl);
					}

					element.setAttribute(QLatin1String("src"), src);

					m_page->mainFrame()->documentElement().evaluateJavaScript(QStringLiteral("let images = document.querySelectorAll('img[src=\"%1\"]'); for (let i = 0; i < images.length; ++i) { images[i].src = ''; images[i].src = '%1'; }").arg(src));
				}
			}

			break;
		case ActionsManager::ImagePropertiesAction:
			{
				QMap<ImagePropertiesDialog::ImageProperty, QVariant> properties({{ImagePropertiesDialog::AlternativeTextProperty, hitResult.alternateText}, {ImagePropertiesDialog::LongDescriptionProperty, hitResult.longDescription}});
				const QPixmap pixmap(m_page->mainFrame()->hitTestContent(hitResult.hitPosition).pixmap());

				if (!pixmap.isNull())
				{
					properties[ImagePropertiesDialog::SizeProperty] = pixmap.size();
					properties[ImagePropertiesDialog::DepthProperty] = pixmap.depth();
				}

				ImagePropertiesDialog *imagePropertiesDialog(new ImagePropertiesDialog(hitResult.imageUrl, properties, (m_networkManager->cache() ? m_networkManager->cache()->data(hitResult.imageUrl) : nullptr), this));
				imagePropertiesDialog->setButtonsVisible(false);

				ContentsDialog *dialog(new ContentsDialog(ThemesManager::createIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), {}, {}, QDialogButtonBox::Close, imagePropertiesDialog, this));

				connect(this, &QtWebKitWebWidget::aboutToReload, dialog, &ContentsDialog::close);
				connect(imagePropertiesDialog, &ImagePropertiesDialog::finished, dialog, &ContentsDialog::close);

				showDialog(dialog, false);
			}

			break;
		case ActionsManager::SaveMediaToDiskAction:
			if (hitResult.mediaUrl.isValid())
			{
				handleDownloadRequested(QNetworkRequest(hitResult.mediaUrl));
			}

			break;
		case ActionsManager::CopyMediaUrlToClipboardAction:
			if (!hitResult.mediaUrl.isEmpty())
			{
				Application::clipboard()->setText(hitResult.mediaUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			break;
		case ActionsManager::MediaControlsAction:
			m_page->triggerAction(QWebPage::ToggleMediaControls, parameters.value(QLatin1String("isChecked"), !getActionState(identifier, parameters).isChecked).toBool());

			break;
		case ActionsManager::MediaLoopAction:
			m_page->triggerAction(QWebPage::ToggleMediaLoop, parameters.value(QLatin1String("isChecked"), !getActionState(identifier, parameters).isChecked).toBool());

			break;
		case ActionsManager::MediaPlayPauseAction:
			m_page->triggerAction(QWebPage::ToggleMediaPlayPause);

			break;
		case ActionsManager::MediaMuteAction:
			m_page->triggerAction(QWebPage::ToggleMediaMute);

			break;
		case ActionsManager::MediaPlaybackRateAction:
			m_page->mainFrame()->hitTestContent(hitResult.hitPosition).element().evaluateJavaScript(QStringLiteral("this.playbackRate = %1").arg(parameters.value(QLatin1String("rate"), 1).toReal()));

			break;
		case ActionsManager::GoBackAction:
			m_page->triggerAction(QWebPage::Back);

			break;
		case ActionsManager::GoForwardAction:
			m_page->triggerAction(QWebPage::Forward);

			break;
		case ActionsManager::GoToHistoryIndexAction:
			if (parameters.contains(QLatin1String("index")))
			{
				const int index(parameters[QLatin1String("index")].toInt());

				if (index >= 0 && index < m_page->history()->count())
				{
					m_page->history()->goToItem(m_page->history()->itemAt(index));
				}
			}

			break;
		case ActionsManager::RewindAction:
			m_page->history()->goToItem(m_page->history()->itemAt(0));

			break;
		case ActionsManager::FastForwardAction:
			{
				const QUrl fastForwardUrl(m_page->mainFrame()->evaluateJavaScript(getFastForwardScript(true)).toUrl());

				if (fastForwardUrl.isValid())
				{
					setUrl(fastForwardUrl);
				}
				else if (canGoForward())
				{
					m_page->triggerAction(QWebPage::Forward);
				}
			}

			break;
		case ActionsManager::RemoveHistoryIndexAction:
			if (parameters.contains(QLatin1String("index")))
			{
				const int index(parameters[QLatin1String("index")].toInt());

				if (index >= 0 && index < m_page->history()->count())
				{
					if (parameters.value(QLatin1String("clearGlobalHistory"), false).toBool())
					{
						const quint64 entryIdentifier(getGlobalHistoryEntryIdentifier(index));

						if (entryIdentifier > 0)
						{
							HistoryManager::removeEntry(entryIdentifier);
						}
					}

					Session::Window::History history(getHistory());
					history.entries.removeAt(index);

					if (history.index >= index)
					{
						history.index = (history.index - 1);
					}

					setHistory(history);
				}
			}

			break;
		case ActionsManager::StopAction:
			m_page->triggerAction(QWebPage::Stop);

			break;
		case ActionsManager::StopScheduledReloadAction:
			m_page->triggerAction(QWebPage::StopScheduledPageRefresh);

			break;
		case ActionsManager::ReloadAction:
			emit aboutToReload();

			m_page->triggerAction(QWebPage::Stop);
			m_page->triggerAction(QWebPage::Reload);

			break;
		case ActionsManager::ReloadOrStopAction:
			if (m_loadingState == OngoingLoadingState)
			{
				triggerAction(ActionsManager::StopAction, {}, trigger);
			}
			else
			{
				triggerAction(ActionsManager::ReloadAction, {}, trigger);
			}

			break;
		case ActionsManager::ReloadAndBypassCacheAction:
			m_page->triggerAction(QWebPage::ReloadAndBypassCache);

			break;
		case ActionsManager::ContextMenuAction:
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

			break;
		case ActionsManager::UndoAction:
			m_page->triggerAction(QWebPage::Undo);

			break;
		case ActionsManager::RedoAction:
			m_page->triggerAction(QWebPage::Redo);

			break;
		case ActionsManager::CutAction:
			m_page->triggerAction(QWebPage::Cut);

			break;
		case ActionsManager::CopyAction:
			if (parameters.value(QLatin1String("mode")) == QLatin1String("plainText"))
			{
				const QString text(getSelectedText());

				if (!text.isEmpty())
				{
					Application::clipboard()->setText(text);
				}
			}
			else
			{
				m_page->triggerAction(QWebPage::Copy);
			}

			break;
		case ActionsManager::CopyPlainTextAction:
			triggerAction(ActionsManager::CopyAction, {{QLatin1String("mode"), QLatin1String("plainText")}}, trigger);

			break;
		case ActionsManager::CopyAddressAction:
			Application::clipboard()->setText(getUrl().toString());

			break;
		case ActionsManager::CopyToNoteAction:
			NotesManager::addNote(BookmarksModel::UrlBookmark, {{BookmarksModel::UrlRole, getUrl()}, {BookmarksModel::DescriptionRole, getSelectedText()}});

			break;
		case ActionsManager::PasteAction:
			if (parameters.contains(QLatin1String("note")))
			{
				const BookmarksModel::Bookmark *bookmark(NotesManager::getModel()->getBookmark(parameters[QLatin1String("note")].toULongLong()));

				if (bookmark)
				{
					triggerAction(ActionsManager::PasteAction, {{QLatin1String("text"), bookmark->getDescription()}}, trigger);
				}
			}
			else if (parameters.contains(QLatin1String("text")))
			{
				QMimeData *mimeData(new QMimeData());
				const QStringList mimeTypes(Application::clipboard()->mimeData()->formats());

				for (int i = 0; i < mimeTypes.count(); ++i)
				{
					mimeData->setData(mimeTypes.at(i), Application::clipboard()->mimeData()->data(mimeTypes.at(i)));
				}

				Application::clipboard()->setText(parameters[QLatin1String("text")].toString());

				m_page->triggerAction(QWebPage::Paste);

				Application::clipboard()->setMimeData(mimeData);
			}
			else if (parameters.value(QLatin1String("mode")) == QLatin1String("plainText"))
			{
				m_page->triggerAction(QWebPage::PasteAndMatchStyle);
			}
			else
			{
				m_page->triggerAction(QWebPage::Paste);
			}

			break;
		case ActionsManager::DeleteAction:
			m_page->triggerAction(QWebPage::DeleteEndOfWord);

			break;
		case ActionsManager::SelectAllAction:
			m_page->triggerAction(QWebPage::SelectAll);

			break;
		case ActionsManager::UnselectAction:
			m_page->triggerAction(QWebPage::Unselect);

			break;
		case ActionsManager::ClearAllAction:
			m_page->triggerAction(QWebPage::SelectAll);
			m_page->triggerAction(QWebPage::DeleteEndOfWord);

			break;
		case ActionsManager::CheckSpellingAction:
			{
				QWebElement element(m_page->mainFrame()->hitTestContent(hitResult.hitPosition).element());
				element.evaluateJavaScript(QStringLiteral("this.spellcheck = %1").arg(parameters.value(QLatin1String("isChecked"), !getActionState(identifier, parameters).isChecked).toBool() ? QLatin1String("true") : QLatin1String("false")));

				if (parameters.contains(QLatin1String("dictionary")))
				{
					const QString dictionary(parameters.value(QLatin1String("dictionary")).toString());
					QStringList dictionaries(getOption(SettingsManager::Browser_SpellCheckDictionaryOption).toStringList());

					if (dictionaries.contains(dictionary))
					{
						dictionaries.removeAll(dictionary);
					}
					else
					{
						dictionaries.append(dictionary);
					}

					setOption(SettingsManager::Browser_SpellCheckDictionaryOption, dictionaries);
				}
				else
				{
					resetSpellCheck(element);
				}

				emit arbitraryActionsStateChanged({ActionsManager::CheckSpellingAction});
			}

			break;
		case ActionsManager::CreateSearchAction:
			{
				const QWebElement element(m_page->mainFrame()->hitTestContent(hitResult.hitPosition).element());
				QWebElement parentElement(element.parent());

				while (!parentElement.isNull() && parentElement.tagName().toLower() != QLatin1String("form"))
				{
					parentElement = parentElement.parent();
				}

				if (!parentElement.hasAttribute(QLatin1String("action")))
				{
					break;
				}

				QList<QWebElement> inputElements(parentElement.findAll(QLatin1String("button:not([disabled])[name][type='submit'], input:not([disabled])[name], select:not([disabled])[name], textarea:not([disabled])[name]")).toList());

				if (inputElements.isEmpty())
				{
					break;
				}

				QWebElement searchTermsElement;
				const QString tagName(element.tagName().toLower());
				const QUrl targetUrl(parentElement.attribute(QLatin1String("action")));
				const QIcon icon(m_page->mainFrame()->icon());
				SearchEnginesManager::SearchEngineDefinition searchEngine;
				searchEngine.title = getTitle();
				searchEngine.formUrl = getUrl();
				searchEngine.icon = (icon.isNull() ? ThemesManager::createIcon(QLatin1String("edit-find")) : icon);
				searchEngine.resultsUrl.url = (targetUrl.isEmpty() ? getUrl() : resolveUrl(parentElement.webFrame(), targetUrl)).toString();
				searchEngine.resultsUrl.enctype = parentElement.attribute(QLatin1String("enctype"));
				searchEngine.resultsUrl.method = ((parentElement.attribute(QLatin1String("method"), QLatin1String("get")).toLower() == QLatin1String("post")) ? QLatin1String("post") : QLatin1String("get"));

				if (tagName != QLatin1String("select"))
				{
					const QString type(element.attribute(QLatin1String("type")));

					if (!inputElements.contains(element) && (type == QLatin1String("image") || type == QLatin1String("submit")))
					{
						inputElements.append(element);
					}

					if (tagName == QLatin1String("textarea") || type == QLatin1String("email") || type == QLatin1String("password") || type == QLatin1String("search") || type == QLatin1String("tel") || type == QLatin1String("text") || type == QLatin1String("url"))
					{
						searchTermsElement = element;
					}
				}

				if (searchTermsElement.isNull())
				{
					searchTermsElement = parentElement.findFirst(QLatin1String("input:not([disabled])[name][type='search']"));
				}

				for (int i = 0; i < inputElements.count(); ++i)
				{
					const QString name(inputElements.at(i).attribute(QLatin1String("name")));

					if (inputElements.at(i).tagName().toLower() != QLatin1String("select"))
					{
						const QString type(inputElements.at(i).attribute(QLatin1String("type")));
						const bool isSubmit(type == QLatin1String("image") || type == QLatin1String("submit"));

						if ((isSubmit && inputElements.at(i) != element) || ((type == QLatin1String("checkbox") || type == QLatin1String("radio")) && !inputElements[i].evaluateJavaScript(QLatin1String("this.checked")).toBool()))
						{
							continue;
						}

						if (isSubmit && inputElements.at(i) == element)
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

			break;
		case ActionsManager::ScrollToStartAction:
			m_page->mainFrame()->setScrollPosition(QPoint(m_page->mainFrame()->scrollPosition().x(), 0));

			break;
		case ActionsManager::ScrollToEndAction:
			m_page->mainFrame()->setScrollPosition(QPoint(m_page->mainFrame()->scrollPosition().x(), m_page->mainFrame()->scrollBarMaximum(Qt::Vertical)));

			break;
		case ActionsManager::ScrollPageUpAction:
			m_page->mainFrame()->setScrollPosition(QPoint(m_page->mainFrame()->scrollPosition().x(), qMax(0, (m_page->mainFrame()->scrollPosition().y() - m_webView->height()))));

			break;
		case ActionsManager::ScrollPageDownAction:
			m_page->mainFrame()->setScrollPosition(QPoint(m_page->mainFrame()->scrollPosition().x(), qMin(m_page->mainFrame()->scrollBarMaximum(Qt::Vertical), (m_page->mainFrame()->scrollPosition().y() + m_webView->height()))));

			break;
		case ActionsManager::ScrollPageLeftAction:
			m_page->mainFrame()->setScrollPosition(QPoint(qMax(0, (m_page->mainFrame()->scrollPosition().x() - m_webView->width())), m_page->mainFrame()->scrollPosition().y()));

			break;
		case ActionsManager::ScrollPageRightAction:
			m_page->mainFrame()->setScrollPosition(QPoint(qMin(m_page->mainFrame()->scrollBarMaximum(Qt::Horizontal), (m_page->mainFrame()->scrollPosition().x() + m_webView->width())), m_page->mainFrame()->scrollPosition().y()));

			break;
		case ActionsManager::TakeScreenshotAction:
			{
				const QString mode(parameters.value(QLatin1String("mode"), QLatin1String("viewport")).toString());
				const QSize viewportSize(m_page->viewportSize());
				const QSize contentsSize((mode == QLatin1String("viewport")) ? viewportSize : m_page->mainFrame()->contentsSize());
				const QRect rectangle((mode == QLatin1String("area")) ? JsonSettings::readRectangle(parameters.value(QLatin1String("geometry"))) : QRect());
				QPixmap pixmap(rectangle.isValid() ? rectangle.size() : contentsSize);
				QPainter painter(&pixmap);

				m_page->setViewportSize(contentsSize);

				if (rectangle.isValid())
				{
					painter.translate(-rectangle.topLeft());

					m_page->mainFrame()->render(&painter, {rectangle});
				}
				else
				{
					m_page->mainFrame()->render(&painter);
				}

				m_page->setViewportSize(viewportSize);

				const QStringList filters({tr("PNG image (*.png)"), tr("JPEG image (*.jpg *.jpeg)")});
				const SaveInformation result(Utils::getSavePath(suggestSaveFileName(QLatin1String(".png")), {}, filters));

				if (result.canSave)
				{
					pixmap.save(result.path, ((filters.indexOf(result.filter) == 0) ? "PNG" : "JPEG"));
				}
			}

			break;
		case ActionsManager::ActivateContentAction:
			{
				m_webView->setFocus();

				m_page->mainFrame()->setFocus();

				const QString tagName(m_page->mainFrame()->findFirstElement(QLatin1String(":focus")).tagName().toLower());

				if (tagName == QLatin1String("textarea") || tagName == QLatin1String("input"))
				{
					m_page->mainFrame()->documentElement().evaluateJavaScript(QLatin1String("document.activeElement.blur()"));
				}
			}

			break;
#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
		case ActionsManager::LoadPluginsAction:
			{
				m_canLoadPlugins = true;

				QList<QWebFrame*> frames({m_page->mainFrame()});

				while (!frames.isEmpty())
				{
					const QWebFrame *frame(frames.takeFirst());
					const QWebElementCollection elements(frame->documentElement().findAll(QLatin1String("object, embed")));

					for (int i = 0; i < elements.count(); ++i)
					{
						elements.at(i).replace(elements.at(i).clone());
					}

					frames.append(frame->childFrames());
				}

				m_amountOfDeferredPlugins = 0;

				emit arbitraryActionsStateChanged({ActionsManager::LoadPluginsAction});
			}

			break;
#endif
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

				connect(reply, &QNetworkReply::finished, this, &QtWebKitWebWidget::handleViewSourceReplyFinished);

				emit requestedNewWindow(sourceViewer, SessionsManager::DefaultOpen, {});
			}

			break;
		case ActionsManager::InspectPageAction:
			{
				const bool showInspector(parameters.value(QLatin1String("isChecked"), !getActionState(identifier, parameters).isChecked).toBool());

				if (showInspector && !m_inspectorWidget)
				{
					getInspector();
				}

				emit requestedInspectorVisibilityChange(showInspector);
				emit arbitraryActionsStateChanged({ActionsManager::InspectPageAction});
			}

			break;
		case ActionsManager::InspectElementAction:
			triggerAction(ActionsManager::InspectPageAction, {{QLatin1String("isChecked"), true}}, trigger);

			m_page->triggerAction(QWebPage::InspectElement);

			break;
		case ActionsManager::FullScreenAction:
			{
				const MainWindow *mainWindow(MainWindow::findMainWindow(this));

				if (mainWindow && !mainWindow->isFullScreen())
				{
					m_page->mainFrame()->evaluateJavaScript(QLatin1String("document.webkitExitFullscreen()"));
				}
			}

			break;
		case ActionsManager::WebsitePreferencesAction:
			{
				const QUrl pageUrl(getUrl());
				CookieJar *cookieJar(m_networkManager->getCookieJar());
				WebsitePreferencesDialog dialog(Utils::extractHost(pageUrl), (pageUrl.host().isEmpty() ? QVector<QNetworkCookie>() : cookieJar->getCookies(pageUrl.host())), this);

				if (dialog.exec() == QDialog::Accepted)
				{
					updateOptions(pageUrl);

					const QVector<QNetworkCookie> cookiesToDelete(dialog.getCookiesToDelete());
					const QVector<QNetworkCookie> cookiesToInsert(dialog.getCookiesToInsert());

					for (int i = 0; i < cookiesToDelete.count(); ++i)
					{
						cookieJar->forceDeleteCookie(cookiesToDelete.at(i));
					}

					for (int i = 0; i < cookiesToInsert.count(); ++i)
					{
						cookieJar->forceInsertCookie(cookiesToInsert.at(i));
					}
				}
			}

			break;
		default:
			break;
	}
}

void QtWebKitWebWidget::findInPage(const QString &text, FindFlags flags)
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
		m_page->findText({}, QWebPage::HighlightAllOccurrences);
		m_page->findText(text, (nativeFlags | QWebPage::HighlightAllOccurrences));
	}

	const bool hasMatches(m_page->findText(text, nativeFlags));

	emit findInPageResultsChanged(text, (hasMatches ? -1 : 0), (hasMatches ? -1 : 0));
}

void QtWebKitWebWidget::search(const QString &query, const QString &searchEngine)
{
	const SearchEnginesManager::SearchQuery searchQuery(SearchEnginesManager::setupSearchQuery(query, searchEngine));

	if (searchQuery.isValid())
	{
		setRequestedUrl(searchQuery.request.url(), false, true);
		updateOptions(searchQuery.request.url());

		m_page->mainFrame()->load(searchQuery.request, searchQuery.method, searchQuery.body);
	}
}

void QtWebKitWebWidget::print(QPrinter *printer)
{
	m_page->mainFrame()->print(printer);
}

#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
void QtWebKitWebWidget::clearPluginToken()
{
	QList<QWebFrame*> frames({m_page->mainFrame()});

	while (!frames.isEmpty())
	{
		const QWebFrame *frame(frames.takeFirst());
		QWebElement element(frame->documentElement().findFirst(QStringLiteral("object[data-otter-browser='%1'], embed[data-otter-browser='%1']").arg(m_pluginToken)));

		if (!element.isNull())
		{
			element.removeAttribute(QLatin1String("data-otter-browser"));

			break;
		}

		frames.append(frame->childFrames());
	}

	emit arbitraryActionsStateChanged({ActionsManager::LoadPluginsAction});

	m_pluginToken.clear();
}
#endif

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

void QtWebKitWebWidget::muteAudio(QWebFrame *frame, bool isMuted)
{
	if (!frame)
	{
		return;
	}

	const QString script(QLatin1String("this.muted = ") + (isMuted ? QLatin1String("true") : QLatin1String("false")));
	const QWebElementCollection elements(frame->findAllElements(QLatin1String("audio, video")));

	for (int i = 0; i < elements.count(); ++i)
	{
		elements.at(i).evaluateJavaScript(script);
	}

	const QList<QWebFrame*> frames(frame->childFrames());

	for (int i = 0; i < frames.count(); ++i)
	{
		muteAudio(frames.at(i), isMuted);
	}
}

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

	QTimer::singleShot(50, this, [&]()
	{
		m_page->mainFrame()->load(m_formRequest, m_formRequestOperation, m_formRequestBody);

		m_formRequest = QNetworkRequest();
		m_formRequestBody.clear();
	});
}

void QtWebKitWebWidget::openFormRequest(const QNetworkRequest &request, QNetworkAccessManager::Operation operation, QIODevice *outgoingData)
{
	m_page->triggerAction(QWebPage::Stop);

	QtWebKitWebWidget *widget(qobject_cast<QtWebKitWebWidget*>(clone(false)));
	widget->openRequest(request, operation, outgoingData);

	emit requestedNewWindow(widget, SessionsManager::calculateOpenHints(SessionsManager::NewTabOpen), {});
}

void QtWebKitWebWidget::startDelayedTransfer(Transfer *transfer)
{
	m_transfers.enqueue(transfer);

	if (m_transfersTimer == 0)
	{
		m_transfersTimer = startTimer(250);
	}
}

void QtWebKitWebWidget::handleDownloadRequested(const QNetworkRequest &request)
{
	const HitTestResult hitResult(getCurrentHitTestResult());

	if ((!hitResult.imageUrl.isEmpty() && request.url() == hitResult.imageUrl) || (!hitResult.mediaUrl.isEmpty() && request.url() == hitResult.mediaUrl))
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

				if (file.open(QIODevice::WriteOnly))
				{
					file.write(device->readAll());
					file.close();
				}
				else
				{
					QMessageBox::critical(this, tr("Error"), tr("Failed to open file for writing."), QMessageBox::Close);
				}

				device->deleteLater();

				return;
			}

			if (device)
			{
				device->deleteLater();
			}
		}
		else if (!hitResult.imageUrl.isEmpty() && hitResult.imageUrl.scheme() == QLatin1String("data") && hitResult.imageUrl.url().contains(QLatin1String(";base64,")))
		{
			const QString imageUrl(hitResult.imageUrl.url());
			const QString imageType(imageUrl.mid(11, (imageUrl.indexOf(QLatin1Char(';')) - 11)));
			const QString path(Utils::getSavePath(tr("file") + QLatin1Char('.') + imageType).path);

			if (!path.isEmpty())
			{
				QImageWriter writer(path);

				if (!writer.write(QImage::fromData(QByteArray::fromBase64(imageUrl.mid(imageUrl.indexOf(QLatin1String(";base64,")) + 7).toUtf8()), imageType.toStdString().c_str())))
				{
					Console::addMessage(tr("Failed to save image: %1").arg(writer.errorString()), Console::OtherCategory, Console::ErrorLevel, path, -1, getWindowIdentifier());
				}
			}

			return;
		}

		QNetworkRequest mutableRequest(request);
		mutableRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

		TransfersManager::startTransfer(m_networkManager->get(mutableRequest), {}, (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::IsPrivateOption));
	}
	else
	{
		startDelayedTransfer(TransfersManager::startTransfer(request, {}, (Transfer::CanNotifyOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));
	}
}

void QtWebKitWebWidget::handleUnsupportedContent(QNetworkReply *reply)
{
	m_networkManager->registerTransfer(reply);

	startDelayedTransfer(TransfersManager::startTransfer(reply, {}, (Transfer::CanNotifyOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));
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
			disconnect(m_page, &QtWebKitPage::statusBarMessage, this, &QtWebKitWebWidget::setStatusMessage);

			if (value.toBool() || SettingsManager::getOption(identifier, Utils::extractHost(getUrl())).toBool())
			{
				connect(m_page, &QtWebKitPage::statusBarMessage, this, &QtWebKitWebWidget::setStatusMessage);
			}
			else
			{
				setStatusMessage({});
			}

			break;
		default:
			break;
	}
}

void QtWebKitWebWidget::handleLoadStarted()
{
	if (m_loadingState == OngoingLoadingState)
	{
		return;
	}

	m_thumbnail = {};
	m_messageToken = QUuid::createUuid().toString();
#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
	m_canLoadPlugins = (getOption(SettingsManager::Permissions_EnablePluginsOption, getUrl()).toString() == QLatin1String("enabled"));
#endif
	m_loadingState = OngoingLoadingState;

	setStatusMessage({});
	setStatusMessageOverride({});

	emit geometryChanged();
	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::NavigationCategory});
	emit loadingStateChanged(OngoingLoadingState);
}

void QtWebKitWebWidget::handleLoadProgress(int progress)
{
	m_networkManager->setPageInformation(TotalLoadingProgressInformation, progress);
}

void QtWebKitWebWidget::handleLoadFinished(bool result)
{
	if (m_isAudioMuted)
	{
		muteAudio(m_page->mainFrame(), true);
	}

	if (m_loadingState != OngoingLoadingState)
	{
		return;
	}

	m_networkManager->handleLoadFinished(result);

	m_thumbnail = {};
	m_loadingState = FinishedLoadingState;

#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
	updateAmountOfDeferredPlugins();
#endif
	handleHistory();
	startReloadTimer();

	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::NavigationCategory});
	emit contentStateChanged(getContentState());
	emit loadingStateChanged(FinishedLoadingState);
	emit watchedDataChanged(FeedsWatcher);
	emit watchedDataChanged(LinksWatcher);
	emit watchedDataChanged(MetaDataWatcher);
	emit watchedDataChanged(SearchEnginesWatcher);
	emit watchedDataChanged(StylesheetsWatcher);
}

void QtWebKitWebWidget::handleViewSourceReplyFinished()
{
	QNetworkReply *reply(qobject_cast<QNetworkReply*>(sender()));

	if (reply)
	{
		if (reply->error() == QNetworkReply::NoError && m_viewSourceReplies.contains(reply) && m_viewSourceReplies[reply])
		{
			m_viewSourceReplies[reply]->setContents(reply->readAll(), reply->header(QNetworkRequest::ContentTypeHeader).toString());
		}

		m_viewSourceReplies.remove(reply);

		reply->deleteLater();
	}
}

void QtWebKitWebWidget::handlePrintRequest(QWebFrame *frame)
{
	QPrintPreviewDialog printPreviewDialog(this);
	printPreviewDialog.setWindowFlags(printPreviewDialog.windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
	printPreviewDialog.setWindowTitle(tr("Print Preview"));

	if (Application::getActiveWindow())
	{
		printPreviewDialog.resize(printPreviewDialog.size().expandedTo(Application::getActiveWindow()->size()));
	}

	connect(&printPreviewDialog, &QPrintPreviewDialog::paintRequested, frame, &QWebFrame::print);

	printPreviewDialog.exec();
}

void QtWebKitWebWidget::handleHistory()
{
	if (isPrivate() || m_page->history()->count() == 0)
	{
		return;
	}

	const QUrl url(m_page->history()->currentItem().url());
	const QVariant state(m_page->history()->currentItem().userData());

	if (state.isValid())
	{
		const quint64 identifier(state.toList().value(QtWebKitPage::IdentifierEntryData).toULongLong());

		if (identifier > 0)
		{
			HistoryManager::updateEntry(identifier, url, getTitle(), m_page->mainFrame()->icon());
		}
	}
	else
	{
		m_page->history()->currentItem().setUserData(QVariantList({(Utils::isUrlEmpty(url) ? 0 : HistoryManager::addEntry(url, getTitle(), m_page->mainFrame()->icon(), m_isTypedIn)), getZoom(), QPoint(0, 0), QDateTime::currentDateTimeUtc()}));

		if (m_isTypedIn)
		{
			m_isTypedIn = false;
		}

		SessionsManager::markSessionAsModified();
		BookmarksManager::updateVisits(url.toString());
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

void QtWebKitWebWidget::handleFullScreenRequest(QWebFullScreenRequest request)
{
	request.accept();

	if (request.toggleOn())
	{
		const QString value(SettingsManager::getOption(SettingsManager::Permissions_EnableFullScreenOption, Utils::extractHost(request.origin())).toString());

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

void QtWebKitWebWidget::handlePermissionRequest(QWebFrame *frame, QWebPage::Feature feature)
{
	notifyPermissionRequested(frame, feature, false);
}

void QtWebKitWebWidget::handlePermissionCancel(QWebFrame *frame, QWebPage::Feature feature)
{
	notifyPermissionRequested(frame, feature, true);
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

	emit urlChanged(url);
	emit arbitraryActionsStateChanged({ActionsManager::InspectPageAction, ActionsManager::InspectElementAction});
	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::NavigationCategory, ActionsManager::ActionDefinition::PageCategory});

	SessionsManager::markSessionAsModified();
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
	}

	if (feature == UnknownFeature)
	{
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

#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
void QtWebKitWebWidget::updateAmountOfDeferredPlugins()
{
	const int amountOfDeferredPlugins(m_canLoadPlugins ? 0 : findChildren<QtWebKitPluginWidget*>().count());

	if (amountOfDeferredPlugins != m_amountOfDeferredPlugins)
	{
		const bool needsActionUpdate(amountOfDeferredPlugins == 0 || m_amountOfDeferredPlugins == 0);

		m_amountOfDeferredPlugins = amountOfDeferredPlugins;

		if (needsActionUpdate)
		{
			emit arbitraryActionsStateChanged({ActionsManager::LoadPluginsAction});
		}
	}
}
#endif

void QtWebKitWebWidget::updateOptions(const QUrl &url)
{
	const QString encoding(getOption(SettingsManager::Content_DefaultCharacterEncodingOption, url).toString());
#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
	const bool arePluginsEnabled(getOption(SettingsManager::Permissions_EnablePluginsOption, url).toString() != QLatin1String("disabled"));
#endif
	QWebSettings *settings(m_page->settings());
	settings->setAttribute(QWebSettings::AutoLoadImages, (getOption(SettingsManager::Permissions_EnableImagesOption, url).toString() != QLatin1String("onlyCached")));
	settings->setAttribute(QWebSettings::DnsPrefetchEnabled, getOption(SettingsManager::Network_EnableDnsPrefetchOption, url).toBool());
#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
	settings->setAttribute(QWebSettings::PluginsEnabled, arePluginsEnabled);
	settings->setAttribute(QWebSettings::JavaEnabled, arePluginsEnabled);
#endif
	settings->setAttribute(QWebSettings::JavascriptEnabled, (m_page->isDisplayingErrorPage() || m_page->isViewingMedia() || getOption(SettingsManager::Permissions_EnableJavaScriptOption, url).toBool()));
	settings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, getOption(SettingsManager::Permissions_ScriptsCanAccessClipboardOption, url).toBool());
	settings->setAttribute(QWebSettings::JavascriptCanOpenWindows, (getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, url).toString() != QLatin1String("blockAll")));
	settings->setAttribute(QWebSettings::WebGLEnabled, getOption(SettingsManager::Permissions_EnableWebglOption, url).toBool());
	settings->setAttribute(QWebSettings::LocalStorageEnabled, getOption(SettingsManager::Permissions_EnableLocalStorageOption, url).toBool());
	settings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, getOption(SettingsManager::Permissions_EnableOfflineStorageDatabaseOption, url).toBool());
	settings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, getOption(SettingsManager::Permissions_EnableOfflineWebApplicationCacheOption, url).toBool());
	settings->setAttribute(QWebSettings::AllowRunningInsecureContent, getOption(SettingsManager::Security_AllowMixedContentOption, url).toBool());
	settings->setAttribute(QWebSettings::MediaEnabled, getOption(QtWebKitWebBackend::getOptionIdentifier(QtWebKitWebBackend::QtWebKitBackend_EnableMediaOption), url).toBool());
	settings->setAttribute(QWebSettings::MediaSourceEnabled, getOption(QtWebKitWebBackend::getOptionIdentifier(QtWebKitWebBackend::QtWebKitBackend_EnableMediaSourceOption), url).toBool());
	settings->setAttribute(QWebSettings::SiteSpecificQuirksEnabled, getOption(QtWebKitWebBackend::getOptionIdentifier(QtWebKitWebBackend::QtWebKitBackend_EnableSiteSpecificQuirksOption), url).toBool());
	settings->setAttribute(QWebSettings::WebSecurityEnabled, getOption(QtWebKitWebBackend::getOptionIdentifier(QtWebKitWebBackend::QtWebKitBackend_EnableWebSecurityOption), url).toBool());
	settings->setDefaultTextEncoding((encoding == QLatin1String("auto")) ? QString() : encoding);

	disconnect(m_page, &QtWebKitPage::geometryChangeRequested, this, &QtWebKitWebWidget::requestedGeometryChange);
	disconnect(m_page, &QtWebKitPage::statusBarMessage, this, &QtWebKitWebWidget::setStatusMessage);

	if (getOption(SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption, url).toBool())
	{
		connect(m_page, &QtWebKitPage::geometryChangeRequested, this, &QtWebKitWebWidget::requestedGeometryChange);
	}

	if (getOption(SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption, url).toBool())
	{
		connect(m_page, &QtWebKitPage::statusBarMessage, this, &QtWebKitWebWidget::setStatusMessage);
	}
	else
	{
		setStatusMessage({});
	}

	m_page->updateStyleSheets(url);

	m_networkManager->updateOptions(url);

#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
	m_canLoadPlugins = (getOption(SettingsManager::Permissions_EnablePluginsOption, url).toString() == QLatin1String("enabled"));
#endif
}

void QtWebKitWebWidget::clearOptions()
{
	WebWidget::clearOptions();

	updateOptions(getUrl());
}

void QtWebKitWebWidget::fillPassword(const PasswordsManager::PasswordInformation &password)
{
	QFile file(QLatin1String(":/modules/backends/web/qtwebkit/resources/formFiller.js"));

	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	QJsonArray fieldsArray;

	for (int i = 0; i < password.fields.count(); ++i)
	{
		fieldsArray.append(QJsonObject({{QLatin1String("name"), password.fields.at(i).name}, {QLatin1String("value"), password.fields.at(i).value}, {QLatin1String("type"), ((password.fields.at(i).type == PasswordsManager::PasswordField) ? QLatin1String("password") : QLatin1String("text"))}}));
	}

	const QString script(QString::fromLatin1(file.readAll()).arg(QString::fromLatin1(QJsonDocument(fieldsArray).toJson(QJsonDocument::Indented))));

	file.close();

	QList<QWebFrame*> frames({m_page->mainFrame()});

	while (!frames.isEmpty())
	{
		const QWebFrame *frame(frames.takeFirst());
		frame->documentElement().evaluateJavaScript(script);

		frames.append(frame->childFrames());
	}
}

void QtWebKitWebWidget::setActiveStyleSheet(const QString &styleSheet)
{
	const QWebElementCollection elements(m_page->mainFrame()->findAllElements(QLatin1String("link[rel='alternate stylesheet']")));

	for (int i = 0; i < elements.count(); ++i)
	{
		elements.at(i).evaluateJavaScript(QLatin1String("this.disabled = ") + ((elements.at(i).attribute(QLatin1String("title")) == styleSheet) ? QLatin1String("false") : QLatin1String("true")));
	}
}

void QtWebKitWebWidget::setHistory(const Session::Window::History &history)
{
	if (history.entries.isEmpty())
	{
		m_page->history()->clear();

		updateOptions({});

		if (m_page->getMainFrame())
		{
			m_page->getMainFrame()->runUserScripts(QUrl(QLatin1String("about:blank")));
		}

		emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::NavigationCategory, ActionsManager::ActionDefinition::PageCategory});

		return;
	}

	const int index(qMin(history.index, (m_page->history()->maximumItemCount() - 1)));
	QVariantList entries;
	entries.reserve(history.entries.count());

	for (int i = 0; i < history.entries.count(); ++i)
	{
		QVariantMap entry;
		entry[QLatin1String("pageScaleFactor")] = 0;
		entry[QLatin1String("title")] = history.entries.at(i).title;
		entry[QLatin1String("urlString")] = QString::fromLatin1(QUrl::fromUserInput(history.entries.at(i).url).toEncoded());
		entry[QLatin1String("scrollPosition")] = QVariantMap({{QLatin1String("x"), history.entries.at(i).position.x()}, {QLatin1String("y"), history.entries.at(i).position.y()}});

		entries.append(entry);
	}

	m_page->history()->loadFromMap({{QLatin1String("currentItemIndex"), index}, {QLatin1String("history"), entries}});

	for (int i = 0; i < history.entries.count(); ++i)
	{
		m_page->history()->itemAt(i).setUserData(QVariantList({-1, history.entries.at(i).zoom, history.entries.at(i).position, history.entries.at(i).time}));
	}

	m_page->history()->goToItem(m_page->history()->itemAt(index));

	const QUrl url(m_page->history()->itemAt(index).url());

	setRequestedUrl(url, false, true);
	updateOptions(url);

	m_page->triggerAction(QWebPage::Reload);

	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::PageCategory});
}

void QtWebKitWebWidget::setHistory(const QVariantMap &history)
{
	m_page->history()->loadFromMap(history);

	const QUrl url(m_page->history()->currentItem().url());

	setRequestedUrl(url, false, true);
	updateOptions(url);

	m_page->triggerAction(QWebPage::Reload);

	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::PageCategory});
}

void QtWebKitWebWidget::setZoom(int zoom)
{
	if (zoom != getZoom())
	{
		m_page->mainFrame()->setZoomFactor(qBound(0.1, (static_cast<qreal>(zoom) / 100), static_cast<qreal>(100)));

		SessionsManager::markSessionAsModified();

		emit zoomChanged(zoom);
		emit geometryChanged();
	}
}

void QtWebKitWebWidget::setUrl(const QUrl &url, bool isTypedIn)
{
	if (url.scheme() == QLatin1String("javascript"))
	{
		m_page->mainFrame()->documentElement().evaluateJavaScript(url.toDisplayString(QUrl::RemoveScheme | QUrl::DecodeReserved));
	}
	else if (!url.fragment().isEmpty() && url.matches(getUrl(), (QUrl::RemoveFragment | QUrl::StripTrailingSlash | QUrl::NormalizePathSegments)))
	{
		m_page->mainFrame()->scrollToAnchor(url.fragment());
	}
	else
	{
		m_isTypedIn = isTypedIn;

		const QUrl targetUrl(Utils::expandUrl(url));

		updateOptions(targetUrl);

		m_page->mainFrame()->load(targetUrl);

		notifyTitleChanged();
		notifyIconChanged();
	}
}

void QtWebKitWebWidget::setPermission(FeaturePermission feature, const QUrl &url, PermissionPolicies policies)
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

	switch (identifier)
	{
		case SettingsManager::Content_DefaultCharacterEncodingOption:
			m_page->triggerAction(QWebPage::Reload);

			break;
		case SettingsManager::Browser_SpellCheckDictionaryOption:
			emit widgetActivated(this);

			resetSpellCheck(m_page->mainFrame()->hitTestContent(getCurrentHitTestResult().hitPosition).element());

			break;
		default:
			break;
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

WebWidget* QtWebKitWebWidget::clone(bool cloneHistory, bool isPrivate, const QStringList &excludedOptions) const
{
	QtWebKitNetworkManager *networkManager((this->isPrivate() != isPrivate) ? nullptr : m_networkManager->clone());
	QtWebKitWebWidget *widget((this->isPrivate() || isPrivate) ? new QtWebKitWebWidget({{QLatin1String("hints"), SessionsManager::PrivateOpen}}, getBackend(), networkManager, nullptr) : new QtWebKitWebWidget({}, getBackend(), networkManager, nullptr));
	widget->getPage()->setViewportSize(m_page->viewportSize());
	widget->setOptions(getOptions(), excludedOptions);

	if (cloneHistory)
	{
		widget->setHistory(m_page->history()->toMap());
	}

	widget->setZoom(getZoom());

	return widget;
}

QWidget* QtWebKitWebWidget::getInspector()
{
	if (!m_inspectorWidget)
	{
		m_inspectorWidget = new QtWebKitInspectorWidget(m_page, this);
	}

	return m_inspectorWidget;
}

QWidget* QtWebKitWebWidget::getViewport()
{
	return m_webView;
}

QtWebKitPage* QtWebKitWebWidget::getPage() const
{
	return m_page;
}

QString QtWebKitWebWidget::getTitle() const
{
	const QString title(m_page->mainFrame()->title());

	if (!title.isEmpty())
	{
		return title;
	}

	const QUrl url(getUrl());

	if (Utils::isUrlEmpty(url))
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

QString QtWebKitWebWidget::getDescription() const
{
	const QString description(m_page->mainFrame()->findFirstElement(QLatin1String("[name='description']")).attribute(QLatin1String("content")));

	return (description.isEmpty() ? m_page->mainFrame()->findFirstElement(QLatin1String("[name='og:description']")).attribute(QLatin1String("property")) : description);
}

QString QtWebKitWebWidget::getActiveStyleSheet() const
{
	if (m_page->mainFrame()->documentElement().evaluateJavaScript(QLatin1String("let isDefault = true; for (let i = 0; i < document.styleSheets.length; ++i) { if (document.styleSheets[i].ownerNode.rel.indexOf('alt') >= 0) { isDefault = false; break; } } isDefault")).toBool())
	{
		return {};
	}

	return m_page->mainFrame()->findFirstElement(QLatin1String("link[rel='alternate stylesheet']:not([disabled])")).attribute(QLatin1String("title"));
}

QString QtWebKitWebWidget::getSelectedText() const
{
	return m_page->selectedText();
}

QString QtWebKitWebWidget::getMessageToken() const
{
	return m_messageToken;
}

#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
QString QtWebKitWebWidget::getPluginToken() const
{
	return m_pluginToken;
}
#endif

QVariant QtWebKitWebWidget::getPageInformation(PageInformation key) const
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
		return (frame ? frame->baseUrl() : getUrl()).resolved(url);
	}

	return url;
}

QUrl QtWebKitWebWidget::getUrl() const
{
	const QUrl url(m_page->mainFrame()->url());

	return (Utils::isUrlEmpty(url) ? m_page->mainFrame()->requestedUrl() : url);
}

QIcon QtWebKitWebWidget::getIcon() const
{
	if (isPrivate())
	{
		return ThemesManager::createIcon(QLatin1String("tab-private"));
	}

	const QIcon icon(m_page->mainFrame()->icon());

	return (icon.isNull() ? ThemesManager::createIcon(QLatin1String("tab")) : icon);
}

QPixmap QtWebKitWebWidget::createThumbnail(const QSize &size)
{
	if ((size.isNull() || size == m_thumbnail.size()) && ((!m_thumbnail.isNull() && qFuzzyCompare(m_thumbnail.devicePixelRatio(), devicePixelRatio())) || m_loadingState == OngoingLoadingState))
	{
		return m_thumbnail;
	}

	const QSize thumbnailSize(size.isValid() ? size : getDefaultThumbnailSize());
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

	contentsSize.setHeight(qRound(thumbnailSize.height() * (static_cast<qreal>(contentsSize.width()) / thumbnailSize.width())));

	m_page->setViewportSize(contentsSize);

	QPixmap pixmap(contentsSize);
	pixmap.fill(Qt::white);

	QPainter painter(&pixmap);

	m_page->mainFrame()->render(&painter, QWebFrame::ContentsLayer, QRegion({{0, 0}, contentsSize}));
	m_page->mainFrame()->setZoomFactor(zoom);
	m_page->setView(oldView);
	m_page->setViewportSize(oldViewportSize);
	m_page->mainFrame()->setScrollPosition(position);

	painter.end();

	pixmap = pixmap.scaled((thumbnailSize * devicePixelRatio()), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	pixmap.setDevicePixelRatio(devicePixelRatio());

	newView->deleteLater();

	if (size.isNull())
	{
		m_thumbnail = pixmap;
	}

	return pixmap;
}

QPoint QtWebKitWebWidget::getScrollPosition() const
{
	return m_page->mainFrame()->scrollPosition();
}

QRect QtWebKitWebWidget::getGeometry(bool excludeScrollBars) const
{
	if (!excludeScrollBars)
	{
		return geometry();
	}

	QRect geometry(this->geometry());
	const QRect horizontalScrollBar(m_page->mainFrame()->scrollBarGeometry(Qt::Horizontal));
	const QRect verticalScrollBar(m_page->mainFrame()->scrollBarGeometry(Qt::Vertical));

	if (horizontalScrollBar.height() > 0 && geometry.intersects(horizontalScrollBar))
	{
		geometry.moveTop(m_webView->height() - horizontalScrollBar.height());
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
	const QWebFrame *frame(m_page->frameAt(getClickPosition()));
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

WebWidget::LinkUrl QtWebKitWebWidget::getActiveMedia() const
{
	const QWebHitTestResult result(m_page->mainFrame()->hitTestContent(getClickPosition()));
	LinkUrl link;

	if (!result.mediaUrl().isEmpty())
	{
		link.title = result.title();
		link.url = result.mediaUrl();
	}

	return link;
}

WebWidget::SslInformation QtWebKitWebWidget::getSslInformation() const
{
	return m_networkManager->getSslInformation();
}

Session::Window::History QtWebKitWebWidget::getHistory() const
{
	QVariantList state(m_page->history()->currentItem().userData().toList());

	if (state.isEmpty() || state.count() < 4)
	{
		state = {0, getZoom(), m_page->mainFrame()->scrollPosition(), QDateTime::currentDateTimeUtc()};
	}
	else
	{
		state[QtWebKitPage::ZoomEntryData] = getZoom();
		state[QtWebKitPage::PositionEntryData] = m_page->mainFrame()->scrollPosition();
	}

	m_page->history()->currentItem().setUserData(state);

	const QWebHistory *pageHistory(m_page->history());
	const QUrl requestedUrl(m_page->mainFrame()->requestedUrl());
	const int historyCount(pageHistory->count());
	Session::Window::History history;
	history.entries.reserve(historyCount);
	history.index = pageHistory->currentItemIndex();

	for (int i = 0; i < historyCount; ++i)
	{
		const QWebHistoryItem item(pageHistory->itemAt(i));
		const QVariantList itemState(item.userData().toList());
		Session::Window::History::Entry entry;
		entry.url = item.url().toString();
		entry.title = item.title();
		entry.time = itemState.value(QtWebKitPage::VisitTimeEntryData).toDateTime();
		entry.position = itemState.value(QtWebKitPage::PositionEntryData, QPoint(0, 0)).toPoint();
		entry.zoom = itemState.value(QtWebKitPage::ZoomEntryData).toInt();

		const quint64 identifier(itemState.value(QtWebKitPage::IdentifierEntryData).toULongLong());

		if (identifier > 0)
		{
			const HistoryModel::Entry *globalEntry(HistoryManager::getEntry(identifier));

			if (globalEntry)
			{
				entry.icon = globalEntry->icon();
			}
		}

		history.entries.append(entry);
	}

	if (m_loadingState == OngoingLoadingState && requestedUrl != pageHistory->itemAt(pageHistory->currentItemIndex()).url())
	{
		Session::Window::History::Entry entry;
		entry.url = requestedUrl.toString();
		entry.title = getTitle();
		entry.position = state.value(QtWebKitPage::PositionEntryData, QPoint(0, 0)).toPoint();
		entry.zoom = state.value(QtWebKitPage::ZoomEntryData).toInt();

		history.entries.append(entry);
		history.index = historyCount;
	}

	return history;
}

WebWidget::HitTestResult QtWebKitWebWidget::getHitTestResult(const QPoint &position)
{
	const QWebFrame *frame(m_page->frameAt(position));

	if (!frame)
	{
		return {};
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
	result.hitPosition = position;
	result.elementGeometry = nativeResult.element().geometry();

	const QString type(nativeResult.element().attribute(QLatin1String("type")).toLower());
	const bool isSubmit((result.tagName == QLatin1String("button") || result.tagName == QLatin1String("input")) && (type == QLatin1String("image") || type == QLatin1String("submit")));

	if (isSubmit || result.tagName == QLatin1String("button") || result.tagName == QLatin1String("input") || result.tagName == QLatin1String("select") || result.tagName == QLatin1String("textarea"))
	{
		QWebElement parentElement(nativeResult.element().parent());

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
				result.flags |= HitTestResult::IsFormTest;
			}
		}
	}

	if (result.tagName == QLatin1String("textarea") || (result.tagName == QLatin1String("input") && (type.isEmpty() || type == QLatin1String("text") || type == QLatin1String("password") || type == QLatin1String("search"))))
	{
		if (!nativeResult.element().hasAttribute(QLatin1String("disabled")) && !nativeResult.element().hasAttribute(QLatin1String("readonly")))
		{
			result.flags |= HitTestResult::IsContentEditableTest;
		}

		if (nativeResult.element().evaluateJavaScript(QLatin1String("this.value")).toString().isEmpty())
		{
			result.flags |= HitTestResult::IsEmptyTest;
		}
	}
	else if (nativeResult.isContentEditable())
	{
		result.flags |= HitTestResult::IsContentEditableTest;
	}

	if (nativeResult.isContentSelected())
	{
		result.flags |= HitTestResult::IsSelectedTest;
	}

	if (nativeResult.element().evaluateJavaScript(QLatin1String("this.spellcheck")).toBool())
	{
		result.flags |= HitTestResult::IsSpellCheckEnabledTest;
	}

	if (result.mediaUrl.isValid())
	{
		if (nativeResult.element().evaluateJavaScript(QLatin1String("this.controls")).toBool())
		{
			result.flags |= HitTestResult::MediaHasControlsTest;
		}

		if (nativeResult.element().evaluateJavaScript(QLatin1String("this.looped")).toBool())
		{
			result.flags |= HitTestResult::MediaIsLoopedTest;
		}

		if (nativeResult.element().evaluateJavaScript(QLatin1String("this.muted")).toBool())
		{
			result.flags |= HitTestResult::MediaIsMutedTest;
		}

		if (nativeResult.element().evaluateJavaScript(QLatin1String("this.paused")).toBool())
		{
			result.flags |= HitTestResult::MediaIsPausedTest;
		}

		result.playbackRate = nativeResult.element().evaluateJavaScript(QLatin1String("this.playbackRate")).toReal();
	}

	if (result.flags.testFlag(HitTestResult::IsSelectedTest) && !result.linkUrl.isValid() && Utils::isUrl(m_page->selectedText()))
	{
		result.flags |= HitTestResult::IsLinkFromSelectionTest;
		result.linkUrl = QUrl::fromUserInput(m_page->selectedText());
	}

	return result;
}

QStringList QtWebKitWebWidget::getBlockedElements() const
{
	return m_networkManager->getBlockedElements();
}

QStringList QtWebKitWebWidget::getStyleSheets() const
{
	const QWebElementCollection elements(m_page->mainFrame()->findAllElements(QLatin1String("link[rel='alternate stylesheet']")));
	QStringList titles;
	titles.reserve(elements.count());

	for (int i = 0; i < elements.count(); ++i)
	{
		const QString title(elements.at(i).attribute(QLatin1String("title")));

		if (!title.isEmpty() && !titles.contains(title))
		{
			titles.append(title);
		}
	}

	return titles;
}

QVector<WebWidget::LinkUrl> QtWebKitWebWidget::getFeeds() const
{
	return getLinks(QLatin1String("a[type='application/atom+xml'], a[type='application/rss+xml'], link[type='application/atom+xml'], link[type='application/rss+xml']"));
}

QVector<WebWidget::LinkUrl> QtWebKitWebWidget::getLinks(const QString &query) const
{
	const QWebElementCollection elements(m_page->mainFrame()->findAllElements(query));
	QSet<QUrl> urls;
	urls.reserve(elements.count());

	QVector<LinkUrl> links;
	links.reserve(elements.count());

	for (int i = 0; i < elements.count(); ++i)
	{
		const QUrl url(resolveUrl(m_page->mainFrame(), QUrl(elements.at(i).attribute(QLatin1String("href")))));

		if (urls.contains(url))
		{
			continue;
		}

		urls.insert(url);

		LinkUrl link;
		link.title = elements.at(i).attribute(QLatin1String("title"));
		link.mimeType = elements.at(i).attribute(QLatin1String("type"));
		link.url = url;

		if (link.title.isEmpty())
		{
			link.title = elements.at(i).toPlainText().simplified();
		}

		if (link.title.isEmpty())
		{
			const QWebElement imageElement(elements.at(i).findFirst(QLatin1String("img[alt]:not([alt=''])")));

			if (!imageElement.isNull())
			{
				link.title = imageElement.attribute(QLatin1String("alt"));
			}
		}

		links.append(link);
	}

	links.squeeze();

	return links;
}

QVector<WebWidget::LinkUrl> QtWebKitWebWidget::getLinks() const
{
	return getLinks(QLatin1String("a[href]"));
}

QVector<WebWidget::LinkUrl> QtWebKitWebWidget::getSearchEngines() const
{
	return getLinks(QLatin1String("link[type='application/opensearchdescription+xml']"));
}

QVector<NetworkManager::ResourceInformation> QtWebKitWebWidget::getBlockedRequests() const
{
	return m_networkManager->getBlockedRequests();
}

QMap<QByteArray, QByteArray> QtWebKitWebWidget::getHeaders() const
{
	return m_networkManager->getHeaders();
}

QMultiMap<QString, QString> QtWebKitWebWidget::getMetaData() const
{
	return m_page->mainFrame()->metaData();
}

WebWidget::ContentStates QtWebKitWebWidget::getContentState() const
{
	const QUrl url(getUrl());

	if (url.isEmpty() || url.scheme() == QLatin1String("about"))
	{
		return ApplicationContentState;
	}

	if (url.scheme() == QLatin1String("file"))
	{
		return LocalContentState;
	}

	ContentStates state(m_networkManager->getContentState() | RemoteContentState);

	if (getOption(SettingsManager::Security_EnableFraudCheckingOption, url).toBool() && ContentFiltersManager::isFraud(url))
	{
		state |= FraudContentState;
	}

	if (Utils::isUrlAmbiguous(url))
	{
		state |= AmbiguousContentState;
	}

	return state;
}

WebWidget::LoadingState QtWebKitWebWidget::getLoadingState() const
{
	return m_loadingState;
}

quint64 QtWebKitWebWidget::getGlobalHistoryEntryIdentifier(int index) const
{
	if (index >= 0 && index < m_page->history()->count())
	{
		const QVariant state(m_page->history()->itemAt(index).userData());

		if (state.isValid())
		{
			return state.toList().value(QtWebKitPage::IdentifierEntryData).toULongLong();
		}
	}

	return 0;
}

int QtWebKitWebWidget::getZoom() const
{
	return static_cast<int>(m_page->mainFrame()->zoomFactor() * 100);
}

#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
int QtWebKitWebWidget::getAmountOfDeferredPlugins() const
{
	return m_amountOfDeferredPlugins;
}

bool QtWebKitWebWidget::canLoadPlugins() const
{
	return m_canLoadPlugins;
}
#endif

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

	if (Utils::isUrlEmpty(getUrl()))
	{
		return false;
	}

	return m_page->mainFrame()->evaluateJavaScript(getFastForwardScript(false)).toBool();
}

bool QtWebKitWebWidget::canInspect() const
{
	return !Utils::isUrlEmpty(getUrl());
}

bool QtWebKitWebWidget::canTakeScreenshot() const
{
	return true;
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
	return (!m_page->isDisplayingErrorPage() && !m_page->isViewingMedia() && !Utils::isUrlEmpty(getUrl()));
}

bool QtWebKitWebWidget::hasSelection() const
{
	return (m_page->hasSelection() && !m_page->selectedText().isEmpty());
}

bool QtWebKitWebWidget::hasWatchedChanges(ChangeWatcher watcher) const
{
	Q_UNUSED(watcher)

	return true;
}

bool QtWebKitWebWidget::isAudible() const
{
	return m_page->recentlyAudible();
}

bool QtWebKitWebWidget::isAudioMuted() const
{
	return m_isAudioMuted;
}

bool QtWebKitWebWidget::isFullScreen() const
{
	return m_isFullScreen;
}

bool QtWebKitWebWidget::isInspecting() const
{
	return (m_inspectorWidget && m_inspectorWidget->isVisible());
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
		const QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
		if (event->type() == QEvent::MouseButtonPress && mouseEvent && mouseEvent->button() == Qt::LeftButton && SettingsManager::getOption(SettingsManager::Permissions_EnablePluginsOption, Utils::extractHost(getUrl())).toString() == QLatin1String("onDemand"))
		{
			const QWidget *widget(childAt(mouseEvent->pos()));

			if (widget && widget->metaObject()->className() == QLatin1String("Otter::QtWebKitPluginWidget"))
			{
				QWebElement element(m_page->mainFrame()->hitTestContent(mouseEvent->pos()).element());
				const QString tagName(element.tagName().toLower());

				if (tagName == QLatin1String("object") || tagName == QLatin1String("embed"))
				{
					m_pluginToken = QUuid::createUuid().toString();

					QWebElement clonedElement(element.clone());
					clonedElement.setAttribute(QLatin1String("data-otter-browser"), m_pluginToken);

					element.replace(clonedElement);

					return true;
				}
			}
		}
#endif

		switch (event->type())
		{
#ifdef OTTER_QTWEBKIT_PLUGINS_AVAILABLE
			case QEvent::ChildAdded:
			case QEvent::ChildRemoved:
				if (!m_canLoadPlugins && m_loadingState == FinishedLoadingState)
				{
					updateAmountOfDeferredPlugins();
				}

				break;
#endif
			case QEvent::ContextMenu:
				{
					const QContextMenuEvent *contextMenuEvent(static_cast<QContextMenuEvent*>(event));

					if (contextMenuEvent && contextMenuEvent->reason() != QContextMenuEvent::Mouse)
					{
						triggerAction(ActionsManager::ContextMenuAction, {{QLatin1String("context"), contextMenuEvent->reason()}});
					}

					if (!getOption(SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption).toBool())
					{
						return true;
					}
				}

				break;
			case QEvent::KeyPress:
				{
					const QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

					if (keyEvent->key() == Qt::Key_Escape && m_page->hasSelection())
					{
						event->ignore();

						return true;
					}
				}

				break;
			case QEvent::MouseButtonDblClick:
			case QEvent::MouseButtonPress:
			case QEvent::Wheel:
				{
					if (mouseEvent)
					{
						if (isScrollBar(mouseEvent->pos()))
						{
							return QObject::eventFilter(object, event);
						}

						setClickPosition(mouseEvent->pos());
						updateHitTestResult(mouseEvent->pos());
					}

					const HitTestResult hitResult(getCurrentHitTestResult());
					QVector<GesturesManager::GesturesContext> contexts;
					contexts.reserve(1);

					if (hitResult.flags.testFlag(HitTestResult::IsContentEditableTest))
					{
						contexts.append(GesturesManager::ContentEditableContext);
					}

					if (hitResult.linkUrl.isValid())
					{
						contexts.append(GesturesManager::LinkContext);
					}

					contexts.append(GesturesManager::GenericContext);

					if (GesturesManager::startGesture(object, event, contexts))
					{
						return true;
					}

					if (event->type() != QEvent::MouseButtonDblClick || mouseEvent->button() != Qt::LeftButton || !SettingsManager::getOption(SettingsManager::Browser_ShowSelectionContextMenuOnDoubleClickOption).toBool())
					{
						return QObject::eventFilter(object, event);
					}

					if (!hitResult.flags.testFlag(HitTestResult::IsContentEditableTest) && hitResult.tagName != QLatin1String("textarea") && hitResult.tagName!= QLatin1String("select") && hitResult.tagName != QLatin1String("input"))
					{
						QTimer::singleShot(250, this, [&]()
						{
							showContextMenu();
						});
					}
				}

				break;
			case QEvent::MouseButtonRelease:
				if (mouseEvent && mouseEvent->button() == Qt::MiddleButton && !isScrollBar(mouseEvent->pos()) && !getCurrentHitTestResult().flags.testFlag(HitTestResult::IsContentEditableTest))
				{
					return true;
				}

				break;
			case QEvent::MouseMove:
				event->ignore();

				return QObject::eventFilter(object, event);
			case QEvent::Move:
			case QEvent::Resize:
				emit geometryChanged();

				break;
			case QEvent::ShortcutOverride:
				{
					const QString tagName(m_page->mainFrame()->findFirstElement(QLatin1String("*:focus")).tagName().toLower());

					if (tagName == QLatin1String("object") || tagName == QLatin1String("embed"))
					{
						event->accept();

						return true;
					}

					const QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

					if (keyEvent->modifiers() == Qt::ControlModifier)
					{
						switch (keyEvent->key())
						{
							case Qt::Key_Backspace:
							case Qt::Key_Left:
							case Qt::Key_Right:
								if (m_page->currentFrame()->hitTestContent(m_page->inputMethodQuery(Qt::ImCursorRectangle).toRect().center()).isContentEditable())
								{
									event->accept();
								}

								break;
							default:
								break;
						}

						return true;
					}

					if ((keyEvent->modifiers().testFlag(Qt::AltModifier) || keyEvent->modifiers().testFlag(Qt::GroupSwitchModifier)) && m_page->currentFrame()->hitTestContent(m_page->inputMethodQuery(Qt::ImCursorRectangle).toRect().center()).isContentEditable())
					{
						event->accept();

						return true;
					}
				}

				break;
			case QEvent::ToolTip:
				handleToolTipEvent(static_cast<QHelpEvent*>(event), m_webView);

				return true;
			default:
				break;
		}
	}

	return QObject::eventFilter(object, event);
}

}
