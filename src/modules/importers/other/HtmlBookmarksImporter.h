/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_HTMLBOOKMARKSIMPORTER_H
#define OTTER_HTMLBOOKMARKSIMPORTER_H

#include "../../../core/BookmarksImporter.h"
#include "../../../ui/BookmarksImporterWidget.h"

#include <QtCore/QFile>
#include <QWebElement>

namespace Otter
{

class HtmlBookmarksImporter : public BookmarksImporter
{
	Q_OBJECT

public:
	explicit HtmlBookmarksImporter(QObject *parent = NULL);
	~HtmlBookmarksImporter();

	QWidget* getOptionsWidget();
	QString getTitle() const;
	QString getDescription() const;
	QString getVersion() const;
	QString getSuggestedPath() const;
	QString getBrowser() const;

public slots:
	bool setPath(const QString &path, bool isPrefix);
	bool import();

protected:
	void handleOptions();
	void processElement(const QWebElement &element);

private:
	QFile *m_file;
	BookmarksImporterWidget *m_optionsWidget;
	bool m_duplicate;

};
}

#endif
