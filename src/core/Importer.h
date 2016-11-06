/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_IMPORTER_H
#define OTTER_IMPORTER_H

#include "AddonsManager.h"

namespace Otter
{

enum ImportType
{
	OtherImport = 0,
	FullImport = 1,
	BookmarksImport = 2,
	SettingsImport = 4,
	PasswordsImport = 8,
	SearchEnginesImport = 16,
	SessionsImport = 32,
	CookiesImport = 64,
	HistoryImport = 128,
	FeedsImport = 256,
	MailImport = 512,
	NotesImport = 1024
};

class Importer : public Addon
{
	Q_OBJECT

public:
	explicit Importer(QObject *parent = nullptr);

	virtual QWidget* getOptionsWidget() = 0;
	virtual QString getSuggestedPath(const QString &path = QString()) const = 0;
	virtual QString getBrowser() const = 0;
	QUrl getUpdateUrl() const;
	virtual QStringList getFileFilters() const = 0;
	AddonType getType() const;
	virtual ImportType getImportType() const = 0;

public slots:
	virtual bool import(const QString &path) = 0;

signals:
	void importProgress(int amount, int total, ImportType type);
};

}

#endif
