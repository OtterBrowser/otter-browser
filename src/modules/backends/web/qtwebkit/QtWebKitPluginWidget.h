/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Martin Rejda <rejdi@otter.ksp.sk>
* Copyright (C) 2014 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_QTWEBKITPLUGINWIDGET_H
#define OTTER_QTWEBKITPLUGINWIDGET_H

#include <QtCore/QUrl>
#include <QtWidgets/QWidget>

namespace Otter
{

class QtWebKitPluginWidget : public QWidget
{
	Q_OBJECT

public:
	explicit QtWebKitPluginWidget(const QString &mimeType, const QUrl &url, QWidget *parent = nullptr);

protected:
	void changeEvent(QEvent *event);
	void paintEvent(QPaintEvent *event);
	void enterEvent(QEvent *event);
	void leaveEvent(QEvent *event);

private:
	QString m_mimeType;
	QString m_url;
	bool m_isHovered;
};

}

#endif
