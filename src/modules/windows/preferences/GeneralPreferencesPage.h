/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_GENERALPREFERENCESPAGE_H
#define OTTER_GENERALPREFERENCESPAGE_H

#include "../../../ui/CategoriesTabWidget.h"

namespace Otter
{

namespace Ui
{
	class GeneralPreferencesPage;
}

class GeneralPreferencesPage final : public CategoryPage
{
	Q_OBJECT

public:
	explicit GeneralPreferencesPage(QWidget *parent);
	~GeneralPreferencesPage();

	void load() override;
	QString getTitle() const override;

public slots:
	void save() override;

protected:
	void changeEvent(QEvent *event) override;

private:
	QString m_acceptLanguage;
	Ui::GeneralPreferencesPage *m_ui;
};

}

#endif
