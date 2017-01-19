/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "HandlersManager.h"
#include "SessionsManager.h"
#include "Settings.h"
#include "SettingsManager.h"

#include <QtCore/QFile>

namespace Otter
{

HandlersManager* HandlersManager::m_instance(nullptr);

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
	Settings settings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
	HandlerDefinition definition;
	definition.isExplicit = settings.getGroups().contains(type);

	if (definition.isExplicit)
	{
		settings.beginGroup(type);
	}
	else
	{
		settings.beginGroup(QLatin1String("*"));
	}

	const QString downloadsPath(settings.getValue(QLatin1String("downloadsPath"), QString()).toString());
	const QString transferMode(settings.getValue(QLatin1String("transferMode"), QString()).toString());

	definition.openCommand = settings.getValue(QLatin1String("openCommand"), QString()).toString();
	definition.downloadsPath = (downloadsPath.isEmpty() ? SettingsManager::getValue(SettingsManager::Paths_DownloadsOption).toString() : downloadsPath);

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

void HandlersManager::setHandler(const QString &type, const HandlerDefinition &definition)
{
	if (SessionsManager::isReadOnly())
	{
		return;
	}

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("handlers.ini")));
	Settings settings(QFile::exists(path) ? path : SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
	QString transferMode(QLatin1String("ask"));

	if (definition.transferMode == IgnoreTransferMode)
	{
		transferMode = QLatin1String("ignore");
	}
	else if (definition.transferMode == OpenTransferMode)
	{
		transferMode = QLatin1String("open");
	}
	else if (definition.transferMode == SaveTransferMode)
	{
		transferMode = QLatin1String("save");
	}
	else if (definition.transferMode == SaveAsTransferMode)
	{
		transferMode = QLatin1String("saveAs");
	}

	settings.beginGroup(type);
	settings.setValue(QLatin1String("openCommand"), definition.openCommand);
	settings.setValue(QLatin1String("downloadsPath"), definition.downloadsPath);
	settings.setValue(QLatin1String("transferMode"), transferMode);
	settings.save(path);
}

}
