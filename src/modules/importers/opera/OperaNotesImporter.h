/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_OPERANOTESIMPORTER_H
#define OTTER_OPERANOTESIMPORTER_H

#include "../../../core/Importer.h"
#include "../../../ui/BookmarksComboBoxWidget.h"

#include <QtCore/QFile>

namespace Otter
{

class OperaNotesImporter : public Importer
{
	Q_OBJECT

public:
	explicit OperaNotesImporter(QObject *parent = nullptr);
	~OperaNotesImporter();

	BookmarksItem* getCurrentFolder() const;
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

protected:
	enum OperaNoteEntry
	{
		NoEntry = 0,
		NoteEntry = 1,
		FolderStartEntry = 2,
		FolderEndEntry = 3,
		SeparatorEntry = 4
	};

	void goToParent();
	void setCurrentFolder(BookmarksItem *folder);
	void setImportFolder(BookmarksItem *folder);

private:
	BookmarksComboBoxWidget *m_folderComboBox;
	BookmarksItem *m_currentFolder;
	BookmarksItem *m_importFolder;
	QWidget *m_optionsWidget;
};

}

#endif
