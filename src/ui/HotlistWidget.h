/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_PANELWIDGET_H
#define OTTER_PANELWIDGET_H

#include "Window.h"

#include <QtGui/QIcon>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class HotlistWidget;
}

class HotlistWidget : public QWidget
{
	Q_OBJECT

public:
	explicit HotlistWidget(QWidget *parent = NULL);
	~HotlistWidget();

	QSize sizeHint() const;

public slots:
	void locationChanged(Qt::DockWidgetArea area);

protected:
	void openPanel(const QString &identifier);
	void registerPanel(const QString &identifier, const QIcon &icon = QIcon());

protected slots:
	void openPanel();
	void openUrl(const QUrl &url, OpenHints);
	void optionChanged(const QString &option, const QVariant &value);

private:
	QWidget *m_currentWidget;
	QString m_currentPanel;
	QHash<QString, QToolButton*> m_buttons;
	Ui::HotlistWidget *m_ui;
};

}

#endif
