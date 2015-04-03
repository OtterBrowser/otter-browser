/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "WebContentsWidget.h"
#include "PermissionBarWidget.h"
#include "ProgressBarWidget.h"
#include "SearchBarWidget.h"
#include "../../../core/AddonsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/WebBackend.h"
#include "../../../ui/MainWindow.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

QString WebContentsWidget::m_sharedQuickFindQuery = NULL;

WebContentsWidget::WebContentsWidget(bool isPrivate, WebWidget *widget, Window *window) : ContentsWidget(window),
	m_webWidget(widget),
	m_searchBarWidget(NULL),
	m_progressBarWidget(NULL),
	m_quickFindTimer(0),
	m_isTabPreferencesMenuVisible(false)
{
	if (m_webWidget)
	{
		m_webWidget->setParent(this);
	}
	else
	{
		m_webWidget = AddonsManager::getWebBackend()->createWidget(isPrivate, this);
	}

	setFocusPolicy(Qt::StrongFocus);
	setStyleSheet(QLatin1String("workaround"));

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_webWidget);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	setLayout(layout);

	if (window)
	{
		connect(m_webWidget, SIGNAL(requestedCloseWindow()), window, SLOT(close()));
	}

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(m_webWidget, SIGNAL(requestedAddBookmark(QUrl,QString,QString)), this, SIGNAL(requestedAddBookmark(QUrl,QString,QString)));
	connect(m_webWidget, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SLOT(notifyRequestedOpenUrl(QUrl,OpenHints)));
	connect(m_webWidget, SIGNAL(requestedNewWindow(WebWidget*,OpenHints)), this, SLOT(notifyRequestedNewWindow(WebWidget*,OpenHints)));
	connect(m_webWidget, SIGNAL(requestedSearch(QString,QString,OpenHints)), this, SIGNAL(requestedSearch(QString,QString,OpenHints)));
	connect(m_webWidget, SIGNAL(requestedPermission(QString,QUrl,bool)), this, SLOT(handlePermissionRequest(QString,QUrl,bool)));
	connect(m_webWidget, SIGNAL(statusMessageChanged(QString)), this, SIGNAL(statusMessageChanged(QString)));
	connect(m_webWidget, SIGNAL(titleChanged(QString)), this, SIGNAL(titleChanged(QString)));
	connect(m_webWidget, SIGNAL(urlChanged(QUrl)), this, SIGNAL(urlChanged(QUrl)));
	connect(m_webWidget, SIGNAL(iconChanged(QIcon)), this, SIGNAL(iconChanged(QIcon)));
	connect(m_webWidget, SIGNAL(loadingChanged(bool)), this, SIGNAL(loadingChanged(bool)));
	connect(m_webWidget, SIGNAL(loadingChanged(bool)), this, SLOT(setLoading(bool)));
	connect(m_webWidget, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
}

void WebContentsWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_quickFindTimer && m_searchBarWidget)
	{
		killTimer(m_quickFindTimer);

		m_quickFindTimer = 0;

		m_searchBarWidget->hide();
	}

	ContentsWidget::timerEvent(event);
}

void WebContentsWidget::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);

	m_webWidget->setFocus();
}

void WebContentsWidget::resizeEvent(QResizeEvent *event)
{
	ContentsWidget::resizeEvent(event);

	if (m_progressBarWidget)
	{
		m_progressBarWidget->scheduleGeometryUpdate();
	}
}

void WebContentsWidget::keyPressEvent(QKeyEvent *event)
{
	QWidget::keyPressEvent(event);

	if (event->key() == Qt::Key_Escape)
	{
		if (m_webWidget->isLoading())
		{
			triggerAction(Action::StopAction);

			ActionsManager::triggerAction(Action::ActivateAddressFieldAction, this);

			event->accept();
		}
		else if (!m_quickFindQuery.isEmpty())
		{
			m_quickFindQuery = QString();

			m_webWidget->findInPage(QString());

			event->accept();
		}
		else if (!m_webWidget->getSelectedText().isEmpty())
		{
			m_webWidget->clearSelection();

			event->accept();
		}
		else
		{
			MainWindow *window = MainWindow::findMainWindow(this);

			if (window && window->isFullScreen())
			{
				window->triggerAction(Action::FullScreenAction);
			}
		}
	}
}

void WebContentsWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Browser/ShowDetailedProgressBar") && !value.toBool() && m_progressBarWidget)
	{
		m_progressBarWidget->deleteLater();
		m_progressBarWidget = NULL;
	}
	else if (option == QLatin1String("Search/EnableFindInPageAsYouType") && m_searchBarWidget)
	{
		if (value.toBool())
		{
			connect(m_searchBarWidget, SIGNAL(queryChanged(QString)), this, SLOT(findInPage()));
		}
		else
		{
			disconnect(m_searchBarWidget, SIGNAL(queryChanged(QString)), this, SLOT(findInPage()));
		}
	}
}

void WebContentsWidget::search(const QString &search, const QString &query)
{
	m_webWidget->search(search, query);
}

void WebContentsWidget::print(QPrinter *printer)
{
	m_webWidget->print(printer);
}

void WebContentsWidget::goToHistoryIndex(int index)
{
	m_webWidget->goToHistoryIndex(index);
}

void WebContentsWidget::triggerAction(int identifier, bool checked)
{
	switch (identifier)
	{
		case Action::FindAction:
		case Action::QuickFindAction:
		case Action::FindNextAction:
		case Action::FindPreviousAction:
			{
				if (identifier == Action::FindAction || identifier == Action::QuickFindAction)
				{
					if (!m_searchBarWidget)
					{
						m_searchBarWidget = new SearchBarWidget(this);
						m_searchBarWidget->hide();

						qobject_cast<QVBoxLayout*>(layout())->insertWidget(0, m_searchBarWidget);

						connect(m_searchBarWidget, SIGNAL(requestedSearch(WebWidget::FindFlags)), this, SLOT(findInPage(WebWidget::FindFlags)));
						connect(m_searchBarWidget, SIGNAL(flagsChanged(WebWidget::FindFlags)), this, SLOT(updateFindHighlight(WebWidget::FindFlags)));

						if (SettingsManager::getValue(QLatin1String("Search/EnableFindInPageAsYouType")).toBool())
						{
							connect(m_searchBarWidget, SIGNAL(queryChanged()), this, SLOT(findInPage()));
						}
					}

					if (!m_searchBarWidget->isVisible())
					{
						if (identifier == Action::QuickFindAction)
						{
							killTimer(m_quickFindTimer);

							m_quickFindTimer = startTimer(2000);
						}

						if (SettingsManager::getValue(QLatin1String("Search/ReuseLastQuickFindQuery")).toBool())
						{
							m_searchBarWidget->setQuery(m_sharedQuickFindQuery);
						}

						m_searchBarWidget->show();
					}

					m_searchBarWidget->selectAll();
				}
				else
				{
					WebWidget::FindFlags flags = (m_searchBarWidget ? m_searchBarWidget->getFlags() : WebWidget::NoFlagsFind);

					if (identifier == Action::FindPreviousAction)
					{
						flags |= WebWidget::BackwardFind;
					}

					findInPage(flags);
				}
			}

			break;
		case Action::QuickPreferencesAction:
			{
				if (m_isTabPreferencesMenuVisible)
				{
					return;
				}

				m_isTabPreferencesMenuVisible = true;

				QActionGroup popupsGroup(this);
				popupsGroup.setExclusive(true);
				popupsGroup.setEnabled(false);

				QMenu menu;

				popupsGroup.addAction(menu.addAction(tr("Open all pop-ups")));
				popupsGroup.addAction(menu.addAction(tr("Open pop-ups in background")));
				popupsGroup.addAction(menu.addAction(tr("Block all pop-ups")));

				menu.addSeparator();

				QAction *enableImagesAction = menu.addAction(tr("Enable Images"));
				enableImagesAction->setCheckable(true);
				enableImagesAction->setChecked(m_webWidget->getOption(QLatin1String("Browser/EnableImages")).toBool());
				enableImagesAction->setData(QLatin1String("Browser/EnableImages"));

				QAction *enableJavaScriptAction = menu.addAction(tr("Enable JavaScript"));
				enableJavaScriptAction->setCheckable(true);
				enableJavaScriptAction->setChecked(m_webWidget->getOption(QLatin1String("Browser/EnableJavaScript")).toBool());
				enableJavaScriptAction->setData(QLatin1String("Browser/EnableJavaScript"));

				QAction *enableJavaAction = menu.addAction(tr("Enable Java"));
				enableJavaAction->setCheckable(true);
				enableJavaAction->setChecked(m_webWidget->getOption(QLatin1String("Browser/EnableJava")).toBool());
				enableJavaAction->setData(QLatin1String("Browser/EnableJava"));

				QAction *enablePluginsAction = menu.addAction(tr("Enable Plugins"));
				enablePluginsAction->setCheckable(true);
				enablePluginsAction->setChecked(m_webWidget->getOption(QLatin1String("Browser/EnablePlugins")).toString() == QLatin1String("enabled"));
				enablePluginsAction->setData(QLatin1String("Browser/EnablePlugins"));

				menu.addSeparator();

				QAction *enableCookiesAction = menu.addAction(tr("Enable Cookies"));
				enableCookiesAction->setCheckable(true);
				enableCookiesAction->setEnabled(false);

				QAction *enableReferrerAction = menu.addAction(tr("Enable Referrer"));
				enableReferrerAction->setCheckable(true);
				enableReferrerAction->setEnabled(false);

				QAction *enableProxyAction = menu.addAction(tr("Enable Proxy"));
				enableProxyAction->setCheckable(true);
				enableProxyAction->setEnabled(false);

				menu.addSeparator();
				menu.addAction(tr("Reset Options"), m_webWidget, SLOT(clearOptions()))->setEnabled(!m_webWidget->getOptions().isEmpty());
				menu.addSeparator();
				menu.addAction(ActionsManager::getAction(Action::WebsitePreferencesAction, parent()));

				QAction *triggeredAction = menu.exec(QCursor::pos());

				if (triggeredAction && triggeredAction->data().isValid())
				{
					if (triggeredAction->data().toString() == QLatin1String("Browser/EnablePlugins"))
					{
						m_webWidget->setOption(QLatin1String("Browser/EnablePlugins"), (triggeredAction->isChecked() ? QLatin1String("enabled") : QLatin1String("disabled")));
					}
					else
					{
						m_webWidget->setOption(triggeredAction->data().toString(), triggeredAction->isChecked());
					}
				}

				m_isTabPreferencesMenuVisible = false;
			}

			break;
		case Action::ZoomInAction:
			setZoom(qMin((getZoom() + 10), 10000));

			break;
		case Action::ZoomOutAction:
			setZoom(qMax((getZoom() - 10), 10));

			break;
		case Action::ZoomOriginalAction:
			setZoom(100);

			break;
		default:
			m_webWidget->triggerAction(identifier, checked);

			break;
	}
}

void WebContentsWidget::findInPage(WebWidget::FindFlags flags)
{
	if (m_quickFindTimer != 0)
	{
		killTimer(m_quickFindTimer);

		m_quickFindTimer = startTimer(2000);
	}

	if (flags == WebWidget::NoFlagsFind)
	{
		flags = (m_searchBarWidget ? m_searchBarWidget->getFlags() : WebWidget::HighlightAllFind);
	}

	m_quickFindQuery = ((m_searchBarWidget && !m_searchBarWidget->getQuery().isEmpty()) ? m_searchBarWidget->getQuery() : m_sharedQuickFindQuery);

	const bool found = m_webWidget->findInPage(m_quickFindQuery, flags);

	if (!m_quickFindQuery.isEmpty() && !isPrivate() && SettingsManager::getValue(QLatin1String("Search/ReuseLastQuickFindQuery")).toBool())
	{
		m_sharedQuickFindQuery = m_quickFindQuery;
	}

	if (m_searchBarWidget)
	{
		m_searchBarWidget->setResultsFound(found);
	}
}

void WebContentsWidget::handlePermissionRequest(const QString &option, QUrl url, bool cancel)
{
	if (cancel)
	{
		for (int i = 0; i < m_permissionBarWidgets.count(); ++i)
		{
			if (m_permissionBarWidgets.at(i)->getOption() == option && m_permissionBarWidgets.at(i)->getUrl() == url)
			{
				layout()->removeWidget(m_permissionBarWidgets.at(i));

				m_permissionBarWidgets.at(i)->deleteLater();

				m_permissionBarWidgets.removeAt(i);

				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < m_permissionBarWidgets.count(); ++i)
		{
			if (m_permissionBarWidgets.at(i)->getOption() == option && m_permissionBarWidgets.at(i)->getUrl() == url)
			{
				return;
			}
		}

		PermissionBarWidget *widget = new PermissionBarWidget(option, url, this);

		qobject_cast<QVBoxLayout*>(layout())->insertWidget((m_permissionBarWidgets.count() + (m_searchBarWidget ? 1 : 0)), widget);

		widget->show();

		m_permissionBarWidgets.append(widget);

		connect(widget, SIGNAL(permissionChanged(WebWidget::PermissionPolicies)), this, SLOT(notifyPermissionChanged(WebWidget::PermissionPolicies)));
	}
}

void WebContentsWidget::notifyPermissionChanged(WebWidget::PermissionPolicies policies)
{
	PermissionBarWidget *widget = qobject_cast<PermissionBarWidget*>(sender());

	if (widget)
	{
		m_webWidget->setPermission(widget->getOption(), widget->getUrl(), policies);

		m_permissionBarWidgets.removeAll(widget);

		layout()->removeWidget(widget);

		widget->deleteLater();
	}
}

void WebContentsWidget::notifyRequestedOpenUrl(const QUrl &url, OpenHints hints)
{
	if (isPrivate())
	{
		hints |= PrivateOpen;
	}

	emit requestedOpenUrl(url, hints);
}

void WebContentsWidget::notifyRequestedNewWindow(WebWidget *widget, OpenHints hints)
{
	emit requestedNewWindow(new WebContentsWidget(isPrivate(), widget, NULL), hints);
}

void WebContentsWidget::updateFindHighlight(WebWidget::FindFlags flags)
{
	if (m_searchBarWidget)
	{
		m_webWidget->findInPage(QString(), (flags | WebWidget::HighlightAllFind));
		m_webWidget->findInPage(m_searchBarWidget->getQuery(), flags);
	}
}

void WebContentsWidget::setLoading(bool loading)
{
	if (!m_progressBarWidget && !SettingsManager::getValue(QLatin1String("Browser/ShowDetailedProgressBar")).toBool())
	{
		return;
	}

	if (loading && !m_progressBarWidget)
	{
		m_progressBarWidget = new ProgressBarWidget(m_webWidget, this);
	}
}

void WebContentsWidget::setOption(const QString &key, const QVariant &value)
{
	m_webWidget->setOption(key, value);
}

void WebContentsWidget::setHistory(const WindowHistoryInformation &history)
{
	m_webWidget->setHistory(history);
}

void WebContentsWidget::setZoom(int zoom)
{
	m_webWidget->setZoom(zoom);
}

void WebContentsWidget::setUrl(const QUrl &url, bool typed)
{
	m_webWidget->setRequestedUrl(url, typed);

	if (typed)
	{
		m_webWidget->setFocus();
	}
}

void WebContentsWidget::setParent(Window *window)
{
	ContentsWidget::setParent(window);

	if (window)
	{
		connect(m_webWidget, SIGNAL(requestedCloseWindow()), window, SLOT(close()));
	}
}

WebContentsWidget* WebContentsWidget::clone(bool cloneHistory)
{
	if (!canClone())
	{
		return NULL;
	}

	WebContentsWidget *webWidget = new WebContentsWidget(m_webWidget->isPrivate(), m_webWidget->clone(cloneHistory), NULL);
	webWidget->m_webWidget->setRequestedUrl(m_webWidget->getUrl(), false, true);

	return webWidget;
}

Action* WebContentsWidget::getAction(int identifier)
{
	return m_webWidget->getAction(identifier);
}

WebWidget* WebContentsWidget::getWebWidget()
{
	return m_webWidget;
}

QString WebContentsWidget::getTitle() const
{
	return m_webWidget->getTitle();
}

QString WebContentsWidget::getStatusMessage() const
{
	return m_webWidget->getStatusMessage();
}

QLatin1String WebContentsWidget::getType() const
{
	return QLatin1String("web");
}

QVariant WebContentsWidget::getOption(const QString &key) const
{
	if (m_webWidget->hasOption(key))
	{
		return m_webWidget->getOptions().value(key, QVariant());
	}

	return QVariant();
}

QUrl WebContentsWidget::getUrl() const
{
	return m_webWidget->getRequestedUrl();
}

QIcon WebContentsWidget::getIcon() const
{
	return m_webWidget->getIcon();
}

QPixmap WebContentsWidget::getThumbnail() const
{
	return m_webWidget->getThumbnail();
}

WindowHistoryInformation WebContentsWidget::getHistory() const
{
	return m_webWidget->getHistory();
}

int WebContentsWidget::getZoom() const
{
	return m_webWidget->getZoom();
}

bool WebContentsWidget::canClone() const
{
	return true;
}

bool WebContentsWidget::canZoom() const
{
	return true;
}

bool WebContentsWidget::isLoading() const
{
	return m_webWidget->isLoading();
}

bool WebContentsWidget::isPrivate() const
{
	return m_webWidget->isPrivate();
}

}
