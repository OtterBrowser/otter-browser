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

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));

	optionChanged(QLatin1String("Hotlist/Panels"), SettingsManager::getValue(QLatin1String("Hotlist/Panels")));
	optionChanged(QLatin1String("Hotlist/OpenPanel"), SettingsManager::getValue(QLatin1String("Hotlist/OpenPanel")));
}

HotlistWidget::~HotlistWidget()
{
	delete m_ui;
}

void HotlistWidget::notifyLocationChange(Qt::DockWidgetArea area)
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
		SettingsManager::setValue(QLatin1String("Hotlist/OpenPanel"), (action->data().toString() == m_currentPanel ? QString() : action->data().toString()));
	}
}

void HotlistWidget::openPanel(const QString &id)
{
	QWidget *widget = NULL;

	if (id.isEmpty())
	{
		widget = NULL;
	}
	else if (id == QLatin1String("bookmarks") || id == QLatin1String("cache") || id == QLatin1String("config") || id == QLatin1String("cookies") || id == QLatin1String("history") || id == QLatin1String("transfers"))
	{
		Window *window = new Window(false, NULL, this);
		window->setUrl(QLatin1String("about:") + id, false);

		connect(window, SIGNAL(requestedOpenUrl(QUrl, OpenHints)), this, SLOT(openUrl(QUrl, OpenHints)));

		widget = window;
	}
	else if (id.startsWith(QLatin1String("web:")))
	{
		Window *window = new Window(false, NULL, this);
		window->setUrl(id.section(':', 1, -1));

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

		if (m_buttons.contains(id) && m_buttons[id])
		{
			m_buttons[id]->setChecked(true);
		}
	}

	m_currentPanel = id;
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

void HotlistWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Hotlist/OpenPanel"))
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
		QHash<QString, QIcon> allPanels = knownPanels();

		for (int i = 0; i < panels.count(); ++i)
		{
			registerPanel(panels.at(i), allPanels.value(panels.at(i)));
		}
	}
}

void HotlistWidget::registerPanel(const QString &id, const QIcon &icon)
{
	QToolButton *button = new QToolButton(this);
	button->setIcon(icon);
	button->setCheckable(true);

	QAction *action = new QAction(button);
	action->setData(id);

	m_ui->buttonsLayout->addWidget(button);

	m_buttons.insert(id, button);

	connect(button, SIGNAL(clicked()), action, SLOT(trigger()));
	connect(action, SIGNAL(triggered()), this, SLOT(openPanel()));
}

QHash<QString, QIcon> HotlistWidget::knownPanels()
{
	QHash<QString, QIcon> result;
	result["bookmarks"] = Utils::getIcon(QLatin1String("bookmarks"));
	result["cache"] = Utils::getIcon(QLatin1String("cache"));
	result["config"] = Utils::getIcon(QLatin1String("configuration"));
	result["cookies"] = Utils::getIcon(QLatin1String("cookies"));
	result["history"] = Utils::getIcon(QLatin1String("view-history"));
	result["transfers"] = Utils::getIcon(QLatin1String("transfers"));

	return result;
}

QSize HotlistWidget::sizeHint() const
{
	return QSize((m_currentWidget ? SettingsManager::getValue(QLatin1String("Hotlist/Width")).toInt() : 1), 100);
}

}
