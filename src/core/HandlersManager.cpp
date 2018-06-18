/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "AdblockContentFiltersProfile.h"
#include "ContentFiltersManager.h"
#include "IniSettings.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "Utils.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtCore/QUrlQuery>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QMessageBox>

namespace Otter
{

HandlersManager* HandlersManager::m_instance(nullptr);

HandlersManager::HandlersManager(QObject *parent) : QObject(parent)
{
}

void HandlersManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new HandlersManager(QCoreApplication::instance());
	}
}

void HandlersManager::setHandler(const QMimeType &mimeType, const HandlerDefinition &definition)
{
	if (SessionsManager::isReadOnly())
	{
		return;
	}

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("handlers.ini")));
	IniSettings settings(QFile::exists(path) ? path : SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
	QString transferMode;

	switch (definition.transferMode)
	{
		case HandlerDefinition::IgnoreTransfer:
			transferMode = QLatin1String("ignore");

			break;
		case HandlerDefinition::OpenTransfer:
			transferMode = QLatin1String("open");

			break;
		case HandlerDefinition::SaveTransfer:
			transferMode = QLatin1String("save");

			break;
		case HandlerDefinition::SaveAsTransfer:
			transferMode = QLatin1String("saveAs");

			break;
		default:
			transferMode = QLatin1String("ask");

			break;
	}

	settings.beginGroup(mimeType.isValid() ? mimeType.name() : QLatin1String("*"));
	settings.setValue(QLatin1String("openCommand"), definition.openCommand);
	settings.setValue(QLatin1String("downloadsPath"), definition.downloadsPath);
	settings.setValue(QLatin1String("transferMode"), transferMode);
	settings.save(path);
}

HandlersManager* HandlersManager::getInstance()
{
	return m_instance;
}

HandlersManager::HandlerDefinition HandlersManager::getHandler(const QMimeType &mimeType)
{
	IniSettings settings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
	const QString name(mimeType.isValid() ? mimeType.name() : QLatin1String("*"));
	HandlerDefinition definition;
	definition.mimeType = mimeType;
	definition.isExplicit = settings.getGroups().contains(name);

	if (definition.isExplicit)
	{
		settings.beginGroup(name);
	}
	else
	{
		settings.beginGroup(QLatin1String("*"));
	}

	const QString downloadsPath(settings.getValue(QLatin1String("downloadsPath"), {}).toString());
	const QString transferMode(settings.getValue(QLatin1String("transferMode"), {}).toString());

	definition.openCommand = settings.getValue(QLatin1String("openCommand"), {}).toString();
	definition.downloadsPath = (downloadsPath.isEmpty() ? SettingsManager::getOption(SettingsManager::Paths_DownloadsOption).toString() : downloadsPath);

	if (transferMode == QLatin1String("ignore"))
	{
		definition.transferMode = HandlerDefinition::IgnoreTransfer;
	}
	else if (transferMode == QLatin1String("open"))
	{
		definition.transferMode = HandlerDefinition::OpenTransfer;
	}
	else if (transferMode == QLatin1String("save"))
	{
		definition.transferMode = HandlerDefinition::SaveTransfer;
	}
	else if (transferMode == QLatin1String("saveAs"))
	{
		definition.transferMode = HandlerDefinition::SaveAsTransfer;
	}
	else
	{
		definition.transferMode = HandlerDefinition::AskTransfer;
	}

	return definition;
}

QVector<HandlersManager::HandlerDefinition> HandlersManager::getHandlers()
{
	const QMimeDatabase mimeDatabase;
	const QStringList mimeTypes(IniSettings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini"))).getGroups());
	QVector<HandlersManager::HandlerDefinition> handlers;
	handlers.reserve(mimeTypes.count());

	for (int i = 0; i < mimeTypes.count(); ++i)
	{
		handlers.append(getHandler(mimeDatabase.mimeTypeForName(mimeTypes.at(i))));
	}

	return handlers;
}

bool HandlersManager::handleUrl(const QUrl &url)
{
	if (url.scheme() == QLatin1String("abp"))
	{
		const QUrlQuery query(url.query());
		const QUrl location(QUrl::fromPercentEncoding(query.queryItemValue(QLatin1String("location")).toUtf8()));

		if (location.isValid())
		{
			if (ContentFiltersManager::getProfile(location))
			{
				QMessageBox::critical(nullptr, tr("Error"), tr("Profile with this address already exists."), QMessageBox::Close);
			}
			else if (QMessageBox::question(nullptr, tr("Question"), tr("Do you want to add this content blocking profile?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
			{
				ContentFiltersProfile *profile(new AdblockContentFiltersProfile(Utils::createIdentifier(QFileInfo(location.path()).baseName(), ContentFiltersManager::getProfileNames()), QByteArray::fromPercentEncoding(query.queryItemValue(QLatin1String("title")).toUtf8()), location, {}, {}, 0, ContentFiltersProfile::OtherCategory, ContentFiltersProfile::NoFlags));

				ContentFiltersManager::addProfile(profile);

				profile->update();
///TODO Some sort of passive confirmation that profile was added
			}
		}

		return true;
	}

	if (url.scheme() == QLatin1String("mailto"))
	{
		QDesktopServices::openUrl(url);

		return true;
	}

	return false;
}

bool HandlersManager::canHandleUrl(const QUrl &url)
{
	return (url.scheme() == QLatin1String("abp") || url.scheme() == QLatin1String("mailto"));
}

}
