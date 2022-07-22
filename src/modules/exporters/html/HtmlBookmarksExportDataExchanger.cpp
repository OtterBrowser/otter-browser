/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "HtmlBookmarksExportDataExchanger.h"

#include <QtCore/QDir>

namespace Otter
{

HtmlBookmarksExportDataExchanger::HtmlBookmarksExportDataExchanger(QObject *parent) : ExportDataExchanger(parent)
{
}

QString HtmlBookmarksExportDataExchanger::getName() const
{
	return QLatin1String("html");
}

QString HtmlBookmarksExportDataExchanger::getTitle() const
{
	return tr("Export HTML Bookmarks");
}

QString HtmlBookmarksExportDataExchanger::getDescription() const
{
	return tr("Exports bookmarks to HTML file (Netscape format).");
}

QString HtmlBookmarksExportDataExchanger::getVersion() const
{
	return QLatin1String("1.0");
}

QString HtmlBookmarksExportDataExchanger::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty() && QFileInfo(path).isDir())
	{
		return QDir(path).filePath(QLatin1String("bookmarks.html"));
	}

	return path;
}

QString HtmlBookmarksExportDataExchanger::getGroup() const
{
	return QLatin1String("other");
}

QUrl HtmlBookmarksExportDataExchanger::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList HtmlBookmarksExportDataExchanger::getFileFilters() const
{
	return {tr("HTML files (*.htm *.html)")};
}

DataExchanger::ExchangeType HtmlBookmarksExportDataExchanger::getExchangeType() const
{
	return BookmarksExchange;
}

bool HtmlBookmarksExportDataExchanger::exportData(const QString &path, bool canOverwriteExisting)
{
	Q_UNUSED(path)
	Q_UNUSED(canOverwriteExisting)

	return false;
}

}
