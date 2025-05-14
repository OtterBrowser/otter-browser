/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_CONTENTBLOCKINGINFORMATIONWIDGET_H
#define OTTER_CONTENTBLOCKINGINFORMATIONWIDGET_H

#include "../../../ui/ToolButtonWidget.h"

namespace Otter
{

class Window;

class ContentBlockingInformationWidget final : public ToolButtonWidget
{
	Q_OBJECT

public:
	explicit ContentBlockingInformationWidget(Window *window, const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent = nullptr);

	QString getText() const override;
	QIcon getIcon() const override;

protected:
	void changeEvent(QEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void updateState();

protected slots:
	void clear();
	void openElement(QAction *action);
	void toggleContentBlocking();
	void toggleOption(QAction *action);
	void populateElementsMenu();
	void populateProfilesMenu();
	void handleBlockedRequest();
	void setWindow(Window *window);

private:
	Window *m_window;
	QMenu *m_elementsMenu;
	QMenu *m_profilesMenu;
	QIcon m_icon;
	int m_requestsAmount;
	bool m_isContentBlockingEnabled;
};

}

#endif
