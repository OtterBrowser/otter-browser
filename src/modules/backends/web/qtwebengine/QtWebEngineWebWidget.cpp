/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "QtWebEngineUrlRequestInterceptor.h"
#include "QtWebEngineWebBackend.h"
#include "../../../../core/Application.h"
#include "../../../../core/Console.h"
#include "../../../../core/GesturesManager.h"
#include "../../../../core/NetworkManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/NotesManager.h"
#include "../../../../core/SearchEnginesManager.h"
#include "../../../../core/ThemesManager.h"
#include "../../../../core/TransfersManager.h"
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
#include <QtCore/QtMath>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QImageWriter>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtWebEngineCore/QWebEngineCookieStore>
#include <QtWebEngineCore/QWebEngineFindTextResult>
#include <QtWebEngineWidgets/QWebEngineHistory>
#include <QtWebEngineWidgets/QWebEngineProfile>
#include <QtWebEngineWidgets/QWebEngineScript>
#include <QtWebEngineWidgets/QWebEngineSettings>
#include <QtWidgets/QAction>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

QtWebEngineInspectorWidget::QtWebEngineInspectorWidget(QWebEnginePage *inspectedPage, QtWebEngineWebWidget *parent) : QWebEngineView(parent),
	m_page(new QWebEnginePage(qobject_cast<QtWebEngineWebBackend*>(parent->getBackend())->getDefaultProfile(), this)),
	m_inspectedPage(inspectedPage)
{
	setMinimumHeight(200);
	setPage(m_page);

	connect(m_page, &QWebEnginePage::windowCloseRequested, this, [&]()
	{
		hide();
	});
}

void QtWebEngineInspectorWidget::showEvent(QShowEvent *event)
{
	if (!m_page->inspectedPage())
	{
		m_page->setInspectedPage(m_inspectedPage);
	}

	QWebEngineView::showEvent(event);
}

QtWebEngineWebWidget::QtWebEngineWebWidget(const QVariantMap &parameters, WebBackend *backend, ContentsWidget *parent) : WebWidget(backend, parent),
	m_webView(nullptr),
	m_inspectorWidget(nullptr),
	m_page(new QtWebEnginePage(SessionsManager::calculateOpenHints(parameters).testFlag(SessionsManager::PrivateOpen), this)),
	m_requestInterceptor(new QtWebEngineUrlRequestInterceptor(this)),
	m_loadingState(FinishedLoadingState),
	m_canGoForwardValue(UnknownValue),
	m_documentLoadingProgress(0),
	m_focusProxyTimer(0),
	m_updateNavigationActionsTimer(0),
	m_isClosing(false),
	m_isEditing(false),
	m_isFullScreen(false),
	m_isTypedIn(false)
{
	setFocusPolicy(Qt::StrongFocus);

	m_page->setUrlRequestInterceptor(m_requestInterceptor);

	connect(m_page, &QtWebEnginePage::loadProgress, m_page, [&](int progress)
	{
		m_documentLoadingProgress = progress;

		emit pageInformationChanged(DocumentLoadingProgressInformation, progress);
	});
	connect(m_page, &QtWebEnginePage::loadStarted, this, &QtWebEngineWebWidget::handleLoadStarted);
	connect(m_page, &QtWebEnginePage::loadFinished, this, &QtWebEngineWebWidget::handleLoadFinished);
	connect(m_page, &QtWebEnginePage::linkHovered, this, &QtWebEngineWebWidget::setStatusMessageOverride);
	connect(m_page, &QtWebEnginePage::iconChanged, this, &QtWebEngineWebWidget::notifyIconChanged);
	connect(m_page, &QtWebEnginePage::requestedPopupWindow, this, &QtWebEngineWebWidget::requestedPopupWindow);
	connect(m_page, &QtWebEnginePage::aboutToNavigate, this, &QtWebEngineWebWidget::aboutToNavigate);
	connect(m_page, &QtWebEnginePage::requestedNewWindow, this, &QtWebEngineWebWidget::requestedNewWindow);
	connect(m_page, &QtWebEnginePage::authenticationRequired, this, &QtWebEngineWebWidget::handleAuthenticationRequired);
	connect(m_page, &QtWebEnginePage::proxyAuthenticationRequired, this, &QtWebEngineWebWidget::handleProxyAuthenticationRequired);
	connect(m_page, &QtWebEnginePage::printRequested, this, &QtWebEngineWebWidget::handlePrintRequest);
	connect(m_page, &QtWebEnginePage::windowCloseRequested, this, &QtWebEngineWebWidget::handleWindowCloseRequest);
	connect(m_page, &QtWebEnginePage::fullScreenRequested, this, &QtWebEngineWebWidget::handleFullScreenRequest);
	connect(m_page, &QtWebEnginePage::featurePermissionRequested, m_page, [&](const QUrl &url, QWebEnginePage::Feature feature)
	{
		notifyPermissionRequested(url, feature, false);
	});
	connect(m_page, &QtWebEnginePage::featurePermissionRequestCanceled, m_page, [&](const QUrl &url, QWebEnginePage::Feature feature)
	{
		notifyPermissionRequested(url, feature, true);
	});
	connect(m_page, &QtWebEnginePage::findTextFinished, m_page, [&](const QWebEngineFindTextResult &result)
	{
		emit findInPageResultsChanged(m_findInPageText, result.numberOfMatches(), result.activeMatch());
	});
	connect(m_page, &QtWebEnginePage::recentlyAudibleChanged, this, &QtWebEngineWebWidget::isAudibleChanged);
	connect(m_page, &QtWebEnginePage::viewingMediaChanged, this, &QtWebEngineWebWidget::notifyNavigationActionsChanged);
	connect(m_page, &QtWebEnginePage::titleChanged, this, &QtWebEngineWebWidget::notifyTitleChanged);
	connect(m_page, &QtWebEnginePage::urlChanged, this, &QtWebEngineWebWidget::notifyUrlChanged);
	connect(m_page, &QtWebEnginePage::renderProcessTerminated, this, &QtWebEngineWebWidget::notifyRenderProcessTerminated);
	connect(m_page->action(QWebEnginePage::Redo), &QAction::changed, this, &QtWebEngineWebWidget::notifyRedoActionStateChanged);
	connect(m_page->action(QWebEnginePage::Undo), &QAction::changed, this, &QtWebEngineWebWidget::notifyUndoActionStateChanged);
	connect(m_page, &QtWebEnginePage::aboutToNavigate, m_requestInterceptor, &QtWebEngineUrlRequestInterceptor::resetStatistics);
	connect(m_requestInterceptor, &QtWebEngineUrlRequestInterceptor::pageInformationChanged, this, &QtWebEngineWebWidget::pageInformationChanged);
	connect(m_requestInterceptor, &QtWebEngineUrlRequestInterceptor::requestBlocked, this, &QtWebEngineWebWidget::requestBlocked);
}

QtWebEngineWebWidget::~QtWebEngineWebWidget()
{
	m_isClosing = true;
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
	else if (event->timerId() == m_updateNavigationActionsTimer)
	{
		killTimer(m_updateNavigationActionsTimer);

		m_updateNavigationActionsTimer = 0;

		emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::NavigationCategory});
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
		m_webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		m_webView->installEventFilter(this);

		QVBoxLayout *layout(new QVBoxLayout(this));
		layout->addWidget(m_webView);
		layout->setContentsMargins(0, 0, 0, 0);

		setLayout(layout);
	}
}

void QtWebEngineWebWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	const QUrl url(extractUrl(parameters));

	switch (identifier)
	{
		case ActionsManager::SaveAction:
			if (m_page->isViewingMedia())
			{
				const SaveInformation information(Utils::getSavePath(suggestSaveFileName(SingleFileSaveFormat)));

				if (information.canSave)
				{
					m_page->save(information.path, QWebEngineDownloadItem::SingleHtmlSaveFormat);
				}
			}
			else
			{
				SaveFormat format(UnknownSaveFormat);
				const QString path(getSavePath({SingleFileSaveFormat, CompletePageSaveFormat, MhtmlSaveFormat, PdfSaveFormat}, &format));

				if (!path.isEmpty())
				{
					switch (format)
					{
						case CompletePageSaveFormat:
							m_page->save(path, QWebEngineDownloadItem::CompleteHtmlSaveFormat);

							break;
						case MhtmlSaveFormat:
							m_page->save(path, QWebEngineDownloadItem::MimeHtmlSaveFormat);

							break;
						case PdfSaveFormat:
							m_page->printToPdf(path);

							break;
						default:
							m_page->save(path, QWebEngineDownloadItem::SingleHtmlSaveFormat);

							break;
					}
				}
			}

			break;
		case ActionsManager::ClearTabHistoryAction:
			setUrl(QUrl(QLatin1String("about:blank")));

			m_page->history()->clear();

			notifyNavigationActionsChanged();

			break;
		case ActionsManager::MuteTabMediaAction:
			m_page->setAudioMuted(!m_page->isAudioMuted());

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

				if (hints == SessionsManager::DefaultOpen && !getCurrentHitTestResult().flags.testFlag(HitTestResult::IsLinkFromSelectionTest))
				{
					m_page->runJavaScript(parsePosition(QStringLiteral("let element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement); if (element) { element.click(); }"), getClickPosition()));

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
				Application::triggerAction(ActionsManager::BookmarkPageAction, {{QLatin1String("url"), url}, {QLatin1String("title"), m_hitResult.title}}, parentWidget(), trigger);
			}

			break;
		case ActionsManager::SaveLinkToDiskAction:
			startTransfer(TransfersManager::startTransfer(url.toString(), {}, (Transfer::CanNotifyOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption))));

			break;
		case ActionsManager::SaveLinkToDownloadsAction:
			TransfersManager::startTransfer(url.toString(), {}, (Transfer::CanNotifyOption | Transfer::CanAskForPathOption | Transfer::IsQuickTransferOption | (isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption)));

			break;
		case ActionsManager::OpenFrameAction:
		case ActionsManager::OpenFrameInCurrentTabAction:
		case ActionsManager::OpenFrameInNewTabAction:
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
			if (m_hitResult.frameUrl.isValid())
			{
				openUrl(m_hitResult.frameUrl, ((identifier == ActionsManager::OpenFrameAction) ? SessionsManager::calculateOpenHints(parameters) : mapOpenActionToOpenHints(identifier)));
			}

			break;
		case ActionsManager::CopyFrameLinkToClipboardAction:
			if (m_hitResult.frameUrl.isValid())
			{
				Application::clipboard()->setText(m_hitResult.frameUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			break;
		case ActionsManager::ReloadFrameAction:
			if (m_hitResult.frameUrl.isValid())
			{
//TODO Add support for subframes
				m_page->runJavaScript(QStringLiteral("let frames = document.querySelectorAll('iframe[src=\"%1\"], frame[src=\"%1\"]'); for (let i = 0; i < frames.length; ++i) { frames[i].contentWindow.location.replace('%1'); }").arg(m_hitResult.frameUrl.toString()));
			}

			break;
		case ActionsManager::ViewFrameSourceAction:
			if (m_hitResult.frameUrl.isValid())
			{
				viewSource(m_hitResult.frameUrl);
			}

			break;
		case ActionsManager::OpenImageAction:
		case ActionsManager::OpenImageInNewTabAction:
		case ActionsManager::OpenImageInNewTabBackgroundAction:
			if (m_hitResult.imageUrl.isValid())
			{
				openUrl(m_hitResult.imageUrl, ((identifier == ActionsManager::OpenImageAction) ? SessionsManager::calculateOpenHints(parameters) : mapOpenActionToOpenHints(identifier)));
			}

			break;
		case ActionsManager::SaveImageToDiskAction:
			if (m_hitResult.imageUrl.isValid())
			{
				if (m_hitResult.imageUrl.scheme() == QLatin1String("data") && m_hitResult.imageUrl.url().contains(QLatin1String(";base64,")))
				{
					const QString imageUrl(m_hitResult.imageUrl.url());
					const QString imageType(imageUrl.mid(11, (imageUrl.indexOf(QLatin1Char(';')) - 11)));
					const QString path(Utils::getSavePath(tr("file") + QLatin1Char('.') + imageType).path);

					if (path.isEmpty())
					{
						break;
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

					TransfersManager::startTransfer(request, {}, (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::IsPrivateOption));
				}
			}

			break;
		case ActionsManager::CopyImageToClipboardAction:
			m_page->triggerAction(QWebEnginePage::CopyImageToClipboard);

			break;
		case ActionsManager::CopyImageUrlToClipboardAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				Application::clipboard()->setText(m_hitResult.imageUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			break;
		case ActionsManager::ReloadImageAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				if (getUrl().matches(m_hitResult.imageUrl, (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash)))
				{
					triggerAction(ActionsManager::ReloadAndBypassCacheAction, {}, trigger);
				}
				else
				{
//TODO Improve
					m_page->runJavaScript(QStringLiteral("let images = document.querySelectorAll('img[src=\"%1\"]'); for (let i = 0; i < images.length; ++i) { images[i].src = ''; images[i].src = \'%1\'; }").arg(m_hitResult.imageUrl.toString()));
				}
			}

			break;
		case ActionsManager::ImagePropertiesAction:
			if (m_hitResult.imageUrl.scheme() == QLatin1String("data"))
			{
				ImagePropertiesDialog *imagePropertiesDialog(new ImagePropertiesDialog(m_hitResult.imageUrl, {{ImagePropertiesDialog::AlternativeTextProperty, m_hitResult.alternateText}, {ImagePropertiesDialog::LongDescriptionProperty, m_hitResult.longDescription}}, nullptr, this));
				imagePropertiesDialog->setButtonsVisible(false);

				ContentsDialog *dialog(new ContentsDialog(ThemesManager::createIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), {}, {}, QDialogButtonBox::Close, imagePropertiesDialog, this));

				connect(this, &QtWebEngineWebWidget::aboutToReload, dialog, &ContentsDialog::close);
				connect(imagePropertiesDialog, &ImagePropertiesDialog::finished, dialog, &ContentsDialog::close);

				showDialog(dialog, false);
			}
			else
			{
				m_page->runJavaScript(parsePosition(QStringLiteral("let element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement); if (element && element.tagName && element.tagName.toLowerCase() == 'img') { [element.naturalWidth, element.naturalHeight]; }"), getClickPosition()), [&](const QVariant &result)
				{
					QMap<ImagePropertiesDialog::ImageProperty, QVariant> properties({{ImagePropertiesDialog::AlternativeTextProperty, m_hitResult.alternateText}, {ImagePropertiesDialog::LongDescriptionProperty, m_hitResult.longDescription}});

					if (result.isValid())
					{
						const QVariantList size(result.toList());

						properties[ImagePropertiesDialog::SizeProperty] = QSize(size[0].toInt(), size[1].toInt());
					}

					ImagePropertiesDialog *imagePropertiesDialog(new ImagePropertiesDialog(m_hitResult.imageUrl, properties, nullptr, this));
					imagePropertiesDialog->setButtonsVisible(false);

					ContentsDialog dialog(ThemesManager::createIcon(QLatin1String("dialog-information")), imagePropertiesDialog->windowTitle(), {}, {}, QDialogButtonBox::Close, imagePropertiesDialog, this);

					connect(this, &QtWebEngineWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

					showDialog(&dialog);
				});
			}

			break;
		case ActionsManager::SaveMediaToDiskAction:
			if (m_hitResult.mediaUrl.isValid())
			{
				QNetworkRequest request(m_hitResult.mediaUrl);
				request.setHeader(QNetworkRequest::UserAgentHeader, m_page->profile()->httpUserAgent());

				TransfersManager::startTransfer(request, {}, (Transfer::CanAskForPathOption | Transfer::CanAutoDeleteOption | Transfer::IsPrivateOption));
			}

			break;
		case ActionsManager::CopyMediaUrlToClipboardAction:
			if (!m_hitResult.mediaUrl.isEmpty())
			{
				Application::clipboard()->setText(m_hitResult.mediaUrl.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
			}

			break;
		case ActionsManager::MediaControlsAction:
			m_page->triggerAction(QWebEnginePage::ToggleMediaControls, parameters.value(QLatin1String("isChecked"), !getActionState(identifier, parameters).isChecked).toBool());

			break;
		case ActionsManager::MediaLoopAction:
			m_page->triggerAction(QWebEnginePage::ToggleMediaLoop, parameters.value(QLatin1String("isChecked"), !getActionState(identifier, parameters).isChecked).toBool());

			break;
		case ActionsManager::MediaPlayPauseAction:
			m_page->triggerAction(QWebEnginePage::ToggleMediaPlayPause);

			break;
		case ActionsManager::MediaMuteAction:
			m_page->triggerAction(QWebEnginePage::ToggleMediaMute);

			break;
		case ActionsManager::MediaPlaybackRateAction:
			m_page->runJavaScript(parsePosition(QStringLiteral("let element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement); if (element) { element.playbackRate = %3; }"), getClickPosition()).arg(parameters.value(QLatin1String("rate"), 1).toReal()));

			break;
		case ActionsManager::GoBackAction:
			m_page->triggerAction(QWebEnginePage::Back);

			break;
		case ActionsManager::GoForwardAction:
			m_page->triggerAction(QWebEnginePage::Forward);

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

			break;
		case ActionsManager::RemoveHistoryIndexAction:
			if (parameters.contains(QLatin1String("index")))
			{
				const int index(parameters[QLatin1String("index")].toInt());

				if (index >= 0 && index < m_page->history()->count())
				{
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
			m_page->triggerAction(QWebEnginePage::Stop);

			break;
		case ActionsManager::ReloadAction:
			emit aboutToReload();

			m_page->triggerAction(QWebEnginePage::Stop);
			m_page->triggerAction(QWebEnginePage::Reload);

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
			m_page->triggerAction(QWebEnginePage::ReloadAndBypassCache);

			break;
		case ActionsManager::ContextMenuAction:
			showContextMenu(getClickPosition());

			break;
		case ActionsManager::UndoAction:
			m_page->triggerAction(QWebEnginePage::Undo);

			break;
		case ActionsManager::RedoAction:
			m_page->triggerAction(QWebEnginePage::Redo);

			break;
		case ActionsManager::CutAction:
			m_page->triggerAction(QWebEnginePage::Cut);

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
				m_page->triggerAction(QWebEnginePage::Copy);
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

				m_page->triggerAction(QWebEnginePage::Paste);

				Application::clipboard()->setMimeData(mimeData);
			}
			else if (parameters.value(QLatin1String("mode")) == QLatin1String("plainText"))
			{
				m_page->triggerAction(QWebEnginePage::PasteAndMatchStyle);
			}
			else
			{
				m_page->triggerAction(QWebEnginePage::Paste);
			}

			break;
		case ActionsManager::DeleteAction:
			m_page->runJavaScript(QLatin1String("window.getSelection().deleteFromDocument()"));

			break;
		case ActionsManager::SelectAllAction:
			m_page->triggerAction(QWebEnginePage::SelectAll);

			break;
		case ActionsManager::UnselectAction:
			m_page->triggerAction(QWebEnginePage::Unselect);

			break;
		case ActionsManager::ClearAllAction:
			triggerAction(ActionsManager::SelectAllAction, {}, trigger);
			triggerAction(ActionsManager::DeleteAction, {}, trigger);

			break;
		case ActionsManager::CreateSearchAction:
			{
				QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/createSearch.js"));

				if (!file.open(QIODevice::ReadOnly))
				{
					break;
				}

				m_page->runJavaScript(parsePosition(QString::fromLatin1(file.readAll()), getClickPosition()), [&](const QVariant &result)
				{
					if (result.isNull())
					{
						return;
					}

					const QIcon icon(getIcon());
					const QUrl url(result.toMap().value(QLatin1String("url")).toString());
					SearchEnginesManager::SearchEngineDefinition searchEngine;
					searchEngine.title = getTitle();
					searchEngine.formUrl = getUrl();
					searchEngine.icon = (icon.isNull() ? ThemesManager::createIcon(QLatin1String("edit-find")) : icon);
					searchEngine.resultsUrl.url = (url.isEmpty() ? getUrl() : (url.isRelative() ? getUrl().resolved(url) : url)).toString();
					searchEngine.resultsUrl.enctype = result.toMap().value(QLatin1String("enctype")).toString();
					searchEngine.resultsUrl.method = result.toMap().value(QLatin1String("method")).toString();
					searchEngine.resultsUrl.parameters = QUrlQuery(result.toMap().value(QLatin1String("query")).toString());

					SearchEnginePropertiesDialog dialog(searchEngine, SearchEnginesManager::getSearchKeywords(), this);

					if (dialog.exec() == QDialog::Accepted)
					{
						SearchEnginesManager::addSearchEngine(dialog.getSearchEngine());
					}
				});

				file.close();
			}

			break;
		case ActionsManager::ScrollToStartAction:
			m_page->runJavaScript(QLatin1String("window.scrollTo(0, 0)"));

			break;
		case ActionsManager::ScrollToEndAction:
			m_page->runJavaScript(QLatin1String("window.scrollTo(0, document.body.scrollHeigh)"));

			break;
		case ActionsManager::ScrollPageUpAction:
			m_page->runJavaScript(QLatin1String("window.scrollByPages(1)"));

			break;
		case ActionsManager::ScrollPageDownAction:
			m_page->runJavaScript(QLatin1String("window.scrollByPages(-1)"));

			break;
		case ActionsManager::ScrollPageLeftAction:
			ensureInitialized();

			m_page->runJavaScript(QStringLiteral("window.scrollBy(-%1, 0)").arg(m_webView->width()));

			break;
		case ActionsManager::ScrollPageRightAction:
			ensureInitialized();

			m_page->runJavaScript(QStringLiteral("window.scrollBy(%1, 0)").arg(m_webView->width()));

			break;
		case ActionsManager::ActivateContentAction:
			ensureInitialized();

			m_webView->setFocus();

			m_page->runJavaScript(QLatin1String("let element = document.activeElement; if (element && element.tagName && (element.tagName.toLowerCase() == 'input' || element.tagName.toLowerCase() == 'textarea'))) { document.activeElement.blur(); }"));

			break;
		case ActionsManager::ViewSourceAction:
			if (canViewSource())
			{
				viewSource(getUrl());
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

			m_page->triggerAction(QWebEnginePage::InspectElement);

			break;
		case ActionsManager::FullScreenAction:
			{
				const MainWindow *mainWindow(MainWindow::findMainWindow(this));

				if (mainWindow && !mainWindow->isFullScreen())
				{
					m_page->triggerAction(QWebEnginePage::ExitFullScreen);
				}
			}

			break;
		case ActionsManager::WebsitePreferencesAction:
			{
				const QUrl pageUrl(getUrl());
				WebsitePreferencesDialog dialog(Utils::extractHost(pageUrl), {}, this);

				if (dialog.exec() == QDialog::Accepted)
				{
					updateOptions(pageUrl);

					QWebEngineCookieStore *cookieStore(m_page->profile()->cookieStore());
					const QVector<QNetworkCookie> cookiesToDelete(dialog.getCookiesToDelete());

					for (int i = 0; i < cookiesToDelete.count(); ++i)
					{
						cookieStore->deleteCookie(cookiesToDelete.at(i));
					}

					const QVector<QNetworkCookie> cookiesToInsert(dialog.getCookiesToInsert());

					for (int i = 0; i < cookiesToInsert.count(); ++i)
					{
						cookieStore->setCookie(cookiesToInsert.at(i));
					}
				}
			}

			break;
		default:
			break;
	}
}

void QtWebEngineWebWidget::clearOptions()
{
	WebWidget::clearOptions();

	updateOptions(getUrl());
}

void QtWebEngineWebWidget::findInPage(const QString &text, FindFlags flags)
{
	m_findInPageText = text;

	if (text.isEmpty())
	{
		m_page->findText(text);

		return;
	}

	QWebEnginePage::FindFlags nativeFlags;

	if (flags.testFlag(BackwardFind))
	{
		nativeFlags |= QWebEnginePage::FindBackward;
	}

	if (flags.testFlag(CaseSensitiveFind))
	{
		nativeFlags |= QWebEnginePage::FindCaseSensitively;
	}

	m_page->findText(text, nativeFlags);
}

void QtWebEngineWebWidget::search(const QString &query, const QString &searchEngine)
{
	const SearchEnginesManager::SearchQuery searchQuery(SearchEnginesManager::setupSearchQuery(query, searchEngine));

	if (searchQuery.isValid())
	{
		setRequestedUrl(searchQuery.request.url(), false, true);
		updateOptions(searchQuery.request.url());

		if (searchQuery.method == QNetworkAccessManager::PostOperation)
		{
			QWebEngineHttpRequest httpRequest(searchQuery.request.url(), QWebEngineHttpRequest::Post);
			httpRequest.setPostData(searchQuery.body);

			m_page->load(httpRequest);
		}
		else
		{
			setUrl(searchQuery.request.url(), false);
		}
	}
}

void QtWebEngineWebWidget::print(QPrinter *printer)
{
	QEventLoop eventLoop;

	m_page->print(printer, [&](bool)
	{
		eventLoop.quit();
	});

	eventLoop.exec();
}

void QtWebEngineWebWidget::viewSource(const QUrl &url)
{
	const QString defaultEncoding(m_page->settings()->defaultTextEncoding());
	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
	request.setHeader(QNetworkRequest::UserAgentHeader, m_page->profile()->httpUserAgent());

	QNetworkReply *reply(NetworkManagerFactory::getNetworkManager(isPrivate())->get(request));
	SourceViewerWebWidget *sourceViewer(new SourceViewerWebWidget(isPrivate()));
	sourceViewer->setRequestedUrl(QUrl(QLatin1String("view-source:") + url.toString()), false);

	if (!defaultEncoding.isEmpty())
	{
		sourceViewer->setOption(SettingsManager::Content_DefaultCharacterEncodingOption, defaultEncoding);
	}

	m_viewSourceReplies[reply] = sourceViewer;

	connect(reply, &QNetworkReply::finished, this, &QtWebEngineWebWidget::handleViewSourceReplyFinished);

	emit requestedNewWindow(sourceViewer, SessionsManager::DefaultOpen, {});
}

void QtWebEngineWebWidget::handleLoadStarted()
{
	m_lastUrlClickTime = {};
	m_metaData.clear();
	m_styleSheets.clear();
	m_feeds.clear();
	m_links.clear();
	m_searchEngines.clear();
	m_watchedChanges.clear();
	m_loadingState = OngoingLoadingState;
	m_documentLoadingProgress = 0;

	setStatusMessage({});
	setStatusMessageOverride({});

	emit geometryChanged();
	emit loadingStateChanged(OngoingLoadingState);
	emit pageInformationChanged(DocumentLoadingProgressInformation, 0);
}

void QtWebEngineWebWidget::handleLoadFinished()
{
	m_loadingState = FinishedLoadingState;
	m_canGoForwardValue = UnknownValue;

	notifyNavigationActionsChanged();
	startReloadTimer();

	QTimer::singleShot(250, this, [&]()
	{
		m_page->runJavaScript(getFastForwardScript(false), [&](const QVariant &result)
		{
			m_canGoForwardValue = (result.toBool() ? TrueValue : FalseValue);

			emit arbitraryActionsStateChanged({ActionsManager::FastForwardAction});
		});

		const QVector<ChangeWatcher> watchers({FeedsWatcher, LinksWatcher, MetaDataWatcher, SearchEnginesWatcher, StylesheetsWatcher});

		for (int i = 0; i < watchers.count(); ++i)
		{
			const ChangeWatcher watcher(watchers.at(i));

			if (isWatchingChanges(watcher))
			{
				updateWatchedData(watcher);
			}
		}
	});

	emit contentStateChanged(getContentState());
	emit loadingStateChanged(FinishedLoadingState);
}

void QtWebEngineWebWidget::handleViewSourceReplyFinished()
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

void QtWebEngineWebWidget::handlePrintRequest()
{
	QPrintPreviewDialog printPreviewDialog(this);
	printPreviewDialog.setWindowFlags(printPreviewDialog.windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
	printPreviewDialog.setWindowTitle(tr("Print Preview"));

	if (Application::getActiveWindow())
	{
		printPreviewDialog.resize(printPreviewDialog.size().expandedTo(Application::getActiveWindow()->size()));
	}

	connect(&printPreviewDialog, &QPrintPreviewDialog::paintRequested, this, [&](QPrinter *printer)
	{
		QEventLoop eventLoop;

		m_page->print(printer, [&](bool)
		{
			eventLoop.quit();
		});

		eventLoop.exec();
	});

	printPreviewDialog.exec();
}

void QtWebEngineWebWidget::handleAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator)
{
	AuthenticationDialog *authenticationDialog(new AuthenticationDialog(url, authenticator, AuthenticationDialog::HttpAuthentication, this));
	authenticationDialog->setButtonsVisible(false);

	ContentsDialog dialog(ThemesManager::createIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), {}, {}, (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, this);

	connect(&dialog, &ContentsDialog::accepted, authenticationDialog, &AuthenticationDialog::accept);
	connect(this, &QtWebEngineWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

	showDialog(&dialog);
}

void QtWebEngineWebWidget::handleProxyAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator, const QString &proxy)
{
	Q_UNUSED(url)

	AuthenticationDialog *authenticationDialog(new AuthenticationDialog(proxy, authenticator, AuthenticationDialog::ProxyAuthentication, this));
	authenticationDialog->setButtonsVisible(false);

	ContentsDialog dialog(ThemesManager::createIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), {}, {}, (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, this);

	connect(&dialog, &ContentsDialog::accepted, authenticationDialog, &AuthenticationDialog::accept);
	connect(this, &QtWebEngineWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

	showDialog(&dialog);
}

void QtWebEngineWebWidget::handleFullScreenRequest(QWebEngineFullScreenRequest request)
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

void QtWebEngineWebWidget::notifyTitleChanged()
{
	emit titleChanged(getTitle());
}

void QtWebEngineWebWidget::notifyUrlChanged()
{
	const QUrl &url(getUrl());

	notifyNavigationActionsChanged();
	updateOptions(url);

	emit iconChanged(getIcon());
	emit urlChanged((url.toString() == QLatin1String("about:blank")) ? m_page->requestedUrl() : url);
	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::PageCategory});

	SessionsManager::markSessionAsModified();
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
		m_loadingState = CrashedLoadingState;

		emit loadingStateChanged(CrashedLoadingState);
	}
}

void QtWebEngineWebWidget::notifyNavigationActionsChanged()
{
	if (m_updateNavigationActionsTimer == 0)
	{
		m_updateNavigationActionsTimer = startTimer(0);
	}
}

void QtWebEngineWebWidget::notifyWatchedDataChanged(ChangeWatcher watcher)
{
	if (m_isClosing)
	{
		return;
	}

	if (watcher >= m_watchedChanges.count())
	{
		m_watchedChanges.resize(watcher + 1);
	}

	m_watchedChanges[watcher] = true;

	emit watchedDataChanged(watcher);
}

void QtWebEngineWebWidget::updateOptions(const QUrl &url)
{
	const QString encoding(getOption(SettingsManager::Content_DefaultCharacterEncodingOption, url).toString());
	QWebEngineSettings *settings(m_page->settings());
	settings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, getOption(SettingsManager::Security_AllowMixedContentOption, url).toBool());
	settings->setAttribute(QWebEngineSettings::AutoLoadImages, (getOption(SettingsManager::Permissions_EnableImagesOption, url).toString() != QLatin1String("onlyCached")));
	settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, getOption(SettingsManager::Network_EnableDnsPrefetchOption, url).toBool());
	settings->setAttribute(QWebEngineSettings::JavascriptEnabled, getOption(SettingsManager::Permissions_EnableJavaScriptOption, url).toBool());
	settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, getOption(SettingsManager::Permissions_ScriptsCanAccessClipboardOption, url).toBool());
	settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, (getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, url).toString() != QLatin1String("blockAll")));
	settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, getOption(SettingsManager::Permissions_EnableLocalStorageOption, url).toBool());
	settings->setAttribute(QWebEngineSettings::ShowScrollBars, getOption(SettingsManager::Interface_ShowScrollBarsOption, url).toBool());
	settings->setAttribute(QWebEngineSettings::WebGLEnabled, getOption(SettingsManager::Permissions_EnableWebglOption, url).toBool());
	settings->setDefaultTextEncoding((encoding == QLatin1String("auto")) ? QString() : encoding);

	disconnect(m_page, &QtWebEnginePage::geometryChangeRequested, this, &QtWebEngineWebWidget::requestedGeometryChange);

	if (getOption(SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption, url).toBool())
	{
		connect(m_page, &QtWebEnginePage::geometryChangeRequested, this, &QtWebEngineWebWidget::requestedGeometryChange);
	}

	m_requestInterceptor->updateOptions(url);
}

void QtWebEngineWebWidget::updateWatchedData(ChangeWatcher watcher)
{
	switch (watcher)
	{
		case FeedsWatcher:
			m_page->runJavaScript(m_page->createScriptSource(QLatin1String("getLinks"), {QLatin1String(R"(a[type=\'application/atom+xml\'], a[type=\'application/rss+xml\'], link[type=\'application/atom+xml\'], link[type=\'application/rss+xml\'])")}), [&](const QVariant &result)
			{
				m_feeds = processLinks(result.toList());

				notifyWatchedDataChanged(FeedsWatcher);
			});

			break;
		case LinksWatcher:
			m_page->runJavaScript(m_page->createScriptSource(QLatin1String("getLinks"), {QLatin1String("a[href]")}), [&](const QVariant &result)
			{
				m_links = processLinks(result.toList());

				notifyWatchedDataChanged(LinksWatcher);
			});

			break;
		case MetaDataWatcher:
			m_page->runJavaScript(QLatin1String("let elements = document.querySelectorAll('meta'); let metaData = []; for (let i = 0; i < elements.length; ++i) { if (elements[i].name !== '') { metaData.push({key: elements[i].name, value: elements[i].content}); } } metaData;"), [&](const QVariant &result)
			{
				QMultiMap<QString, QString> metaData;
				const QVariantList rawMetaData(result.toList());

				for (int i = 0; i < rawMetaData.count(); ++i)
				{
					const QVariantHash entry(rawMetaData.at(i).toHash());

					metaData.insert(entry.value(QLatin1String("key")).toString(), entry.value(QLatin1String("value")).toString());
				}

				m_metaData = metaData;

				notifyWatchedDataChanged(MetaDataWatcher);
			});

			break;
		case SearchEnginesWatcher:
			m_page->runJavaScript(m_page->createScriptSource(QLatin1String("getLinks"), {QLatin1String("link[type=\\'application/opensearchdescription+xml\\']")}), [&](const QVariant &result)
			{
				m_searchEngines = processLinks(result.toList());

				notifyWatchedDataChanged(SearchEnginesWatcher);
			});

			break;
		case StylesheetsWatcher:
			m_page->runJavaScript(m_page->createScriptSource(QLatin1String("getStyleSheets")), [&](const QVariant &result)
			{
				m_styleSheets = result.toStringList();

				notifyWatchedDataChanged(StylesheetsWatcher);
			});

			break;
		default:
			break;
	}
}

void QtWebEngineWebWidget::setScrollPosition(const QPoint &position)
{
	m_page->runJavaScript(QStringLiteral("window.scrollTo(%1, %2); [window.scrollX, window.scrollY];").arg(position.x()).arg(position.y()));
}

void QtWebEngineWebWidget::setHistory(const Session::Window::History &history)
{
	m_page->setHistory(history);

	if (history.entries.isEmpty())
	{
		updateOptions({});
	}
	else
	{
		const QUrl url(m_page->history()->itemAt(history.index).url());

		setRequestedUrl(url, false, true);
		updateOptions(url);

		m_page->triggerAction(QWebEnginePage::Reload);
	}

	notifyNavigationActionsChanged();

	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::PageCategory});
}

void QtWebEngineWebWidget::setHistory(QDataStream &stream)
{
	stream.device()->reset();
	stream >> *(m_page->history());

	const QUrl url(m_page->history()->currentItem().url());

	setRequestedUrl(url, false, true);
	updateOptions(url);
}

void QtWebEngineWebWidget::setZoom(int zoom)
{
	if (zoom != getZoom())
	{
		m_page->setZoomFactor(qBound(0.1, (static_cast<qreal>(zoom) / 100), static_cast<qreal>(100)));

		SessionsManager::markSessionAsModified();

		emit zoomChanged(zoom);
		emit geometryChanged();
	}
}

void QtWebEngineWebWidget::setUrl(const QUrl &url, bool isTypedIn)
{
	if (url.scheme() == QLatin1String("javascript"))
	{
		m_page->runJavaScript(url.toDisplayString(QUrl::RemoveScheme | QUrl::DecodeReserved));
	}
	else if (!url.fragment().isEmpty() && url.matches(getUrl(), (QUrl::RemoveFragment | QUrl::StripTrailingSlash | QUrl::NormalizePathSegments)))
	{
		m_page->runJavaScript(QStringLiteral("let element = document.querySelector('a[name=%1], [id=%1]'); if (element) { let geometry = element.getBoundingClientRect(); [(geometry.left + window.scrollX), (geometry.top + window.scrollY)]; }").arg(url.fragment()), [&](const QVariant &result)
		{
			if (result.isValid())
			{
				setScrollPosition({result.toList().value(0).toInt(), result.toList().value(1).toInt()});
			}
		});
	}
	else
	{
		m_isTypedIn = isTypedIn;

		const QUrl targetUrl(Utils::expandUrl(url));

		updateOptions(targetUrl);

		m_page->load(targetUrl);

		notifyTitleChanged();
		notifyIconChanged();
	}
}

void QtWebEngineWebWidget::setActiveStyleSheet(const QString &styleSheet)
{
	m_page->runJavaScript(QStringLiteral("let elements = document.querySelectorAll('link[rel=\\'alternate stylesheet\\']'); for (let i = 0; i < elements.length; ++i) { elements[i].disabled = (elements[i].title !== '%1'); }").arg(QString(styleSheet).replace(QLatin1Char('\''), QLatin1String("\\'"))));
}

void QtWebEngineWebWidget::setPermission(FeaturePermission feature, const QUrl &url, PermissionPolicies policies)
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

WebWidget* QtWebEngineWebWidget::clone(bool cloneHistory, bool isPrivate, const QStringList &excludedOptions) const
{
	QtWebEngineWebWidget *widget((this->isPrivate() || isPrivate) ? new QtWebEngineWebWidget({{QLatin1String("hints"), SessionsManager::PrivateOpen}}, getBackend()) : new QtWebEngineWebWidget({}, getBackend()));
	widget->setOptions(getOptions(), excludedOptions);

	if (cloneHistory)
	{
		QByteArray byteArray;
		QDataStream stream(&byteArray, QIODevice::ReadWrite);
		stream << *(m_page->history());

		widget->setHistory(stream);
	}

	widget->setZoom(getZoom());

	return widget;
}

QWidget* QtWebEngineWebWidget::getInspector()
{
	if (!m_inspectorWidget)
	{
		m_inspectorWidget = new QtWebEngineInspectorWidget(m_page, this);
	}

	return m_inspectorWidget;
}

QWidget* QtWebEngineWebWidget::getViewport()
{
	return (focusWidget() ? focusWidget() : m_webView);
}

QWebEnginePage* QtWebEngineWebWidget::getPage() const
{
	return m_page;
}

QString QtWebEngineWebWidget::parsePosition(const QString &script, const QPoint &position) const
{
	return script.arg(position.x() / m_page->zoomFactor()).arg(position.y() / m_page->zoomFactor());
}

QString QtWebEngineWebWidget::getTitle() const
{
	const QString title(m_page->title());

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

QString QtWebEngineWebWidget::getDescription() const
{
	return m_page->runScriptSource(QLatin1String(R"(let element = document.querySelector('[name=\'description\']'); let description = (element ? element.content : ''); if (description == '') { element = document.querySelector('[name=\'og:description\']'); description = (element ? element.property : ''); } description;)")).toString();
}

QString QtWebEngineWebWidget::getActiveStyleSheet() const
{
	return m_page->runScriptFile(QLatin1String("getActiveStyleSheet")).toString();
}

QString QtWebEngineWebWidget::getSelectedText() const
{
	return m_page->selectedText();
}

QVariant QtWebEngineWebWidget::getPageInformation(PageInformation key) const
{
	switch (key)
	{
		case DocumentLoadingProgressInformation:
		case TotalLoadingProgressInformation:
			return m_documentLoadingProgress;
		case RequestsBlockedInformation:
		case RequestsStartedInformation:
			return m_requestInterceptor->getPageInformation(key);
		case RequestsFinishedInformation:
			return -1;
		default:
			break;
	}

	return WebWidget::getPageInformation(key);
}

QUrl QtWebEngineWebWidget::getUrl() const
{
	const QUrl url(m_page->url());

	return (Utils::isUrlEmpty(url) ? ((url.toString() == QLatin1String("about:blank#blocked")) ? QUrl(QLatin1String("about:start")) : m_page->requestedUrl()) : url);
}

QIcon QtWebEngineWebWidget::getIcon() const
{
	if (isPrivate())
	{
		return ThemesManager::createIcon(QLatin1String("tab-private"));
	}

	const QIcon icon(m_page->icon());

	return (icon.isNull() ? ThemesManager::createIcon(QLatin1String("tab")) : icon);
}

QPixmap QtWebEngineWebWidget::createThumbnail(const QSize &size)
{
	if (!m_webView)
	{
		return {};
	}

	if ((size.isNull() || size == m_thumbnail.size()) && ((!m_thumbnail.isNull() && qFuzzyCompare(m_thumbnail.devicePixelRatio(), devicePixelRatio())) || m_loadingState == OngoingLoadingState))
	{
		return m_thumbnail;
	}

	const QSize thumbnailSize(size.isValid() ? size : getDefaultThumbnailSize());
	const QSize contentsSize(m_webView->size());
	QPixmap pixmap(m_webView->grab(QRect({0, 0}, contentsSize)));

	if (pixmap.isNull())
	{
		return WebWidget::createThumbnail(size);
	}

	const qreal thumbnailAspectRatio(static_cast<qreal>(thumbnailSize.width()) / thumbnailSize.height());
	const qreal contentsAspectRatio(static_cast<qreal>(contentsSize.width()) / contentsSize.height());

	if (!qFuzzyCompare(thumbnailAspectRatio, contentsAspectRatio))
	{
		if (thumbnailAspectRatio > contentsAspectRatio)
		{
			pixmap = pixmap.copy({{0, 0}, QSize(pixmap.width(), qFloor(pixmap.width() / thumbnailAspectRatio))});
		}
		else
		{
			const int offset((pixmap.width() - qFloor(pixmap.height() * thumbnailAspectRatio)) / 2);

			pixmap = pixmap.copy({{offset, 0}, QSize((pixmap.width() - (offset * 2)), pixmap.height())});
		}
	}

	pixmap = pixmap.scaled((thumbnailSize * devicePixelRatio()), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	pixmap.setDevicePixelRatio(devicePixelRatio());

	if (size.isNull())
	{
		m_thumbnail = pixmap;
	}

	return pixmap;
}

QDateTime QtWebEngineWebWidget::getLastUrlClickTime() const
{
	return m_lastUrlClickTime;
}

QStringList QtWebEngineWebWidget::getBlockedElements() const
{
	return m_requestInterceptor->getBlockedElements();
}

QPoint QtWebEngineWebWidget::getScrollPosition() const
{
	return m_page->scrollPosition().toPoint();
}

WebWidget::LinkUrl QtWebEngineWebWidget::getActiveFrame() const
{
	LinkUrl link;

	if (!m_hitResult.frameUrl.isEmpty())
	{
		link.title = m_hitResult.title;
		link.url = m_hitResult.frameUrl;
	}

	return link;
}

WebWidget::LinkUrl QtWebEngineWebWidget::getActiveImage() const
{
	LinkUrl link;

	if (!m_hitResult.imageUrl.isEmpty())
	{
		link.title = m_hitResult.alternateText;
		link.url = m_hitResult.imageUrl;
	}

	return link;
}

WebWidget::LinkUrl QtWebEngineWebWidget::getActiveLink() const
{
	LinkUrl link;

	if (!m_hitResult.linkUrl.isEmpty())
	{
//TODO Extract text?
		link.title = m_hitResult.title;
		link.url = m_hitResult.linkUrl;
	}

	return link;
}

WebWidget::LinkUrl QtWebEngineWebWidget::getActiveMedia() const
{
	LinkUrl link;

	if (!m_hitResult.mediaUrl.isEmpty())
	{
		link.title = m_hitResult.title;
		link.url = m_hitResult.mediaUrl;
	}

	return link;
}

WebWidget::SslInformation QtWebEngineWebWidget::getSslInformation() const
{
	return m_page->getSslInformation();
}

Session::Window::History QtWebEngineWebWidget::getHistory() const
{
	return m_page->getHistory();
}

WebWidget::HitTestResult QtWebEngineWebWidget::getHitTestResult(const QPoint &position)
{
	const QVariantMap map(m_page->runScriptFile(QLatin1String("hitTest"), {QString::number(position.x() / m_page->zoomFactor()), QString::number(position.y() / m_page->zoomFactor())}).toMap());
	const QVariantMap geometryMap(map.value(QLatin1String("geometry")).toMap());

	m_hitResult = {};
	m_hitResult.title = map.value(QLatin1String("title")).toString();
	m_hitResult.tagName = map.value(QLatin1String("tagName")).toString();
	m_hitResult.alternateText = map.value(QLatin1String("alternateText")).toString();
	m_hitResult.longDescription = map.value(QLatin1String("longDescription")).toString();
	m_hitResult.formUrl = QUrl(map.value(QLatin1String("formUrl")).toString());
	m_hitResult.frameUrl = QUrl(map.value(QLatin1String("frameUrl")).toString());
	m_hitResult.imageUrl = QUrl(map.value(QLatin1String("imageUrl")).toString());
	m_hitResult.linkUrl = QUrl(map.value(QLatin1String("linkUrl")).toString());
	m_hitResult.mediaUrl = QUrl(map.value(QLatin1String("mediaUrl")).toString());
	m_hitResult.elementGeometry = {geometryMap.value(QLatin1String("x")).toInt(), geometryMap.value(QLatin1String("y")).toInt(), geometryMap.value(QLatin1String("w")).toInt(), geometryMap.value(QLatin1String("h")).toInt()};
	m_hitResult.hitPosition = map.value(QLatin1String("position")).toPoint();
	m_hitResult.playbackRate = map.value(QLatin1String("playbackRate")).toReal();
	m_hitResult.flags = static_cast<HitTestResult::HitTestFlags>(map.value(QLatin1String("flags")).toInt());

	if (m_hitResult.flags.testFlag(HitTestResult::IsSelectedTest) && !m_hitResult.linkUrl.isValid() && Utils::isUrl(m_page->selectedText()))
	{
		m_hitResult.flags |= HitTestResult::IsLinkFromSelectionTest;
		m_hitResult.linkUrl = QUrl::fromUserInput(m_page->selectedText());
	}

	return m_hitResult;
}

QStringList QtWebEngineWebWidget::getStyleSheets() const
{
	return m_styleSheets;
}

QVector<WebWidget::LinkUrl> QtWebEngineWebWidget::processLinks(const QVariantList &rawLinks) const
{
	QVector<LinkUrl> links;
	links.reserve(rawLinks.count());

	for (int i = 0; i < rawLinks.count(); ++i)
	{
		const QVariantHash rawLink(rawLinks.at(i).toHash());
		LinkUrl link;
		link.title = rawLink.value(QLatin1String("title")).toString();
		link.mimeType = rawLink.value(QLatin1String("mimeType")).toString();
		link.url = QUrl(rawLink.value(QLatin1String("url")).toString());

		links.append(link);
	}

	return links;
}

QVector<WebWidget::LinkUrl> QtWebEngineWebWidget::getFeeds() const
{
	return m_feeds;
}

QVector<WebWidget::LinkUrl> QtWebEngineWebWidget::getLinks() const
{
	return m_links;
}

QVector<WebWidget::LinkUrl> QtWebEngineWebWidget::getSearchEngines() const
{
	return m_searchEngines;
}

QVector<NetworkManager::ResourceInformation> QtWebEngineWebWidget::getBlockedRequests() const
{
	return m_requestInterceptor->getBlockedRequests();
}

QMultiMap<QString, QString> QtWebEngineWebWidget::getMetaData() const
{
	return m_metaData;
}

WebWidget::LoadingState QtWebEngineWebWidget::getLoadingState() const
{
	return m_loadingState;
}

int QtWebEngineWebWidget::getZoom() const
{
	return static_cast<int>(m_page->zoomFactor() * 100);
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
	return (m_canGoForwardValue == TrueValue || canGoForward());
}

bool QtWebEngineWebWidget::canInspect() const
{
	return !Utils::isUrlEmpty(getUrl());
}

bool QtWebEngineWebWidget::canRedo() const
{
	return m_page->action(QWebEnginePage::Redo)->isEnabled();
}

bool QtWebEngineWebWidget::canUndo() const
{
	return m_page->action(QWebEnginePage::Undo)->isEnabled();
}

bool QtWebEngineWebWidget::canShowContextMenu(const QPoint &position) const
{
	Q_UNUSED(position)

	return true;
}

bool QtWebEngineWebWidget::canViewSource() const
{
	return (!m_page->isViewingMedia() && !Utils::isUrlEmpty(getUrl()));
}

bool QtWebEngineWebWidget::hasSelection() const
{
	return (m_page->hasSelection() && !m_page->selectedText().isEmpty());
}

bool QtWebEngineWebWidget::hasWatchedChanges(WebWidget::ChangeWatcher watcher) const
{
	return m_watchedChanges.value(watcher, false);
}

bool QtWebEngineWebWidget::isAudible() const
{
	return m_page->recentlyAudible();
}

bool QtWebEngineWebWidget::isAudioMuted() const
{
	return m_page->isAudioMuted();
}

bool QtWebEngineWebWidget::isFullScreen() const
{
	return m_isFullScreen;
}

bool QtWebEngineWebWidget::isInspecting() const
{
	return (m_inspectorWidget && m_inspectorWidget->isVisible());
}

bool QtWebEngineWebWidget::isPopup() const
{
	return m_page->isPopup();
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

bool QtWebEngineWebWidget::isTypedIn() const
{
	return m_isTypedIn;
}

bool QtWebEngineWebWidget::eventFilter(QObject *object, QEvent *event)
{
	switch (event->type())
	{
		case QEvent::ChildAdded:
			if (object == m_webView)
			{
				const QChildEvent *childEvent(static_cast<QChildEvent*>(event));

				if (childEvent->child())
				{
					childEvent->child()->installEventFilter(this);
				}
			}

			break;
		case QEvent::ContextMenu:
			if (object == m_webView)
			{
				const QContextMenuEvent *contextMenuEvent(static_cast<QContextMenuEvent*>(event));

				if (contextMenuEvent && contextMenuEvent->reason() != QContextMenuEvent::Mouse)
				{
					triggerAction(ActionsManager::ContextMenuAction, {{QLatin1String("context"), contextMenuEvent->reason()}});
				}
			}

			break;
		case QEvent::MouseButtonDblClick:
		case QEvent::MouseButtonPress:
		case QEvent::Wheel:
			{
				QMouseEvent *mouseEvent(nullptr);

				if (event->type() != QEvent::Wheel)
				{
					mouseEvent = static_cast<QMouseEvent*>(event);

					if (mouseEvent)
					{
						setClickPosition(mouseEvent->pos());
						updateHitTestResult(mouseEvent->pos());

						if (mouseEvent->button() == Qt::LeftButton && !getCurrentHitTestResult().linkUrl.isEmpty())
						{
							m_lastUrlClickTime = QDateTime::currentDateTimeUtc();
						}
					}
				}

				QVector<GesturesManager::GesturesContext> contexts;
				contexts.reserve(1);

				if (getCurrentHitTestResult().flags.testFlag(HitTestResult::IsContentEditableTest))
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
					const HitTestResult hitResult(getHitTestResult(mouseEvent->pos()));

					if (!hitResult.flags.testFlag(HitTestResult::IsContentEditableTest) && hitResult.tagName != QLatin1String("textarea") && hitResult.tagName!= QLatin1String("select") && hitResult.tagName != QLatin1String("input"))
					{
						setClickPosition(mouseEvent->pos());

						QTimer::singleShot(250, this, [&]()
						{
							showContextMenu();
						});
					}
				}
			}

			break;
		case QEvent::Move:
		case QEvent::Resize:
			if (object == m_webView)
			{
				emit geometryChanged();
			}

			break;
		case QEvent::ShortcutOverride:
			m_isEditing = m_page->runScriptSource(QLatin1String("let element = document.body.querySelector(':focus'); let tagName = (element ? element.tagName.toLowerCase() : ''); let result = false; if (tagName == 'textarea' || tagName == 'input') { let type = (element.type ? element.type.toLowerCase() : ''); if ((type == '' || tagName == 'textarea' || type == 'text' || type == 'search') && !element.hasAttribute('readonly') && !element.hasAttribute('disabled')) { result = true; } } result;")).toBool();

			if (m_isEditing)
			{
				event->accept();

				return true;
			}

			break;
		case QEvent::ToolTip:
			handleToolTipEvent(static_cast<QHelpEvent*>(event), m_webView);

			return true;
		default:
			break;
	}

	return QObject::eventFilter(object, event);
}

}
