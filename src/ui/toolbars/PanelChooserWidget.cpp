/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#include "PanelChooserWidget.h"
#include "../SidebarWidget.h"
#include "../../core/SettingsManager.h"

namespace Otter
{

PanelChooserWidget::PanelChooserWidget(QWidget *parent) : QToolButton(parent)
{
	setMenu(new QMenu(this));
	setPopupMode(QToolButton::InstantPopup);
	optionChanged(QLatin1String("Sidebar/CurrentPanel"), SettingsManager::getValue(QLatin1String("Sidebar/CurrentPanel")));

	connect(menu(), SIGNAL(aboutToShow()), this, SLOT(showMenu()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

void PanelChooserWidget::changeEvent(QEvent *event)
{
	QToolButton::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			setText(SidebarWidget::getPanelTitle(SettingsManager::getValue(QLatin1String("Sidebar/CurrentPanel")).toString()));

			break;
		default:
			break;
	}
}

void PanelChooserWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Sidebar/CurrentPanel"))
	{
		setText(SidebarWidget::getPanelTitle(value.toString()));
	}
}

void PanelChooserWidget::selectPanel()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (!action)
	{
		return;
	}

	SettingsManager::setValue(QLatin1String("Sidebar/CurrentPanel"), action->data());
}

void PanelChooserWidget::showMenu()
{
	menu()->clear();

	const QStringList panels = SettingsManager::getValue(QLatin1String("Sidebar/Panels")).toStringList();

	for (int i = 0; i < panels.count(); ++i)
	{
		QAction *action = new QAction(SidebarWidget::getPanelTitle(panels[i]), this);
		action->setData(panels[i]);

		menu()->addAction(action);

		connect(action, SIGNAL(triggered()), this, SLOT(selectPanel()));
	}
}

}
