/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

	void openPanel();
	void selectPanel(const QString &identifier);
	static QString getPanelTitle(const QString &identifier);
	QSize sizeHint() const;

public slots:
	void scheduleSizeSave();

protected:
	void timerEvent(QTimerEvent *event);
	void changeEvent(QEvent *event);
	void registerPanel(const QString &identifier);
	void updatePanelsMenu();

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void addWebPanel();
	void choosePanel(bool checked);
	void selectPanel();

private:
	QWidget *m_currentWidget;
	QString m_currentPanel;
	QHash<QString, QToolButton*> m_buttons;
	int m_resizeTimer;
	Ui::SidebarWidget *m_ui;
};

}

#endif
