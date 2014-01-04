/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ADDRESSWIDGET_H
#define OTTER_ADDRESSWIDGET_H

#include <QtCore/QUrl>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

namespace Otter
{

class Window;

class AddressWidget : public QLineEdit
{
	Q_OBJECT

public:
	explicit AddressWidget(QWidget *parent = NULL);

	void setWindow(Window *window);
	QUrl getUrl() const;
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void setUrl(const QUrl &url);

protected:
	void resizeEvent(QResizeEvent *event);
	void mousePressEvent(QMouseEvent *event);

protected slots:
	void removeIcon();
	void optionChanged(const QString &option, const QVariant &value);
	void notifyRequestedLoadUrl();
	void updateBookmark();
	void setIcon(const QIcon &icon);

private:
	Window *m_window;
	QCompleter *m_completer;
	QLabel *m_bookmarkLabel;
	QLabel *m_urlIconLabel;

signals:
	void requestedLoadUrl(QUrl url);
	void requestedSearch(QString query, QString engine);
};

}

#endif
