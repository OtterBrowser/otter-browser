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
	model->setHorizontalHeaderLabels({tr("Title"), tr("Address")});

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
	connect(m_ui->historyViewWidget, &ItemViewWidget::clicked, [&](const QModelIndex &index)
	{
		if (m_window && m_window->getWebWidget())
		{
			m_window->getWebWidget()->goToHistoryIndex(index.row());
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
		m_ui->historyViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Title"), tr("Address")});
	}
}

void TabHistoryContentsWidget::updateHistory()
{
	m_ui->historyViewWidget->getSourceModel()->clear();
	m_ui->historyViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Title"), tr("Address")});

	if (m_window)
	{
		const WindowHistoryInformation history(m_window->getHistory());

		for (int i = 0; i < history.entries.count(); ++i)
		{
			QList<QStandardItem*> items({new QStandardItem(history.entries.at(i).getTitle()), new QStandardItem(history.entries.at(i).url)});
			items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
			items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);

			m_ui->historyViewWidget->getSourceModel()->appendRow(items);
		}
	}
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
