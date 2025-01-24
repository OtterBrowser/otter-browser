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

#ifndef OTTER_HTMLBOOKMARKSEXPORTDATAEXCHANGER_H
#define OTTER_HTMLBOOKMARKSEXPORTDATAEXCHANGER_H

#include "../../../core/DataExchanger.h"
#include "../../../core/BookmarksManager.h"

namespace Otter
{

class HtmlBookmarksExportDataExchanger final : public ExportDataExchanger
{
	Q_OBJECT

public:
	explicit HtmlBookmarksExportDataExchanger(QObject *parent = nullptr);

	QString getName() const override;
	QString getTitle() const override;
	QString getDescription() const override;
	QString getVersion() const override;
	QString getSuggestedPath(const QString &path = {}) const override;
	QString getGroup() const override;
	QUrl getHomePage() const override;
	QStringList getFileFilters() const override;
	ExchangeType getExchangeType() const override;

public slots:
	bool exportData(const QString &path, bool canOverwriteExisting) override;

protected:
	void writeBookmark(QTextStream *stream, BookmarksModel::Bookmark *bookmark);
};

}

#endif
