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

#include "MainWindow.h"

#include <QtCore/QUrl>
#include <QtNetwork/QHostInfo>
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
	explicit AddressWidget(Window *window, bool simpleMode = false, QWidget *parent = NULL);

	void handleUserInput(const QString &text, OpenHints hints = DefaultOpen);
	void setWindow(Window *window);
	QUrl getUrl() const;
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void setUrl(const QUrl &url);

protected:
	void timerEvent(QTimerEvent *event);
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void focusInEvent(QFocusEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void removeIcon();
	void verifyLookup(const QHostInfo &host);
	void notifyRequestedLoadUrl();
	void updateBookmark();
	void updateLoadPlugins();
	void updateIcons();
	void setCompletion(const QString &text);
	void setIcon(const QIcon &icon);

private:
	Window *m_window;
	QCompleter *m_completer;
	QLabel *m_bookmarkLabel;
	QLabel *m_loadPluginsLabel;
	QLabel *m_urlIconLabel;
	QRect m_securityBadgeRectangle;
	QString m_lookupQuery;
	OpenHints m_hints;
	int m_lookupIdentifier;
	int m_lookupTimer;
	bool m_simpleMode;

signals:
	void requestedOpenUrl(QUrl url, OpenHints hints = DefaultOpen);
	void requestedSearch(QString query, QString engine, OpenHints hints = DefaultOpen);
};

}

#endif
