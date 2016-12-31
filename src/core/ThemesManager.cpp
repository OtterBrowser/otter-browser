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
#include "Application.h"
#include "PlatformIntegration.h"
#include "SettingsManager.h"
#include "../ui/Style.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QIcon>
#include <QtWidgets/QStyleFactory>

namespace Otter
{

ThemesManager* ThemesManager::m_instance(nullptr);
QString ThemesManager::m_iconThemePath(QLatin1String(":/icons/"));
bool ThemesManager::m_useSystemIconTheme(false);

ThemesManager::ThemesManager(QObject *parent) : QObject(parent)
{
	m_useSystemIconTheme = SettingsManager::getValue(SettingsManager::Interface_UseSystemIconThemeOption).toBool();

	optionChanged(SettingsManager::Interface_IconThemePathOption, SettingsManager::getValue(SettingsManager::Interface_IconThemePathOption));

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
	switch (identifier)
	{
		case SettingsManager::Interface_IconThemePathOption:
			{
				QString path(value.toString());

				if (path.isEmpty())
				{
					path = QLatin1String(":/icons/");
				}
				else if (!path.endsWith(QDir::separator()))
				{
					path.append(QDir::separator());
				}

				if (path != m_iconThemePath)
				{
					m_iconThemePath = path;

					emit iconThemeChanged();
				}
			}

			break;
		case SettingsManager::Interface_UseSystemIconThemeOption:
			if (value.toBool() != m_useSystemIconTheme)
			{
				m_useSystemIconTheme = value.toBool();

				emit iconThemeChanged();
			}
		default:
			break;
	}
}

ThemesManager* ThemesManager::getInstance()
{
	return m_instance;
}

Style* ThemesManager::createStyle(const QString &name)
{
	Style *style(nullptr);
	PlatformIntegration *integration(Application::getPlatformIntegration());

	if (integration)
	{
		style = integration->createStyle(name);
	}

	if (!style)
	{
		style = new Style(name);
	}

	return style;
}

QIcon ThemesManager::getIcon(const QString &name, bool fromTheme)
{
	if (m_useSystemIconTheme && fromTheme && QIcon::hasThemeIcon(name))
	{
		return QIcon::fromTheme(name);
	}

	const QString iconPath(m_iconThemePath + name);
	const QString svgPath(iconPath + QLatin1String(".svg"));

	if (QFile::exists(svgPath))
	{
		return QIcon(svgPath);
	}

	return QIcon(iconPath + QLatin1String(".png"));
}

}
