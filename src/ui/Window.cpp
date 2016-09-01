/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "MainWindow.h"
#include "OpenAddressDialog.h"
#include "ToolBarWidget.h"
#include "WebWidget.h"
#include "toolbars/AddressWidget.h"
#include "toolbars/SearchWidget.h"
#include "../core/HistoryManager.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"
#include "../modules/windows/addons/AddonsContentsWidget.h"
#include "../modules/windows/bookmarks/BookmarksContentsWidget.h"
#include "../modules/windows/cache/CacheContentsWidget.h"
#include "../modules/windows/cookies/CookiesContentsWidget.h"
#include "../modules/windows/configuration/ConfigurationContentsWidget.h"
#include "../modules/windows/history/HistoryContentsWidget.h"
#include "../modules/windows/notes/NotesContentsWidget.h"
#include "../modules/windows/transfers/TransfersContentsWidget.h"
#include "../modules/windows/web/WebContentsWidget.h"

#include <QtCore/QTimer>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>

namespace Otter
{

quint64 Window::m_identifierCounter = 0;

Window::Window(bool isPrivate, ContentsWidget *widget, QWidget *parent) : QWidget(parent),
	m_navigationBar(NULL),
	m_contentsWidget(NULL),
	m_identifier(++m_identifierCounter),
	m_areControlsHidden(false),
	m_isAboutToClose(false),
	m_isPinned(false),
	m_isPrivate(isPrivate)
{
	QBoxLayout *layout(new QBoxLayout(QBoxLayout::TopToBottom, this));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	setLayout(layout);

	if (widget)
	{
		widget->setParent(this);

		setContentsWidget(widget);
	}

	connect(this, SIGNAL(titleChanged(QString)), this, SLOT(setWindowTitle(QString)));
	connect(this, SIGNAL(iconChanged(QIcon)), this, SLOT(handleIconChanged(QIcon)));
}

void Window::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);

	AddressWidget *addressWidget(findAddressWidget());

	if (Utils::isUrlEmpty(getUrl()) && m_contentsWidget->getLoadingState() != WindowsManager::OngoingLoadingState && addressWidget)
	{
		addressWidget->setFocus();
	}
	else if (m_contentsWidget)
	{
		m_contentsWidget->setFocus();
	}
}

void Window::triggerAction(int identifier, const QVariantMap &parameters)
{
	if (parameters.contains(QLatin1String("isBounced")))
	{
		AddressWidget *addressWidget(NULL);
		SearchWidget *searchWidget(NULL);

		if (identifier == ActionsManager::ActivateAddressFieldAction || identifier == ActionsManager::ActivateSearchFieldAction)
		{
			addressWidget = findAddressWidget();

			for (int i = 0; i < m_searchWidgets.count(); ++i)
			{
				if (m_searchWidgets.at(i) && m_searchWidgets.at(i)->isVisible())
				{
					searchWidget = m_searchWidgets.at(i);

					break;
				}
			}
		}

		if (identifier == ActionsManager::ActivateSearchFieldAction && searchWidget)
		{
			searchWidget->activate(Qt::ShortcutFocusReason);
		}
		else if (addressWidget)
		{
			if (identifier == ActionsManager::ActivateAddressFieldAction)
			{
				addressWidget->activate(Qt::ShortcutFocusReason);
			}
			else if (identifier == ActionsManager::ActivateSearchFieldAction)
			{
				addressWidget->setText(QLatin1String("? "));
				addressWidget->activate(Qt::OtherFocusReason);
			}
			else if (identifier == ActionsManager::GoAction)
			{
				addressWidget->handleUserInput(addressWidget->getText(), WindowsManager::CurrentTabOpen);

				return;
			}
		}
		else if (identifier == ActionsManager::ActivateAddressFieldAction || identifier == ActionsManager::ActivateSearchFieldAction)
		{
			OpenAddressDialog dialog(this);

			if (identifier == ActionsManager::ActivateSearchFieldAction)
			{
				dialog.setText(QLatin1String("? "));
			}

			connect(&dialog, SIGNAL(requestedLoadUrl(QUrl,WindowsManager::OpenHints)), this, SLOT(handleOpenUrlRequest(QUrl,WindowsManager::OpenHints)));
			connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), this, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)));
			connect(&dialog, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), this, SLOT(handleSearchRequest(QString,QString,WindowsManager::OpenHints)));

			dialog.exec();
		}
	}

	switch (identifier)
	{
		case ActionsManager::SuspendTabAction:
			if (m_contentsWidget)
			{
				m_session = getSession();

				setContentsWidget(NULL);
			}

			break;
		case ActionsManager::PrintAction:
			{
				QPrinter printer;
				printer.setCreator(QStringLiteral("Otter Browser %1").arg(Application::getInstance()->getFullVersion()));

				QPrintDialog printDialog(&printer, this);
				printDialog.setWindowTitle(tr("Print Page"));

				if (printDialog.exec() != QDialog::Accepted)
				{
					return;
				}

				getContentsWidget()->print(&printer);
			}

			break;
		case ActionsManager::PrintPreviewAction:
			{
				QPrintPreviewDialog printPreviewDialog(this);
				printPreviewDialog.printer()->setCreator(QStringLiteral("Otter Browser %1").arg(Application::getInstance()->getFullVersion()));
				printPreviewDialog.setWindowFlags(printPreviewDialog.windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
				printPreviewDialog.setWindowTitle(tr("Print Preview"));

				if (QApplication::activeWindow())
				{
					printPreviewDialog.resize(QApplication::activeWindow()->size());
				}

				connect(&printPreviewDialog, SIGNAL(paintRequested(QPrinter*)), getContentsWidget(), SLOT(print(QPrinter*)));

				printPreviewDialog.exec();
			}

			break;
		default:
			getContentsWidget()->triggerAction(identifier, parameters);

			break;
	}
}

void Window::clear()
{
	setContentsWidget(new WebContentsWidget(m_isPrivate, NULL, this));

	m_isAboutToClose = false;

	emit urlChanged(getUrl(), true);
}

void Window::attachAddressWidget(AddressWidget *widget)
{
	if (!m_addressWidgets.contains(widget))
	{
		m_addressWidgets.append(widget);
	}
}

void Window::detachAddressWidget(AddressWidget *widget)
{
	m_addressWidgets.removeAll(widget);
}

void Window::attachSearchWidget(SearchWidget *widget)
{
	if (!m_searchWidgets.contains(widget))
	{
		m_searchWidgets.append(widget);
	}
}

void Window::detachSearchWidget(SearchWidget *widget)
{
	m_searchWidgets.removeAll(widget);
}

void Window::close()
{
	m_isAboutToClose = true;

	emit aboutToClose();

	QTimer::singleShot(50, this, SLOT(notifyRequestedCloseWindow()));
}

void Window::search(const QString &query, const QString &searchEngine)
{
	WebContentsWidget *widget(qobject_cast<WebContentsWidget*>(m_contentsWidget));

	if (!widget)
	{
		widget = new WebContentsWidget(isPrivate(), NULL, this);

		setContentsWidget(widget);
	}

	widget->search(query, searchEngine);

	emit urlChanged(getUrl(), true);
}

void Window::markActive()
{
	if (!m_contentsWidget)
	{
		setUrl(m_session.getUrl(), false);
	}

	m_lastActivity = QDateTime::currentDateTime();
}

void Window::handleIconChanged(const QIcon &icon)
{
	QMdiSubWindow *subWindow(qobject_cast<QMdiSubWindow*>(parentWidget()));

	if (subWindow)
	{
		subWindow->setWindowIcon(icon);
	}
}

void Window::handleOpenUrlRequest(const QUrl &url, WindowsManager::OpenHints hints)
{
	if (hints == WindowsManager::DefaultOpen || hints == WindowsManager::CurrentTabOpen)
	{
		setUrl(url);

		return;
	}

	if (isPrivate())
	{
		hints |= WindowsManager::PrivateOpen;
	}

	emit requestedOpenUrl(url, hints);
}

void Window::handleSearchRequest(const QString &query, const QString &searchEngine, WindowsManager::OpenHints hints)
{
	if ((getType() == QLatin1String("web") && Utils::isUrlEmpty(getUrl())) || (hints == WindowsManager::DefaultOpen || hints == WindowsManager::CurrentTabOpen))
	{
		search(query, searchEngine);
	}
	else
	{
		emit requestedSearch(query, searchEngine, hints);
	}
}

void Window::handleGeometryChangeRequest(const QRect &geometry)
{
	QVariantMap parameters;
	parameters[QLatin1String("window")] = getIdentifier();

	ActionsManager::triggerAction(ActionsManager::RestoreTabAction, MainWindow::findMainWindow(this), parameters);

	QMdiSubWindow *subWindow(qobject_cast<QMdiSubWindow*>(parentWidget()));

	if (subWindow)
	{
		subWindow->setWindowFlags(Qt::SubWindow);
		subWindow->showNormal();
		subWindow->resize((geometry.size() * (static_cast<qreal>(getContentsWidget()->getZoom()) / 100)) + (subWindow->geometry().size() - m_contentsWidget->size()));
		subWindow->move(geometry.topLeft());
	}
}

void Window::notifyRequestedCloseWindow()
{
	emit requestedCloseWindow(this);
}

void Window::updateNavigationBar()
{
	if (m_navigationBar)
	{
		m_navigationBar->reload();
	}
}

void Window::setSession(const SessionWindow &session)
{
	m_session = session;

	setSearchEngine(session.overrides.value(SettingsManager::Search_DefaultSearchEngineOption, QString()).toString());
	setPinned(session.isPinned);

	if (SettingsManager::getValue(SettingsManager::Browser_DelayRestoringOfBackgroundTabsOption).toBool())
	{
		setWindowTitle(session.getTitle());
	}
	else
	{
		setUrl(session.getUrl(), false);
	}
}

void Window::setSearchEngine(const QString &searchEngine)
{
	if (searchEngine == m_searchEngine)
	{
		return;
	}

	m_searchEngine = searchEngine;

	emit searchEngineChanged(searchEngine);
}

void Window::setUrl(const QUrl &url, bool typed)
{
	ContentsWidget *newWidget(NULL);

	if (url.scheme() == QLatin1String("about"))
	{
		if (m_session.historyIndex < 0 && !Utils::isUrlEmpty(getUrl()) && SessionsManager::hasUrl(url, true))
		{
			emit urlChanged(url, true);

			return;
		}

		if (url.path() == QLatin1String("addons"))
		{
			newWidget = new AddonsContentsWidget(this);
		}
		else if (url.path() == QLatin1String("bookmarks"))
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
		else if (url.path() == QLatin1String("notes"))
		{
			newWidget = new NotesContentsWidget(this);
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

	const bool isRestoring(!m_contentsWidget && m_session.historyIndex >= 0);

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

		if (!Utils::isUrlEmpty(getUrl()) || m_contentsWidget->getLoadingState() == WindowsManager::OngoingLoadingState)
		{
			emit urlChanged(url, true);
		}
	}
}

void Window::setControlsHidden(bool hidden)
{
	m_areControlsHidden = hidden;

	if (m_navigationBar)
	{
		if (hidden)
		{
			m_navigationBar->hide();
		}
		else if (ToolBarsManager::getToolBarDefinition(ToolBarsManager::NavigationBar).normalVisibility != ToolBarsManager::AlwaysHiddenToolBar)
		{
			m_navigationBar->show();
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
		if (m_navigationBar)
		{
			layout()->removeWidget(m_navigationBar);

			m_navigationBar->deleteLater();
			m_navigationBar = NULL;
		}

		emit widgetChanged();

		return;
	}

	if (!m_navigationBar)
	{
		m_navigationBar = new ToolBarWidget(ToolBarsManager::NavigationBar, this, this);
		m_navigationBar->setVisible(!m_areControlsHidden && ToolBarsManager::getToolBarDefinition(ToolBarsManager::NavigationBar).normalVisibility != ToolBarsManager::AlwaysHiddenToolBar);

		layout()->addWidget(m_navigationBar);
	}

	layout()->addWidget(m_contentsWidget);

	if (m_session.historyIndex >= 0)
	{
		if (m_contentsWidget->getType() == QLatin1String("web"))
		{
			WebContentsWidget *webWidget(qobject_cast<WebContentsWidget*>(m_contentsWidget));

			if (webWidget)
			{
				webWidget->setOptions(m_session.overrides);
			}
		}

		WindowHistoryInformation history;
		history.index = m_session.historyIndex;
		history.entries = m_session.history;

		m_contentsWidget->setHistory(history);
		m_contentsWidget->setZoom(m_session.getZoom());
		m_contentsWidget->setFocus();

		m_session = SessionWindow();
	}
	else
	{
		AddressWidget *addressWidget(findAddressWidget());

		if (Utils::isUrlEmpty(getUrl()) && addressWidget)
		{
			addressWidget->setFocus();
		}

		if (m_contentsWidget->getUrl().scheme() == QLatin1String("about") || Utils::isUrlEmpty(getUrl()))
		{
			emit titleChanged(m_contentsWidget->getTitle());
		}
	}

	emit widgetChanged();
	emit titleChanged(m_contentsWidget->getTitle());
	emit iconChanged(m_contentsWidget->getIcon());
	emit canZoomChanged(m_contentsWidget->canZoom());

	connect(this, SIGNAL(aboutToClose()), m_contentsWidget, SLOT(close()));
	connect(m_contentsWidget, SIGNAL(webWidgetChanged()), this, SLOT(updateNavigationBar()));
	connect(m_contentsWidget, SIGNAL(requestedAddBookmark(QUrl,QString,QString)), this, SIGNAL(requestedAddBookmark(QUrl,QString,QString)));
	connect(m_contentsWidget, SIGNAL(requestedEditBookmark(QUrl)), this, SIGNAL(requestedEditBookmark(QUrl)));
	connect(m_contentsWidget, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)));
	connect(m_contentsWidget, SIGNAL(requestedNewWindow(ContentsWidget*,WindowsManager::OpenHints)), this, SIGNAL(requestedNewWindow(ContentsWidget*,WindowsManager::OpenHints)));
	connect(m_contentsWidget, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)));
	connect(m_contentsWidget, SIGNAL(requestedGeometryChange(QRect)), this, SLOT(handleGeometryChangeRequest(QRect)));
	connect(m_contentsWidget, SIGNAL(statusMessageChanged(QString)), this, SIGNAL(statusMessageChanged(QString)));
	connect(m_contentsWidget, SIGNAL(titleChanged(QString)), this, SIGNAL(titleChanged(QString)));
	connect(m_contentsWidget, SIGNAL(urlChanged(QUrl)), this, SIGNAL(urlChanged(QUrl)));
	connect(m_contentsWidget, SIGNAL(iconChanged(QIcon)), this, SIGNAL(iconChanged(QIcon)));
	connect(m_contentsWidget, SIGNAL(contentStateChanged(WindowsManager::ContentStates)), this, SIGNAL(contentStateChanged(WindowsManager::ContentStates)));
	connect(m_contentsWidget, SIGNAL(loadingStateChanged(WindowsManager::LoadingState)), this, SIGNAL(loadingStateChanged(WindowsManager::LoadingState)));
	connect(m_contentsWidget, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
	connect(m_contentsWidget, SIGNAL(canZoomChanged(bool)), this, SIGNAL(canZoomChanged(bool)));
}

AddressWidget* Window::findAddressWidget() const
{
	for (int i = 0; i < m_addressWidgets.count(); ++i)
	{
		if (m_addressWidgets.at(i) && m_addressWidgets.at(i)->isVisible())
		{
			return m_addressWidgets.at(i);
		}
	}

	return m_addressWidgets.value(0, NULL);
}

Window* Window::clone(bool cloneHistory, QWidget *parent)
{
	if (!m_contentsWidget || !canClone())
	{
		return NULL;
	}

	Window *window(new Window(false, m_contentsWidget->clone(cloneHistory), parent));
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
	return (m_contentsWidget ? m_contentsWidget->getIcon() : HistoryManager::getIcon(m_session.getUrl()));
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
	SessionWindow session;

	if (m_contentsWidget)
	{
		const WindowHistoryInformation history(m_contentsWidget->getHistory());

		if (m_contentsWidget->getType() == QLatin1String("web"))
		{
			WebContentsWidget *webWidget(qobject_cast<WebContentsWidget*>(m_contentsWidget));

			if (webWidget)
			{
				session.overrides = webWidget->getOptions();
			}
		}

		session.overrides[SettingsManager::Search_DefaultSearchEngineOption] = getSearchEngine();
		session.history = history.entries;
		session.parentGroup = 0;
		session.historyIndex = history.index;
		session.isPinned = isPinned();
	}
	else
	{
		session = m_session;
	}

	QMdiSubWindow *subWindow(qobject_cast<QMdiSubWindow*>(parentWidget()));

	if (subWindow)
	{
		if (subWindow->isMaximized())
		{
			session.state = MaximizedWindowState;
			session.geometry = QRect();
		}
		else if (subWindow->isMinimized())
		{
			session.state = MinimizedWindowState;
			session.geometry = QRect();
		}
		else
		{
			session.state = NormalWindowState;
			session.geometry = subWindow->geometry();
		}
	}
	else
	{
		session.state = MaximizedWindowState;
		session.geometry = QRect();
	}

	return session;
}

QSize Window::sizeHint() const
{
	return QSize(800, 600);
}

WindowsManager::LoadingState Window::getLoadingState() const
{
	return (m_contentsWidget ? m_contentsWidget->getLoadingState() : WindowsManager::DelayedLoadingState);
}

WindowsManager::ContentStates Window::getContentState() const
{
	return (m_contentsWidget ? m_contentsWidget->getContentState() : WindowsManager::UnknownContentState);
}

quint64 Window::getIdentifier() const
{
	return m_identifier;
}

bool Window::canClone() const
{
	return (m_contentsWidget ? m_contentsWidget->canClone() : false);
}

bool Window::isAboutToClose() const
{
	return m_isAboutToClose;
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
