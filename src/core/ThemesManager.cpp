/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ThemesManager.h"
#include "SettingsManager.h"

#include <QtGui/QIcon>

namespace Otter
{

ThemesManager* ThemesManager::m_instance(nullptr);
bool ThemesManager::m_useSystemIconTheme(false);

ThemesManager::ThemesManager(QObject *parent) : QObject(parent)
{
	m_useSystemIconTheme = SettingsManager::getValue(SettingsManager::Interface_UseSystemIconThemeOption).toBool();

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int,QVariant)));
}

void ThemesManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new ThemesManager(parent);
	}
}

void ThemesManager::optionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::Interface_UseSystemIconThemeOption)
	{
		m_useSystemIconTheme = value.toBool();
	}
}

ThemesManager* ThemesManager::getInstance()
{
	return m_instance;
}

QIcon ThemesManager::getIcon(const QString &name, bool fromTheme)
{
	if (m_useSystemIconTheme && fromTheme && QIcon::hasThemeIcon(name))
	{
		return QIcon::fromTheme(name);
	}

	return QIcon(QStringLiteral(":/icons/%1.png").arg(name));
}

}
