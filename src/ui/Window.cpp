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

#include "Window.h"
#include "ActionWidget.h"
#include "AddressWidget.h"
#include "GoBackActionWidget.h"
#include "GoForwardActionWidget.h"
#include "SearchWidget.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/SettingsManager.h"
#include "../core/WebBackend.h"
#include "../core/WebBackendsManager.h"
#include "../ui/WebWidget.h"
#include "../modules/windows/bookmarks/BookmarksContentsWidget.h"
#include "../modules/windows/cache/CacheContentsWidget.h"
#include "../modules/windows/cookies/CookiesContentsWidget.h"
#include "../modules/windows/configuration/ConfigurationContentsWidget.h"
#include "../modules/windows/history/HistoryContentsWidget.h"
#include "../modules/windows/transfers/TransfersContentsWidget.h"
#include "../modules/windows/web/WebContentsWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>

namespace Otter
{

qint64 Window::m_identifierCounter = 0;

Window::Window(bool isPrivate, ContentsWidget *widget, QWidget *parent) : QWidget(parent),
	m_navigationBar(NULL),
	m_addressWidget(NULL),
	m_searchWidget(NULL),
	m_contentsWidget(NULL),
	m_identifier(++m_identifierCounter),
	m_isPinned(false),
	m_isPrivate(isPrivate)
{
	QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);

	if (widget)
	{
		widget->setParent(this);

		setContentsWidget(widget);
	}
}

void Window::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);

	if (!m_contentsWidget)
	{
		setUrl(m_session.getUrl(), false);
	}
}

void Window::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);

	if (isUrlEmpty() && !m_contentsWidget->isLoading() && m_addressWidget)
	{
		m_addressWidget->setFocus();
	}
	else if (m_contentsWidget)
	{
		m_contentsWidget->setFocus();
	}
}

void Window::triggerAction(ActionIdentifier action, bool checked)
{
	if (action == ActivateSearchFieldAction && m_searchWidget)
	{
		m_searchWidget->setFocus();
	}
	else if (m_addressWidget)
	{
		if (action == ActivateAddressFieldAction)
		{
			m_addressWidget->setFocus(Qt::ShortcutFocusReason);
		}
		else if (action == ActivateSearchFieldAction)
		{
			m_addressWidget->setText(QLatin1String("? "));
			m_addressWidget->setFocus(Qt::OtherFocusReason);
		}
		else if (action == PasteAndGoAction)
		{
			if (!QApplication::clipboard()->text().isEmpty())
			{
				m_addressWidget->handleUserInput(QApplication::clipboard()->text().trimmed());
			}

			return;
		}
		else if (action == GoAction)
		{
			m_addressWidget->handleUserInput(m_addressWidget->text());

			return;
		}
	}

	if (action == GoToParentDirectoryAction && getContentsWidget()->getType() == QLatin1String("web"))
	{
		const QUrl url = getContentsWidget()->getUrl();

		if (url.toString().endsWith(QLatin1Char('/')))
		{
			getContentsWidget()->setUrl(url.resolved(QUrl(QLatin1String(".."))));
		}
		else
		{
			getContentsWidget()->setUrl(url.resolved(QUrl(QLatin1String("."))));
		}
	}
	else if (action == PrintAction)
	{
		QPrinter printer;
		QPrintDialog printDialog(&printer, this);
		printDialog.setWindowTitle(tr("Print Page"));

		if (printDialog.exec() != QDialog::Accepted)
		{
			return;
		}

		getContentsWidget()->print(&printer);
	}
	else if (action == PrintPreviewAction)
	{
		QPrintPreviewDialog printPreviewtDialog(this);
		printPreviewtDialog.setWindowFlags(printPreviewtDialog.windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
		printPreviewtDialog.setWindowTitle(tr("Print Preview"));

		connect(&printPreviewtDialog, SIGNAL(paintRequested(QPrinter*)), getContentsWidget(), SLOT(print(QPrinter*)));

		printPreviewtDialog.exec();
	}
	else
	{
		getContentsWidget()->triggerAction(action, checked);
	}
}

void Window::clear()
{
	if (m_addressWidget)
	{
		m_addressWidget->clear();
	}

	setContentsWidget(new WebContentsWidget(m_isPrivate, NULL, this));
}

void Window::close()
{
	emit aboutToClose();

	QTimer::singleShot(50, this, SLOT(notifyRequestedCloseWindow()));
}

void Window::search(const QString &query, const QString &engine)
{
	WebContentsWidget *widget = qobject_cast<WebContentsWidget*>(m_contentsWidget);

	if (!widget)
	{
		widget = new WebContentsWidget(isPrivate(), NULL, this);

		setContentsWidget(widget);
	}

	if (m_addressWidget)
	{
		m_addressWidget->clearFocus();
	}

	widget->search(query, engine);

	if (m_addressWidget)
	{
		m_addressWidget->setUrl(getUrl());
	}
}

void Window::handleOpenUrlRequest(const QUrl &url, OpenHints hints)
{
	if (hints == DefaultOpen || hints == CurrentTabOpen)
	{
		setUrl(url);

		return;
	}

	if (isPrivate())
	{
		hints |= PrivateOpen;
	}

	emit requestedOpenUrl(url, hints);
}

void Window::handleSearchRequest(const QString &query, const QString &engine, OpenHints hints)
{
	if ((getType() == QLatin1String("web") && isUrlEmpty()) || (hints == DefaultOpen || hints == CurrentTabOpen))
	{
		search(query, engine);
	}
	else
	{
		emit requestedSearch(query, engine, hints);
	}
}

void Window::notifyLoadingStateChanged(bool loading)
{
	emit loadingStateChanged(loading ? LoadingState : LoadedState);
}

void Window::notifyRequestedCloseWindow()
{
	emit requestedCloseWindow(this);
}

void Window::setSession(const SessionWindow &session)
{
	m_session = session;

	setSearchEngine(session.searchEngine);
	setPinned(session.pinned);

	if (!SettingsManager::getValue(QLatin1String("Browser/DelayRestoringOfBackgroundTabs")).toBool())
	{
		setUrl(session.getUrl(), false);
	}
}

void Window::setOption(const QString &key, const QVariant &value)
{
	if (m_contentsWidget->getType() == QLatin1String("web"))
	{
		WebContentsWidget *webWidget = qobject_cast<WebContentsWidget*>(m_contentsWidget);

		if (webWidget)
		{
			if (key == QLatin1String("Network/UserAgent") && value.toString() == QLatin1String("custom"))
			{
				bool confirmed = false;
				const QString userAgent = QInputDialog::getText(this, tr("Select User Agent"), tr("Enter User Agent:"), QLineEdit::Normal, NetworkManagerFactory::getUserAgent(webWidget->getWebWidget()->getOption(QLatin1String("Network/UserAgent")).toString()).value, &confirmed);

				if (confirmed)
				{
					webWidget->getWebWidget()->setOption(QLatin1String("Network/UserAgent"), QLatin1String("custom;") + userAgent);
				}
			}
			else
			{
				webWidget->getWebWidget()->setOption(key, value);
			}
		}
	}
}

void Window::setSearchEngine(const QString &engine)
{
	if (m_searchWidget)
	{
		m_searchWidget->setCurrentSearchEngine(engine);
	}
}

void Window::setUrl(const QUrl &url, bool typed)
{
	ContentsWidget *newWidget = NULL;

	if (url.scheme() == QLatin1String("about"))
	{
		if (m_addressWidget && m_session.index < 0 && !isUrlEmpty() && SessionsManager::hasUrl(url, true))
		{
			m_addressWidget->setUrl(m_contentsWidget ? m_contentsWidget->getUrl() : m_session.getUrl());

			return;
		}

		if (url.path() == QLatin1String("bookmarks"))
		{
			newWidget = new BookmarksContentsWidget(this);
		}
		else if (url.path() == QLatin1String("cache"))
		{
			newWidget = new CacheContentsWidget(this);
		}
		else if (url.path() == QLatin1String("config"))
		{
			newWidget = new ConfigurationContentsWidget(this);
		}
		else if (url.path() == QLatin1String("cookies"))
		{
			newWidget = new CookiesContentsWidget(this);
		}
		else if (url.path() == QLatin1String("history"))
		{
			newWidget = new HistoryContentsWidget(this);
		}
		else if (url.path() == QLatin1String("transfers"))
		{
			newWidget = new TransfersContentsWidget(this);
		}

		if (newWidget && !newWidget->canClone())
		{
			SessionsManager::removeStoredUrl(newWidget->getUrl().toString());
		}
	}

	const bool isRestoring = (!m_contentsWidget && m_session.index >= 0);

	if (!newWidget && (!m_contentsWidget || m_contentsWidget->getType() != QLatin1String("web")))
	{
		newWidget = new WebContentsWidget(m_isPrivate, NULL, this);
	}

	if (newWidget)
	{
		setContentsWidget(newWidget);
	}

	if (m_contentsWidget && url.isValid())
	{
		if (!isRestoring)
		{
			m_contentsWidget->setUrl(url, typed);
		}

		if (m_addressWidget)
		{
			if (!isUrlEmpty() || m_contentsWidget->isLoading())
			{
				m_addressWidget->clearFocus();
			}

			m_addressWidget->setUrl(url);
		}
	}
}

void Window::setPinned(bool pinned)
{
	if (pinned != m_isPinned)
	{
		m_isPinned = pinned;

		emit isPinnedChanged(pinned);
	}
}

void Window::setContentsWidget(ContentsWidget *widget)
{
	if (m_contentsWidget)
	{
		layout()->removeWidget(m_contentsWidget);

		m_contentsWidget->deleteLater();
	}

	m_contentsWidget = widget;

	if (!m_contentsWidget)
	{
		return;
	}

	if (m_contentsWidget->getType() == QLatin1String("web") && !m_navigationBar)
	{
		const ToolBarDefinition toolBar = ActionsManager::getToolBarDefinition(QLatin1String("NavigationBar"));

		m_navigationBar = new QWidget(this);

		QBoxLayout *navigationLayout = new QBoxLayout(QBoxLayout::LeftToRight, m_navigationBar);
		navigationLayout->setContentsMargins(0, 0, 0, 0);

		for (int i = 0; i < toolBar.actions.count(); ++i)
		{
			if (toolBar.actions.at(i).action == QLatin1String("AddressWidget"))
			{
				m_addressWidget = new AddressWidget(this, false, this);
				m_addressWidget->setUrl(m_contentsWidget->getUrl());

				navigationLayout->addWidget(m_addressWidget, 3);

				connect(m_contentsWidget, SIGNAL(urlChanged(QUrl)), m_addressWidget, SLOT(setUrl(QUrl)));
				connect(m_addressWidget, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SLOT(handleOpenUrlRequest(QUrl,OpenHints)));
				connect(m_addressWidget, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)), this, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)));
				connect(m_addressWidget, SIGNAL(requestedSearch(QString,QString,OpenHints)), this, SLOT(handleSearchRequest(QString,QString,OpenHints)));
			}
			else if (toolBar.actions.at(i).action == QLatin1String("SearchWidget"))
			{
				m_searchWidget = new SearchWidget(this);

				navigationLayout->addWidget(m_searchWidget);

				connect(m_searchWidget, SIGNAL(requestedSearch(QString,QString,OpenHints)), this, SIGNAL(requestedSearch(QString,QString,OpenHints)));
			}
			else if (toolBar.actions.at(i).action == QLatin1String("GoBackAction"))
			{
				navigationLayout->addWidget(new GoBackActionWidget(this, m_navigationBar));
			}
			else if (toolBar.actions.at(i).action == QLatin1String("GoForwardAction"))
			{
				navigationLayout->addWidget(new GoForwardActionWidget(this, m_navigationBar));
			}
			else
			{
				const ActionIdentifier action = ActionsManager::getActionIdentifier(toolBar.actions.at(i).action.left(toolBar.actions.at(i).action.length() - 6));

				if (action != UnknownAction)
				{
					navigationLayout->addWidget(new ActionWidget(action, this, m_navigationBar));
				}
			}
		}

		m_navigationBar->setLayout(navigationLayout);

		layout()->addWidget(m_navigationBar);
	}
	else if (m_contentsWidget->getType() != QLatin1String("web") && m_navigationBar)
	{
		m_navigationBar->deleteLater();

		layout()->removeWidget(m_navigationBar);

		m_navigationBar = NULL;
		m_addressWidget = NULL;
		m_searchWidget = NULL;
	}

	layout()->addWidget(m_contentsWidget);

	if (m_session.index >= 0)
	{
		if (!m_session.userAgent.isEmpty() && m_contentsWidget->getType() == QLatin1String("web"))
		{
			WebContentsWidget *webWidget = qobject_cast<WebContentsWidget*>(m_contentsWidget);

			if (webWidget)
			{
				webWidget->getWebWidget()->setOption(QLatin1String("Network/UserAgent"), m_session.userAgent);
			}
		}

		if (m_session.reloadTime != -1 && m_contentsWidget->getType() == QLatin1String("web"))
		{
			WebContentsWidget *webWidget = qobject_cast<WebContentsWidget*>(m_contentsWidget);

			if (webWidget)
			{
				webWidget->setReloadTime(m_session.reloadTime);
			}
		}

		WindowHistoryInformation history;
		history.index = m_session.index;
		history.entries = m_session.history;

		m_contentsWidget->setHistory(history);
		m_contentsWidget->setZoom(m_session.getZoom());
		m_contentsWidget->setFocus();

		m_session = SessionWindow();
	}
	else
	{
		if (isUrlEmpty() && m_addressWidget)
		{
			m_addressWidget->setFocus();
		}

		if (m_contentsWidget->getUrl().scheme() == QLatin1String("about") || isUrlEmpty())
		{
			emit titleChanged(m_contentsWidget->getTitle());
		}
	}

	emit actionsChanged();
	emit canZoomChanged(m_contentsWidget->canZoom());
	emit iconChanged(m_contentsWidget->getIcon());

	connect(this, SIGNAL(aboutToClose()), m_contentsWidget, SLOT(close()));
	connect(m_contentsWidget, SIGNAL(requestedAddBookmark(QUrl,QString)), this, SIGNAL(requestedAddBookmark(QUrl,QString)));
	connect(m_contentsWidget, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SIGNAL(requestedOpenUrl(QUrl,OpenHints)));
	connect(m_contentsWidget, SIGNAL(requestedNewWindow(ContentsWidget*,OpenHints)), this, SIGNAL(requestedNewWindow(ContentsWidget*,OpenHints)));
	connect(m_contentsWidget, SIGNAL(requestedSearch(QString,QString,OpenHints)), this, SIGNAL(requestedSearch(QString,QString,OpenHints)));
	connect(m_contentsWidget, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
	connect(m_contentsWidget, SIGNAL(canZoomChanged(bool)), this, SIGNAL(canZoomChanged(bool)));
	connect(m_contentsWidget, SIGNAL(statusMessageChanged(QString)), this, SIGNAL(statusMessageChanged(QString)));
	connect(m_contentsWidget, SIGNAL(titleChanged(QString)), this, SIGNAL(titleChanged(QString)));
	connect(m_contentsWidget, SIGNAL(urlChanged(QUrl)), this, SIGNAL(urlChanged(QUrl)));
	connect(m_contentsWidget, SIGNAL(iconChanged(QIcon)), this, SIGNAL(iconChanged(QIcon)));
	connect(m_contentsWidget, SIGNAL(loadingChanged(bool)), this, SLOT(notifyLoadingStateChanged(bool)));
	connect(m_contentsWidget, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
}

Window* Window::clone(bool cloneHistory, QWidget *parent)
{
	if (!m_contentsWidget || !canClone())
	{
		return NULL;
	}

	Window *window = new Window(false, m_contentsWidget->clone(cloneHistory), parent);
	window->setSearchEngine(getSearchEngine());
	window->setPinned(isPinned());

	return window;
}

ContentsWidget* Window::getContentsWidget()
{
	if (!m_contentsWidget)
	{
		setUrl(m_session.getUrl(), false);
	}

	return m_contentsWidget;
}

QVariant Window::getOption(const QString &key) const
{
	if (m_contentsWidget && m_contentsWidget->getType() == QLatin1String("web"))
	{
		WebContentsWidget *webWidget = qobject_cast<WebContentsWidget*>(m_contentsWidget);

		if (webWidget && webWidget->getWebWidget()->hasOption(key))
		{
			return webWidget->getWebWidget()->getOption(key);
		}
	}

	return QVariant();
}

QString Window::getSearchEngine() const
{
	return (m_searchWidget ? m_searchWidget->getCurrentSearchEngine() : QString());
}

QString Window::getTitle() const
{
	return (m_contentsWidget ? m_contentsWidget->getTitle() : m_session.getTitle());
}

QLatin1String Window::getType() const
{
	return (m_contentsWidget ? m_contentsWidget->getType() : QLatin1String("unknown"));
}

QUrl Window::getUrl() const
{
	return (m_contentsWidget ? m_contentsWidget->getUrl() : m_session.getUrl());
}

QIcon Window::getIcon() const
{
	return (m_contentsWidget ? m_contentsWidget->getIcon() : WebBackendsManager::getBackend()->getIconForUrl(m_session.getUrl()));
}

QPixmap Window::getThumbnail() const
{
	return (m_contentsWidget ? m_contentsWidget->getThumbnail() : QPixmap());
}

SessionWindow Window::getSession() const
{
	if (!m_contentsWidget)
	{
		return m_session;
	}

	const WindowHistoryInformation history = m_contentsWidget->getHistory();
	SessionWindow session;
	session.searchEngine = getSearchEngine();
	session.history = history.entries;
	session.group = 0;
	session.index = history.index;
	session.pinned = isPinned();

	if (m_contentsWidget->getType() == QLatin1String("web"))
	{
		WebContentsWidget *webWidget = qobject_cast<WebContentsWidget*>(m_contentsWidget);

		if (webWidget)
		{
			if (webWidget->getReloadTime() != -1)
			{
				session.reloadTime = webWidget->getReloadTime();
			}

			if (webWidget->getWebWidget()->hasOption(QLatin1String("Network/UserAgent")))
			{
				session.userAgent = webWidget->getWebWidget()->getOption(QLatin1String("Network/UserAgent")).toString();
			}
		}
	}

	return session;
}

WindowLoadingState Window::getLoadingState() const
{
	return (m_contentsWidget ? (m_contentsWidget->isLoading() ? LoadingState : LoadedState) : DelayedState);
}

qint64 Window::getIdentifier() const
{
	return m_identifier;
}

bool Window::canClone() const
{
	return (m_contentsWidget ? m_contentsWidget->canClone() : false);
}

bool Window::isPinned() const
{
	return m_isPinned;
}

bool Window::isPrivate() const
{
	return (m_contentsWidget ? m_contentsWidget->isPrivate() : m_isPrivate);
}

bool Window::isUrlEmpty() const
{
	const QUrl url = getUrl();

	return (url.isEmpty() || (url.scheme() == QLatin1String("about") && (url.path().isEmpty() || url.path() == QLatin1String("blank") || url.path() == QLatin1String("start"))));
}

}
