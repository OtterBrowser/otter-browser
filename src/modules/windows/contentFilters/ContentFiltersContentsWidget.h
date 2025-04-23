/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_CONTENTFILTERSCONTENTSWIDGET_H
#define OTTER_CONTENTFILTERSCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"

namespace Otter
{

namespace Ui
{
	class ContentFiltersContentsWidget;
}

class Window;

class ContentFiltersContentsWidget final : public ActiveWindowObserverContentsWidget
{
	Q_OBJECT

public:
	explicit ContentFiltersContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent);
	~ContentFiltersContentsWidget();

	void print(QPrinter *printer) override;
	QString getTitle() const override;
	QLatin1String getType() const override;
	QUrl getUrl() const override;
	QIcon getIcon() const override;

protected:
	void changeEvent(QEvent *event) override;
	void initializeSettingsPage();
	bool canClose() override;

protected slots:
	void updateActions();

private:
	bool m_isSettingsPageInitialized;
	Ui::ContentFiltersContentsWidget *m_ui;
};

}

#endif
