/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Animation.h"
#include "MainWindow.h"
#include "Style.h"
#include "Window.h"
#include "../core/Application.h"
#include "../core/SessionModel.h"
#include "../core/ThemesManager.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
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
	m_spinnerAnimation(nullptr),
	m_reason(KeyboardReason),
	m_isIgnoringMinimizedTabs(SettingsManager::getOption(SettingsManager::TabSwitcher_IgnoreMinimizedTabsOption).toBool())
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

	connect(m_tabsView, &ItemViewWidget::clicked, this, [&](const QModelIndex &index)
	{
		if (index.isValid())
		{
			m_mainWindow->setActiveWindowByIdentifier(index.data(IdentifierRole).toULongLong());

			hide();
		}
	});
	connect(m_tabsView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TabSwitcherWidget::handleCurrentTabChanged);
}

void TabSwitcherWidget::showEvent(QShowEvent *event)
{
	setFocus();

	const MainWindowSessionItem *mainWindowItem(SessionsManager::getModel()->getMainWindowItem(m_mainWindow));

	if (mainWindowItem)
	{
		const bool useSorting(SettingsManager::getOption(SettingsManager::TabSwitcher_OrderByLastActivityOption).toBool());

		for (int i = 0; i < mainWindowItem->rowCount(); ++i)
		{
			const WindowSessionItem *windowItem(static_cast<WindowSessionItem*>(mainWindowItem->child(i, 0)));

			if (windowItem && (!m_isIgnoringMinimizedTabs || (windowItem->getActiveWindow() && !windowItem->getActiveWindow()->isMinimized())))
			{
				m_model->appendRow(createRow(windowItem->getActiveWindow(), (useSorting ? QVariant(windowItem->getActiveWindow()->getLastActivity()) : QVariant(i))));
			}
		}
	}

	m_model->sort(0, (SettingsManager::getOption(SettingsManager::TabSwitcher_OrderByLastActivityOption).toBool() ? Qt::DescendingOrder : Qt::AscendingOrder));

	const Window *activeWindow(m_mainWindow->getActiveWindow());
	const int contentsHeight(m_model->rowCount() * 22);

	m_tabsView->setCurrentIndex(m_model->index((activeWindow ? findRow(activeWindow->getIdentifier()) : 0), 0));
	m_tabsView->setMinimumHeight(qMin(contentsHeight, int(height() * 0.9)));

	QWidget::showEvent(event);

	connect(m_mainWindow, &MainWindow::windowAdded, this, &TabSwitcherWidget::handleWindowAdded);
	connect(m_mainWindow, &MainWindow::windowRemoved, this, &TabSwitcherWidget::handleWindowRemoved);
}

void TabSwitcherWidget::hideEvent(QHideEvent *event)
{
	QWidget::hideEvent(event);

	disconnect(m_mainWindow, &MainWindow::windowAdded, this, &TabSwitcherWidget::handleWindowAdded);
	disconnect(m_mainWindow, &MainWindow::windowRemoved, this, &TabSwitcherWidget::handleWindowRemoved);

	m_model->clear();
}

void TabSwitcherWidget::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
		case Qt::Key_Tab:
		case Qt::Key_Down:
			selectTab(true);

			break;
		case Qt::Key_Backtab:
		case Qt::Key_Up:
			selectTab(false);

			break;
		case Qt::Key_Enter:
		case Qt::Key_Return:
			accept();

			break;
		case Qt::Key_Escape:
			hide();

			break;
		default:
			break;
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

void TabSwitcherWidget::handleCurrentTabChanged(const QModelIndex &index)
{
	const Window *window(m_mainWindow->getWindowByIdentifier(index.data(IdentifierRole).toULongLong()));

	m_previewLabel->setMovie(nullptr);
	m_previewLabel->setPixmap({});

	if (!window)
	{
		return;
	}

	const WebWidget::LoadingState loadingState(window->getLoadingState());

	if (loadingState == WebWidget::DeferredLoadingState || loadingState == WebWidget::OngoingLoadingState)
	{
		if (!m_spinnerAnimation)
		{
			m_spinnerAnimation = ThemesManager::createAnimation(QLatin1String("spinner"), this);

			connect(m_spinnerAnimation, &Animation::frameChanged, m_previewLabel, [&]()
			{
				m_previewLabel->setPixmap(m_spinnerAnimation->getCurrentPixmap());
			});
		}

		m_spinnerAnimation->start();

		m_previewLabel->setPixmap(m_spinnerAnimation->getCurrentPixmap());
	}
	else
	{
		if (m_spinnerAnimation && m_spinnerAnimation->isRunning())
		{
			m_spinnerAnimation->stop();
		}

		QPixmap pixmap;

		if (loadingState != WebWidget::CrashedLoadingState)
		{
			pixmap = window->createThumbnail();
		}

		if (pixmap.isNull())
		{
			pixmap = QPixmap(32, 32);
			pixmap.setDevicePixelRatio(devicePixelRatio());
			pixmap.fill(Qt::transparent);

			QRect rectangle(0, 0, 32, 32);
			QPainter painter(&pixmap);

			window->getIcon().paint(&painter, rectangle);

			if (loadingState == WebWidget::CrashedLoadingState)
			{
				ThemesManager::getStyle()->drawIconOverlay(rectangle, ThemesManager::createIcon(QLatin1String("emblem-crashed")), &painter);
			}
		}

		m_previewLabel->setPixmap(pixmap);
	}
}

void TabSwitcherWidget::handleWindowAdded(quint64 identifier)
{
	Window *window(m_mainWindow->getWindowByIdentifier(identifier));

	if (window && (!m_isIgnoringMinimizedTabs || !window->isMinimized()))
	{
		m_model->insertRow(0, createRow(window, (SettingsManager::getOption(SettingsManager::TabSwitcher_OrderByLastActivityOption).toBool() ? QVariant(window->getLastActivity()) : QVariant(-1))));
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

QStandardItem* TabSwitcherWidget::createRow(Window *window, const QVariant &index) const
{
	QColor color(palette().color(QPalette::Text));

	if (window->getLoadingState() == WebWidget::DeferredLoadingState)
	{
		color.setAlpha(150);
	}

	QStandardItem *item(new QStandardItem(window->getIcon(), window->getTitle()));
	item->setData(color, Qt::ForegroundRole);
	item->setData(window->getIdentifier(), IdentifierRole);
	item->setData(index, OrderRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	connect(window, &Window::titleChanged, this, [&](const QString &title)
	{
		const int row(findRow(window->getIdentifier()));

		if (row >= 0)
		{
			m_model->setData(m_model->index(row, 0), title, Qt::DisplayRole);
		}
	});
	connect(window, &Window::iconChanged, this, [&](const QIcon &icon)
	{
		const int row(findRow(window->getIdentifier()));

		if (row >= 0)
		{
			m_model->setData(m_model->index(row, 0), icon, Qt::DecorationRole);
		}
	});
	connect(window, &Window::loadingStateChanged, this, [&](WebWidget::LoadingState state)
	{
		const int row(findRow(window->getIdentifier()));

		if (row >= 0)
		{
			QColor color(palette().color(QPalette::Text));

			if (state == WebWidget::DeferredLoadingState)
			{
				color.setAlpha(150);
			}

			m_model->setData(m_model->index(row, 0), color, Qt::ForegroundRole);
		}
	});

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
		const QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent->button() == Qt::MiddleButton)
		{
			const QModelIndex index(m_tabsView->indexAt(mouseEvent->pos()));

			if (index.isValid())
			{
				Application::triggerAction(ActionsManager::CloseTabAction, {{QLatin1String("tab"), index.data(IdentifierRole).toULongLong()}}, parentWidget());
			}

			return true;
		}
	}

	return QObject::eventFilter(object, event);
}

}
