/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "HandlersManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QSettings>

namespace Otter
{

HandlersManager* HandlersManager::m_instance = NULL;

HandlersManager::HandlersManager(QObject *parent) : QObject(parent)
{
}

void HandlersManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new HandlersManager(parent);
	}
}

HandlersManager* HandlersManager::getInstance()
{
	return m_instance;
}

HandlerDefinition HandlersManager::getHandler(const QString &type)
{
	QSettings handlersSettings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")), QSettings::IniFormat);
	const QString group = (handlersSettings.contains(type) ? type : QLatin1String("*"));

	handlersSettings.beginGroup(group);

	const QString downloadsPath = handlersSettings.value(QLatin1String("downloadsPath"), QString()).toString();
	const QString transferMode = handlersSettings.value(QLatin1String("transferMode"), QString()).toString();

	HandlerDefinition definition;
	definition.openCommand = handlersSettings.value(QLatin1String("openCommand"), QString()).toString();
	definition.downloadsPath = (downloadsPath.isEmpty() ? SettingsManager::getValue(QLatin1String("Paths/Downloads")).toString() : downloadsPath);

	if (transferMode == QLatin1String("ignore"))
	{
		definition.transferMode = IgnoreTransferMode;
	}
	else if (transferMode == QLatin1String("open"))
	{
		definition.transferMode = OpenTransferMode;
	}
	else if (transferMode == QLatin1String("save"))
	{
		definition.transferMode = SaveTransferMode;
	}
	else if (transferMode == QLatin1String("saveAs"))
	{
		definition.transferMode = SaveAsTransferMode;
	}
	else
	{
		definition.transferMode = AskTransferMode;
	}

	return definition;
}

}
