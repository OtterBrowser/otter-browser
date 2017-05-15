/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ContentsWidget.h"
#include "Action.h"
#include "ContentsDialog.h"
#include "Window.h"

namespace Otter
{

ContentsWidget::ContentsWidget(const QVariantMap &parameters, Window *window) : QWidget(window),
	m_window(window),
	m_layer(nullptr),
	m_layerTimer(0),
	m_sidebar(parameters.value(QLatin1String("sidebar"), -1).toInt())
{
	if (window)
	{
		connect(window, SIGNAL(aboutToClose()), this, SLOT(handleAboutToClose()));
	}
}

void ContentsWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_layerTimer)
	{
		killTimer(m_layerTimer);

		m_layerTimer = 0;

		if (m_layer)
		{
			m_layer->hide();
			m_layer->deleteLater();
			m_layer = nullptr;
		}
	}
}

void ContentsWidget::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);

	if (m_layer)
	{
		m_layer->resize(size());

		for (int i = 0; i < m_dialogs.count(); ++i)
		{
			if (m_dialogs.at(i))
			{
				m_dialogs.at(i)->raise();
				m_dialogs.at(i)->adjustSize();
				m_dialogs.at(i)->move(geometry().center() - QRect(QPoint(0, 0), m_dialogs.at(i)->size()).center());
			}
		}
	}
}

void ContentsWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	if (m_layer)
	{
		m_layer->resize(size());

		for (int i = 0; i < m_dialogs.count(); ++i)
		{
			if (m_dialogs.at(i))
			{
				m_dialogs.at(i)->updateSize();
				m_dialogs.at(i)->raise();
				m_dialogs.at(i)->move(geometry().center() - QRect(QPoint(0, 0), m_dialogs.at(i)->size()).center());
			}
		}
	}
}

void ContentsWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	Q_UNUSED(identifier)
	Q_UNUSED(parameters)
}

void ContentsWidget::triggerAction()
{
	Action *action(qobject_cast<Action*>(sender()));

	if (action)
	{
		if (action->isCheckable())
		{
			triggerAction(action->getIdentifier(), {{QLatin1String("isChecked"), action->isChecked()}});
		}
		else
		{
			triggerAction(action->getIdentifier());
		}
	}
}

void ContentsWidget::handleAboutToClose()
{
	for (int i = 0; i < m_dialogs.count(); ++i)
	{
		if (m_dialogs.at(i))
		{
			m_dialogs.at(i)->close();
		}
	}
}

void ContentsWidget::handleDialogFinished()
{
	ContentsDialog *dialog(qobject_cast<ContentsDialog*>(sender()));

	if (dialog)
	{
		m_dialogs.removeAll(dialog);

		dialog->hide();
		dialog->deleteLater();
	}

	if (m_dialogs.isEmpty() && m_layer && m_layerTimer == 0)
	{
		m_layerTimer = startTimer(100);
	}
}

void ContentsWidget::showDialog(ContentsDialog *dialog, bool lockEventLoop)
{
	if (!dialog)
	{
		return;
	}

	Window *window(qobject_cast<Window*>(parent()));

	if (window && window->isAboutToClose())
	{
		dialog->deleteLater();

		return;
	}

	if (m_layerTimer != 0)
	{
		killTimer(m_layerTimer);

		m_layerTimer = 0;
	}

	m_dialogs.append(dialog);

	if (!m_layer)
	{
		QPalette palette(this->palette());
		palette.setColor(QPalette::Window, QColor(0, 0, 0, 70));

		m_layer = new QWidget(this);
		m_layer->setAutoFillBackground(true);
		m_layer->setPalette(palette);
		m_layer->resize(size());
		m_layer->show();
		m_layer->raise();
	}

	connect(dialog, SIGNAL(finished(int,bool)), this, SLOT(handleDialogFinished()));

	dialog->setParent(m_layer);
	dialog->show();

	if (m_dialogs.count() > 1)
	{
		dialog->stackUnder(m_dialogs.value(m_dialogs.count() - 2));
	}
	else
	{
		dialog->raise();
		dialog->setFocus();
	}

	dialog->move(geometry().center() - QRect(QPoint(0, 0), dialog->size()).center());

	emit needsAttention();

	if (lockEventLoop)
	{
		QEventLoop eventLoop;

		connect(dialog, SIGNAL(finished(int,bool)), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();
	}
}

void ContentsWidget::goToHistoryIndex(int index)
{
	Q_UNUSED(index)
}

void ContentsWidget::removeHistoryIndex(int index, bool purge)
{
	Q_UNUSED(index)
	Q_UNUSED(purge)
}

void ContentsWidget::setOption(int identifier, const QVariant &value)
{
	Q_UNUSED(identifier)
	Q_UNUSED(value)
}

void ContentsWidget::setActiveStyleSheet(const QString &styleSheet)
{
	Q_UNUSED(styleSheet)
}

void ContentsWidget::setHistory(const WindowHistoryInformation &history)
{
	Q_UNUSED(history)
}

void ContentsWidget::setZoom(int zoom)
{
	Q_UNUSED(zoom)
}

void ContentsWidget::setUrl(const QUrl &url, bool isTyped)
{
	Q_UNUSED(url)
	Q_UNUSED(isTyped)
}

void ContentsWidget::setParent(Window *window)
{
	if (m_window)
	{
		disconnect(m_window, SIGNAL(aboutToClose()), this, SLOT(handleAboutToClose()));
	}

	m_window = window;

	if (window)
	{
		connect(window, SIGNAL(aboutToClose()), this, SLOT(handleAboutToClose()));
	}

	QWidget::setParent(window);
}

ContentsWidget* ContentsWidget::clone(bool cloneHistory)
{
	Q_UNUSED(cloneHistory)

	return nullptr;
}

Action* ContentsWidget::createAction(int identifier, const QVariantMap parameters, bool followState)
{
	Q_UNUSED(identifier)
	Q_UNUSED(parameters)
	Q_UNUSED(followState)

	return nullptr;
}

WebWidget* ContentsWidget::getWebWidget()
{
	return nullptr;
}

QString ContentsWidget::parseQuery(const QString &query) const
{
	return query;
}

QString ContentsWidget::getVersion() const
{
	return QCoreApplication::applicationVersion();
}

QString ContentsWidget::getActiveStyleSheet() const
{
	return QString();
}

QString ContentsWidget::getStatusMessage() const
{
	return QString();
}

QVariant ContentsWidget::getOption(int identifier) const
{
	Q_UNUSED(identifier)

	return QVariant();
}

QVariant ContentsWidget::getPageInformation(WebWidget::PageInformation key) const
{
	Q_UNUSED(key)

	return QVariant();
}

QPixmap ContentsWidget::getThumbnail()
{
	return QPixmap();
}

ActionsManager::ActionDefinition::State ContentsWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	Q_UNUSED(parameters)

	return ActionsManager::getActionDefinition(identifier).defaultState;
}

WindowHistoryInformation ContentsWidget::getHistory() const
{
	WindowHistoryEntry entry;
	entry.url = getUrl().toString(QUrl::RemovePassword);
	entry.title = getTitle();
	entry.position = QPoint(0, 0);
	entry.zoom = 100;

	WindowHistoryInformation information;
	information.index = 0;
	information.entries.append(entry);

	return information;
}

QStringList ContentsWidget::getStyleSheets() const
{
	return QStringList();
}

QVector<WebWidget::LinkUrl> ContentsWidget::getFeeds() const
{
	return QVector<WebWidget::LinkUrl>();
}

QVector<WebWidget::LinkUrl> ContentsWidget::getSearchEngines() const
{
	return QVector<WebWidget::LinkUrl>();
}

QVector<NetworkManager::ResourceInformation> ContentsWidget::getBlockedRequests() const
{
	return QVector<NetworkManager::ResourceInformation>();
}

WebWidget::ContentStates ContentsWidget::getContentState() const
{
	return WebWidget::ApplicationContentState;
}

WebWidget::LoadingState ContentsWidget::getLoadingState() const
{
	return WebWidget::FinishedLoadingState;
}

int ContentsWidget::getSidebar() const
{
	return m_sidebar;
}

int ContentsWidget::getZoom() const
{
	return 100;
}

bool ContentsWidget::canClone() const
{
	return false;
}

bool ContentsWidget::canZoom() const
{
	return false;
}

bool ContentsWidget::isPrivate() const
{
	return false;
}

bool ContentsWidget::isSidebarPanel() const
{
	return (m_sidebar >= 0);
}

}
