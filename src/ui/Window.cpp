/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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
#include "OpenAddressDialog.h"
#include "ToolBarWidget.h"
#include "WebWidget.h"
#include "toolbars/AddressWidget.h"
#include "toolbars/SearchWidget.h"
#include "../core/AddonsManager.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"
#include "../core/WebBackend.h"
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
	m_contentsWidget(NULL),
	m_identifier(++m_identifierCounter),
	m_areControlsHidden(false),
	m_isPinned(false),
	m_isPrivate(isPrivate)
{
	QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

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

	if (Utils::isUrlEmpty(getUrl()) && !m_contentsWidget->isLoading() && !m_addressWidgets.isEmpty() && m_addressWidgets.at(0))
	{
		m_addressWidgets.at(0)->setFocus();
	}
	else if (m_contentsWidget)
	{
		m_contentsWidget->setFocus();
	}
}

void Window::triggerAction(int identifier, bool checked)
{
	if (identifier == Action::ActivateSearchFieldAction && !m_searchWidgets.isEmpty() && m_searchWidgets.at(0))
	{
		m_searchWidgets.at(0)->setFocus(Qt::ShortcutFocusReason);
	}
	else if (!m_addressWidgets.isEmpty() && m_addressWidgets.at(0))
	{
		if (identifier == Action::ActivateAddressFieldAction)
		{
			m_addressWidgets.at(0)->setFocus(Qt::ShortcutFocusReason);
		}
		else if (identifier == Action::ActivateSearchFieldAction)
		{
			m_addressWidgets.at(0)->setText(QLatin1String("? "));
			m_addressWidgets.at(0)->setFocus(Qt::OtherFocusReason);
		}
		else if (identifier == Action::PasteAndGoAction)
		{
			if (!QApplication::clipboard()->text().isEmpty())
			{
				m_addressWidgets.at(0)->handleUserInput(QApplication::clipboard()->text().trimmed());
			}

			return;
		}
		else if (identifier == Action::GoAction)
		{
			m_addressWidgets.at(0)->handleUserInput(m_addressWidgets.at(0)->text());

			return;
		}
	}
	else if (identifier == Action::ActivateAddressFieldAction || identifier == Action::ActivateSearchFieldAction)
	{
		OpenAddressDialog dialog(this);

		if (identifier == Action::ActivateSearchFieldAction)
		{
			dialog.setText(QLatin1String("? "));
		}

		connect(&dialog, SIGNAL(requestedLoadUrl(QUrl,OpenHints)), this, SLOT(handleOpenUrlRequest(QUrl,OpenHints)));
		connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)), this, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)));
		connect(&dialog, SIGNAL(requestedSearch(QString,QString,OpenHints)), this, SLOT(handleSearchRequest(QString,QString,OpenHints)));

		dialog.exec();
	}

	if (identifier == Action::GoToParentDirectoryAction && getContentsWidget()->getType() == QLatin1String("web"))
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
	else if (identifier == Action::PrintAction)
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
	else if (identifier == Action::PrintPreviewAction)
	{
		QPrintPreviewDialog printPreviewDialog(this);
		printPreviewDialog.setWindowFlags(printPreviewDialog.windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
		printPreviewDialog.setWindowTitle(tr("Print Preview"));

		if (QApplication::activeWindow())
		{
			printPreviewDialog.resize(QApplication::activeWindow()->size());
		}

		connect(&printPreviewDialog, SIGNAL(paintRequested(QPrinter*)), getContentsWidget(), SLOT(print(QPrinter*)));

		printPreviewDialog.exec();
	}
	else
	{
		getContentsWidget()->triggerAction(identifier, checked);
	}
}

void Window::clear()
{
	if (!m_addressWidgets.isEmpty())
	{
		for (int i = 0; i < m_addressWidgets.count(); ++i)
		{
			if (m_addressWidgets.at(i))
			{
				m_addressWidgets.at(i)->clear();
			}
		}
	}

	setContentsWidget(new WebContentsWidget(m_isPrivate, NULL, this));
}

void Window::attachAddressWidget(AddressWidget *widget)
{
	m_addressWidgets.append(widget);
}

void Window::detachAddressWidget(AddressWidget *widget)
{
	m_addressWidgets.removeAll(widget);
}

void Window::attachSearchWidget(SearchWidget *widget)
{
	m_searchWidgets.append(widget);
}

void Window::detachSearchWidget(SearchWidget *widget)
{
	m_searchWidgets.removeAll(widget);
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

	if (!m_addressWidgets.isEmpty())
	{
		for (int i = 0; i < m_addressWidgets.count(); ++i)
		{
			if (m_addressWidgets.at(i))
			{
				m_addressWidgets.at(i)->clearFocus();
			}
		}
	}

	widget->search(query, engine);

	if (!m_addressWidgets.isEmpty())
	{
		for (int i = 0; i < m_addressWidgets.count(); ++i)
		{
			if (m_addressWidgets.at(i))
			{
				m_addressWidgets.at(i)->setUrl(getUrl());
			}
		}
	}
}

void Window::markActive()
{
	m_lastActivity = QDateTime::currentDateTime();
}

void Window::handleOpenUrlRequest(const QUrl &url, OpenHints hints)
{
	if (getType() == QLatin1String("web") && (hints == DefaultOpen || hints == CurrentTabOpen))
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
	if ((getType() == QLatin1String("web") && Utils::isUrlEmpty(getUrl())) || (hints == DefaultOpen || hints == CurrentTabOpen))
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
	if (engine == m_searchEngine)
	{
		return;
	}

	m_searchEngine = engine;

	emit searchEngineChanged(engine);
}

void Window::setUrl(const QUrl &url, bool typed)
{
	ContentsWidget *newWidget = NULL;

	if (url.scheme() == QLatin1String("about"))
	{
		if (!m_addressWidgets.isEmpty() && m_session.index < 0 && !Utils::isUrlEmpty(getUrl()) && SessionsManager::hasUrl(url, true))
		{
			for (int i = 0; i < m_addressWidgets.count(); ++i)
			{
				if (m_addressWidgets.at(i))
				{
					m_addressWidgets.at(i)->setUrl(m_contentsWidget ? m_contentsWidget->getUrl() : m_session.getUrl());
				}
			}

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

		if (!m_addressWidgets.isEmpty())
		{
			for (int i = 0; i < m_addressWidgets.count(); ++i)
			{
				if (m_addressWidgets.at(i))
				{
					if (!Utils::isUrlEmpty(getUrl()) || m_contentsWidget->isLoading())
					{
						m_addressWidgets.at(i)->clearFocus();
					}

					m_addressWidgets.at(i)->setUrl(url);
				}
			}
		}
	}
}

void Window::setControlsHidden(bool hidden)
{
	m_areControlsHidden = hidden;

	if (m_navigationBar)
	{
		m_navigationBar->setVisible(!m_areControlsHidden);
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
		m_navigationBar = new ToolBarWidget(ActionsManager::getToolBarDefinition(QLatin1String("NavigationBar")), this, this);
		m_navigationBar->setVisible(!m_areControlsHidden);

		layout()->addWidget(m_navigationBar);
	}
	else if (m_contentsWidget->getType() != QLatin1String("web") && m_navigationBar)
	{
		m_navigationBar->deleteLater();

		layout()->removeWidget(m_navigationBar);

		m_navigationBar = NULL;
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
				webWidget->setOption(QLatin1String("Content/PageReloadTime"), m_session.reloadTime);
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
		if (Utils::isUrlEmpty(getUrl()) && !m_addressWidgets.isEmpty() && m_addressWidgets.at(0))
		{
			m_addressWidgets.at(0)->setFocus();
		}

		if (m_contentsWidget->getUrl().scheme() == QLatin1String("about") || Utils::isUrlEmpty(getUrl()))
		{
			emit titleChanged(m_contentsWidget->getTitle());
		}
	}

	emit canZoomChanged(m_contentsWidget->canZoom());
	emit iconChanged(m_contentsWidget->getIcon());

	connect(this, SIGNAL(aboutToClose()), m_contentsWidget, SLOT(close()));
	connect(m_contentsWidget, SIGNAL(requestedAddBookmark(QUrl,QString,QString)), this, SIGNAL(requestedAddBookmark(QUrl,QString,QString)));
	connect(m_contentsWidget, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SIGNAL(requestedOpenUrl(QUrl,OpenHints)));
	connect(m_contentsWidget, SIGNAL(requestedNewWindow(ContentsWidget*,OpenHints)), this, SIGNAL(requestedNewWindow(ContentsWidget*,OpenHints)));
	connect(m_contentsWidget, SIGNAL(requestedSearch(QString,QString,OpenHints)), this, SIGNAL(requestedSearch(QString,QString,OpenHints)));
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
	return m_searchEngine;
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
	return (m_contentsWidget ? m_contentsWidget->getIcon() : AddonsManager::getWebBackend()->getIconForUrl(m_session.getUrl()));
}

QPixmap Window::getThumbnail() const
{
	return (m_contentsWidget ? m_contentsWidget->getThumbnail() : QPixmap());
}

QDateTime Window::getLastActivity() const
{
	return m_lastActivity;
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
			const int reloadTime = webWidget->getOption(QLatin1String("Content/PageReloadTime")).toInt();

			if (reloadTime >= 0)
			{
				session.reloadTime = reloadTime;
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

}
