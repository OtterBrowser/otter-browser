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

class StartPageContentsWidget : public QWidget
{
public:
	enum BackgroundMode
	{
		NoCustomBackground = 0,
		BestFitBackground,
		CenterBackground,
		StretchBackground,
		TileBackground
	};

	explicit StartPageContentsWidget(QWidget *parent);

	void setBackgroundMode(BackgroundMode mode);

protected:
	void paintEvent(QPaintEvent *event);

private:
	QString m_backgroundPath;
	BackgroundMode m_backgroundMode;
};

class StartPageWidget : public QScrollArea
{
	Q_OBJECT

public:
	explicit StartPageWidget(Otter::Window *window, QWidget *parent = NULL);

	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void showContextMenu(const QPoint &position = QPoint());

protected:
	void resizeEvent(QResizeEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);
	int getTilesPerRow() const;

protected slots:
	void optionChanged(const QString &option);
	void configure();
	void addTile();
	void addTile(const QUrl &url);
	void openTile();
	void editTile();
	void reloadTile();
	void removeTile();
	void updateTile(const QModelIndex &index);
	void updateSize();
	void updateTiles();

private:
	StartPageContentsWidget *m_contentsWidget;
	QListView *m_listView;
	SearchWidget *m_searchWidget;
	QModelIndex m_currentIndex;
	bool m_ignoreEnter;

	static StartPageModel *m_model;
};

}

#endif
