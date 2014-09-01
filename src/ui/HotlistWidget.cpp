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

#include "HotlistWidget.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"

#include "ui_HotlistWidget.h"

namespace Otter
{

HotlistWidget::HotlistWidget(QWidget *parent) : QWidget(parent),
	m_currentWidget(NULL),
	m_ui(new Ui::HotlistWidget)
{
	m_ui->setupUi(this);

	optionChanged(QLatin1String("Hotlist/CurrentPanel"), SettingsManager::getValue(QLatin1String("Hotlist/CurrentPanel")));
	optionChanged(QLatin1String("Hotlist/Panels"), SettingsManager::getValue(QLatin1String("Hotlist/Panels")));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

HotlistWidget::~HotlistWidget()
{
	delete m_ui;
}

void HotlistWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Hotlist/CurrentPanel"))
	{
		openPanel(value.toString());
	}
	else if (option == QLatin1String("Hotlist/Panels"))
	{
		for (QHash<QString, QToolButton*>::const_iterator iterator = m_buttons.constBegin(); iterator != m_buttons.constEnd(); ++iterator)
		{
			m_ui->buttonsLayout->removeWidget(iterator.value());

			iterator.value()->deleteLater();
		}

		const QStringList panels = value.toString().split(QLatin1Char(','), QString::SkipEmptyParts);
		QHash<QString, QIcon> allPanels;
		allPanels[QLatin1String("bookmarks")] = Utils::getIcon(QLatin1String("bookmarks"));
		allPanels[QLatin1String("cache")] = Utils::getIcon(QLatin1String("cache"));
		allPanels[QLatin1String("config")] = Utils::getIcon(QLatin1String("configuration"));
		allPanels[QLatin1String("cookies")] = Utils::getIcon(QLatin1String("cookies"));
		allPanels[QLatin1String("history")] = Utils::getIcon(QLatin1String("view-history"));
		allPanels[QLatin1String("transfers")] = Utils::getIcon(QLatin1String("transfers"));

		for (int i = 0; i < panels.count(); ++i)
		{
			registerPanel(panels.at(i), allPanels.value(panels.at(i)));
		}
	}
}

void HotlistWidget::locationChanged(Qt::DockWidgetArea area)
{
	if (area == Qt::RightDockWidgetArea)
	{
		qobject_cast<QBoxLayout*>(layout())->setDirection(QBoxLayout::RightToLeft);
	}
	else
	{
		qobject_cast<QBoxLayout*>(layout())->setDirection(QBoxLayout::LeftToRight);
	}

	if (area == Qt::LeftDockWidgetArea || area == Qt::RightDockWidgetArea)
	{
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	}
	else
	{
		setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	}
}

void HotlistWidget::openPanel()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		SettingsManager::setValue(QLatin1String("Hotlist/CurrentPanel"), (action->data().toString() == m_currentPanel ? QString() : action->data().toString()));
	}
}

void HotlistWidget::openPanel(const QString &identifier)
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

		connect(window, SIGNAL(requestedOpenUrl(QUrl, OpenHints)), this, SLOT(openUrl(QUrl, OpenHints)));

		widget = window;
	}
	else if (identifier.startsWith(QLatin1String("web:")))
	{
		Window *window = new Window(false, NULL, this);
		window->setUrl(identifier.section(':', 1, -1));

		connect(window, SIGNAL(requestedOpenUrl(QUrl, OpenHints)), this, SLOT(openUrl(QUrl, OpenHints)));

		widget = window;
	}

	if (m_currentWidget)
	{
		m_ui->currentLayout->removeWidget(m_currentWidget);

		m_currentWidget->deleteLater();
	}

	if (m_buttons.contains(m_currentPanel) && m_buttons[m_currentPanel])
	{
		m_buttons[m_currentPanel]->setChecked(false);
	}

	if (widget)
	{
		m_ui->currentLayout->addWidget(widget);

		if (m_buttons.contains(identifier) && m_buttons[identifier])
		{
			m_buttons[identifier]->setChecked(true);
		}
	}

	m_currentPanel = identifier;
	m_currentWidget = widget;
}

void HotlistWidget::openUrl(const QUrl &url, OpenHints hints)
{
	WindowsManager *manager = SessionsManager::getWindowsManager();

	if (manager)
	{
		manager->open(url, hints);
	}
}

void HotlistWidget::registerPanel(const QString &identifier, const QIcon &icon)
{
	QToolButton *button = new QToolButton(this);
	button->setIcon(icon);
	button->setCheckable(true);
	button->setAutoRaise(true);

	QAction *action = new QAction(button);
	action->setData(identifier);

	m_ui->buttonsLayout->insertWidget(qMax(0, (m_ui->buttonsLayout->count() - 2)), button);

	m_buttons.insert(identifier, button);

	connect(button, SIGNAL(clicked()), action, SLOT(trigger()));
	connect(action, SIGNAL(triggered()), this, SLOT(openPanel()));
}

QSize HotlistWidget::sizeHint() const
{
	return QSize((m_currentWidget ? SettingsManager::getValue(QLatin1String("Hotlist/Width")).toInt() : 1), 100);
}

}
