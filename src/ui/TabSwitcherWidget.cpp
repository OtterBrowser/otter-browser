/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TabSwitcherWidget.h"
#include "MainWindow.h"
#include "Window.h"
#include "../core/Application.h"
#include "../core/SessionModel.h"
#include "../core/ThemesManager.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMovie>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>

namespace Otter
{

TabSwitcherWidget::TabSwitcherWidget(MainWindow *parent) : QWidget(parent),
	m_mainWindow(parent),
	m_model(new QStandardItemModel(this)),
	m_tabsView(new ItemViewWidget(this)),
	m_previewLabel(new QLabel(this)),
	m_loadingMovie(nullptr),
	m_reason(KeyboardReason)
{
	QFrame *frame(new QFrame(this));
	QHBoxLayout *mainLayout(new QHBoxLayout(this));
	mainLayout->addWidget(frame, 0, Qt::AlignCenter);

	setLayout(mainLayout);
	setAutoFillBackground(false);

	QHBoxLayout *frameLayout(new QHBoxLayout(frame));
	frameLayout->addWidget(m_tabsView, 1);
	frameLayout->addWidget(m_previewLabel, 0, Qt::AlignCenter);

	m_model->setSortRole(OrderRole);

	frame->setLayout(frameLayout);
	frame->setAutoFillBackground(true);
	frame->setMinimumWidth(600);
	frame->setObjectName(QLatin1String("tabSwitcher"));
	frame->setStyleSheet(QStringLiteral("#tabSwitcher {background:%1;border:1px solid #B3B3B3;border-radius:4px;}").arg(palette().color(QPalette::Base).name()));

	m_tabsView->setModel(m_model);
	m_tabsView->setStyleSheet(QLatin1String("border:0;"));
	m_tabsView->header()->hide();
	m_tabsView->header()->setStretchLastSection(true);
	m_tabsView->viewport()->installEventFilter(this);

	m_previewLabel->setFixedSize(260, 170);
	m_previewLabel->setAlignment(Qt::AlignCenter);
	m_previewLabel->setStyleSheet(QLatin1String("border:1px solid gray;"));

	connect(m_tabsView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(handleCurrentTabChanged(QModelIndex)));
}

void TabSwitcherWidget::showEvent(QShowEvent *event)
{
	grabKeyboard();

	MainWindowSessionItem *mainWindowItem(SessionsManager::getModel()->getMainWindowItem(m_mainWindow));

	if (mainWindowItem)
	{
		const bool useSorting(SettingsManager::getOption(SettingsManager::Interface_TabSwitchingModeOption).toString() != QLatin1String("noSort"));

		for (int i = 0; i < mainWindowItem->rowCount(); ++i)
		{
			WindowSessionItem *windowItem(dynamic_cast<WindowSessionItem*>(mainWindowItem->child(i, 0)));

			if (windowItem)
			{
				m_model->appendRow(createRow(windowItem->getActiveWindow(), (useSorting ? QVariant(windowItem->getActiveWindow()->getLastActivity()) : QVariant(i))));
			}
		}
	}

	m_model->sort(0, ((SettingsManager::getOption(SettingsManager::Interface_TabSwitchingModeOption).toString() == QLatin1String("noSort")) ? Qt::AscendingOrder : Qt::DescendingOrder));

	Window *activeWindow(m_mainWindow->getWindowByIndex(-1));
	const int contentsHeight(m_model->rowCount() * 22);

	m_tabsView->setCurrentIndex(m_model->index((activeWindow ? findRow(activeWindow->getIdentifier()) : 0), 0));
	m_tabsView->setMinimumHeight(qMin(contentsHeight, int(height() * 0.9)));

	QWidget::showEvent(event);

	connect(m_mainWindow, SIGNAL(windowAdded(quint64)), this, SLOT(handleWindowAdded(quint64)));
	connect(m_mainWindow, SIGNAL(windowRemoved(quint64)), this, SLOT(handleWindowRemoved(quint64)));
}

void TabSwitcherWidget::hideEvent(QHideEvent *event)
{
	releaseKeyboard();

	QWidget::hideEvent(event);

	disconnect(m_mainWindow, SIGNAL(windowAdded(quint64)), this, SLOT(handleWindowAdded(quint64)));
	disconnect(m_mainWindow, SIGNAL(windowRemoved(quint64)), this, SLOT(handleWindowRemoved(quint64)));

	m_model->clear();
}

void TabSwitcherWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Down)
	{
		selectTab(true);
	}
	else if (event->key() == Qt::Key_Backtab || event->key() == Qt::Key_Up)
	{
		selectTab(false);
	}
	else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
	{
		accept();
	}
	else if (event->key() == Qt::Key_Escape)
	{
		hide();
	}
}

void TabSwitcherWidget::keyReleaseEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Control && m_reason == KeyboardReason)
	{
		accept();

		event->accept();
	}
}

void TabSwitcherWidget::handleCurrentTabChanged(const QModelIndex &index)
{
	Window *window(m_mainWindow->getWindowByIdentifier(index.data(IdentifierRole).toULongLong()));

	m_previewLabel->setMovie(nullptr);
	m_previewLabel->setPixmap(QPixmap());

	if (!window)
	{
		return;
	}

	if (window->getLoadingState() == WebWidget::DelayedLoadingState || window->getLoadingState() == WebWidget::OngoingLoadingState)
	{
		if (!m_loadingMovie)
		{
			m_loadingMovie = new QMovie(QLatin1String(":/icons/loading.gif"), QByteArray(), m_previewLabel);
			m_loadingMovie->start();
		}

		m_previewLabel->setMovie(m_loadingMovie);

		m_loadingMovie->setSpeed((window->getLoadingState() == WebWidget::OngoingLoadingState) ? 100 : 10);
	}
	else
	{
		m_previewLabel->setPixmap((window->getLoadingState() == WebWidget::CrashedLoadingState) ? ThemesManager::getIcon(QLatin1String("tab-crashed")).pixmap(32, 32) : window->getThumbnail());
	}
}

void TabSwitcherWidget::handleWindowAdded(quint64 identifier)
{
	Window *window(m_mainWindow->getWindowByIdentifier(identifier));

	if (window)
	{
		m_model->insertRow(0, createRow(window, ((SettingsManager::getOption(SettingsManager::Interface_TabSwitchingModeOption).toString() == QLatin1String("noSort")) ? QVariant(-1) : QVariant(window->getLastActivity()))));
	}
}

void TabSwitcherWidget::handleWindowRemoved(quint64 identifier)
{
	const int row(findRow(identifier));

	if (row >= 0)
	{
		m_model->removeRow(row);
	}
}

void TabSwitcherWidget::show(SwitcherReason reason)
{
	m_reason = reason;

	QWidget::show();
}

void TabSwitcherWidget::accept()
{
	m_mainWindow->setActiveWindowByIdentifier(m_tabsView->currentIndex().data(IdentifierRole).toULongLong());

	hide();
}

void TabSwitcherWidget::selectTab(bool next)
{
	const int currentRow(m_tabsView->currentIndex().row());

	m_tabsView->setCurrentIndex(m_model->index((next ? ((currentRow == (m_model->rowCount() - 1)) ? 0 : (currentRow + 1)) : ((currentRow == 0) ? (m_model->rowCount() - 1) : (currentRow - 1))), 0));
}

void TabSwitcherWidget::setTitle(const QString &title)
{
	Window *window(qobject_cast<Window*>(sender()));

	if (window)
	{
		const int row(findRow(window->getIdentifier()));

		if (row >= 0)
		{
			m_model->setData(m_model->index(row, 0), title, Qt::DisplayRole);
		}
	}
}

void TabSwitcherWidget::setIcon(const QIcon &icon)
{
	Window *window(qobject_cast<Window*>(sender()));

	if (window)
	{
		const int row(findRow(window->getIdentifier()));

		if (row >= 0)
		{
			m_model->setData(m_model->index(row, 0), icon, Qt::DecorationRole);
		}
	}
}

void TabSwitcherWidget::setLoadingState(WebWidget::LoadingState state)
{
	Window *window(qobject_cast<Window*>(sender()));

	if (window)
	{
		const int row(findRow(window->getIdentifier()));

		if (row >= 0)
		{
			QColor color(palette().color(QPalette::Text));

			if (state == WebWidget::DelayedLoadingState)
			{
				color.setAlpha(150);
			}

			m_model->setData(m_model->index(row, 0), color, Qt::TextColorRole);
		}
	}
}

QStandardItem* TabSwitcherWidget::createRow(Window *window, const QVariant &index) const
{
	QColor color(palette().color(QPalette::Text));

	if (window->getLoadingState() == WebWidget::DelayedLoadingState)
	{
		color.setAlpha(150);
	}

	QStandardItem* item(new QStandardItem(window->getIcon(), window->getTitle()));
	item->setData(color, Qt::TextColorRole);
	item->setData(window->getIdentifier(), IdentifierRole);
	item->setData(index, OrderRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	connect(window, SIGNAL(titleChanged(QString)), this, SLOT(setTitle(QString)));
	connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
	connect(window, SIGNAL(loadingStateChanged(WebWidget::LoadingState)), this, SLOT(setLoadingState(WebWidget::LoadingState)));

	return item;
}

TabSwitcherWidget::SwitcherReason TabSwitcherWidget::getReason() const
{
	return m_reason;
}

int TabSwitcherWidget::findRow(quint64 identifier) const
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		if (m_model->index(i, 0).data(IdentifierRole).toULongLong() == identifier)
		{
			return i;
		}
	}

	return -1;
}

bool TabSwitcherWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_tabsView->viewport() && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent && mouseEvent->button() == Qt::MiddleButton)
		{
			const QModelIndex index(m_tabsView->indexAt(mouseEvent->pos()));

			if (index.isValid())
			{
				Application::triggerAction(ActionsManager::CloseTabAction, {{QLatin1String("window"), index.data(IdentifierRole).toULongLong()}}, parentWidget());
			}

			return true;
		}
	}

	return QObject::eventFilter(object, event);
}

}
