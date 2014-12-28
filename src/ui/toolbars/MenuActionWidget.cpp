/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "MenuActionWidget.h"
#include "../Menu.h"
#include "../../core/SessionsManager.h"
#include "../../core/Utils.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>

namespace Otter
{

MenuActionWidget::MenuActionWidget(QWidget *parent) : ActionWidget(-1, NULL, parent),
	m_menu(new Menu(this))
{
	setIcon(Utils::getIcon(QLatin1String("otter-browser"), false));
	setText(tr("Menu"));
	setMenu(m_menu);
	setPopupMode(QToolButton::InstantPopup);
	setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	setVisible(!SettingsManager::getValue(QLatin1String("Interface/ShowMenuBar")).toBool());

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(m_menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
}

void MenuActionWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Interface/ShowMenuBar"))
	{
		setVisible(!value.toBool());
	}
}

void MenuActionWidget::updateMenu()
{
	disconnect(m_menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));

	const QString menuButtonPath = (SessionsManager::getProfilePath() + QLatin1String("/menuButton.json"));
	QFile menuBarFile(QFile::exists(menuButtonPath) ? menuButtonPath : QLatin1String(":/other/menuButton.json"));
	menuBarFile.open(QFile::ReadOnly);

	m_menu->load(QJsonDocument::fromJson(menuBarFile.readAll()).object());
}

}
