/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 - 2017 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_STARTPAGEWIDGET_H
#define OTTER_STARTPAGEWIDGET_H

#include "StartPagePreferencesDialog.h"
#include "../../../core/ActionsManager.h"

#include <QtCore/QTime>
#include <QtWidgets/QListView>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QStyledItemDelegate>

namespace Otter
{

class Animation;
class SearchWidget;
class StartPageModel;
class Window;

class TileDelegate final : public QStyledItemDelegate
{
	Q_OBJECT

public:
	enum BackgroundMode
	{
		NoBackground = 0,
		FaviconBackground,
		ThumbnailBackground
	};

	explicit TileDelegate(QWidget *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setPixmapCachePrefix(const QString &prefix);
	QString createPixmapCacheKey(const QRect &rectangle, quint64 identifier) const;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
	void drawAnimation(QPainter *painter, const QRect &rectangle) const;
#ifdef OTTER_ENABLE_STARTPAGEBLUR
	void drawBlurBehind(QPainter *painter, const QRect &rectangle) const;
#endif
	void drawFocusIndicator(QPainter *painter, const QPainterPath &path, const QStyleOptionViewItem &option, QPalette::ColorGroup colorGroup) const;

protected slots:
	void handleOptionChanged(int identifier, const QVariant &value);

private:
	QWidget *m_widget;
	QString m_pixmapCachePrefix;
	BackgroundMode m_mode;
#ifdef OTTER_ENABLE_STARTPAGEBLUR
	bool m_needsBlur;
#endif
};

class StartPageContentsWidget final : public QWidget
{
	Q_OBJECT

public:
	enum BackgroundMode
	{
		DefaultBackground = 0,
		NoBackground,
		BestFitBackground,
		CenterBackground,
		StretchBackground,
		TileBackground
	};

	explicit StartPageContentsWidget(QWidget *parent);

	void setBackgroundMode(BackgroundMode mode);
	QString getPixmapCachePrefix() const;

protected:
	void paintEvent(QPaintEvent *event) override;

protected slots:
	void handleOptionChanged(int identifier);

private:
	QString m_backgroundPath;
	QColor m_backgroundColor;
	BackgroundMode m_backgroundMode;
};

class StartPageWidget final : public QScrollArea
{
	Q_OBJECT

public:
	explicit StartPageWidget(Window *parent);
	~StartPageWidget();

	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger);
	void scrollContents(const QPoint &delta);
	void markForDeletion();
	static Animation* getLoadingAnimation();
	QPixmap createThumbnail();
	bool event(QEvent *event) override;
	bool eventFilter(QObject *object, QEvent *event) override;

protected:
	void timerEvent(QTimerEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void startReloadingAnimation();

protected slots:
	void configure();
	void addTile();
	void openTile();
	void editTile();
	void reloadTile();
	void removeTile();
	void handleOptionChanged(int identifier, const QVariant &value);
	void handleIsReloadingTileChanged(const QModelIndex &index);
	void updateSize();
	void showContextMenu(const QPoint &position = {});

private:
	Window *m_window;
	StartPageContentsWidget *m_contentsWidget;
	QListView *m_listView;
	SearchWidget *m_searchWidget;
	TileDelegate *m_tileDelegate;
	QPixmap m_thumbnail;
	QTime m_urlOpenTime;
	QModelIndex m_currentIndex;
	int m_deleteTimer;
	bool m_isIgnoringEnter;

	static StartPageModel *m_model;
	static Animation *m_spinnerAnimation;
	static QPointer<StartPagePreferencesDialog> m_preferencesDialog;
};

}

#endif
