/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2016 - 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

class ContentBlockingProfileDialog final : public Dialog
{
	Q_OBJECT

public:
	struct ProfileSummary
	{
		QString name;
		QString title;
		QUrl updateUrl;
		ContentFiltersProfile::ProfileCategory category = ContentFiltersProfile::OtherCategory;
		int updateInterval = -1;
	};

	explicit ContentBlockingProfileDialog(const ProfileSummary &profileSummary, QWidget *parent = nullptr);
	~ContentBlockingProfileDialog();

	ProfileSummary getProfile() const;

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void handleCurrentTabChanged(int index);
	void saveSource();

private:
	QString m_name;
	bool m_isSourceLoaded;
	Ui::ContentBlockingProfileDialog *m_ui;
};

}

#endif
