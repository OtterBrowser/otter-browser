/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef CONTENTBLOCKINGPROFILEDIALOG
#define CONTENTBLOCKINGPROFILEDIALOG

#include "Dialog.h"
#include "../core/ContentFiltersManager.h"

namespace Otter
{

namespace Ui
{
	class ContentBlockingProfileDialog;
}

class ContentFiltersProfile;
class DataFetchJob;

class ContentBlockingProfileDialog final : public Dialog
{
	Q_OBJECT

public:
	explicit ContentBlockingProfileDialog(const ContentFiltersProfile::ProfileSummary &profileSummary, const QString &rulesPath = {}, QWidget *parent = nullptr);
	~ContentBlockingProfileDialog();

	QString getRulesPath() const;
	ContentFiltersProfile::ProfileSummary getProfile() const;

protected:
	void closeEvent(QCloseEvent *event) override;
	void changeEvent(QEvent *event) override;
	QString createTemporaryFile();

protected slots:
	void handleCurrentTabChanged(int index);
	void handleJobFinished(bool isSuccess);
	void saveSource();

private:
	DataFetchJob *m_dataFetchJob;
	QString m_name;
	QString m_rulesPath;
	QDateTime m_lastUpdate;
	bool m_isSourceLoaded;
	bool m_isUsingTemporaryFile;
	Ui::ContentBlockingProfileDialog *m_ui;
};

}

#endif
