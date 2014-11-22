/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2009 Jakub Wieczorek <faw217@gmail.com>
* Copyright (C) 2009 by Benjamin C. Meyer <ben@meyerhome.net>
* Copyright (C) 2010-2011 by Matthieu Gicquel <matgic78@gmail.com>
* Copyright (C) 2014 Martin Rejda <rejdi@otter.ksp.sk>
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

#ifndef OTTER_LOADPLUGINWIDGET_H
#define OTTER_LOADPLUGINWIDGET_H

#include <QtCore/QUrl>
#include <QtWebKit/QWebElement>
#include <QtWebKitWidgets/QWebPage>

namespace Otter
{

class LoadPluginWidget : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(bool isSwapping READ isSwapping)

public:
	explicit LoadPluginWidget(QWebPage *page, const QString &mimeType, const QUrl &url, QWidget *parent = NULL);
	bool isSwapping() const;

public slots:
	void loadPlugin();

protected:
	void paintEvent(QPaintEvent *event);
	void enterEvent(QEvent *event);
	void leaveEvent(QEvent *event);
	void mousePressEvent(QMouseEvent *event);

private:
	QWebPage *m_page;
	bool m_isHovered;
	bool m_isSwapping;

signals:
	void pluginLoaded();
};

}

#endif
