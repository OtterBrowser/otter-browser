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

#include "StartPageWidget.h"
#include "StartPageModel.h"
#include "StartPagePreferencesDialog.h"
#include "TileDelegate.h"
#include "WebContentsWidget.h"
#include "../../../core/BookmarksModel.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/BookmarkPropertiesDialog.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/Menu.h"
#include "../../../ui/OpenAddressDialog.h"
#include "../../../ui/toolbars/SearchWidget.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmapCache>
#include <QtCore/QtMath>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QScrollBar>

namespace Otter
{

StartPageModel* StartPageWidget::m_model = NULL;

StartPageContentsWidget::StartPageContentsWidget(QWidget *parent) : QWidget(parent),
	m_backgroundMode(NoCustomBackground)
{
	setAutoFillBackground(true);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void StartPageContentsWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	painter.fillRect(geometry(), Qt::transparent);

	if (m_backgroundMode != NoCustomBackground && !m_backgroundPath.isEmpty())
	{
		QPixmap pixmap(m_backgroundPath);

		if (pixmap.isNull())
		{
			return;
		}

		switch (m_backgroundMode)
		{
			case BestFitBackground:
				{
					const QString key = QLatin1String("start-page-best-fit-") + QString::number(width()) + QLatin1Char('-') + QString::number(height());
					QPixmap *cachedBackground = QPixmapCache::find(key);

					if (cachedBackground)
					{
						painter.drawPixmap(contentsRect(), *cachedBackground, contentsRect());
					}
					else
					{
						const qreal pixmapAscpectRatio = (pixmap.width() / qreal(pixmap.height()));
						const qreal backgroundAscpectRatio = (width() / qreal(height()));
						QPixmap newBackground(size());

						if (pixmapAscpectRatio > backgroundAscpectRatio)
						{
							newBackground = pixmap.scaled(QSize((width() / backgroundAscpectRatio), height()), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
						}
						else if (backgroundAscpectRatio > pixmapAscpectRatio)
						{
							newBackground = pixmap.scaled(QSize(width(), (height() * backgroundAscpectRatio)), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
						}
						else
						{
							newBackground = pixmap.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
						}

						painter.drawPixmap(contentsRect(), newBackground, contentsRect());

						QPixmapCache::insert(key, newBackground);
					}
				}

				break;
			case CenterBackground:
				painter.drawPixmap((contentsRect().center() - pixmap.rect().center()), pixmap);

				break;
			case StretchBackground:
				{
					const QString key = QLatin1String("start-page-stretch-") + QString::number(width()) + QLatin1Char('-') + QString::number(height());
					QPixmap *cachedBackground = QPixmapCache::find(key);

					if (cachedBackground)
					{
						painter.drawPixmap(contentsRect(), *cachedBackground, contentsRect());
					}
					else
					{
						const QPixmap newBackground = pixmap.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

						painter.drawPixmap(contentsRect(), newBackground, contentsRect());

						QPixmapCache::insert(key, newBackground);
					}
				}

				break;
			case TileBackground:
				painter.drawTiledPixmap(contentsRect(), pixmap);

				break;
			default:
				break;
		}
	}
}

void StartPageContentsWidget::setBackgroundMode(StartPageContentsWidget::BackgroundMode mode)
{
	m_backgroundMode = mode;
	m_backgroundPath = ((mode == NoCustomBackground) ? QString() : SettingsManager::getValue(QLatin1String("StartPage/BackgroundPath")).toString());

	update();
}

StartPageWidget::StartPageWidget(Window *window, QWidget *parent) : QScrollArea(parent),
	m_contentsWidget(new StartPageContentsWidget(this)),
	m_listView(new QListView(this)),
	m_searchWidget(new SearchWidget(window, this))
{
	if (!m_model)
	{
		m_model = new StartPageModel(QCoreApplication::instance());
	}

	QGridLayout *layout = new QGridLayout(m_contentsWidget);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addItem(new QSpacerItem(1, 50, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 1);
	layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding), 1, 0);
	layout->addWidget(m_searchWidget, 1, 1, 1, 1, Qt::AlignCenter);
	layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding), 1, 2);
	layout->addItem(new QSpacerItem(1, 50, QSizePolicy::Fixed, QSizePolicy::Fixed), 2, 1);
	layout->addWidget(m_listView, 3, 0, 1, 3, Qt::AlignCenter);
	layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), 4, 1);

	m_searchWidget->setFixedWidth(300);

	m_listView->setFrameStyle(QFrame::NoFrame);
	m_listView->setStyleSheet(QLatin1String("QListView {padding:0;border:0;background:transparent;}"));
	m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_listView->setDragEnabled(true);
	m_listView->setDragDropMode(QAbstractItemView::DragDrop);
	m_listView->setDefaultDropAction(Qt::CopyAction);
	m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_listView->setViewMode(QListView::IconMode);
	m_listView->setModel(m_model);
	m_listView->setItemDelegate(new TileDelegate(m_listView));
	m_listView->viewport()->setAttribute(Qt::WA_Hover);
	m_listView->viewport()->setMouseTracking(true);
	m_listView->viewport()->installEventFilter(this);

	setWidget(m_contentsWidget);
	setWidgetResizable(true);
	setAlignment(Qt::AlignHCenter);
	updateTiles();
	optionChanged(QLatin1String("StartPage/BackgroundPath"));

	connect(m_model, SIGNAL(modelModified()), this, SLOT(updateTiles()));
	connect(m_model, SIGNAL(isReloadingTileChanged(QModelIndex)), this, SLOT(updateTile(QModelIndex)));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

void StartPageWidget::resizeEvent(QResizeEvent *event)
{
	QScrollArea::resizeEvent(event);

	updateSize();

	if (widget()->width() > width() && horizontalScrollBar()->value() == 0)
	{
		horizontalScrollBar()->setValue((widget()->width() / 2) - (width() / 2));
	}
}

void StartPageWidget::contextMenuEvent(QContextMenuEvent *event)
{
	const QModelIndex index(m_listView->indexAt(m_listView->mapFromGlobal(event->globalPos())));
	QMenu menu(this);

	if (index.isValid() && index.data(Qt::AccessibleDescriptionRole).toString() != QLatin1String("add"))
	{
		menu.addAction(tr("Open"), this, SLOT(openTile()));
		menu.addSeparator();
		menu.addAction(tr("Edit…"), this, SLOT(editTile()));
		menu.addAction(tr("Reload"), this, SLOT(reloadTile()))->setEnabled(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark);
		menu.addSeparator();
		menu.addAction(tr("Delete"), this, SLOT(removeTile()));
	}
	else
	{
		menu.addAction(tr("Configure…"), this, SLOT(configure()));
		menu.addAction(tr("Add Tile…"), this, SLOT(addTile()));
	}

	menu.exec(event->globalPos());

	event->accept();
}

void StartPageWidget::optionChanged(const QString &option)
{
	if (option == QLatin1String("StartPage/TilesPerRow") || option == QLatin1String("StartPage/TileHeight") || option == QLatin1String("StartPage/TileWidth") || option == QLatin1String("StartPage/ZoomLevel"))
	{
		updateSize();
	}
	else if (option == QLatin1String("StartPage/BackgroundMode") || option == QLatin1String("StartPage/BackgroundPath"))
	{
		const QString backgroundMode = SettingsManager::getValue(QLatin1String("StartPage/BackgroundMode")).toString();

		if (backgroundMode == QLatin1String("bestFit"))
		{
			m_contentsWidget->setBackgroundMode(StartPageContentsWidget::BestFitBackground);
		}
		else if (backgroundMode == QLatin1String("center"))
		{
			m_contentsWidget->setBackgroundMode(StartPageContentsWidget::CenterBackground);
		}
		else if (backgroundMode == QLatin1String("stretch"))
		{
			m_contentsWidget->setBackgroundMode(StartPageContentsWidget::StretchBackground);
		}
		else if (backgroundMode == QLatin1String("tile"))
		{
			m_contentsWidget->setBackgroundMode(StartPageContentsWidget::TileBackground);
		}
		else
		{
			m_contentsWidget->setBackgroundMode(StartPageContentsWidget::NoCustomBackground);
		}
	}
}

void StartPageWidget::configure()
{
	StartPagePreferencesDialog dialog(this);
	dialog.exec();
}

void StartPageWidget::addTile()
{
	OpenAddressDialog dialog(this);

	connect(&dialog, SIGNAL(requestedLoadUrl(QUrl,OpenHints)), this, SLOT(addTile(QUrl)));

	dialog.exec();
}

void StartPageWidget::addTile(const QUrl &url)
{
	BookmarksItem *bookmark = BookmarksManager::getModel()->addBookmark(BookmarksModel::UrlBookmark, 0, url, QString(), BookmarksManager::getModel()->getItem(SettingsManager::getValue(QLatin1String("StartPage/BookmarksFolder")).toString()));

	if (bookmark)
	{
		m_model->reloadTile(bookmark->index(), true);
	}
}

void StartPageWidget::openTile()
{
	const QModelIndex index = m_listView->currentIndex();

	if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("add"))
	{
		addTile();

		return;
	}

	const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt());

	if (type == BookmarksModel::FolderBookmark)
	{
		MainWindow *mainWindow = MainWindow::findMainWindow(this);
		BookmarksItem *bookmark = BookmarksManager::getModel()->getBookmark(index.data(BookmarksModel::IdentifierRole).toULongLong());

		if (mainWindow && bookmark && bookmark->rowCount() > 0)
		{
			mainWindow->getWindowsManager()->open(bookmark);
		}

		return;
	}

	if (!index.isValid() || type != BookmarksModel::UrlBookmark)
	{
		return;
	}

	const QUrl url(index.data(BookmarksModel::UrlRole).toUrl());

	if (url.isValid())
	{
		qobject_cast<WebContentsWidget*>(parentWidget())->setUrl(url);
	}
}

void StartPageWidget::editTile()
{
	BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(BookmarksManager::getModel()->getBookmark(m_listView->currentIndex().data(BookmarksModel::IdentifierRole).toULongLong()));

	if (bookmark)
	{
		BookmarkPropertiesDialog(bookmark, (bookmark->isInTrash() ? BookmarkPropertiesDialog::ViewBookmarkMode : BookmarkPropertiesDialog::EditBookmarkMode), this).exec();

		m_model->reloadModel();
	}
}

void StartPageWidget::reloadTile()
{
	m_model->reloadTile(m_listView->currentIndex());

	m_listView->openPersistentEditor(m_listView->currentIndex());
}

void StartPageWidget::removeTile()
{
	BookmarksItem *bookmark = dynamic_cast<BookmarksItem*>(BookmarksManager::getModel()->getBookmark(m_listView->currentIndex().data(BookmarksModel::IdentifierRole).toULongLong()));

	if (bookmark)
	{
		const QString path = SessionsManager::getWritableDataPath(QLatin1String("thumbnails/")) + QString::number(bookmark->data(BookmarksModel::IdentifierRole).toULongLong()) + QLatin1String(".png");

		if (QFile::exists(path))
		{
			QFile::remove(path);
		}

		bookmark->remove();
	}
}

void StartPageWidget::updateTile(const QModelIndex &index)
{
	m_listView->update(index);
	m_listView->closePersistentEditor(index);
}

void StartPageWidget::updateSize()
{
	const qreal zoom = (SettingsManager::getValue(QLatin1String("StartPage/ZoomLevel")).toInt() / qreal(100));
	const int tileHeight = (SettingsManager::getValue(QLatin1String("StartPage/TileHeight")).toInt() * zoom);
	const int tileWidth = (SettingsManager::getValue(QLatin1String("StartPage/TileWidth")).toInt() * zoom);
	const int amount = m_model->rowCount();
	const int columns = getTilesPerRow();
	const int rows = qCeil(amount / qreal(columns));

	m_listView->setGridSize(QSize(tileWidth, tileHeight));
	m_listView->setFixedSize(((qMin(amount, columns) * tileWidth) + 2), ((rows * tileHeight) + 20));
}

void StartPageWidget::updateTiles()
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		if (m_model->item(i) && m_model->isReloadingTile(m_model->index(i, 0)))
		{
			m_listView->openPersistentEditor(m_model->index(i, 0));
		}
	}

	updateSize();
}

int StartPageWidget::getTilesPerRow() const
{
	const int tilesPerRow = SettingsManager::getValue(QLatin1String("StartPage/TilesPerRow")).toInt();

	if (tilesPerRow > 0)
	{
		return tilesPerRow;
	}

	return qMax(1, int((width() - 50) / (SettingsManager::getValue(QLatin1String("StartPage/TileWidth")).toInt() * (SettingsManager::getValue(QLatin1String("StartPage/ZoomLevel")).toInt() / qreal(100)))));
}

bool StartPageWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonRelease && object == m_listView->viewport())
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent)
		{
			const QModelIndex index = m_listView->currentIndex();
			const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt());

			if (type == BookmarksModel::FolderBookmark)
			{
				BookmarksItem *bookmark = BookmarksManager::getModel()->getBookmark(index.data(BookmarksModel::IdentifierRole).toULongLong());

				if (bookmark && bookmark->rowCount() > 0)
				{
					Menu menu(Menu::BookmarksMenuRole, this);
					menu.menuAction()->setData(bookmark->index());
					menu.exec(mouseEvent->globalPos());
				}

				return true;
			}

			if (!index.isValid() || type != BookmarksModel::UrlBookmark)
			{
				if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("add"))
				{
					addTile();

					return true;
				}

				return QObject::eventFilter(object, event);
			}

			const QUrl url(index.data(BookmarksModel::UrlRole).toUrl());

			if (url.isValid())
			{
				if ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton)
				{
					MainWindow *mainWindow = MainWindow::findMainWindow(this);

					if (mainWindow)
					{
						mainWindow->getWindowsManager()->open(url, WindowsManager::calculateOpenHints(mouseEvent->modifiers(), mouseEvent->button(), NewTabOpen));
					}
				}
				else if (mouseEvent->button() == Qt::LeftButton)
				{
					qobject_cast<WebContentsWidget*>(parentWidget())->setUrl(url);
				}

				return true;
			}
		}
	}

	return QObject::eventFilter(object, event);
}

}
