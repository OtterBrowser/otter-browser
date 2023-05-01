/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2023 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_DATAEXCHANGER_H
#define OTTER_DATAEXCHANGER_H

#include "AddonsManager.h"
#include "BookmarksModel.h"
#include "Job.h"

namespace Otter
{

class DataExchanger : public QObject, public Addon
{
	Q_OBJECT

public:
	enum ExchangeDirection
	{
		UnknownDirection = 0,
		ExportDirection,
		ImportDirection
	};

	enum ExchangeType
	{
		UnknownExchange = 0,
		FullExchange = 1,
		BookmarksExchange = 2,
		SettingsExchange = 4,
		PasswordsExchange = 8,
		SearchEnginesExchange = 16,
		SessionsExchange = 32,
		CookiesExchange = 64,
		HistoryExchange = 128,
		FeedsExchange = 256,
		MailExchange = 512,
		NotesExchange = 1024,
		OtherExchange = 2048
	};

	enum OperationResult
	{
		SuccessfullOperation = 0,
		FailedOperation,
		CancelledOperation
	};

	explicit DataExchanger(QObject *parent = nullptr);

	virtual QWidget* createOptionsWidget(QWidget *parent);
	virtual QString getSuggestedPath(const QString &path = {}) const = 0;
	virtual QString getGroup() const = 0;
	virtual QStringList getFileFilters() const = 0;
	AddonType getType() const override;
	virtual ExchangeDirection getExchangeDirection() const = 0;
	virtual ExchangeType getExchangeType() const = 0;
	virtual bool canCancel() const;
	virtual bool hasOptions() const;

public slots:
	virtual void cancel();

signals:
	void exchangeStarted(ExchangeType type, int total);
	void exchangeProgress(ExchangeType type, int total, int amount);
	void exchangeFinished(ExchangeType type, OperationResult result, int total);
};

class ExportDataExchanger : public DataExchanger
{
	Q_OBJECT

public:
	explicit ExportDataExchanger(QObject *parent);

	ExchangeDirection getExchangeDirection() const override;

public slots:
	virtual bool exportData(const QString &path, bool canOverwriteExisting) = 0;
};

class ImportDataExchanger : public DataExchanger
{
	Q_OBJECT

public:
	explicit ImportDataExchanger(QObject *parent);

	ExchangeDirection getExchangeDirection() const override;

public slots:
	virtual bool importData(const QString &path) = 0;
};

class ImportJob : public Job
{
	Q_OBJECT

public:
	explicit ImportJob(QObject *parent = nullptr);

signals:
	void importStarted(DataExchanger::ExchangeType type, int total);
	void importProgress(DataExchanger::ExchangeType type, int total, int amount);
	void importFinished(DataExchanger::ExchangeType type, DataExchanger::OperationResult result, int total);
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
