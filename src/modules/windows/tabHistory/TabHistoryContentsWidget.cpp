/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TabHistoryContentsWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Action.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/Window.h"

#include "ui_TabHistoryContentsWidget.h"

namespace Otter
{

TabHistoryContentsWidget::TabHistoryContentsWidget(const QVariantMap &parameters, QWidget *parent) : ContentsWidget(parameters, nullptr, parent),
	m_window(nullptr),
	m_ui(new Ui::TabHistoryContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);

	QStandardItemModel *model(new QStandardItemModel(this));
	model->setHorizontalHeaderLabels({tr("Address"), tr("Title"), tr("Date")});

	m_ui->historyViewWidget->setModel(model);

	const MainWindow *mainWindow(MainWindow::findMainWindow(parentWidget()));

	if (mainWindow)
	{
		m_window = mainWindow->getActiveWindow();

		connect(mainWindow, &MainWindow::currentWindowChanged, this, [=]()
		{
			Window *window(mainWindow->getActiveWindow());

			if (window != m_window)
			{
				if (m_window)
				{
					disconnect(m_window, &Window::urlChanged, this, &TabHistoryContentsWidget::updateHistory);
				}

				m_window = window;

				if (window)
				{
					connect(window, &Window::urlChanged, this, &TabHistoryContentsWidget::updateHistory);
				}
			}

			updateHistory();
		});
	}

	updateHistory();

	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->historyViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->historyViewWidget, &ItemViewWidget::customContextMenuRequested, this, &TabHistoryContentsWidget::showContextMenu);
	connect(m_ui->historyViewWidget, &ItemViewWidget::clicked, [&](const QModelIndex &index)
	{
		if (m_window && m_window->getWebWidget())
		{
			m_window->triggerAction(ActionsManager::GoToHistoryIndexAction, {{QLatin1String("index"), index.row()}});
		}
	});
}

TabHistoryContentsWidget::~TabHistoryContentsWidget()
{
	delete m_ui;
}

void TabHistoryContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
		m_ui->historyViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Address"), tr("Title"), tr("Date")});
	}
}

void TabHistoryContentsWidget::updateHistory()
{
	m_ui->historyViewWidget->getSourceModel()->clear();
	m_ui->historyViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Address"), tr("Title"), tr("Date")});

	if (m_window)
	{
		const WindowHistoryInformation history(m_window->getHistory());

		for (int i = 0; i < history.entries.count(); ++i)
		{
			QList<QStandardItem*> items({new QStandardItem(history.entries.at(i).url), new QStandardItem(history.entries.at(i).getTitle()), new QStandardItem(Utils::formatDateTime(history.entries.at(i).time))});
			items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
			items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);
			items[2]->setFlags(items[2]->flags() | Qt::ItemNeverHasChildren);

			m_ui->historyViewWidget->getSourceModel()->appendRow(items);
		}
	}
}

void TabHistoryContentsWidget::showContextMenu(const QPoint &position)
{
	const QModelIndex index(m_ui->historyViewWidget->indexAt(position));
	ActionExecutor::Object executor(m_window, m_window);
	QMenu menu(this);

	if (index.isValid())
	{
		menu.addAction(new Action(ActionsManager::RemoveHistoryIndexAction, {{QLatin1String("index"), index.row()}}, executor, &menu));
		menu.addAction(new Action(ActionsManager::RemoveHistoryIndexAction, {{QLatin1String("index"), index.row()}, {QLatin1String("clearGlobalHistory"), true}}, executor, &menu));
		menu.addSeparator();
	}

	menu.addAction(new Action(ActionsManager::ClearTabHistoryAction, {}, executor, &menu));
	menu.addAction(new Action(ActionsManager::ClearTabHistoryAction, {{QLatin1String("clearGlobalHistory"), true}}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Purge Tab History")}}, executor, &menu));
	menu.exec(m_ui->historyViewWidget->mapToGlobal(position));
}

QString TabHistoryContentsWidget::getTitle() const
{
	return tr("Tab History");
}

QLatin1String TabHistoryContentsWidget::getType() const
{
	return QLatin1String("tabHistory");
}

QUrl TabHistoryContentsWidget::getUrl() const
{
	return {};
}

QIcon TabHistoryContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("view-history"), false);
}

}
