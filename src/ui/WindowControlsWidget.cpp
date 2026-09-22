/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2026 Water Phoenix Browser contributors
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

#include "WindowControlsWidget.h"
#include "MainWindow.h"

#include <QtCore/QEvent>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QStyle>
#include <QtWidgets/QToolButton>

namespace Otter
{

WindowControlsWidget::WindowControlsWidget(MainWindow *mainWindow, QWidget *parent) : QWidget(parent),
	m_mainWindow(mainWindow),
	m_minimizeButton(new QToolButton(this)),
	m_maximizeButton(new QToolButton(this)),
	m_closeButton(new QToolButton(this))
{
	QHBoxLayout *layout(new QHBoxLayout(this));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	m_minimizeButton->setAutoRaise(true);
	m_minimizeButton->setToolTip(tr("Minimize"));
	m_minimizeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
	m_minimizeButton->setFixedSize(32, 28);

	m_maximizeButton->setAutoRaise(true);
	m_maximizeButton->setFixedSize(32, 28);

	m_closeButton->setAutoRaise(true);
	m_closeButton->setToolTip(tr("Close"));
	m_closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
	m_closeButton->setFixedSize(32, 28);

	layout->addWidget(m_minimizeButton);
	layout->addWidget(m_maximizeButton);
	layout->addWidget(m_closeButton);

	setLayout(layout);

	updateMaximizeButton();

	connect(m_minimizeButton, &QToolButton::clicked, this, &WindowControlsWidget::handleMinimize);
	connect(m_maximizeButton, &QToolButton::clicked, this, &WindowControlsWidget::handleMaximizeRestore);
	connect(m_closeButton, &QToolButton::clicked, this, &WindowControlsWidget::handleClose);

	if (m_mainWindow)
	{
		m_mainWindow->installEventFilter(this);
	}
}

void WindowControlsWidget::updateMaximizeButton()
{
	if (!m_mainWindow)
	{
		return;
	}

	if (m_mainWindow->isMaximized())
	{
		m_maximizeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
		m_maximizeButton->setToolTip(tr("Restore"));
	}
	else
	{
		m_maximizeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
		m_maximizeButton->setToolTip(tr("Maximize"));
	}
}

void WindowControlsWidget::handleMinimize()
{
	if (m_mainWindow)
	{
		m_mainWindow->showMinimized();
	}
}

void WindowControlsWidget::handleMaximizeRestore()
{
	if (!m_mainWindow)
	{
		return;
	}

	if (m_mainWindow->isMaximized())
	{
		m_mainWindow->showNormal();
	}
	else
	{
		m_mainWindow->showMaximized();
	}

	updateMaximizeButton();
}

void WindowControlsWidget::handleClose()
{
	if (m_mainWindow)
	{
		m_mainWindow->close();
	}
}

bool WindowControlsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_mainWindow && event->type() == QEvent::WindowStateChange)
	{
		updateMaximizeButton();
	}

	return QWidget::eventFilter(object, event);
}

}
