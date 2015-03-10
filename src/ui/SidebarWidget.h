/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_SIDEBARWIDGET_H
#define OTTER_SIDEBARWIDGET_H

#include "../core/WindowsManager.h"

#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class SidebarWidget;
}

class SidebarWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SidebarWidget(QWidget *parent = NULL);
	~SidebarWidget();

	void changeEvent(QEvent *event);
	void openPanel();
	void selectPanel(const QString &identifier);
	void setButtonsEdge(Qt::Edge);
	static QString getPanelTitle(const QString &identifier);
	QSize sizeHint() const;

protected:
	void registerPanel(const QString &identifier);
	void updatePanelsMenu();

protected slots:
	void addWebPanel();
	void choosePanel(bool checked);
	void openUrl(const QUrl &url, OpenHints);
	void optionChanged(const QString &option, const QVariant &value);
	void selectPanel();

private:
	QWidget *m_currentWidget;
	QString m_currentPanel;
	QHash<QString, QToolButton*> m_buttons;
	Ui::SidebarWidget *m_ui;
};

}

#endif
