/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "WidgetFactory.h"
#include "../core/Application.h"
#include "../core/HandlersManager.h"
#include "../core/HistoryManager.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"
#include "../modules/widgets/address/AddressWidget.h"
#include "../modules/widgets/search/SearchWidget.h"
#include "../modules/windows/web/WebContentsWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtWidgets/QBoxLayout>

namespace Otter
{

quint64 Window::m_identifierCounter(0);

WindowToolBarWidget::WindowToolBarWidget(int identifier, Window *parent) : ToolBarWidget(identifier, parent, parent)
{
}

void WindowToolBarWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	QStyleOptionToolBar toolBarOption;
	toolBarOption.initFrom(this);
	toolBarOption.lineWidth = style()->pixelMetric(QStyle::PM_ToolBarFrameWidth, nullptr, this);
	toolBarOption.positionOfLine = QStyleOptionToolBar::End;
	toolBarOption.positionWithinLine = QStyleOptionToolBar::OnlyOne;
	toolBarOption.state |= QStyle::State_Horizontal;
	toolBarOption.toolBarArea = Qt::TopToolBarArea;

	style()->drawControl(QStyle::CE_ToolBar, &toolBarOption, &painter, this);
}

Window::Window(const QVariantMap &parameters, ContentsWidget *widget, MainWindow *mainWindow) : QWidget(mainWindow->centralWidget()),
	m_mainWindow(mainWindow),
	m_addressBarWidget(nullptr),
	m_contentsWidget(nullptr),
	m_parameters(parameters),
	m_identifier(++m_identifierCounter),
	m_suspendTimer(0),
	m_isAboutToClose(false),
	m_isPinned(false)
{
	QBoxLayout *layout(new QBoxLayout(QBoxLayout::TopToBottom, this));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	setLayout(layout);

	if (widget)
	{
		setContentsWidget(widget);
	}

	connect(this, &Window::titleChanged, this, &Window::setWindowTitle);
	connect(mainWindow, &MainWindow::toolBarStateChanged, this, [&](int identifier, const Session::MainWindow::ToolBarState &state)
	{
		if (m_addressBarWidget && identifier == ToolBarsManager::AddressBar)
		{
			m_addressBarWidget->setState(state);
		}
	});
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

	if (m_suspendTimer == 0 && suspendTime >= 0)
	{
		m_suspendTimer = startTimer(suspendTime * 1000);
	}
}

void Window::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);

	if (m_suspendTimer != 0)
	{
		killTimer(m_suspendTimer);

		m_suspendTimer = 0;
	}

	updateFocus();
}

void Window::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	switch (identifier)
	{
		case ActionsManager::CloneTabAction:
			if (canClone())
			{
				m_mainWindow->addWindow(clone(true, m_mainWindow));
			}

			break;
		case ActionsManager::PinTabAction:
			setPinned(!isPinned());

			break;
		case ActionsManager::DetachTabAction:
			if (m_mainWindow->getWindowCount() > 1 || parameters.value(QLatin1String("minimalInterface")).toBool())
			{
				m_mainWindow->moveWindow(this, nullptr, parameters);
			}

			break;
		case ActionsManager::MaximizeTabAction:
		case ActionsManager::MinimizeTabAction:
		case ActionsManager::RestoreTabAction:
		case ActionsManager::AlwaysOnTopTabAction:
			{
				QVariantMap mutableParameters(parameters);
				mutableParameters[QLatin1String("tab")] = m_identifier;

				m_mainWindow->triggerAction(identifier, mutableParameters, trigger);
			}

			break;
		case ActionsManager::SuspendTabAction:
			if (!m_contentsWidget || m_contentsWidget->close())
			{
				m_session = getSession();

				setContentsWidget(nullptr);
			}

			break;
		case ActionsManager::CloseTabAction:
			if (!isPinned())
			{
				requestClose();
			}

			break;
		case ActionsManager::FullScreenAction:
			if (m_contentsWidget)
			{
				m_contentsWidget->triggerAction(identifier, parameters, trigger);
			}

			break;
		default:
			getContentsWidget()->triggerAction(identifier, parameters, trigger);

			break;
	}
}

void Window::clear()
{
	if (!m_contentsWidget || m_contentsWidget->close())
	{
		setContentsWidget(new WebContentsWidget(m_parameters, {}, nullptr, this, this));

		m_isAboutToClose = false;

		emit urlChanged(getUrl(), true);
	}
}

void Window::requestClose()
{
	if (!m_contentsWidget || m_contentsWidget->close())
	{
		m_isAboutToClose = true;

		emit aboutToClose();

		QTimer::singleShot(50, this, [&]()
		{
			emit requestedCloseWindow(this);
		});
	}
}

void Window::search(const QString &query, const QString &searchEngine)
{
	WebContentsWidget *widget(qobject_cast<WebContentsWidget*>(m_contentsWidget));

	if (!widget)
	{
		if (m_contentsWidget && !m_contentsWidget->close())
		{
			return;
		}

		widget = new WebContentsWidget({{QLatin1String("hints"), (isPrivate() ? SessionsManager::PrivateOpen : SessionsManager::DefaultOpen)}}, {}, nullptr, this, this);

		setContentsWidget(widget);
	}

	widget->search(query, searchEngine);

	emit urlChanged(getUrl(), true);
}

void Window::markAsActive(bool updateLastActivity)
{
	if (m_suspendTimer != 0)
	{
		killTimer(m_suspendTimer);

		m_suspendTimer = 0;
	}

	if (!m_contentsWidget)
	{
		setUrl(m_session.getUrl(), false);
	}

	if (updateLastActivity)
	{
		m_lastActivity = QDateTime::currentDateTimeUtc();
	}

	emit activated();
}

void Window::updateFocus()
{
	QTimer::singleShot(100, this, [&]()
	{
		AddressWidget *addressWidget(m_mainWindow->findAddressField());

		if (addressWidget && Utils::isUrlEmpty(getUrl()) && (!m_contentsWidget || m_contentsWidget->getLoadingState() != WebWidget::OngoingLoadingState))
		{
			addressWidget->setFocus();
		}
		else if (m_contentsWidget)
		{
			m_contentsWidget->setFocus();
		}
	});
}

void Window::setSession(const Session::Window &session, bool deferLoading)
{
	m_session = session;

	setPinned(session.isPinned);

	if (deferLoading)
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

		SessionsManager::markSessionAsModified();

		emit optionChanged(identifier, value);
	}
}

void Window::setUrl(const QUrl &url, bool isTypedIn)
{
	ContentsWidget *newWidget(nullptr);

	if (HandlersManager::canViewUrl(url))
	{
		if (m_session.history.index < 0 && !Utils::isUrlEmpty(getUrl()) && SessionsManager::hasUrl(url, true))
		{
			emit urlChanged(url, true);

			return;
		}

		newWidget = WidgetFactory::createContentsWidget(url, {{QLatin1String("url"), url}}, this, this);

		if (newWidget)
		{
			newWidget->setUrl(url);

			if (!newWidget->canClone())
			{
				SessionsManager::removeStoredUrl(newWidget->getUrl().toString());
			}
		}
	}

	const bool isRestoring(!m_contentsWidget && m_session.history.isValid());

	if (!newWidget && (!m_contentsWidget || m_contentsWidget->getType() != QLatin1String("web")))
	{
		newWidget = new WebContentsWidget(m_parameters, m_session.options, nullptr, this, this);
	}

	if (newWidget)
	{
		if (m_contentsWidget && !m_contentsWidget->close())
		{
			newWidget->deleteLater();

			return;
		}

		setContentsWidget(newWidget);
	}

	if (m_contentsWidget && url.isValid())
	{
		if (!isRestoring)
		{
			m_contentsWidget->setUrl(url, isTypedIn);
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
	else if (m_session.history.index >= 0 && m_session.history.index < m_session.history.entries.count())
	{
		m_session.history.entries[m_session.history.index].zoom = zoom;
	}
}

void Window::setPinned(bool isPinned)
{
	if (isPinned != m_isPinned)
	{
		m_isPinned = isPinned;

		emit arbitraryActionsStateChanged({ActionsManager::PinTabAction, ActionsManager::CloseTabAction});
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
		if (m_addressBarWidget)
		{
			layout()->removeWidget(m_addressBarWidget);

			m_addressBarWidget->deleteLater();
			m_addressBarWidget = nullptr;
		}

		emit actionsStateChanged();

		return;
	}

	m_contentsWidget->setParent(this);

	if (!m_addressBarWidget)
	{
		m_addressBarWidget = new WindowToolBarWidget(ToolBarsManager::AddressBar, this);
		m_addressBarWidget->setState(m_mainWindow->getToolBarState(ToolBarsManager::AddressBar));

		layout()->addWidget(m_addressBarWidget);
		layout()->setAlignment(m_addressBarWidget, Qt::AlignTop);
	}

	layout()->addWidget(m_contentsWidget);

	if (m_session.history.isValid() || !m_contentsWidget->getWebWidget() || m_contentsWidget->getWebWidget()->getRequestedUrl().isEmpty())
	{
		m_contentsWidget->setHistory(m_session.history);
		m_contentsWidget->setZoom(m_session.getZoom());
	}

	if (isActive())
	{
		if (m_session.history.isValid())
		{
			m_contentsWidget->setFocus();
		}
		else
		{
			const AddressWidget *addressWidget(m_mainWindow->findAddressField());

			if (addressWidget && Utils::isUrlEmpty(m_contentsWidget->getUrl()))
			{
				QTimer::singleShot(100, addressWidget, static_cast<void(AddressWidget::*)()>(&AddressWidget::setFocus));
			}
		}
	}

	updateFocus();

	m_session = Session::Window();

	emit titleChanged(m_contentsWidget->getTitle());
	emit urlChanged(m_contentsWidget->getUrl(), false);
	emit iconChanged(m_contentsWidget->getIcon());
	emit actionsStateChanged();
	emit loadingStateChanged(m_contentsWidget->getLoadingState());
	emit canZoomChanged(m_contentsWidget->canZoom());

	connect(m_contentsWidget, &ContentsWidget::aboutToNavigate, this, &Window::aboutToNavigate);
	connect(m_contentsWidget, &ContentsWidget::needsAttention, this, &Window::needsAttention);
	connect(m_contentsWidget, &ContentsWidget::requestedNewWindow, this, &Window::requestedNewWindow);
	connect(m_contentsWidget, &ContentsWidget::requestedSearch, this, &Window::requestedSearch);
	connect(m_contentsWidget, &ContentsWidget::requestedGeometryChange, this, [&](const QRect &geometry)
	{
		setWindowFlags(Qt::SubWindow);
		showNormal();
		resize(geometry.size() + (this->geometry().size() - m_contentsWidget->size()));
		move(geometry.topLeft());
	});
	connect(m_contentsWidget, &ContentsWidget::statusMessageChanged, this, &Window::statusMessageChanged);
	connect(m_contentsWidget, &ContentsWidget::titleChanged, this, &Window::titleChanged);
	connect(m_contentsWidget, &ContentsWidget::urlChanged, this, [&](const QUrl &url)
	{
		emit urlChanged(url, false);
	});
	connect(m_contentsWidget, &ContentsWidget::iconChanged, this, &Window::iconChanged);
	connect(m_contentsWidget, &ContentsWidget::requestBlocked, this, &Window::requestBlocked);
	connect(m_contentsWidget, &ContentsWidget::arbitraryActionsStateChanged, this, &Window::arbitraryActionsStateChanged);
	connect(m_contentsWidget, &ContentsWidget::categorizedActionsStateChanged, this, &Window::categorizedActionsStateChanged);
	connect(m_contentsWidget, &ContentsWidget::contentStateChanged, this, &Window::contentStateChanged);
	connect(m_contentsWidget, &ContentsWidget::loadingStateChanged, this, &Window::loadingStateChanged);
	connect(m_contentsWidget, &ContentsWidget::pageInformationChanged, this, &Window::pageInformationChanged);
	connect(m_contentsWidget, &ContentsWidget::optionChanged, this, &Window::optionChanged);
	connect(m_contentsWidget, &ContentsWidget::zoomChanged, this, &Window::zoomChanged);
	connect(m_contentsWidget, &ContentsWidget::canZoomChanged, this, &Window::canZoomChanged);
	connect(m_contentsWidget, &ContentsWidget::webWidgetChanged, m_addressBarWidget, &WindowToolBarWidget::reload);
}

Window* Window::clone(bool cloneHistory, MainWindow *mainWindow) const
{
	if (!m_contentsWidget || !canClone())
	{
		return nullptr;
	}

	return new Window({{QLatin1String("size"), size()}, {QLatin1String("hints"), (isPrivate() ? SessionsManager::PrivateOpen : SessionsManager::DefaultOpen)}}, m_contentsWidget->clone(cloneHistory), mainWindow);
}

MainWindow* Window::getMainWindow() const
{
	return m_mainWindow;
}

WindowToolBarWidget* Window::getAddressBar() const
{
	return m_addressBarWidget;
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
	return ((m_contentsWidget && !m_isAboutToClose) ? m_contentsWidget->getTitle() : m_session.getTitle());
}

QString Window::getIdentity() const
{
	return m_session.identity;
}

QLatin1String Window::getType() const
{
	return ((m_contentsWidget && !m_isAboutToClose) ? m_contentsWidget->getType() : QLatin1String("unknown"));
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
	return ((m_contentsWidget && !m_isAboutToClose) ? m_contentsWidget->getUrl() : m_session.getUrl());
}

QIcon Window::getIcon() const
{
	return ((m_contentsWidget && !m_isAboutToClose) ? m_contentsWidget->getIcon() : HistoryManager::getIcon(m_session.getUrl()));
}

QPixmap Window::createThumbnail() const
{
	return ((m_contentsWidget && !m_isAboutToClose) ? m_contentsWidget->createThumbnail() : QPixmap());
}

QDateTime Window::getLastActivity() const
{
	return m_lastActivity;
}

ActionsManager::ActionDefinition::State Window::getActionState(int identifier, const QVariantMap &parameters) const
{
	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());

	switch (identifier)
	{
		case ActionsManager::CloneTabAction:
			state.isEnabled = canClone();

			break;
		case ActionsManager::DetachTabAction:
			state.isEnabled = (m_mainWindow->getWindowCount() > 1 || parameters.value(QLatin1String("minimalInterface")).toBool());

			break;
		case ActionsManager::PinTabAction:
			state.text = (m_isPinned ? QCoreApplication::translate("actions", "Unpin Tab") : QCoreApplication::translate("actions", "Pin Tab"));

			break;
		case ActionsManager::CloseTabAction:
			state.isEnabled = !m_isPinned;

			break;
		case ActionsManager::GoToParentDirectoryAction:
			state.isEnabled = !Utils::isUrlEmpty(getUrl());

			break;
		case ActionsManager::MaximizeTabAction:
			state.isEnabled = (parentWidget()->windowType() == Qt::SubWindow && !isMaximized());

			break;
		case ActionsManager::MinimizeTabAction:
			state.isEnabled = !isMinimized();

			break;
		case ActionsManager::RestoreTabAction:
			state.isEnabled = (parentWidget()->windowType() != Qt::SubWindow || isMaximized() || isMinimized());

			break;
		case ActionsManager::AlwaysOnTopTabAction:
			state.isEnabled = windowFlags().testFlag(Qt::WindowStaysOnTopHint);

			break;
		default:
			if (m_contentsWidget)
			{
				state = m_contentsWidget->getActionState(identifier, parameters);
			}

			break;
	}

	return state;
}

Session::Window::History Window::getHistory() const
{
	if (m_contentsWidget)
	{
		return m_contentsWidget->getHistory();
	}

	return m_session.history;
}

Session::Window Window::getSession() const
{
	Session::Window session;

	if (m_contentsWidget)
	{
		session.history = m_contentsWidget->getHistory();
		session.isPinned = isPinned();

		if (m_contentsWidget->getType() == QLatin1String("web"))
		{
			const WebContentsWidget *webWidget(qobject_cast<WebContentsWidget*>(m_contentsWidget));

			if (webWidget)
			{
				session.options = webWidget->getOptions();
			}
		}
	}
	else
	{
		session = m_session;
	}

	session.isAlwaysOnTop = windowFlags().testFlag(Qt::WindowStaysOnTopHint);
	session.state.state = Qt::WindowMaximized;

	if (parentWidget()->windowType() == Qt::SubWindow && !isMaximized())
	{
		if (isMinimized())
		{
			session.state.state = Qt::WindowMinimized;
		}
		else
		{
			session.state.geometry = geometry();
			session.state.state = Qt::WindowNoState;
		}
	}

	return session;
}

QSize Window::sizeHint() const
{
	return {800, 600};
}

WebWidget::LoadingState Window::getLoadingState() const
{
	return ((m_contentsWidget && !m_isAboutToClose) ? m_contentsWidget->getLoadingState() : WebWidget::DeferredLoadingState);
}

WebWidget::ContentStates Window::getContentState() const
{
	return ((m_contentsWidget && !m_isAboutToClose) ? m_contentsWidget->getContentState() : WebWidget::UnknownContentState);
}

quint64 Window::getIdentifier() const
{
	return m_identifier;
}

int Window::getZoom() const
{
	return ((m_contentsWidget && !m_isAboutToClose) ? m_contentsWidget->getZoom() : m_session.getZoom());
}

bool Window::canClone() const
{
	return ((m_contentsWidget && !m_isAboutToClose) ? m_contentsWidget->canClone() : false);
}

bool Window::canZoom() const
{
	return ((m_contentsWidget && !m_isAboutToClose) ? m_contentsWidget->canZoom() : false);
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
	return ((m_contentsWidget && !m_isAboutToClose) ? m_contentsWidget->isPrivate() : SessionsManager::calculateOpenHints(m_parameters).testFlag(SessionsManager::PrivateOpen));
}

}
