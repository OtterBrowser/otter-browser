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
#include "../../../../core/HistoryManager.h"
#include "../../../../core/InputInterpreter.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/WebsitePreferencesDialog.h"

#include <QtCore/QFileInfo>
#include <QtGui/QClipboard>
#include <QtWebEngineWidgets/QWebEngineHistory>
#include <QtWebEngineWidgets/QWebEngineSettings>
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

QtWebEngineWebWidget::QtWebEngineWebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent) : WebWidget(isPrivate, backend, parent),
	m_webView(new QWebEngineView(this)),
	m_isLoading(false),
	m_isTyped(false)
{
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

}
