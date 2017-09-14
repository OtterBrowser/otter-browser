/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_HTMLBOOKMARKSIMPORTER_H
#define OTTER_HTMLBOOKMARKSIMPORTER_H

#include "../../../core/BookmarksImporter.h"

#ifdef OTTER_ENABLE_QTWEBKIT
#include <QtWebKit/QWebElement>
#endif

namespace Otter
{

class BookmarksImporterWidget;

class HtmlBookmarksImporter final : public BookmarksImporter
{
	Q_OBJECT

public:
	explicit HtmlBookmarksImporter(QObject *parent = nullptr);

	QWidget* createOptionsWidget(QWidget *parent) override;
	QString getTitle() const override;
	QString getDescription() const override;
	QString getVersion() const override;
	QString getSuggestedPath(const QString &path = {}) const override;
	QString getBrowser() const override;
	QUrl getHomePage() const override;
	QStringList getFileFilters() const override;

public slots:
	bool import(const QString &path) override;

#ifdef OTTER_ENABLE_QTWEBKIT
protected:
	void processElement(const QWebElement &element);
	static QDateTime getDateTime(const QWebElement &element, const QString &attribute);
#endif

private:
	BookmarksImporterWidget *m_optionsWidget;
	int m_currentAmount;
	int m_totalAmount;
};

}

#endif
