/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "SidebarWidget.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"

#include "ui_SidebarWidget.h"

#include <QtWidgets/QDockWidget>

namespace Otter
{

SidebarWidget::SidebarWidget(QWidget *parent) : QWidget(parent),
	m_currentWidget(NULL),
	m_ui(new Ui::SidebarWidget)
{
	m_ui->setupUi(this);

	optionChanged(QLatin1String("Sidebar/CurrentPanel"), SettingsManager::getValue(QLatin1String("Sidebar/CurrentPanel")));
	optionChanged(QLatin1String("Sidebar/Panels"), SettingsManager::getValue(QLatin1String("Sidebar/Panels")));
	updateSize();

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

SidebarWidget::~SidebarWidget()
{
	delete m_ui;
}

void SidebarWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	if (m_currentWidget)
	{
		SettingsManager::setValue(QLatin1String("Window/SidebarWidth"), width());
	}
}

void SidebarWidget::showEvent(QShowEvent *event)
{
	updateSize();

	QWidget::showEvent(event);
}

void SidebarWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Sidebar/CurrentPanel"))
	{
		openPanel(value.toString());
	}
	else if (option == QLatin1String("Sidebar/Panels"))
	{
		for (QHash<QString, QToolButton*>::const_iterator iterator = m_buttons.constBegin(); iterator != m_buttons.constEnd(); ++iterator)
		{
			m_ui->buttonsLayout->removeWidget(iterator.value());

			iterator.value()->deleteLater();
		}

		const QStringList panels = value.toString().split(QLatin1Char(','), QString::SkipEmptyParts);

		for (int i = 0; i < panels.count(); ++i)
		{
			registerPanel(panels.at(i));
		}
	}
}

void SidebarWidget::locationChanged(Qt::DockWidgetArea area)
{
	if (area == Qt::RightDockWidgetArea)
	{
		qobject_cast<QBoxLayout*>(layout())->setDirection(QBoxLayout::RightToLeft);
	}
	else
	{
		qobject_cast<QBoxLayout*>(layout())->setDirection(QBoxLayout::LeftToRight);
	}
}

void SidebarWidget::openPanel()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		SettingsManager::setValue(QLatin1String("Sidebar/CurrentPanel"), ((action->data().toString() == m_currentPanel) ? QString() : action->data().toString()));
	}
}

void SidebarWidget::openPanel(const QString &identifier)
{
	QWidget *widget = NULL;

	if (identifier.isEmpty())
	{
		widget = NULL;
	}
	else if (identifier == QLatin1String("bookmarks") || identifier == QLatin1String("cache") || identifier == QLatin1String("config") || identifier == QLatin1String("cookies") || identifier == QLatin1String("history") || identifier == QLatin1String("transfers"))
	{
		Window *window = new Window(false, NULL, this);
		window->setUrl(QLatin1String("about:") + identifier, false);

		connect(window, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SLOT(openUrl(QUrl,OpenHints)));

		widget = window;
	}
	else if (identifier.startsWith(QLatin1String("web:")))
	{
		Window *window = new Window(false, NULL, this);
		window->setUrl(identifier.section(':', 1, -1));

		connect(window, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SLOT(openUrl(QUrl,OpenHints)));

		widget = window;
	}

	if (m_currentWidget)
	{
		QWidget *currentWidget = m_currentWidget;

		m_currentWidget = NULL;

		layout()->removeWidget(currentWidget);

		currentWidget->deleteLater();
	}

	if (m_buttons.contains(m_currentPanel) && m_buttons[m_currentPanel])
	{
		m_buttons[m_currentPanel]->setChecked(false);
	}

	if (widget)
	{
		layout()->addWidget(widget);

		if (m_buttons.contains(identifier) && m_buttons[identifier])
		{
			m_buttons[identifier]->setChecked(true);
		}
	}

	m_currentPanel = identifier;
	m_currentWidget = widget;

	updateSize();
}

void SidebarWidget::openUrl(const QUrl &url, OpenHints hints)
{
	WindowsManager *manager = SessionsManager::getWindowsManager();

	if (manager)
	{
		manager->open(url, hints);
	}
}

void SidebarWidget::registerPanel(const QString &identifier)
{
	QString title;
	QIcon icon;

	if (identifier == QLatin1String("bookmarks"))
	{
		icon = Utils::getIcon(QLatin1String("bookmarks"));
		title = tr("Bookmarks");
	}
	else if (identifier == QLatin1String("cache"))
	{
		icon  = Utils::getIcon(QLatin1String("cache"));
		title = tr("Cache");
	}
	else if (identifier == QLatin1String("config"))
	{
		icon  = Utils::getIcon(QLatin1String("configuration"));
		title = tr("Configuration");
	}
	else if (identifier == QLatin1String("cookies"))
	{
		icon  = Utils::getIcon(QLatin1String("cookies"));
		title = tr("Cookies");
	}
	else if (identifier == QLatin1String("history"))
	{
		icon  = Utils::getIcon(QLatin1String("view-history"));
		title = tr("History");
	}
	else if (identifier == QLatin1String("transfers"))
	{
		icon  = Utils::getIcon(QLatin1String("transfers"));
		title = tr("Transfers");
	}
	else
	{
		icon  = Utils::getIcon(QLatin1String("text-html"));
		title = identifier.section(':', 1, -1);
	}

	QToolButton *button = new QToolButton(this);
	button->setIcon(icon);
	button->setToolTip(title);
	button->setCheckable(true);
	button->setAutoRaise(true);

	QAction *action = new QAction(button);
	action->setData(identifier);

	m_ui->buttonsLayout->insertWidget(qMax(0, (m_ui->buttonsLayout->count() - 2)), button);

	m_buttons.insert(identifier, button);

	connect(button, SIGNAL(clicked()), action, SLOT(trigger()));
	connect(action, SIGNAL(triggered()), this, SLOT(openPanel()));
}

void SidebarWidget::updateSize()
{
	QDockWidget *dockWidget = qobject_cast<QDockWidget*>(parentWidget());

	if (dockWidget)
	{
		if (m_currentWidget)
		{
			dockWidget->setMaximumWidth(QWIDGETSIZE_MAX);

			resize(SettingsManager::getValue(QLatin1String("Window/SidebarWidth")).toInt(), height());
			setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
		}
		else
		{
			dockWidget->setMaximumWidth(m_ui->buttonsLayout->contentsRect().width());

			setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
		}
	}
}

}
