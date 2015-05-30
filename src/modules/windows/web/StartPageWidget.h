/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
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

#ifndef OTTER_STARTPAGEWIDGET_H
#define OTTER_STARTPAGEWIDGET_H

#include <QtWidgets/QListView>
#include <QtWidgets/QScrollArea>

namespace Otter
{

class SearchWidget;
class StartPageModel;
class Window;

class StartPageWidget : public QScrollArea
{
	Q_OBJECT

public:
	explicit StartPageWidget(Otter::Window *window, QWidget *parent = NULL);

	bool eventFilter(QObject *object, QEvent *event);

protected:
	void resizeEvent(QResizeEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	int getTilesPerRow() const;

protected slots:
	void optionChanged(const QString &option);
	void configure();
	void addTile();
	void addTile(const QUrl &url);
	void openTile();
	void editTile();
	void reloadTile(bool full = false);
	void removeTile();
	void updateSize();

private:
	QListView *m_listView;
	SearchWidget *m_searchWidget;

	static StartPageModel *m_model;
};

}

#endif
