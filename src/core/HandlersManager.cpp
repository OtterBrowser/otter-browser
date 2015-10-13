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

#include <QtCore/QFile>
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
	HandlerDefinition definition;

	if (type.contains(QLatin1Char('/')))
	{
		const QStringList groups = type.split(QLatin1Char('/'));

		definition.isExplicit = true;

		for (int i = 0; i < groups.count(); ++i)
		{
			if (handlersSettings.childGroups().contains(groups.at(i)))
			{
				handlersSettings.beginGroup(groups.at(i));
			}
			else
			{
				for (int j = 0; j < i; ++j)
				{
					handlersSettings.endGroup();
				}

				handlersSettings.beginGroup(QLatin1String("*"));

				definition.isExplicit = false;

				break;
			}
		}
	}
	else
	{
		definition.isExplicit = handlersSettings.childGroups().contains(type);

		handlersSettings.beginGroup(definition.isExplicit ? type : QLatin1String("*"));
	}

	const QString downloadsPath = handlersSettings.value(QLatin1String("downloadsPath"), QString()).toString();
	const QString transferMode = handlersSettings.value(QLatin1String("transferMode"), QString()).toString();

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

void HandlersManager::setHandler(const QString &type, const HandlerDefinition &definition)
{
	const QString path = SessionsManager::getWritableDataPath(QLatin1String("handlers.ini"));

	if (!QFile::exists(path))
	{
		QFile::copy(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")), path);
		QFile::setPermissions(path, (QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther));
	}

	QString transferMode = QLatin1String("ask");

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

	QSettings settings(path, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");
	settings.beginGroup(type);
	settings.setValue(QLatin1String("openCommand"), definition.openCommand);
	settings.setValue(QLatin1String("downloadsPath"), definition.downloadsPath);
	settings.setValue(QLatin1String("transferMode"), transferMode);
}

}
