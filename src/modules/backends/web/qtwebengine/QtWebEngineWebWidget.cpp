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
#include "../../../windows/web/ImagePropertiesDialog.h"
#include "../../../../core/BookmarksManager.h"
#include "../../../../core/Console.h"
#include "../../../../core/GesturesManager.h"
#include "../../../../core/HistoryManager.h"
#include "../../../../core/InputInterpreter.h"
#include "../../../../core/NotesManager.h"
#include "../../../../core/SearchesManager.h"
#include "../../../../core/Transfer.h"
#include "../../../../core/TransfersManager.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/AuthenticationDialog.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/ContentsWidget.h"
#include "../../../../ui/SearchPropertiesDialog.h"
#include "../../../../ui/WebsitePreferencesDialog.h"

#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QImageWriter>
#include <QtWebEngineWidgets/QWebEngineHistory>
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
	m_childWidget(NULL),
	m_ignoreContextMenu(false),
	m_ignoreContextMenuNextTime(false),
	m_isUsingRockerNavigation(false),
	m_isLoading(false),
	m_isTyped(false)
{
	m_webView->setPage(new QtWebEnginePage(this));
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

	connect(m_webView, SIGNAL(titleChanged(QString)), this, SLOT(notifyTitleChanged()));
	connect(m_webView, SIGNAL(urlChanged(QUrl)), this, SLOT(notifyUrlChanged(QUrl)));
	connect(m_webView, SIGNAL(iconUrlChanged(QUrl)), this, SLOT(notifyIconChanged()));
	connect(m_webView->page(), SIGNAL(loadProgress(int)), this, SIGNAL(loadProgress(int)));
	connect(m_webView->page(), SIGNAL(loadStarted()), this, SLOT(pageLoadStarted()));
	connect(m_webView->page(), SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
	connect(m_webView->page(), SIGNAL(linkHovered(QString)), this, SLOT(linkHovered(QString)));
	connect(m_webView->page(), SIGNAL(authenticationRequired(QUrl,QAuthenticator*)), this, SLOT(handleAuthenticationRequired(QUrl,QAuthenticator*)));
	connect(m_webView->page(), SIGNAL(proxyAuthenticationRequired(QUrl,QAuthenticator*,QString)), this, SLOT(handleProxyAuthenticationRequired(QUrl,QAuthenticator*,QString)));
	connect(m_webView->page(), SIGNAL(windowCloseRequested()), this, SLOT(handleWindowCloseRequest()));
	connect(m_webView->page(), SIGNAL(featurePermissionRequested(QUrl,QWebEnginePage::Feature)), this, SLOT(handlePermissionRequest(QUrl,QWebEnginePage::Feature)));
	connect(m_webView->page(), SIGNAL(featurePermissionRequestCanceled(QUrl,QWebEnginePage::Feature)), this, SLOT(handlePermissionCancel(QUrl,QWebEnginePage::Feature)));
}

void QtWebEngineWebWidget::focusInEvent(QFocusEvent *event)
{
	WebWidget::focusInEvent(event);

	m_webView->setFocus();
}

void QtWebEngineWebWidget::mousePressEvent(QMouseEvent *event)
{
	WebWidget::mousePressEvent(event);

	if (getScrollMode() == MoveScroll)
	{
		triggerAction(Action::EndScrollAction);

		m_ignoreContextMenuNextTime = true;
	}
}

void QtWebEngineWebWidget::search(const QString &query, const QString &engine)
{
	QNetworkRequest request;
	QNetworkAccessManager::Operation method;
	QByteArray body;

	if (SearchesManager::setupSearchQuery(query, engine, &request, &method, &body))
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
	notifyUrlChanged(getUrl());
	startReloadTimer();

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

void QtWebEngineWebWidget::clearSelection()
{
	m_webView->page()->runJavaScript(QLatin1String("window.getSelection().empty()"));
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
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, CurrentTabOpen);
			}

			break;
		case Action::OpenLinkInNewTabAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewTabOpen);
			}

			break;
		case Action::OpenLinkInNewTabBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewBackgroundTabOpen);
			}

			break;
		case Action::OpenLinkInNewWindowAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewWindowOpen);
			}

			break;
		case Action::OpenLinkInNewWindowBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewBackgroundWindowOpen);
			}

			break;
		case Action::OpenLinkInNewPrivateTabAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewPrivateTabOpen);
			}

			break;
		case Action::OpenLinkInNewPrivateTabBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewPrivateBackgroundTabOpen);
			}

			break;
		case Action::OpenLinkInNewPrivateWindowAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewPrivateWindowOpen);
			}

			break;
		case Action::OpenLinkInNewPrivateWindowBackgroundAction:
			if (m_hitResult.linkUrl.isValid())
			{
				openUrl(m_hitResult.linkUrl, NewPrivateBackgroundWindowOpen);
			}

			break;
		case Action::CopyLinkToClipboardAction:
			if (!m_hitResult.linkUrl.isEmpty())
			{
				QGuiApplication::clipboard()->setText(m_hitResult.linkUrl.toString());
			}

			break;
		case Action::BookmarkLinkAction:
			if (m_hitResult.linkUrl.isValid())
			{
				emit requestedAddBookmark(m_hitResult.linkUrl, m_hitResult.title, QString());
			}

			break;
		case Action::SaveLinkToDiskAction:
			TransfersManager::startTransfer(m_hitResult.linkUrl.toString(), QString(), false, isPrivate());

			break;
		case Action::SaveLinkToDownloadsAction:
			TransfersManager::startTransfer(m_hitResult.linkUrl.toString(), QString(), true, isPrivate());

			break;
		case Action::OpenSelectionAsLinkAction:
			{
				const QString text(m_webView->selectedText());

				if (!text.isEmpty())
				{
					InputInterpreter *interpreter = new InputInterpreter(this);

					connect(interpreter, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SIGNAL(requestedOpenUrl(QUrl,OpenHints)));
					connect(interpreter, SIGNAL(requestedSearch(QString,QString,OpenHints)), this, SIGNAL(requestedSearch(QString,QString,OpenHints)));

					interpreter->interpret(text, WindowsManager::calculateOpenHints(QGuiApplication::keyboardModifiers()), true);
				}
			}

			break;
		case Action::OpenFrameInCurrentTabAction:
			if (m_hitResult.frameUrl.isValid())
			{
				setUrl(m_hitResult.frameUrl);
			}

			break;
		case Action::OpenFrameInNewTabAction:
			if (m_hitResult.frameUrl.isValid())
			{
				openUrl(m_hitResult.frameUrl, CurrentTabOpen);
			}

			break;
		case Action::OpenFrameInNewTabBackgroundAction:
			if (m_hitResult.frameUrl.isValid())
			{
				openUrl(m_hitResult.frameUrl, NewBackgroundTabOpen);
			}

			break;
		case Action::CopyFrameLinkToClipboardAction:
			if (m_hitResult.frameUrl.isValid())
			{
				QGuiApplication::clipboard()->setText(m_hitResult.frameUrl.toString());
			}

			break;
		case Action::ReloadFrameAction:
			if (m_hitResult.frameUrl.isValid())
			{
//TODO Improve
				m_webView->page()->runJavaScript(QStringLiteral("var frames = document.querySelectorAll('iframe[src=\"%1\"], frame[src=\"%1\"]'); for (var i = 0; i < frames.length; ++i) { frames[i].src = ''; frames[i].src = \'%1\'; }").arg(m_hitResult.frameUrl.toString()));
			}

			break;
		case Action::OpenImageInNewTabAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				openUrl(m_hitResult.imageUrl, NewTabOpen);
			}

			break;
		case Action::SaveImageToDiskAction:
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
						Console::addMessage(tr("Failed to save image %0: %1").arg(path).arg(writer.errorString()), OtherMessageCategory, ErrorMessageLevel);
					}
				}
				else
				{
					Transfer *transfer = new Transfer(m_hitResult.imageUrl, QString(), false, this);

					if (transfer->getState() == Transfer::RunningState)
					{
						connect(transfer, SIGNAL(finished()), transfer, SLOT(deleteLater()));
					}
					else
					{
						transfer->deleteLater();
					}
				}
			}

			break;
		case Action::CopyImageUrlToClipboardAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				QApplication::clipboard()->setText(m_hitResult.imageUrl.toString());
			}

			break;
		case Action::ReloadImageAction:
			if (!m_hitResult.imageUrl.isEmpty())
			{
				if (getUrl().matches(m_hitResult.imageUrl, (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash)))
				{
					triggerAction(Action::ReloadAndBypassCacheAction);
				}
				else
				{
//TODO Improve
					m_webView->page()->runJavaScript(QStringLiteral("var images = document.querySelectorAll('img[src=\"%1\"]'); for (var i = 0; i < images.length; ++i) { images[i].src = ''; images[i].src = \'%1\'; }").arg(m_hitResult.imageUrl.toString()));
				}
			}

			break;
		case Action::ImagePropertiesAction:
			if (m_hitResult.imageUrl.scheme() == QLatin1String("data"))
			{
				QVariantMap properties;
				properties[QLatin1String("alternativeText")] = m_hitResult.alternateText;
				properties[QLatin1String("longDescription")] = m_hitResult.longDescription;

				ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());
				ImagePropertiesDialog *imagePropertiesDialog = new ImagePropertiesDialog(m_hitResult.imageUrl, properties, NULL, this);
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
			else
			{
				m_webView->page()->runJavaScript(QStringLiteral("var element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement); if (element && element.tagName && element.tagName.toLowerCase() == 'img') { [element.naturalWidth, element.naturalHeight]; }").arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleImageProperties));
			}

			break;
		case Action::SaveMediaToDiskAction:
			if (m_hitResult.mediaUrl.isValid())
			{
				Transfer *transfer = new Transfer(m_hitResult.mediaUrl, QString(), false, this);

				if (transfer->getState() == Transfer::RunningState)
				{
					connect(transfer, SIGNAL(finished()), transfer, SLOT(deleteLater()));
				}
				else
				{
					transfer->deleteLater();
				}
			}

			break;
		case Action::CopyMediaUrlToClipboardAction:
			if (!m_hitResult.mediaUrl.isEmpty())
			{
				QApplication::clipboard()->setText(m_hitResult.mediaUrl.toString());
			}

			break;
		case Action::MediaControlsAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { element.controls = %3; }").arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()).arg(checked ? QLatin1String("true") : QLatin1String("false")));

			break;
		case Action::MediaLoopAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { element.loop = %3; }").arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()).arg(checked ? QLatin1String("true") : QLatin1String("false")));

			break;
		case Action::MediaPlayPauseAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { if (element.paused) { element.play(); } else { element.pause(); } }").arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()));

			break;
		case Action::MediaMuteAction:
			m_webView->page()->runJavaScript(QStringLiteral("var element = document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)); if (element && element.tagName && (element.tagName.toLowerCase() == 'audio' || element.tagName.toLowerCase() == 'video')) { element.muted = !element.muted; }").arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()));

			break;
		case Action::GoBackAction:
			m_webView->triggerPageAction(QWebEnginePage::Back);

			break;
		case Action::GoForwardAction:
			m_webView->triggerPageAction(QWebEnginePage::Forward);

			break;
		case Action::RewindAction:
			m_webView->history()->goToItem(m_webView->history()->itemAt(0));

			break;
		case Action::FastForwardAction:
			m_webView->history()->goToItem(m_webView->history()->itemAt(m_webView->history()->count() - 1));

			break;
		case Action::StopAction:
			m_webView->triggerPageAction(QWebEnginePage::Stop);

			break;
		case Action::ReloadAction:
			emit aboutToReload();

			m_webView->triggerPageAction(QWebEnginePage::Stop);
			m_webView->triggerPageAction(QWebEnginePage::Reload);

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
			m_webView->triggerPageAction(QWebEnginePage::ReloadAndBypassCache);

			break;
		case Action::ContextMenuAction:
			showContextMenu();

			break;
		case Action::UndoAction:
			m_webView->triggerPageAction(QWebEnginePage::Undo);

			break;
		case Action::RedoAction:
			m_webView->triggerPageAction(QWebEnginePage::Redo);

			break;
		case Action::CutAction:
			m_webView->triggerPageAction(QWebEnginePage::Cut);

			break;
		case Action::CopyAction:
			m_webView->triggerPageAction(QWebEnginePage::Copy);

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
		case Action::CopyToNoteAction:
			{
				BookmarksItem *note = NotesManager::addNote(BookmarksModel::UrlBookmark, getUrl());
				note->setData(getSelectedText(), BookmarksModel::DescriptionRole);
			}

			break;
		case Action::PasteAction:
			m_webView->triggerPageAction(QWebEnginePage::Paste);

			break;
		case Action::DeleteAction:
			m_webView->page()->runJavaScript(QLatin1String("window.getSelection().deleteFromDocument()"));

			break;
		case Action::SelectAllAction:
			m_webView->triggerPageAction(QWebEnginePage::SelectAll);

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
				QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/createSearch.js"));
				file.open(QIODevice::ReadOnly);

				m_webView->page()->runJavaScript(QString(file.readAll()).arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleCreateSearch));

				file.close();
			}

			break;
		case Action::ScrollToStartAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollTo(0, 0)"));

			break;
		case Action::ScrollToEndAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollTo(0, document.body.scrollHeigh)"));

			break;
		case Action::ScrollPageUpAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollByPages(1)"));

			break;
		case Action::ScrollPageDownAction:
			m_webView->page()->runJavaScript(QLatin1String("window.scrollByPages(-1)"));

			break;
		case Action::ScrollPageLeftAction:
			m_webView->page()->runJavaScript(QStringLiteral("window.scrollBy(-%1, 0)").arg(m_webView->width()));

			break;
		case Action::ScrollPageRightAction:
			m_webView->page()->runJavaScript(QStringLiteral("window.scrollBy(%1, 0)").arg(m_webView->width()));

			break;
		case Action::StartDragScrollAction:
			setScrollMode(DragScroll);

			break;
		case Action::StartMoveScrollAction:
			setScrollMode(MoveScroll);

			break;
		case Action::EndScrollAction:
			setScrollMode(NoScroll);

			break;
		case Action::ActivateContentAction:
			{
				m_webView->setFocus();
				m_webView->page()->runJavaScript(QLatin1String("var element = document.activeElement; if (element && element.tagName && (element.tagName.toLowerCase() == 'input' || element.tagName.toLowerCase() == 'textarea'))) { document.activeElement.blur(); }"));
			}

			break;
		case Action::AddBookmarkAction:
			emit requestedAddBookmark(getUrl(), getTitle(), QString());

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

void QtWebEngineWebWidget::openUrl(const QUrl &url, OpenHints hints)
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

	triggerAction(Action::PasteAction);

	QGuiApplication::clipboard()->setMimeData(mimeData);
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

	emit hitTestResultReady();

	if (m_ignoreContextMenu || (!m_hitResult.geometry.isValid() && m_clickPosition.isNull()))
	{
		return;
	}

	updateEditActions();

	MenuFlags flags = NoMenu;

	if (m_hitResult.flags.testFlag(IsFormTest))
	{
		flags |= FormMenu;
	}

	if (!m_hitResult.imageUrl.isValid() && m_hitResult.flags.testFlag(IsSelectedTest) && !m_webView->selectedText().isEmpty())
	{
		flags |= SelectionMenu;
	}

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
		flags |= ImageMenu;
	}

	if (m_hitResult.mediaUrl.isValid())
	{
		flags |= MediaMenu;
	}

	if (m_hitResult.flags.testFlag(IsContentEditableTest))
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

	WebWidget::showContextMenu((m_clickPosition.isNull() ? m_hitResult.geometry.center() : m_clickPosition), flags);
}

void QtWebEngineWebWidget::handleCreateSearch(const QVariant &result)
{
	if (result.isNull())
	{
		return;
	}

	const QUrlQuery parameters(result.toMap().value(QLatin1String("query")).toString());
	const QStringList identifiers = SearchesManager::getSearchEngines();
	const QStringList keywords = SearchesManager::getSearchKeywords();
	const QIcon icon = getIcon();
	const QUrl url(result.toMap().value(QLatin1String("url")).toString());
	SearchInformation engine;
	engine.identifier = Utils::createIdentifier(getUrl().host(), identifiers);
	engine.title = getTitle();
	engine.icon = (icon.isNull() ? Utils::getIcon(QLatin1String("edit-find")) : icon);
	engine.resultsUrl.url = (url.isEmpty() ? getUrl() : (url.isRelative() ? getUrl().resolved(url) : url)).toString();
	engine.resultsUrl.enctype = result.toMap().value(QLatin1String("enctype")).toString();
	engine.resultsUrl.method = result.toMap().value(QLatin1String("method")).toString();
	engine.resultsUrl.parameters = parameters;

	SearchPropertiesDialog dialog(engine, keywords, false, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	SearchesManager::addSearchEngine(dialog.getSearchEngine(), dialog.isDefault());
}

void QtWebEngineWebWidget::handleHitTest(const QVariant &result)
{
	m_hitResult = HitTestResult(result);

	emit hitTestResultReady();
	emit unlockEventLoop();
}

void QtWebEngineWebWidget::handleHotClick(const QVariant &result)
{
	m_hitResult = HitTestResult(result);

	emit hitTestResultReady();

	if (!m_hitResult.flags.testFlag(IsContentEditableTest) && m_hitResult.tagName != QLatin1String("textarea") && m_hitResult.tagName != QLatin1String("select") && m_hitResult.tagName != QLatin1String("input"))
	{
		QTimer::singleShot(250, this, SLOT(showHotClickMenu()));
	}
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

	ContentsWidget *parent = qobject_cast<ContentsWidget*>(parentWidget());
	ImagePropertiesDialog *imagePropertiesDialog = new ImagePropertiesDialog(m_hitResult.imageUrl, properties, NULL, this);
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

void QtWebEngineWebWidget::handleToolTip(const QVariant &result)
{
	const HitTestResult hitResult(result);
	const QString toolTipsMode = SettingsManager::getValue(QLatin1String("Browser/ToolTipsMode")).toString();
	const QString link = (hitResult.linkUrl.isValid() ? hitResult.linkUrl : hitResult.formUrl).toString();
	QString text;

	if (toolTipsMode != QLatin1String("disabled"))
	{
		const QString title = QString(hitResult.title).replace(QLatin1Char('&'), QLatin1String("&amp;")).replace(QLatin1Char('<'), QLatin1String("&lt;")).replace(QLatin1Char('>'), QLatin1String("&gt;"));

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

	setStatusMessage((link.isEmpty() ? hitResult.title : link), true);

	if (!text.isEmpty())
	{
		QToolTip::showText(m_webView->mapToGlobal(m_clickPosition), QStringLiteral("<div style=\"white-space:pre-line;\">%1</div>").arg(text), m_webView);
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

	QEventLoop eventLoop;

	showDialog(&dialog);

	connect(&dialog, SIGNAL(closed(bool,QDialogButtonBox::StandardButton)), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

	eventLoop.exec();

	hideDialog(&dialog);

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

	emit urlChanged(url);

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
	else if (feature == QWebEnginePage::Notifications)
	{
		option = QLatin1String("Browser/EnableNotifications");
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
	if (m_actions.contains(Action::UndoAction))
	{
		m_actions[Action::UndoAction]->setText(m_webView->pageAction(QWebEnginePage::Undo)->text());
		m_actions[Action::UndoAction]->setEnabled(m_webView->pageAction(QWebEnginePage::Undo)->isEnabled());
	}
}

void QtWebEngineWebWidget::updateRedo()
{
	if (m_actions.contains(Action::RedoAction))
	{
		m_actions[Action::RedoAction]->setText(m_webView->pageAction(QWebEnginePage::Redo)->text());
		m_actions[Action::RedoAction]->setEnabled(m_webView->pageAction(QWebEnginePage::Redo)->isEnabled());
	}
}

void QtWebEngineWebWidget::updatePageActions(const QUrl &url)
{
	if (m_actions.contains(Action::AddBookmarkAction))
	{
		m_actions[Action::AddBookmarkAction]->setOverrideText(BookmarksManager::hasBookmark(url.toString()) ? QT_TRANSLATE_NOOP("actions", "Edit Bookmark...") : QT_TRANSLATE_NOOP("actions", "Add Bookmark..."));
	}

	if (m_actions.contains(Action::WebsitePreferencesAction))
	{
		m_actions[Action::WebsitePreferencesAction]->setEnabled(!url.isEmpty() && url.scheme() != QLatin1String("about"));
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

	if (m_actions.contains(Action::CopyToNoteAction))
	{
		m_actions[Action::CopyToNoteAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Copy)->isEnabled());
	}

	if (m_actions.contains(Action::PasteAction))
	{
		m_actions[Action::PasteAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Paste)->isEnabled());
	}

	if (m_actions.contains(Action::PasteAndGoAction))
	{
		m_actions[Action::PasteAndGoAction]->setEnabled(!QApplication::clipboard()->text().isEmpty());
	}

	if (m_actions.contains(Action::PasteNoteAction))
	{
		m_actions[Action::PasteNoteAction]->setEnabled(m_webView->page()->action(QWebEnginePage::Paste)->isEnabled() && NotesManager::getModel()->getRootItem()->hasChildren());
	}

	if (m_actions.contains(Action::DeleteAction))
	{
		m_actions[Action::DeleteAction]->setEnabled(m_webView->hasSelection() && m_hitResult.flags.testFlag(IsContentEditableTest));
	}

	if (m_actions.contains(Action::SelectAllAction))
	{
		m_actions[Action::SelectAllAction]->setEnabled(!m_hitResult.flags.testFlag(IsContentEditableTest) || !m_hitResult.flags.testFlag(IsEmptyTest));
	}

	if (m_actions.contains(Action::ClearAllAction))
	{
		m_actions[Action::ClearAllAction]->setEnabled(m_hitResult.flags.testFlag(IsContentEditableTest) && !m_hitResult.flags.testFlag(IsEmptyTest));
	}

	if (m_actions.contains(Action::SearchAction))
	{
		const SearchInformation engine = SearchesManager::getSearchEngine(getOption(QLatin1String("Search/DefaultQuickSearchEngine")).toString());
		const bool isValid = !engine.identifier.isEmpty();

		m_actions[Action::SearchAction]->setEnabled(isValid);
		m_actions[Action::SearchAction]->setData(isValid ? engine.identifier : QVariant());
		m_actions[Action::SearchAction]->setIcon((!isValid || engine.icon.isNull()) ? Utils::getIcon(QLatin1String("edit-find")) : engine.icon);
		m_actions[Action::SearchAction]->setOverrideText(isValid ? engine.title : QT_TRANSLATE_NOOP("actions", "Search"));
		m_actions[Action::SearchAction]->setToolTip(isValid ? engine.description : tr("No search engines defined"));
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

	if (m_actions.contains(Action::OpenLinkInNewPrivateTabAction))
	{
		m_actions[Action::OpenLinkInNewPrivateTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::OpenLinkInNewPrivateTabBackgroundAction))
	{
		m_actions[Action::OpenLinkInNewPrivateTabBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::OpenLinkInNewPrivateWindowAction))
	{
		m_actions[Action::OpenLinkInNewPrivateWindowAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::OpenLinkInNewPrivateWindowBackgroundAction))
	{
		m_actions[Action::OpenLinkInNewPrivateWindowBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::CopyLinkToClipboardAction))
	{
		m_actions[Action::CopyLinkToClipboardAction]->setEnabled(isLink);
	}

	if (m_actions.contains(Action::BookmarkLinkAction))
	{
		m_actions[Action::BookmarkLinkAction]->setOverrideText(BookmarksManager::hasBookmark(m_hitResult.linkUrl.toString()) ? QT_TRANSLATE_NOOP("actions", "Edit Link Bookmark...") : QT_TRANSLATE_NOOP("actions", "Bookmark Link..."));
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
		m_actions[Action::MediaControlsAction]->setChecked(m_hitResult.flags.testFlag(MediaHasControlsTest));
		m_actions[Action::MediaControlsAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(Action::MediaLoopAction))
	{
		m_actions[Action::MediaLoopAction]->setChecked(m_hitResult.flags.testFlag(MediaIsLoopedTest));
		m_actions[Action::MediaLoopAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(Action::MediaPlayPauseAction))
	{
		m_actions[Action::MediaPlayPauseAction]->setOverrideText(m_hitResult.flags.testFlag(MediaIsPausedTest) ? QT_TRANSLATE_NOOP("actions", "Play") : QT_TRANSLATE_NOOP("actions", "Pause"));
		m_actions[Action::MediaPlayPauseAction]->setIcon(Utils::getIcon(m_hitResult.flags.testFlag(MediaIsPausedTest) ? QLatin1String("media-playback-start") : QLatin1String("media-playback-pause")));
		m_actions[Action::MediaPlayPauseAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(Action::MediaMuteAction))
	{
		m_actions[Action::MediaMuteAction]->setOverrideText(m_hitResult.flags.testFlag(MediaIsMutedTest) ? QT_TRANSLATE_NOOP("actions", "Unmute") : QT_TRANSLATE_NOOP("actions", "Mute"));
		m_actions[Action::MediaMuteAction]->setIcon(Utils::getIcon(m_hitResult.flags.testFlag(MediaIsMutedTest) ? QLatin1String("audio-volume-medium") : QLatin1String("audio-volume-muted")));
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

void QtWebEngineWebWidget::showContextMenu(const QPoint &position)
{
	QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
	file.open(QIODevice::ReadOnly);

	m_webView->page()->runJavaScript(QString(file.readAll()).arg(position.x()).arg(position.y()), invoke(this, &QtWebEngineWebWidget::handleContextMenu));

	file.close();
}

void QtWebEngineWebWidget::showHotClickMenu()
{
	if (!m_webView->selectedText().trimmed().isEmpty())
	{
		showContextMenu(m_clickPosition);
	}
}

void QtWebEngineWebWidget::setOptions(const QVariantHash &options)
{
	WebWidget::setOptions(options);

	updateOptions(getUrl());
}

void QtWebEngineWebWidget::setScrollPosition(const QPoint &position)
{
	m_webView->page()->runJavaScript(QStringLiteral("window.scrollTo(%1, %2); [window.scrollX, window.scrollY];").arg(position.x()).arg(position.y()), invoke(this, &QtWebEngineWebWidget::handleScroll));
}

void QtWebEngineWebWidget::setHistory(const WindowHistoryInformation &history)
{
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
	else if (key == QLatin1String("Browser/EnableNotifications"))
	{
		feature = QWebEnginePage::Notifications;
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
	else
	{
		return;
	}

	m_webView->page()->setFeaturePermission(url, feature, (policies.testFlag(GrantedPermission) ? QWebEnginePage::PermissionGrantedByUser : QWebEnginePage::PermissionDeniedByUser));
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

	Action *action = new Action(identifier, this);

	m_actions[identifier] = action;

	connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

	switch (identifier)
	{
		case Action::FindAction:
		case Action::FindNextAction:
		case Action::FindPreviousAction:
			action->setEnabled(true);

			break;
		case Action::StopScheduledReloadAction:
		case Action::CopyImageToClipboardAction:
		case Action::CheckSpellingAction:
		case Action::ViewSourceAction:
		case Action::InspectPageAction:
		case Action::InspectElementAction:
		case Action::LoadPluginsAction:
			action->setEnabled(false);

			break;
		case Action::AddBookmarkAction:
		case Action::WebsitePreferencesAction:
			updatePageActions(getUrl());

			break;
		case Action::GoBackAction:
		case Action::RewindAction:
			action->setEnabled(m_webView->history()->canGoBack());

			break;
		case Action::GoForwardAction:
		case Action::FastForwardAction:
			action->setEnabled(m_webView->history()->canGoForward());

			break;
		case Action::PasteNoteAction:
			action->setMenu(getPasteNoteMenu());

			updateEditActions();

			break;
		case Action::StopAction:
			action->setEnabled(m_isLoading);

			break;

		case Action::ReloadAction:
			action->setEnabled(!m_isLoading);

			break;
		case Action::ReloadOrStopAction:
			action->setup(m_isLoading ? getAction(Action::StopAction) : getAction(Action::ReloadAction));

			break;
		case Action::ScheduleReloadAction:
			action->setMenu(getReloadTimeMenu());

			break;
		case Action::ValidateAction:
			action->setEnabled(false);
			action->setMenu(new QMenu(this));

			break;
		case Action::UndoAction:
			action->setEnabled(m_webView->pageAction(QWebEnginePage::Undo));

			updateUndo();

			connect(m_webView->pageAction(QWebEnginePage::Undo), SIGNAL(changed()), this, SLOT(updateUndo()));

			break;
		case Action::RedoAction:
			action->setEnabled(m_webView->pageAction(QWebEnginePage::Redo));

			updateRedo();

			connect(m_webView->pageAction(QWebEnginePage::Redo), SIGNAL(changed()), this, SLOT(updateRedo()));

			break;
		case Action::SearchMenuAction:
			action->setMenu(getQuickSearchMenu());

		case Action::CutAction:
		case Action::CopyAction:
		case Action::CopyPlainTextAction:
		case Action::CopyToNoteAction:
		case Action::PasteAction:
		case Action::PasteAndGoAction:
		case Action::DeleteAction:
		case Action::ClearAllAction:
		case Action::SearchAction:
			updateEditActions();

			break;
		case Action::OpenLinkAction:
		case Action::OpenLinkInCurrentTabAction:
		case Action::OpenLinkInNewTabAction:
		case Action::OpenLinkInNewTabBackgroundAction:
		case Action::OpenLinkInNewWindowAction:
		case Action::OpenLinkInNewWindowBackgroundAction:
		case Action::OpenLinkInNewPrivateTabAction:
		case Action::OpenLinkInNewPrivateTabBackgroundAction:
		case Action::OpenLinkInNewPrivateWindowAction:
		case Action::OpenLinkInNewPrivateWindowBackgroundAction:
		case Action::CopyLinkToClipboardAction:
		case Action::BookmarkLinkAction:
		case Action::SaveLinkToDiskAction:
		case Action::SaveLinkToDownloadsAction:
			updateLinkActions();

			break;
		case Action::OpenFrameInCurrentTabAction:
		case Action::OpenFrameInNewTabAction:
		case Action::OpenFrameInNewTabBackgroundAction:
		case Action::CopyFrameLinkToClipboardAction:
		case Action::ReloadFrameAction:
		case Action::ViewFrameSourceAction:
			updateFrameActions();

			break;
		case Action::OpenImageInNewTabAction:
		case Action::SaveImageToDiskAction:
		case Action::CopyImageUrlToClipboardAction:
		case Action::ReloadImageAction:
		case Action::ImagePropertiesAction:
			updateImageActions();

			break;
		case Action::SaveMediaToDiskAction:
		case Action::CopyMediaUrlToClipboardAction:
		case Action::MediaControlsAction:
		case Action::MediaLoopAction:
		case Action::MediaPlayPauseAction:
		case Action::MediaMuteAction:
			updateMediaActions();

			break;
		default:
			break;
	}

	return action;
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

bool QtWebEngineWebWidget::findInPage(const QString &text, FindFlags flags)
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
	if (object == m_webView)
	{
		if (event->type() == QEvent::ChildAdded)
		{
			QChildEvent *childEvent = static_cast<QChildEvent*>(event);

			if (childEvent->child())
			{
				childEvent->child()->installEventFilter(this);

				m_childWidget = qobject_cast<QWidget*>(childEvent->child());
			}
		}
		else if (event->type() == QEvent::ContextMenu)
		{
			QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent*>(event);

			m_ignoreContextMenu = (contextMenuEvent->reason() == QContextMenuEvent::Mouse);

			if (contextMenuEvent->reason() == QContextMenuEvent::Keyboard)
			{
				m_clickPosition = contextMenuEvent->pos();

				showContextMenu();
			}

			return true;
		}
		else if (event->type() == QEvent::Move || event->type() == QEvent::Resize)
		{
			emit progressBarGeometryChanged();
		}
		else if (event->type() == QEvent::ToolTip)
		{
			QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
			file.open(QIODevice::ReadOnly);

			m_clickPosition = m_webView->mapFromGlobal(QCursor::pos());

			m_webView->page()->runJavaScript(QString(file.readAll()).arg(m_clickPosition.x() / m_webView->zoomFactor()).arg(m_clickPosition.y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleToolTip));

			file.close();

			event->accept();

			return true;
		}
	}
	else
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			if (mouseEvent->button() == Qt::BackButton)
			{
				triggerAction(Action::GoBackAction);

				event->accept();

				return true;
			}

			if (mouseEvent->button() == Qt::ForwardButton)
			{
				triggerAction(Action::GoForwardAction);

				event->accept();

				return true;
			}

			if (mouseEvent->button() == Qt::LeftButton || mouseEvent->button() == Qt::MiddleButton)
			{
				m_webView->page()->runJavaScript(QStringLiteral("[window.scrollX, window.scrollY]"), invoke(this, &QtWebEngineWebWidget::handleScroll));

				if (mouseEvent->button() == Qt::LeftButton && mouseEvent->buttons().testFlag(Qt::RightButton))
				{
					m_isUsingRockerNavigation = true;

					triggerAction(Action::GoBackAction);

					return true;
				}

				if (mouseEvent->modifiers() != Qt::NoModifier || mouseEvent->button() == Qt::MiddleButton)
				{
					QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
					file.open(QIODevice::ReadOnly);

					QEventLoop eventLoop;

					m_webView->page()->runJavaScript(QString(file.readAll()).arg(mouseEvent->pos().x() / m_webView->zoomFactor()).arg(mouseEvent->pos().y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleHitTest));

					file.close();

					connect(this, SIGNAL(unlockEventLoop()), &eventLoop, SLOT(quit()));
					connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
					connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

					eventLoop.exec();

					if (m_hitResult.linkUrl.isValid())
					{
						openUrl(m_hitResult.linkUrl, WindowsManager::calculateOpenHints(mouseEvent->modifiers(), mouseEvent->button(), CurrentTabOpen));

						event->accept();

						return true;
					}

					if (mouseEvent->button() == Qt::MiddleButton)
					{
						if (!m_hitResult.linkUrl.isValid() && m_hitResult.tagName != QLatin1String("textarea") && m_hitResult.tagName != QLatin1String("input"))
						{
							triggerAction(Action::StartMoveScrollAction);

							return true;
						}
					}
				}
			}
			else if (mouseEvent->button() == Qt::RightButton)
			{
				if (mouseEvent->buttons().testFlag(Qt::LeftButton))
				{
					triggerAction(Action::GoForwardAction);

					event->ignore();
				}
				else
				{
					event->accept();

					QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
					file.open(QIODevice::ReadOnly);

					QEventLoop eventLoop;

					m_webView->page()->runJavaScript(QString(file.readAll()).arg(mouseEvent->pos().x() / m_webView->zoomFactor()).arg(mouseEvent->pos().y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleHitTest));

					file.close();

					connect(this, SIGNAL(unlockEventLoop()), &eventLoop, SLOT(quit()));
					connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
					connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

					eventLoop.exec();

					if (m_hitResult.linkUrl.isValid())
					{
						m_clickPosition = mouseEvent->pos();
					}

					GesturesManager::startGesture((m_hitResult.linkUrl.isValid() ? GesturesManager::LinkGesturesContext : GesturesManager::GenericGesturesContext), m_childWidget, mouseEvent);
				}

				return true;
			}
		}
		else if (event->type() == QEvent::MouseButtonRelease)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			if (getScrollMode() == MoveScroll)
			{
				return true;
			}

			if (mouseEvent->button() == Qt::MiddleButton)
			{
				QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
				file.open(QIODevice::ReadOnly);

				QEventLoop eventLoop;

				m_webView->page()->runJavaScript(QString(file.readAll()).arg(mouseEvent->pos().x() / m_webView->zoomFactor()).arg(mouseEvent->pos().y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleHitTest));

				file.close();

				connect(this, SIGNAL(unlockEventLoop()), &eventLoop, SLOT(quit()));
				connect(this, SIGNAL(aboutToReload()), &eventLoop, SLOT(quit()));
				connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

				eventLoop.exec();

				if (getScrollMode() == DragScroll)
				{
					triggerAction(Action::EndScrollAction);
				}
				else if (m_hitResult.linkUrl.isValid())
				{
					return true;
				}
			}
			else if (mouseEvent->button() == Qt::RightButton && !mouseEvent->buttons().testFlag(Qt::LeftButton))
			{
				if (m_isUsingRockerNavigation)
				{
					m_isUsingRockerNavigation = false;
					m_ignoreContextMenuNextTime = true;

					QMouseEvent mousePressEvent(QEvent::MouseButtonPress, QPointF(mouseEvent->pos()), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
					QMouseEvent mouseReleaseEvent(QEvent::MouseButtonRelease, QPointF(mouseEvent->pos()), Qt::RightButton, Qt::RightButton, Qt::NoModifier);

					QCoreApplication::sendEvent(m_webView, &mousePressEvent);
					QCoreApplication::sendEvent(m_webView, &mouseReleaseEvent);
				}
				else
				{
					m_ignoreContextMenu = false;

					if (!GesturesManager::endGesture(m_childWidget, mouseEvent))
					{
						if (m_ignoreContextMenuNextTime)
						{
							m_ignoreContextMenuNextTime = false;
						}
						else
						{
							m_clickPosition = mouseEvent->pos();

							showContextMenu(mouseEvent->pos());
						}
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
				m_clickPosition = mouseEvent->pos();

				QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hitTest.js"));
				file.open(QIODevice::ReadOnly);

				m_webView->page()->runJavaScript(QString(file.readAll()).arg(mouseEvent->pos().x() / m_webView->zoomFactor()).arg(mouseEvent->pos().y() / m_webView->zoomFactor()), invoke(this, &QtWebEngineWebWidget::handleHotClick));

				file.close();
			}
		}
		else if (event->type() == QEvent::Wheel)
		{
			m_webView->page()->runJavaScript(QStringLiteral("[window.scrollX, window.scrollY]"), invoke(this, &QtWebEngineWebWidget::handleScroll));

			if (getScrollMode() == MoveScroll)
			{
				triggerAction(Action::EndScrollAction);

				return true;
			}
			else
			{
				QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

				if (wheelEvent->modifiers().testFlag(Qt::ControlModifier))
				{
					setZoom(getZoom() + (wheelEvent->delta() / 16));

					event->accept();

					return true;
				}
			}
		}
	}

	return QObject::eventFilter(object, event);
}

}
