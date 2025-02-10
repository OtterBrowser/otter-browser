/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include <QtCore/QFile>
#include <QtCore/QTextStream>

namespace Otter
{

HtmlBookmarksExportDataExchanger::HtmlBookmarksExportDataExchanger(QObject *parent) : ExportDataExchanger(parent)
{
}

void HtmlBookmarksExportDataExchanger::writeBookmark(QTextStream *stream, BookmarksModel::Bookmark *bookmark)
{
	const BookmarksModel::BookmarkType type(bookmark->getType());

	switch (type)
	{
		case BookmarksModel::FeedBookmark:
		case BookmarksModel::UrlBookmark:
			*stream << "<DT><A HREF=\"" << bookmark->getUrl().toString().toHtmlEscaped() << "\" ADD_DATE=\"" << bookmark->getTimeAdded().toSecsSinceEpoch() << "\"";

			if (bookmark->getTimeModified().isValid())
			{
				*stream << " LAST_MODIFIED=\"" << bookmark->getTimeModified().toSecsSinceEpoch() << "\"";
			}

			if (bookmark->getTimeVisited().isValid())
			{
				*stream << " LAST_VISITED=\"" << bookmark->getTimeVisited().toSecsSinceEpoch() << "\"";
			}

			if (type == BookmarksModel::FeedBookmark)
			{
				*stream << " FEEDURL=\"" << bookmark->getUrl().toString().toHtmlEscaped() << "\"";
			}

			if (!bookmark->getKeyword().isEmpty())
			{
				*stream << " SHORTCUTURL=\"" << bookmark->getKeyword().toHtmlEscaped() << "\"";
			}

			*stream << ">" << bookmark->getTitle().toHtmlEscaped() << "</A>\n";

			if (!bookmark->getDescription().isEmpty())
			{
				*stream << "<DD>" << bookmark->getDescription().toHtmlEscaped() << "</DD>\n";
			}

			break;
		case BookmarksModel::FolderBookmark:
		case BookmarksModel::RootBookmark:
			*stream << "<DT><H3 ADD_DATE=\"0\" LAST_MODIFIED=\"0\">" << bookmark->getTitle().toHtmlEscaped() << "</H3>\n";
			*stream << "<DL><P>\n";

			for (int i = 0; i < bookmark->rowCount(); ++i)
			{
				writeBookmark(stream, bookmark->getChild(i));
			}

			*stream << "</DL><P>\n";

			break;
		case BookmarksModel::SeparatorBookmark:
			*stream << "<HR>\n";

			break;
		default:
			break;
	}
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
	const int amount(BookmarksManager::getModel()->getCount());

	emit exchangeStarted(BookmarksExchange, amount);

	if (!canOverwriteExisting && QFile::exists(path))
	{
		emit exchangeFinished(BookmarksExchange, FailedOperation, 0);

		return false;
	}

	QFile file(path);

	if (!file.open(QIODevice::WriteOnly))
	{
		emit exchangeFinished(BookmarksExchange, FailedOperation, 0);

		return false;
	}

	QTextStream stream(&file);
	stream << "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n";
	stream << "<!-- This is an automatically generated file.\n";
	stream << "     It will be read and overwritten.\n";
	stream << "     DO NOT EDIT! -->\n";
	stream << "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=utf-8\">\n";
	stream << "<TITLE>Bookmarks</TITLE>\n";
	stream << "<H1>Bookmarks</H1>\n";
	stream << "<DL><P>\n";

	writeBookmark(&stream, BookmarksManager::getModel()->getRootItem());

	file.close();

	emit exchangeFinished(BookmarksExchange, SuccessfullOperation, amount);

	return true;
}

}
