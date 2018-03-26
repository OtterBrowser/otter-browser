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

#include "LinksContentsWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/Window.h"

#include "ui_LinksContentsWidget.h"

namespace Otter
{

LinksContentsWidget::LinksContentsWidget(const QVariantMap &parameters, QWidget *parent) : ContentsWidget(parameters, nullptr, parent),
	m_window(nullptr),
	m_ui(new Ui::LinksContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->linksViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->linksViewWidget->setModel(new QStandardItemModel(this));
	m_ui->linksViewWidget->expandAll();

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
					disconnect(m_window, &Window::loadingStateChanged, this, &LinksContentsWidget::updateLinks);
				}

				m_window = window;

				if (window)
				{
					connect(window, &Window::loadingStateChanged, this, &LinksContentsWidget::updateLinks);
				}
			}

			updateLinks();
		});
	}

	updateLinks();

	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->linksViewWidget, &ItemViewWidget::setFilterString);
}

LinksContentsWidget::~LinksContentsWidget()
{
	delete m_ui;
}

void LinksContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void LinksContentsWidget::updateLinks()
{
	m_ui->linksViewWidget->getSourceModel()->clear();

	if (m_window && m_window->getWebWidget())
	{
		QStandardItem *pageEntry(addEntry(nullptr, m_window->getTitle(), m_window->getUrl()));
		const QVector<WebWidget::LinkUrl> links(m_window->getWebWidget()->getLinks());

		for (int i = 0; i < links.count(); ++i)
		{
			addEntry(pageEntry, links.at(i).title, links.at(i).url);
		}

		m_ui->linksViewWidget->expandAll();
	}
}

QStandardItem* LinksContentsWidget::addEntry(QStandardItem *parent, const QString &title, const QUrl &url)
{
	QStandardItem *item(new QStandardItem(title.isEmpty() ? url.toDisplayString(QUrl::RemovePassword) : title));
	item->setData(url, Qt::ToolTipRole);

	if (parent)
	{
		parent->appendRow(item);
	}
	else
	{
		m_ui->linksViewWidget->getSourceModel()->appendRow(item);
	}

	return item;
}

QString LinksContentsWidget::getTitle() const
{
	return tr("Links");
}

QLatin1String LinksContentsWidget::getType() const
{
	return QLatin1String("links");
}

QUrl LinksContentsWidget::getUrl() const
{
	return {};
}

QIcon LinksContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("links"), false);
}

}
