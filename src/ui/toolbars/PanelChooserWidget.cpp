/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtWidgets/QMenu>

namespace Otter
{

PanelChooserWidget::PanelChooserWidget(const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : ToolButtonWidget(definition, parent)
{
	setMenu(new QMenu(this));
	setPopupMode(QToolButton::InstantPopup);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	optionChanged(QLatin1String("Sidebar/CurrentPanel"), SettingsManager::getValue(QLatin1String("Sidebar/CurrentPanel")));

	connect(menu(), SIGNAL(aboutToShow()), this, SLOT(menuAboutToShow()));
	connect(menu(), SIGNAL(triggered(QAction*)), this, SLOT(selectPanel(QAction*)));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

void PanelChooserWidget::changeEvent(QEvent *event)
{
	QToolButton::changeEvent(event);

	if (event->type() ==  QEvent::LanguageChange)
	{
		setText(SidebarWidget::getPanelTitle(SettingsManager::getValue(QLatin1String("Sidebar/CurrentPanel")).toString()));
	}
}

void PanelChooserWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Sidebar/CurrentPanel"))
	{
		setText(SidebarWidget::getPanelTitle(value.toString()));
	}
}

void PanelChooserWidget::menuAboutToShow()
{
	menu()->clear();

	const QStringList panels(SettingsManager::getValue(QLatin1String("Sidebar/Panels")).toStringList());

	for (int i = 0; i < panels.count(); ++i)
	{
		menu()->addAction(SidebarWidget::getPanelTitle(panels[i]))->setData(panels[i]);
	}
}

void PanelChooserWidget::selectPanel(QAction *action)
{
	SettingsManager::setValue(QLatin1String("Sidebar/CurrentPanel"), action->data());
}

QSize PanelChooserWidget::minimumSizeHint() const
{
	return QSize(0, 0);
}

}
