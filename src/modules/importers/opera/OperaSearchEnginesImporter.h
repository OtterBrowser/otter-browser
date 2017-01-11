/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014, 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_OPERASEARCHENGINESIMPORTER_H
#define OTTER_OPERASEARCHENGINESIMPORTER_H

#include "../../../core/Importer.h"

#include <QtWidgets/QCheckBox>

namespace Otter
{

class OperaSearchEnginesImporter : public Importer
{
	Q_OBJECT

public:
	explicit OperaSearchEnginesImporter(QObject *parent = nullptr);
	~OperaSearchEnginesImporter();

	QWidget* getOptionsWidget();
	QString getTitle() const;
	QString getDescription() const;
	QString getVersion() const;
	QString getSuggestedPath(const QString &path = QString()) const;
	QString getBrowser() const;
	QUrl getHomePage() const;
	QIcon getIcon() const;
	QStringList getFileFilters() const;
	ImportType getImportType() const;

public slots:
	bool import(const QString &path);

private:
	QCheckBox *m_optionsWidget;
	QString m_path;
};

}

#endif
