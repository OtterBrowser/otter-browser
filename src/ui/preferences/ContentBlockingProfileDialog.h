/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef CONTENTBLOCKINGPROFILEDIALOG
#define CONTENTBLOCKINGPROFILEDIALOG

#include "../Dialog.h"

namespace Otter
{

namespace Ui
{
	class ContentBlockingProfileDialog;
}

class ContentBlockingProfile;

class ContentBlockingProfileDialog : public Dialog
{
	Q_OBJECT

public:
	explicit ContentBlockingProfileDialog(QWidget *parent = nullptr, ContentBlockingProfile *profile = nullptr);
	~ContentBlockingProfileDialog();

	ContentBlockingProfile* getProfile();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void save();

private:
	ContentBlockingProfile *m_profile;
	Ui::ContentBlockingProfileDialog *m_ui;
};

}

#endif

