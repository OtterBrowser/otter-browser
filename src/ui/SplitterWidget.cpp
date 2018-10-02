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

#include "SplitterWidget.h"
#include "MainWindow.h"
#include "../core/Utils.h"

namespace Otter
{

SplitterWidget::SplitterWidget(QWidget *parent) : QSplitter(parent),
	m_mainWindow(nullptr),
	m_isInitialized(false)
{
}

SplitterWidget::SplitterWidget(Qt::Orientation orientation, QWidget *parent) : QSplitter(orientation, parent),
	m_mainWindow(nullptr),
	m_isInitialized(false)
{
}

void SplitterWidget::showEvent(QShowEvent *event)
{
	QSplitter::showEvent(event);

	if (!m_isInitialized)
	{
		const QString name(Utils::normalizeObjectName(objectName(), QLatin1String("SplitterWidget")));

		if (name.isEmpty())
		{
			return;
		}

		if (!m_mainWindow)
		{
			m_mainWindow = MainWindow::findMainWindow(this);
		}

		if (m_mainWindow)
		{
			const QVector<int> sizes(m_mainWindow->getSplitterSizes(name));

			if (!sizes.isEmpty())
			{
				setSizes(sizes.toList());
			}
		}

		connect(this, &SplitterWidget::splitterMoved, this, [=]()
		{
			if (!m_mainWindow)
			{
				m_mainWindow = MainWindow::findMainWindow(this);
			}

			if (m_mainWindow)
			{
				m_mainWindow->setSplitterSizes(name, sizes().toVector());
			}
		});

		m_isInitialized = true;
	}
}

}
