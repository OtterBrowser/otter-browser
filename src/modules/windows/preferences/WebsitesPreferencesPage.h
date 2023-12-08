/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2021 - 2023 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_WEBSITESPREFERENCESPAGE_H
#define OTTER_WEBSITESPREFERENCESPAGE_H

#include "../../../ui/CategoriesTabWidget.h"

namespace Otter
{

namespace Ui
{
	class WebsitesPreferencesPage;
}

class WebsitesPreferencesPage final : public CategoryPage
{
	Q_OBJECT

public:
	explicit WebsitesPreferencesPage(QWidget *parent);
	~WebsitesPreferencesPage();

	void load() override;
	QString getTitle() const override;

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void addWebsite();
	void editWebsite();
	void removeWebsite();
	void updateWebsiteActions();

private:
	Ui::WebsitesPreferencesPage *m_ui;
};

}

#endif
