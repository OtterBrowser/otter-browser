/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2016 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "Window.h"
#include "MainWindow.h"
#include "OpenAddressDialog.h"
#include "ToolBarWidget.h"
#include "WorkspaceWidget.h"
#include "../core/HistoryManager.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"
#include "../modules/widgets/address/AddressWidget.h"
#include "../modules/widgets/search/SearchWidget.h"
#include "../modules/windows/addons/AddonsContentsWidget.h"
#include "../modules/windows/bookmarks/BookmarksContentsWidget.h"
#include "../modules/windows/cache/CacheContentsWidget.h"
#include "../modules/windows/cookies/CookiesContentsWidget.h"
#include "../modules/windows/configuration/ConfigurationContentsWidget.h"
#include "../modules/windows/history/HistoryContentsWidget.h"
#include "../modules/windows/notes/NotesContentsWidget.h"
#include "../modules/windows/passwords/PasswordsContentsWidget.h"
#include "../modules/windows/transfers/TransfersContentsWidget.h"
#include "../modules/windows/web/WebContentsWidget.h"
#include "../modules/windows/windows/WindowsContentsWidget.h"

#include <QtCore/QTimer>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QMdiSubWindow>

namespace Otter
{

quint64 Window::m_identifierCounter(0);

Window::Window(const QVariantMap &parameters, ContentsWidget *widget, MainWindow *mainWindow) : QWidget(mainWindow->getWorkspace()),
	m_mainWindow(mainWindow),
	m_navigationBar(nullptr),
	m_contentsWidget(nullptr),
	m_parameters(parameters),
	m_identifier(++m_identifierCounter),
	m_suspendTimer(0),
	m_areToolBarsVisible(true),
	m_isAboutToClose(false),
	m_isPinned(false)
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

void Window::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_suspendTimer)
	{
		killTimer(m_suspendTimer);

		m_suspendTimer = 0;

		triggerAction(ActionsManager::SuspendTabAction);
	}
}

void Window::hideEvent(QHideEvent *event)
{
	QWidget::hideEvent(event);

	const int suspendTime(SettingsManager::getOption(SettingsManager::Browser_InactiveTabTimeUntilSuspendOption).toInt());

	if (suspendTime >= 0)
	{
		m_suspendTimer = startTimer(suspendTime * 1000);
	}
}

void Window::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);

	if (m_suspendTimer > 0)
	{
		killTimer(m_suspendTimer);

		m_suspendTimer = 0;
	}

	AddressWidget *addressWidget(findAddressWidget());

	if (Utils::isUrlEmpty(getUrl()) && m_contentsWidget->getLoadingState() != WebWidget::OngoingLoadingState && addressWidget)
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
		AddressWidget *addressWidget(nullptr);
		SearchWidget *searchWidget(nullptr);

		if (identifier == ActionsManager::ActivateAddressFieldAction || identifier == ActionsManager::ActivateSearchFieldAction || identifier == ActionsManager::GoAction)
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
				addressWidget->handleUserInput(addressWidget->text(), SessionsManager::CurrentTabOpen);

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

			connect(&dialog, SIGNAL(requestedLoadUrl(QUrl,SessionsManager::OpenHints)), this, SLOT(handleOpenUrlRequest(QUrl,SessionsManager::OpenHints)));
			connect(&dialog, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)), this, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)));
			connect(&dialog, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)), this, SLOT(handleSearchRequest(QString,QString,SessionsManager::OpenHints)));

			dialog.exec();
		}
	}

	switch (identifier)
	{
		case ActionsManager::SuspendTabAction:
			if (m_contentsWidget)
			{
				m_session = getSession();

				setContentsWidget(nullptr);
			}

			break;
		case ActionsManager::PrintAction:
			{
				QPrinter printer;
				printer.setCreator(QStringLiteral("Otter Browser %1").arg(Application::getFullVersion()));

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
				printPreviewDialog.printer()->setCreator(QStringLiteral("Otter Browser %1").arg(Application::getFullVersion()));
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
	setContentsWidget(new WebContentsWidget(m_parameters, QHash<int, QVariant>(), nullptr, this));

	m_isAboutToClose = false;

	emit urlChanged(getUrl(), true);
}

void Window::attachAddressWidget(AddressWidget *widget)
{
	if (!m_addressWidgets.contains(widget))
	{
		m_addressWidgets.append(widget);

		if (widget->isVisible() && isActive() && Utils::isUrlEmpty(m_contentsWidget->getUrl()))
		{
			AddressWidget *addressWidget(qobject_cast<AddressWidget*>(QApplication::focusWidget()));

			if (!addressWidget)
			{
				widget->setFocus();
			}
		}
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
		QVariantMap parameters;

		if (isPrivate())
		{
			parameters[QLatin1String("hints")] = SessionsManager::PrivateOpen;
		}

		widget = new WebContentsWidget(parameters, QHash<int, QVariant>(), nullptr, this);

		setContentsWidget(widget);
	}

	widget->search(query, searchEngine);

	emit urlChanged(getUrl(), true);
}

void Window::markAsActive()
{
	if (m_suspendTimer > 0)
	{
		killTimer(m_suspendTimer);

		m_suspendTimer = 0;
	}

	if (!m_contentsWidget)
	{
		setUrl(m_session.getUrl(), false);
	}

	m_lastActivity = QDateTime::currentDateTime();

	emit activated();
}

void Window::handleIconChanged(const QIcon &icon)
{
	QMdiSubWindow *subWindow(qobject_cast<QMdiSubWindow*>(parentWidget()));

	if (subWindow)
	{
		subWindow->setWindowIcon(icon);
	}
}

void Window::handleOpenUrlRequest(const QUrl &url, SessionsManager::OpenHints hints)
{
	if (hints == SessionsManager::DefaultOpen || hints == SessionsManager::CurrentTabOpen)
	{
		setUrl(url);

		return;
	}

	if (isPrivate())
	{
		hints |= SessionsManager::PrivateOpen;
	}

	emit requestedOpenUrl(url, hints);
}

void Window::handleSearchRequest(const QString &query, const QString &searchEngine, SessionsManager::OpenHints hints)
{
	if ((getType() == QLatin1String("web") && Utils::isUrlEmpty(getUrl())) || (hints == SessionsManager::DefaultOpen || hints == SessionsManager::CurrentTabOpen))
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
	ActionsManager::triggerAction(ActionsManager::RestoreTabAction, m_mainWindow, {{QLatin1String("window"), getIdentifier()}});

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

	setPinned(session.isPinned);

	if (SettingsManager::getOption(SettingsManager::Browser_DelayRestoringOfBackgroundTabsOption).toBool())
	{
		setWindowTitle(session.getTitle());
	}
	else
	{
		setUrl(session.getUrl(), false);
	}
}

void Window::setOption(int identifier, const QVariant &value)
{
	if (m_contentsWidget)
	{
		m_contentsWidget->setOption(identifier, value);
	}
	else if (value != m_session.options.value(identifier))
	{
		if (value.isNull())
		{
			m_session.options.remove(identifier);
		}
		else
		{
			m_session.options[identifier] = value;
		}

		SessionsManager::markSessionModified();

		emit optionChanged(identifier, value);
	}
}

void Window::setUrl(const QUrl &url, bool isTyped)
{
	ContentsWidget *newWidget(nullptr);

	if (url.scheme() == QLatin1String("about"))
	{
		if (m_session.historyIndex < 0 && !Utils::isUrlEmpty(getUrl()) && SessionsManager::hasUrl(url, true))
		{
			emit urlChanged(url, true);

			return;
		}

		if (url.path() == QLatin1String("addons"))
		{
			newWidget = new AddonsContentsWidget(QVariantMap(), this);
		}
		else if (url.path() == QLatin1String("bookmarks"))
		{
			newWidget = new BookmarksContentsWidget(QVariantMap(), this);
		}
		else if (url.path() == QLatin1String("cache"))
		{
			newWidget = new CacheContentsWidget(QVariantMap(), this);
		}
		else if (url.path() == QLatin1String("config"))
		{
			newWidget = new ConfigurationContentsWidget(QVariantMap(), this);
		}
		else if (url.path() == QLatin1String("cookies"))
		{
			newWidget = new CookiesContentsWidget(QVariantMap(), this);
		}
		else if (url.path() == QLatin1String("history"))
		{
			newWidget = new HistoryContentsWidget(QVariantMap(), this);
		}
		else if (url.path() == QLatin1String("notes"))
		{
			newWidget = new NotesContentsWidget(QVariantMap(), this);
		}
		else if (url.path() == QLatin1String("passwords"))
		{
			newWidget = new PasswordsContentsWidget(QVariantMap(), this);
		}
		else if (url.path() == QLatin1String("transfers"))
		{
			newWidget = new TransfersContentsWidget(QVariantMap(), this);
		}
		else if (url.path() == QLatin1String("windows"))
		{
			newWidget = new WindowsContentsWidget(QVariantMap(), this);
		}

		if (newWidget && !newWidget->canClone())
		{
			SessionsManager::removeStoredUrl(newWidget->getUrl().toString());
		}
	}

	const bool isRestoring(!m_contentsWidget && m_session.historyIndex >= 0);

	if (!newWidget && (!m_contentsWidget || m_contentsWidget->getType() != QLatin1String("web")))
	{
		newWidget = new WebContentsWidget(m_parameters, m_session.options, nullptr, this);
	}

	if (newWidget)
	{
		setContentsWidget(newWidget);
	}

	if (m_contentsWidget && url.isValid())
	{
		if (!isRestoring)
		{
			m_contentsWidget->setUrl(url, isTyped);
		}

		if (!Utils::isUrlEmpty(getUrl()) || m_contentsWidget->getLoadingState() == WebWidget::OngoingLoadingState)
		{
			emit urlChanged(url, true);
		}
	}
}

void Window::setZoom(int zoom)
{
	if (m_contentsWidget)
	{
		m_contentsWidget->setZoom(zoom);
	}
	else if (m_session.historyIndex >= 0 && m_session.historyIndex < m_session.history.count())
	{
		m_session.history[m_session.historyIndex].zoom = zoom;
	}
}

void Window::setToolBarsVisible(bool areVisible)
{
	m_areToolBarsVisible = areVisible;

	if (m_navigationBar)
	{
		if (areVisible && ToolBarsManager::getToolBarDefinition(ToolBarsManager::NavigationBar).normalVisibility != ToolBarsManager::AlwaysHiddenToolBar)
		{
			m_navigationBar->show();
		}
		else
		{
			m_navigationBar->hide();
		}
	}
}

void Window::setPinned(bool isPinned)
{
	if (isPinned != m_isPinned)
	{
		m_isPinned = isPinned;

		emit isPinnedChanged(isPinned);
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
			m_navigationBar = nullptr;
		}

		emit widgetChanged();

		return;
	}

	if (!m_navigationBar)
	{
		m_navigationBar = new ToolBarWidget(ToolBarsManager::NavigationBar, this, this);
		m_navigationBar->setVisible(m_areToolBarsVisible && m_navigationBar->getDefinition().normalVisibility != ToolBarsManager::AlwaysHiddenToolBar);

		layout()->addWidget(m_navigationBar);
	}

	layout()->addWidget(m_contentsWidget);

	WindowHistoryInformation history;

	if (m_session.historyIndex >= 0)
	{
		history.index = m_session.historyIndex;
		history.entries = m_session.history;
	}

	m_contentsWidget->setHistory(history);
	m_contentsWidget->setZoom(m_session.getZoom());

	if (isActive())
	{
		if (m_session.historyIndex >= 0)
		{
			m_contentsWidget->setFocus();
		}
		else
		{
			AddressWidget *addressWidget(findAddressWidget());

			if (Utils::isUrlEmpty(m_contentsWidget->getUrl()) && addressWidget)
			{
				addressWidget->setFocus();
			}
		}
	}

	m_session = SessionWindow();

	emit widgetChanged();
	emit titleChanged(m_contentsWidget->getTitle());
	emit urlChanged(m_contentsWidget->getUrl());
	emit iconChanged(m_contentsWidget->getIcon());
	emit loadingStateChanged(m_contentsWidget->getLoadingState());
	emit canZoomChanged(m_contentsWidget->canZoom());

	connect(this, SIGNAL(aboutToClose()), m_contentsWidget, SLOT(close()));
	connect(m_contentsWidget, SIGNAL(aboutToNavigate()), this, SIGNAL(aboutToNavigate()));
	connect(m_contentsWidget, SIGNAL(needsAttention()), this, SIGNAL(needsAttention()));
	connect(m_contentsWidget, SIGNAL(requestedAddBookmark(QUrl,QString,QString)), this, SIGNAL(requestedAddBookmark(QUrl,QString,QString)));
	connect(m_contentsWidget, SIGNAL(requestedOpenUrl(QUrl,SessionsManager::OpenHints)), this, SIGNAL(requestedOpenUrl(QUrl,SessionsManager::OpenHints)));
	connect(m_contentsWidget, SIGNAL(requestedNewWindow(ContentsWidget*,SessionsManager::OpenHints)), this, SIGNAL(requestedNewWindow(ContentsWidget*,SessionsManager::OpenHints)));
	connect(m_contentsWidget, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)), this, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)));
	connect(m_contentsWidget, SIGNAL(requestedGeometryChange(QRect)), this, SLOT(handleGeometryChangeRequest(QRect)));
	connect(m_contentsWidget, SIGNAL(statusMessageChanged(QString)), this, SIGNAL(statusMessageChanged(QString)));
	connect(m_contentsWidget, SIGNAL(titleChanged(QString)), this, SIGNAL(titleChanged(QString)));
	connect(m_contentsWidget, SIGNAL(urlChanged(QUrl)), this, SIGNAL(urlChanged(QUrl)));
	connect(m_contentsWidget, SIGNAL(iconChanged(QIcon)), this, SIGNAL(iconChanged(QIcon)));
	connect(m_contentsWidget, SIGNAL(requestBlocked(NetworkManager::ResourceInformation)), this, SIGNAL(requestBlocked(NetworkManager::ResourceInformation)));
	connect(m_contentsWidget, SIGNAL(contentStateChanged(WebWidget::ContentStates)), this, SIGNAL(contentStateChanged(WebWidget::ContentStates)));
	connect(m_contentsWidget, SIGNAL(loadingStateChanged(WebWidget::LoadingState)), this, SIGNAL(loadingStateChanged(WebWidget::LoadingState)));
	connect(m_contentsWidget, SIGNAL(pageInformationChanged(WebWidget::PageInformation,QVariant)), this, SIGNAL(pageInformationChanged(WebWidget::PageInformation,QVariant)));
	connect(m_contentsWidget, SIGNAL(optionChanged(int,QVariant)), this, SIGNAL(optionChanged(int,QVariant)));
	connect(m_contentsWidget, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
	connect(m_contentsWidget, SIGNAL(canZoomChanged(bool)), this, SIGNAL(canZoomChanged(bool)));
	connect(m_contentsWidget, SIGNAL(webWidgetChanged()), this, SLOT(updateNavigationBar()));
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

	return m_addressWidgets.value(0, nullptr);
}

Window* Window::clone(bool cloneHistory, MainWindow *mainWindow)
{
	if (!m_contentsWidget || !canClone())
	{
		return nullptr;
	}

	QVariantMap parameters;

	if (isPrivate())
	{
		parameters[QLatin1String("hints")] = SessionsManager::PrivateOpen;
	}

	return new Window(parameters, m_contentsWidget->clone(cloneHistory), mainWindow);
}

ContentsWidget* Window::getContentsWidget()
{
	if (!m_contentsWidget)
	{
		setUrl(m_session.getUrl(), false);
	}

	return m_contentsWidget;
}

WebWidget* Window::getWebWidget()
{
	if (!m_contentsWidget)
	{
		setUrl(m_session.getUrl(), false);
	}

	return m_contentsWidget->getWebWidget();
}

QString Window::getTitle() const
{
	return (m_contentsWidget ? m_contentsWidget->getTitle() : m_session.getTitle());
}

QLatin1String Window::getType() const
{
	return (m_contentsWidget ? m_contentsWidget->getType() : QLatin1String("unknown"));
}

QVariant Window::getOption(int identifier) const
{
	if (m_contentsWidget)
	{
		return m_contentsWidget->getOption(identifier);
	}

	return (m_session.options.contains(identifier) ? m_session.options[identifier] : SettingsManager::getOption(identifier, m_session.getUrl()));
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
				session.options = webWidget->getOptions();
			}
		}

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

WebWidget::LoadingState Window::getLoadingState() const
{
	return (m_contentsWidget ? m_contentsWidget->getLoadingState() : WebWidget::DelayedLoadingState);
}

WebWidget::ContentStates Window::getContentState() const
{
	return (m_contentsWidget ? m_contentsWidget->getContentState() : WebWidget::UnknownContentState);
}

quint64 Window::getIdentifier() const
{
	return m_identifier;
}

int Window::getZoom() const
{
	return (m_contentsWidget ? m_contentsWidget->getZoom() : m_session.getZoom());
}

bool Window::canClone() const
{
	return (m_contentsWidget ? m_contentsWidget->canClone() : false);
}

bool Window::canZoom() const
{
	return (m_contentsWidget ? m_contentsWidget->canZoom() : false);
}

bool Window::isAboutToClose() const
{
	return m_isAboutToClose;
}

bool Window::isActive() const
{
	return (isActiveWindow() && isAncestorOf(QApplication::focusWidget()));
}

bool Window::isPinned() const
{
	return m_isPinned;
}

bool Window::isPrivate() const
{
	return (m_contentsWidget ? m_contentsWidget->isPrivate() : SessionsManager::calculateOpenHints(m_parameters).testFlag(SessionsManager::PrivateOpen));
}

}
