/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_SIDEBARWIDGET_H
#define OTTER_SIDEBARWIDGET_H

#include "ToolBarWidget.h"
#include "../core/WindowsManager.h"

#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class SidebarWidget;
}

class ResizerWidget;

class SidebarWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SidebarWidget(ToolBarWidget *parent);
	~SidebarWidget();

	static QString getPanelTitle(const QString &identifier);
	static QUrl getPanelUrl(const QString &identifier);
	QSize sizeHint() const override;

protected:
	void changeEvent(QEvent *event) override;
	void selectPanel(const QString &identifier);

protected slots:
	void addWebPanel();
	void choosePanel(bool checked);
	void selectPanel();
	void saveSize();
	void updateLayout();
	void updatePanels();

private:
	ToolBarWidget *m_toolBarWidget;
	ResizerWidget *m_resizerWidget;
	QString m_currentPanel;
	QHash<QString, QToolButton*> m_buttons;
	QHash<QString, QWidget*> m_panels;
	Ui::SidebarWidget *m_ui;
};

}

#endif
