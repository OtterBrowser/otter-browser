/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2019 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_IMPORTER_H
#define OTTER_IMPORTER_H

#include "AddonsManager.h"
#include "BookmarksModel.h"
#include "Job.h"

namespace Otter
{

class Importer : public QObject, public Addon
{
	Q_OBJECT

public:
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

	enum ImportResult
	{
		SuccessfullImport = 0,
		FailedImport,
		CancelledImport
	};

	struct ImporterInformation final : public AddonsManager::AddonInformation
	{
		QString group;
	};

	explicit Importer(QObject *parent = nullptr);

	virtual QWidget* createOptionsWidget(QWidget *parent) = 0;
	virtual QString getSuggestedPath(const QString &path = {}) const = 0;
	virtual QString getGroup() const = 0;
	virtual QStringList getFileFilters() const = 0;
	AddonType getType() const override;
	virtual ImportType getImportType() const = 0;
	virtual bool canCancel();

public slots:
	virtual void cancel();
	virtual bool import(const QString &path) = 0;

protected slots:
	void notifyImportStarted(int type, int total);
	void notifyImportProgress(int type, int total, int amount);
	void notifyImportFinished(int type, int result, int total);

signals:
	void importStarted(ImportType type, int total);
	void importProgress(ImportType type, int total, int amount);
	void importFinished(ImportType type, ImportResult result, int total);
};

class ImportJob : public Job
{
	Q_OBJECT

public:
	explicit ImportJob(QObject *parent = nullptr);

signals:
	void importStarted(Importer::ImportType type, int total);
	void importProgress(Importer::ImportType type, int total, int amount);
	void importFinished(Importer::ImportType type, Importer::ImportResult result, int total);
};

class BookmarksImportJob : public ImportJob
{
	Q_OBJECT

public:
	explicit BookmarksImportJob(BookmarksModel::Bookmark *folder, bool areDuplicatesAllowed, QObject *parent = nullptr);

protected:
	void goToParent();
	void setCurrentFolder(BookmarksModel::Bookmark *folder);
	BookmarksModel::Bookmark* getCurrentFolder() const;
	BookmarksModel::Bookmark* getImportFolder() const;
	QDateTime getDateTime(const QString &timestamp) const;
	bool areDuplicatesAllowed() const;

private:
	BookmarksModel::Bookmark *m_currentFolder;
	BookmarksModel::Bookmark *m_importFolder;
	bool m_areDuplicatesAllowed;
};

}

#endif
