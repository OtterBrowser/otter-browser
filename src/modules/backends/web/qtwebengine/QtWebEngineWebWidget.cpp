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
#include "../../../../core/HistoryManager.h"
#include "../../../../core/InputInterpreter.h"
#include "../../../../core/SearchesManager.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/AuthenticationDialog.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/ContentsWidget.h"
#include "../../../../ui/WebsitePreferencesDialog.h"

#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtWebEngineWidgets/QWebEngineHistory>
#include <QtWebEngineWidgets/QWebEngineSettings>
#include <QtWidgets/QApplication>
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
	m_ignoreContextMenu(false),
	m_isLoading(false),
	m_isTyped(false)
{
	m_webView->setPage(new QtWebEnginePage(this));
	m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_webView->installEventFilter(this);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_webView);
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);
	setFocusPolicy(Qt::StrongFocus);

	connect(m_webView, SIGNAL(titleChanged(QString)), this, SLOT(notifyTitleChanged()));
	connect(m_webView, SIGNAL(urlChanged(QUrl)), this, SLOT(notifyUrlChanged(QUrl)));
	connect(m_webView, SIGNAL(iconUrlChanged(QUrl)), this, SLOT(notifyIconChanged()));
	connect(m_webView->page(), SIGNAL(loadProgress(int)), this, SIGNAL(loadProgress(int)));
	connect(m_webView->page(), SIGNAL(loadStarted()), this, SLOT(pageLoadStarted()));
	connect(m_webView->page(), SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
	connect(m_webView->page(), SIGNAL(linkHovered(QString)), this, SLOT(linkHovered(QString)));
	connect(m_webView->page(), SIGNAL(authenticationRequired(QUrl,QAuthenticator*)), this, SLOT(handleAuthenticationRequired(QUrl,QAuthenticator*)));
	connect(m_webView->page(), SIGNAL(proxyAuthenticationRequired(QUrl,QAuthenticator*,QString)), this, SLOT(handleProxyAuthenticationRequired(QUrl,QAuthenticator*,QString)));
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
	notifyUrlChanged(getUrl());

	emit loadingChanged(false);
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

void QtWebEngineWebWidget::showDialog(ContentsDialog *dialog)
{
	ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());

	if (parent)
	{
		parent->showDialog(dialog);
	}
}

void QtWebEngineWebWidget::hideDialog(ContentsDialog *dialog)
{
	ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());

	if (parent)
	{
		parent->hideDialog(dialog);
	}
}

void QtWebEngineWebWidget::goToHistoryIndex(int index)
{
	m_webView->history()->goToItem(m_webView->history()->itemAt(index));
}

void QtWebEngineWebWidget::triggerAction(int identifier, bool checked)
{
	Q_UNUSED(checked)

	switch (identifier)
	{
		case Action::OpenSelectionAsLinkAction:
			{
				const QString text(m_webView->selectedText());

				if (!text.isEmpty())
				{
					InputInterpreter *interpreter = new InputInterpreter(this);

					connect(interpreter, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SIGNAL(requestedOpenUrl(QUrl,OpenHints)));
					connect(interpreter, SIGNAL(requestedSearch(QString,QString,OpenHints)), this, SIGNAL(requestedSearch(QString,QString,OpenHints)));

					interpreter->interpret(text, DefaultOpen, true);
				}
			}

			break;
		case Action::GoBackAction:
			m_webView->page()->triggerAction(QWebEnginePage::Back);

			break;
		case Action::GoForwardAction:
			m_webView->page()->triggerAction(QWebEnginePage::Forward);

			break;
		case Action::RewindAction:
			m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(0));

			break;
		case Action::FastForwardAction:
			m_webView->page()->history()->goToItem(m_webView->page()->history()->itemAt(m_webView->page()->history()->count() - 1));

			break;
		case Action::StopAction:
			m_webView->page()->triggerAction(QWebEnginePage::Stop);

			break;
		case Action::ReloadAction:
			emit aboutToReload();

			m_webView->page()->triggerAction(QWebEnginePage::Stop);
			m_webView->page()->triggerAction(QWebEnginePage::Reload);

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
			m_webView->page()->triggerAction(QWebEnginePage::ReloadAndBypassCache);

			break;
		case Action::UndoAction:
			m_webView->page()->triggerAction(QWebEnginePage::Undo);

			break;
		case Action::RedoAction:
			m_webView->page()->triggerAction(QWebEnginePage::Redo);

			break;
		case Action::CutAction:
			m_webView->page()->triggerAction(QWebEnginePage::Cut);

			break;
		case Action::CopyAction:
			m_webView->page()->triggerAction(QWebEnginePage::Copy);

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
			m_webView->page()->triggerAction(QWebEnginePage::Paste);

			break;
		case Action::SelectAllAction:
			m_webView->page()->triggerAction(QWebEnginePage::SelectAll);

			break;
		case Action::SearchAction:
			quickSearch(getAction(Action::SearchAction));

			break;
		case Action::AddBookmarkAction:
			emit requestedAddBookmark(getUrl(), getTitle());

			break;
		case Action::WebsitePreferencesAction:
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

void QtWebEngineWebWidget::handleAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator)
{
	AuthenticationDialog *authenticationDialog = new AuthenticationDialog(url, authenticator, this);
	authenticationDialog->setButtonsVisible(false);

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, this);

	connect(&dialog, SIGNAL(accepted()), authenticationDialog, SLOT(accept()));

	QEventLoop eventLoop;

	showDialog(&dialog);

	connect(&dialog, SIGNAL(closed(bool,QDialogButtonBox::StandardButton)), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

	eventLoop.exec();

	hideDialog(&dialog);
}

void QtWebEngineWebWidget::handleProxyAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator, const QString &proxy)
{
	Q_UNUSED(url)

	AuthenticationDialog *authenticationDialog = new AuthenticationDialog(proxy, authenticator, this);
	authenticationDialog->setButtonsVisible(false);

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-password")), authenticationDialog->windowTitle(), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), authenticationDialog, this);

	connect(&dialog, SIGNAL(accepted()), authenticationDialog, SLOT(accept()));

	QEventLoop eventLoop;

	showDialog(&dialog);

	connect(&dialog, SIGNAL(closed(bool,QDialogButtonBox::StandardButton)), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

	eventLoop.exec();

	hideDialog(&dialog);
}

void QtWebEngineWebWidget::handleContextMenu(const QVariant &result)
{
	m_hitResult = HitTestResult(result);

	if (m_ignoreContextMenu || (m_webView->selectedText().isEmpty() && m_clickPosition.isNull()))
	{
		return;
	}

	updateEditActions();

	MenuFlags flags = NoMenu;

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
		qDebug() << "image";
		flags |= ImageMenu;
	}

	if (m_hitResult.mediaUrl.isValid())
	{
		flags |= MediaMenu;
	}

	if (m_hitResult.isContentEditable)
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
qDebug() << flags;
	WebWidget::showContextMenu(m_clickPosition, flags);
}

void QtWebEngineWebWidget::handleHitTest(const QVariant &result)
{
	m_hitResult = HitTestResult(result);
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

	emit urlChanged(url);

	SessionsManager::markSessionModified();
}

void QtWebEngineWebWidget::notifyIconChanged()
{
	emit iconChanged(getIcon());
}

void QtWebEngineWebWidget::updatePageActions(const QUrl &url)
{
	if (m_actions.contains(Action::AddBookmarkAction))
	{
		m_actions[Action::AddBookmarkAction]->setOverrideText(HistoryManager::hasUrl(url) ? QT_TRANSLATE_NOOP("actions", "Edit Bookmark...") : QT_TRANSLATE_NOOP("actions", "Add Bookmark..."));
	}
}

void QtWebEngineWebWidget::updateNavigationActions()
{
	if (m_actions.contains(Action::GoBackAction))
	{
		m_actions[Action::GoBackAction]->setEnabled(m_webView->history()->canGoBack());
	}

	if (m_actions.contains(Action::GoForwardAction))
	{
		m_actions[Action::GoForwardAction]->setEnabled(m_webView->history()->canGoForward());
	}

	if (m_actions.contains(Action::RewindAction))
	{
		m_actions[Action::RewindAction]->setEnabled(m_webView->history()->canGoBack());
	}

	if (m_actions.contains(Action::FastForwardAction))
	{
		m_actions[Action::FastForwardAction]->setEnabled(m_webView->history()->canGoForward());
	}

	if (m_actions.contains(Action::StopAction))
	{
		m_actions[Action::StopAction]->setEnabled(m_isLoading);
	}

	if (m_actions.contains(Action::ReloadAction))
	{
		m_actions[Action::ReloadAction]->setEnabled(!m_isLoading);
	}

	if (m_actions.contains(Action::ReloadOrStopAction))
	{
		m_actions[Action::ReloadOrStopAction]->setup(ActionsManager::getAction((m_isLoading ? Action::StopAction : Action::ReloadAction), this));
	}

	if (m_actions.contains(Action::LoadPluginsAction))
	{
		m_actions[Action::LoadPluginsAction]->setEnabled(false);
	}
}

void QtWebEngineWebWidget::updateEditActions()
{
	if (m_actions.contains(Action::CutAction))
	{
		m_actions[Action::CutAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Cut)->isEnabled());
	}

	if (m_actions.contains(Action::CopyAction))
	{
		m_actions[Action::CopyAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Copy)->isEnabled());
	}

	if (m_actions.contains(Action::CopyPlainTextAction))
	{
		m_actions[Action::CopyPlainTextAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Copy)->isEnabled());
	}

	if (m_actions.contains(Action::PasteAction))
	{
		m_actions[Action::PasteAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Paste)->isEnabled());
	}

	if (m_actions.contains(Action::PasteAndGoAction))
	{
		m_actions[Action::PasteAndGoAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Paste)->isEnabled());
	}

	if (m_actions.contains(Action::DeleteAction))
	{
		m_actions[Action::DeleteAction]->setEnabled(m_webView->hasSelection() && m_hitResult.isContentEditable);
	}

	if (m_actions.contains(Action::ClearAllAction))
	{
		m_actions[Action::ClearAllAction]->setEnabled(m_webView->hasSelection());
	}

	if (m_actions.contains(Action::SearchAction))
	{
		SearchInformation *engine = SearchesManager::getSearchEngine(getOption(QLatin1String("Search/DefaultQuickSearchEngine")).toString());

		m_actions[Action::SearchAction]->setEnabled(engine != NULL);
		m_actions[Action::SearchAction]->setIcon((!engine || engine->icon.isNull()) ? Utils::getIcon(QLatin1String("edit-find")) : engine->icon);
		m_actions[Action::SearchAction]->setOverrideText(engine ? engine->title : QT_TRANSLATE_NOOP("actions", "Search"));
		m_actions[Action::SearchAction]->setToolTip(engine ? engine->description : tr("No search engines defined"));
	}

	if (m_actions.contains(Action::SearchMenuAction))
	{
		m_actions[Action::SearchMenuAction]->setEnabled(SearchesManager::getSearchEngines().count() > 1);
	}

	updateLinkActions();
	updateFrameActions();
	updateImageActions();
	updateMediaActions();
}

void QtWebEngineWebWidget::updateLinkActions()
{
	const bool isLink = m_hitResult.linkUrl.isValid();

	if (m_actions.contains(Action::OpenLinkAction))
	{
		m_actions[Action::OpenLinkAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::OpenLinkInCurrentTabAction))
	{
		m_actions[Action::OpenLinkInCurrentTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::OpenLinkInNewTabAction))
	{
		m_actions[Action::OpenLinkInNewTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::OpenLinkInNewTabBackgroundAction))
	{
		m_actions[Action::OpenLinkInNewTabBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::OpenLinkInNewWindowAction))
	{
		m_actions[Action::OpenLinkInNewWindowAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::OpenLinkInNewWindowBackgroundAction))
	{
		m_actions[Action::OpenLinkInNewWindowBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::CopyLinkToClipboardAction))
	{
		m_actions[Action::CopyLinkToClipboardAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::BookmarkLinkAction))
	{
		m_actions[Action::BookmarkLinkAction]->setOverrideText(HistoryManager::hasUrl(m_hitResult.linkUrl) ? QT_TRANSLATE_NOOP("actions", "Edit Link Bookmark...") : QT_TRANSLATE_NOOP("actions", "Bookmark Link..."));
		m_actions[Action::BookmarkLinkAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::SaveLinkToDiskAction))
	{
		m_actions[Action::SaveLinkToDiskAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::SaveLinkToDownloadsAction))
	{
		m_actions[Action::SaveLinkToDownloadsAction]->setEnabled(isLink);
	}
}

void QtWebEngineWebWidget::updateFrameActions()
{
	const bool isFrame = m_hitResult.frameUrl.isValid();

	if (m_actions.contains(Action::OpenFrameInCurrentTabAction))
	{
		m_actions[Action::OpenFrameInCurrentTabAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(Action::OpenFrameInNewTabAction))
	{
		m_actions[Action::OpenFrameInNewTabAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(Action::OpenFrameInNewTabBackgroundAction))
	{
		m_actions[Action::OpenFrameInNewTabBackgroundAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(Action::CopyFrameLinkToClipboardAction))
	{
		m_actions[Action::CopyFrameLinkToClipboardAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(Action::ReloadFrameAction))
	{
		m_actions[Action::ReloadFrameAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(Action::ViewFrameSourceAction))
	{
		m_actions[Action::ViewFrameSourceAction]->setEnabled(false);
	}
}

void QtWebEngineWebWidget::updateImageActions()
{
	const bool isImage = !m_hitResult.imageUrl.isEmpty();
	const bool isOpened = getUrl().matches(m_hitResult.imageUrl, (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash));
	const QString fileName = fontMetrics().elidedText(m_hitResult.imageUrl.fileName(), Qt::ElideMiddle, 256);

	if (m_actions.contains(Action::OpenImageInNewTabAction))
	{
		m_actions[Action::OpenImageInNewTabAction]->setOverrideText(isImage ? (fileName.isEmpty() || m_hitResult.imageUrl.scheme() == QLatin1String("data")) ? tr("Open Image (Untitled)") : tr("Open Image (%1)").arg(fileName) : QT_TRANSLATE_NOOP("actions", "Open Image"));
		m_actions[Action::OpenImageInNewTabAction]->setEnabled(isImage && !isOpened);
	}

	if (m_actions.contains(Action::SaveImageToDiskAction))
	{
		m_actions[Action::SaveImageToDiskAction]->setEnabled(isImage);
	}

	if (m_actions.contains(Action::CopyImageToClipboardAction))
	{
		m_actions[Action::CopyImageToClipboardAction]->setEnabled(isImage);
	}

	if (m_actions.contains(Action::CopyImageUrlToClipboardAction))
	{
		m_actions[Action::CopyImageUrlToClipboardAction]->setEnabled(isImage);
	}

	if (m_actions.contains(Action::ReloadImageAction))
	{
		m_actions[Action::ReloadImageAction]->setEnabled(isImage);
	}

	if (m_actions.contains(Action::ImagePropertiesAction))
	{
		m_actions[Action::ImagePropertiesAction]->setEnabled(isImage);
	}
}

void QtWebEngineWebWidget::updateMediaActions()
{
	const bool isMedia = m_hitResult.mediaUrl.isValid();
	const bool isVideo = (m_hitResult.tagName == QLatin1String("video"));

	if (m_actions.contains(Action::SaveMediaToDiskAction))
	{
		m_actions[Action::SaveMediaToDiskAction]->setOverrideText(isVideo ? QT_TRANSLATE_NOOP("actions", "Save Video...") : QT_TRANSLATE_NOOP("actions", "Save Audio..."));
		m_actions[Action::SaveMediaToDiskAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(Action::CopyMediaUrlToClipboardAction))
	{
		m_actions[Action::CopyMediaUrlToClipboardAction]->setOverrideText(isVideo ? QT_TRANSLATE_NOOP("actions", "Copy Video Link to Clipboard") : QT_TRANSLATE_NOOP("actions", "Copy Audio Link to Clipboard"));
		m_actions[Action::CopyMediaUrlToClipboardAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(Action::MediaControlsAction))
	{
		m_actions[Action::MediaControlsAction]->setChecked(m_hitResult.hasControls);
		m_actions[Action::MediaControlsAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(Action::MediaLoopAction))
	{
		m_actions[Action::MediaLoopAction]->setChecked(m_hitResult.isLooped);
		m_actions[Action::MediaLoopAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(Action::MediaPlayPauseAction))
	{
		m_actions[Action::MediaPlayPauseAction]->setOverrideText(m_hitResult.isPaused ? QT_TRANSLATE_NOOP("actions", "Play") : QT_TRANSLATE_NOOP("actions", "Pause"));
		m_actions[Action::MediaPlayPauseAction]->setIcon(Utils::getIcon(m_hitResult.isPaused ? QLatin1String("media-playback-start") : QLatin1String("media-playback-pause")));
		m_actions[Action::MediaPlayPauseAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(Action::MediaMuteAction))
	{
		m_actions[Action::MediaMuteAction]->setOverrideText(m_hitResult.isMuted ? QT_TRANSLATE_NOOP("actions", "Unmute") : QT_TRANSLATE_NOOP("actions", "Mute"));
		m_actions[Action::MediaMuteAction]->setIcon(Utils::getIcon(m_hitResult.isMuted ? QLatin1String("audio-volume-medium") : QLatin1String("audio-volume-muted")));
		m_actions[Action::MediaMuteAction]->setEnabled(isMedia);
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
}

void QtWebEngineWebWidget::setOptions(const QVariantHash &options)
{
	WebWidget::setOptions(options);

	updateOptions(getUrl());
}

void QtWebEngineWebWidget::setHistory(const WindowHistoryInformation &history)
{
	Q_UNUSED(history)

	if (history.index >= 0 && history.index < history.entries.count())
	{
		setUrl(history.entries[history.index].url, false);
	}
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
///FIXME
//		m_webView->page()->scrollToAnchor(url.fragment());

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

WebWidget* QtWebEngineWebWidget::clone(bool cloneHistory)
{
	Q_UNUSED(cloneHistory)

	QtWebEngineWebWidget *widget = new QtWebEngineWebWidget(isPrivate(), getBackend());
	widget->setOptions(getOptions());
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

	switch (identifier)
	{
		case Action::OpenSelectionAsLinkAction:
		case Action::GoBackAction:
		case Action::GoForwardAction:
		case Action::RewindAction:
		case Action::FastForwardAction:
		case Action::StopAction:
		case Action::ReloadAction:
		case Action::ReloadOrStopAction:
		case Action::ReloadAndBypassCacheAction:
		case Action::UndoAction:
		case Action::RedoAction:
		case Action::CutAction:
		case Action::CopyAction:
		case Action::CopyPlainTextAction:
		case Action::CopyAddressAction:
		case Action::PasteAction:
		case Action::SelectAllAction:
		case Action::SearchAction:
		case Action::AddBookmarkAction:
		case Action::WebsitePreferencesAction:
			{
				Action *action = new Action(identifier, this);

				m_actions[identifier] = action;

				connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

				updateNavigationActions();

				return action;
			}
		default:
			return NULL;
	}

	return NULL;
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
	return false;
}

bool QtWebEngineWebWidget::find(const QString &text, FindFlags flags)
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
	if (event->type() == QEvent::ContextMenu)
	{
		QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent*>(event);

		QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
		file.open(QIODevice::ReadOnly);

		m_clickPosition = contextMenuEvent->pos();

		m_webView->page()->runJavaScript(QString(file.readAll()).arg(contextMenuEvent->pos().x() / m_webView->zoomFactor()).arg(contextMenuEvent->pos().y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleContextMenu));

		file.close();

		return true;
	}

	return QObject::eventFilter(object, event);
}

}
