/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_JOB_H
#define OTTER_JOB_H

#include "SearchEnginesManager.h"

#include <QtNetwork/QNetworkReply>

namespace Otter
{

class Job : public QObject
{
	Q_OBJECT

public:
	explicit Job(QObject *parent = nullptr);

public slots:
	virtual void cancel() = 0;

signals:
	void jobFinished(bool isSuccess);
};

class SearchEngineFetchJob final : public Job
{
	Q_OBJECT

public:
	explicit SearchEngineFetchJob(const QUrl &url, const QString &identifier = {}, bool saveSearchEngine = true, QObject *parent = nullptr);
	~SearchEngineFetchJob();

	SearchEnginesManager::SearchEngineDefinition getSearchEngine() const;

public slots:
	void cancel() override;

protected slots:
	void handleDefinitionRequestFinished();
	void handleIconRequestFinished();

private:
	QNetworkReply *m_reply;
	SearchEnginesManager::SearchEngineDefinition m_searchEngine;
	bool m_needsToSaveSearchEngine;
};

}

#endif
