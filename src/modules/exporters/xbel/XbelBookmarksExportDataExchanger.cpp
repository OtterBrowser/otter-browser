/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2021 - 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "XbelBookmarksExportDataExchanger.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/SessionsManager.h"

#include <QtCore/QDir>
#include <QtCore/QFile>

namespace Otter
{

XbelBookmarksExportDataExchanger::XbelBookmarksExportDataExchanger(QObject *parent) : ExportDataExchanger(parent)
{
}

QString XbelBookmarksExportDataExchanger::getName() const
{
	return QLatin1String("xbel");
}

QString XbelBookmarksExportDataExchanger::getTitle() const
{
	return tr("Export XBEL Bookmarks");
}

QString XbelBookmarksExportDataExchanger::getDescription() const
{
	return tr("Exports bookmarks as XBEL file.");
}

QString XbelBookmarksExportDataExchanger::getVersion() const
{
	return QLatin1String("1.0");
}

QString XbelBookmarksExportDataExchanger::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty() && QFileInfo(path).isDir())
	{
		return QDir(path).filePath(QLatin1String("bookmarks.xbel"));
	}

	return path;
}

QString XbelBookmarksExportDataExchanger::getGroup() const
{
	return QLatin1String("other");
}

QUrl XbelBookmarksExportDataExchanger::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList XbelBookmarksExportDataExchanger::getFileFilters() const
{
	return {tr("XBEL files (*.xbel)")};
}

DataExchanger::ExchangeType XbelBookmarksExportDataExchanger::getExchangeType() const
{
	return BookmarksExchange;
}

bool XbelBookmarksExportDataExchanger::exportData(const QString &path, bool canOverwriteExisting)
{
	const int amount(BookmarksManager::getModel()->getCount());

	emit exchangeStarted(BookmarksExchange, amount);

	if (QFile::exists(path) && canOverwriteExisting)
	{
		QFile::remove(path);
	}

	const bool result(QFile::copy(SessionsManager::getWritableDataPath(QLatin1String("bookmarks.xbel")), path));

	emit exchangeFinished(BookmarksExchange, (result ? SuccessfullOperation : FailedOperation), (result ? amount : 0));

	return result;
}

}
