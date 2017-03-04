/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "StartPageWidget.h"
#include "StartPageModel.h"
#include "TileDelegate.h"
#include "WebContentsWidget.h"
#include "../../../core/BookmarksModel.h"
#include "../../../core/GesturesManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"
#include "../../../core/WindowsManager.h"
#include "../../../modules/widgets/search/SearchWidget.h"
#include "../../../ui/BookmarkPropertiesDialog.h"
#include "../../../ui/ContentsDialog.h"
#include "../../../ui/Menu.h"
#include "../../../ui/OpenAddressDialog.h"
#include "../../../ui/Window.h"

#include <QtCore/QtMath>
#include <QtGui/QDrag>
#include <QtGui/QGuiApplication>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmapCache>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QScrollBar>

namespace Otter
{

StartPageModel* StartPageWidget::m_model(nullptr);
QPointer<StartPagePreferencesDialog> StartPageWidget::m_preferencesDialog(nullptr);

StartPageContentsWidget::StartPageContentsWidget(QWidget *parent) : QWidget(parent),
	m_color(Qt::transparent),
	m_mode(NoCustomBackground)
{
	setAutoFillBackground(true);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void StartPageContentsWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	painter.fillRect(contentsRect(), m_color);

	if (m_mode == NoCustomBackground || m_path.isEmpty())
	{
		return;
	}

	QPixmap pixmap(m_path);

	if (pixmap.isNull())
	{
		return;
	}

	switch (m_mode)
	{
		case BestFitBackground:
			{
				const QString key(QLatin1String("start-page-best-fit-") + QString::number(width()) + QLatin1Char('-') + QString::number(height()));
				QPixmap *cachedBackground(QPixmapCache::find(key));

				if (cachedBackground)
				{
					painter.drawPixmap(contentsRect(), *cachedBackground, contentsRect().translated(((cachedBackground->width() - width()) / 2), ((cachedBackground->height() - height()) / 2)));
				}
				else
				{
					const qreal pixmapAspectRatio(pixmap.width() / qreal(pixmap.height()));
					const qreal backgroundAspectRatio(width() / qreal(height()));
					QPixmap newBackground(size());

					if (pixmapAspectRatio > backgroundAspectRatio)
					{
						newBackground = pixmap.scaledToHeight(height(), Qt::SmoothTransformation);
					}
					else
					{
						newBackground = pixmap.scaledToWidth(width(), Qt::SmoothTransformation);
					}

					painter.drawPixmap(contentsRect(), newBackground, contentsRect().translated(((newBackground.width() - width()) / 2), ((newBackground.height() - height()) / 2)));

					QPixmapCache::insert(key, newBackground);
				}
			}

			break;
		case CenterBackground:
			painter.drawPixmap((contentsRect().center() - pixmap.rect().center()), pixmap);

			break;
		case StretchBackground:
			{
				const QString key(QLatin1String("start-page-stretch-") + QString::number(width()) + QLatin1Char('-') + QString::number(height()));
				QPixmap *cachedBackground(QPixmapCache::find(key));

				if (cachedBackground)
				{
					painter.drawPixmap(contentsRect(), *cachedBackground, contentsRect());
				}
				else
				{
					const QPixmap newBackground(pixmap.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

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

void StartPageContentsWidget::setBackgroundMode(StartPageContentsWidget::BackgroundMode mode)
{
	const QString color(SettingsManager::getOption(SettingsManager::StartPage_BackgroundColorOption).toString());

	m_path = ((mode == NoCustomBackground) ? QString() : SettingsManager::getOption(SettingsManager::StartPage_BackgroundPathOption).toString());
	m_color = ((mode == NoCustomBackground || color.isEmpty()) ? QColor(Qt::transparent) : QColor(color));
	m_mode = mode;

	update();
}

StartPageWidget::StartPageWidget(Window *parent) : QScrollArea(parent),
	m_window(parent),
	m_contentsWidget(new StartPageContentsWidget(this)),
	m_listView(new QListView(this)),
	m_searchWidget(nullptr),
	m_deleteTimer(0),
	m_ignoreEnter(false)
{
	if (!m_model)
	{
		m_model = new StartPageModel(QCoreApplication::instance());
	}

	m_listView->setAttribute(Qt::WA_MacShowFocusRect, false);
	m_listView->setFrameStyle(QFrame::NoFrame);
	m_listView->setStyleSheet(QLatin1String("QListView {padding:0;border:0;background:transparent;}"));
	m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_listView->setDragEnabled(true);
	m_listView->setDragDropMode(QAbstractItemView::DragDrop);
	m_listView->setDefaultDropAction(Qt::CopyAction);
	m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_listView->setTabKeyNavigation(true);
	m_listView->setViewMode(QListView::IconMode);
	m_listView->setModel(m_model);
	m_listView->setItemDelegate(new TileDelegate(m_listView));
	m_listView->installEventFilter(this);
	m_listView->viewport()->setAttribute(Qt::WA_Hover);
	m_listView->viewport()->setMouseTracking(true);
	m_listView->viewport()->installEventFilter(this);

	setWidget(m_contentsWidget);
	setWidgetResizable(true);
	setAlignment(Qt::AlignHCenter);
	updateTiles();
	handleOptionChanged(SettingsManager::StartPage_BackgroundPathOption, SettingsManager::getOption(SettingsManager::StartPage_BackgroundPathOption));
	handleOptionChanged(SettingsManager::StartPage_ShowSearchFieldOption, SettingsManager::getOption(SettingsManager::StartPage_ShowSearchFieldOption));

	connect(m_model, SIGNAL(modelModified()), this, SLOT(updateTiles()));
	connect(m_model, SIGNAL(isReloadingTileChanged(QModelIndex)), this, SLOT(updateTile(QModelIndex)));
	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));
}

StartPageWidget::~StartPageWidget()
{
#if QT_VERSION >= 0x050700
	QDrag::cancel();
#endif
}

void StartPageWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_deleteTimer && m_listView->findChildren<QDrag*>().isEmpty())
	{
		killTimer(m_deleteTimer);

		m_deleteTimer = 0;

		deleteLater();
	}
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
	if (event->reason() != QContextMenuEvent::Mouse)
	{
		event->accept();

		showContextMenu(event->globalPos());
	}
}

void StartPageWidget::wheelEvent(QWheelEvent *event)
{
	if (event->buttons() == Qt::NoButton && event->modifiers() == Qt::NoModifier)
	{
		QScrollArea::wheelEvent(event);
	}
}

void StartPageWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	if (parameters.contains(QLatin1String("isBounced")))
	{
		return;
	}

	WindowsManager::OpenHints hints;

	switch (identifier)
	{
		case ActionsManager::OpenLinkAction:
		case ActionsManager::OpenLinkInCurrentTabAction:
			hints = WindowsManager::CurrentTabOpen;

			break;
		case ActionsManager::OpenLinkInNewTabAction:
			hints = WindowsManager::NewTabOpen;

			break;
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
			hints = (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen);

			break;
		case ActionsManager::OpenLinkInNewWindowAction:
			hints = WindowsManager::NewWindowOpen;

			break;
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
			hints = (WindowsManager::NewWindowOpen | WindowsManager::BackgroundOpen);

			break;
		case ActionsManager::OpenLinkInNewPrivateTabAction:
			hints = (WindowsManager::NewTabOpen | WindowsManager::PrivateOpen);

			break;
		case ActionsManager::OpenLinkInNewPrivateTabBackgroundAction:
			hints = (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen | WindowsManager::PrivateOpen);

			break;
		case ActionsManager::OpenLinkInNewPrivateWindowAction:
			hints = (WindowsManager::NewWindowOpen | WindowsManager::PrivateOpen);

			break;
		case ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction:
			hints = (WindowsManager::NewWindowOpen | WindowsManager::BackgroundOpen | WindowsManager::PrivateOpen);

			break;
		case ActionsManager::ContextMenuAction:
			showContextMenu();

			return;
		default:
			return;
	}

	const QUrl url(m_currentIndex.data(BookmarksModel::UrlRole).toUrl());

	if (url.isValid())
	{
		m_urlOpenTime = QTime::currentTime();

		ActionsManager::triggerAction(ActionsManager::OpenUrlAction, this, {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(hints)}});
	}
}

void StartPageWidget::scrollContents(const QPoint &delta)
{
	horizontalScrollBar()->setValue(horizontalScrollBar()->value() + delta.x());
	verticalScrollBar()->setValue(verticalScrollBar()->value() + delta.y());
}

void StartPageWidget::configure()
{
	if (m_preferencesDialog)
	{
		m_preferencesDialog->reject();
	}

	m_preferencesDialog = new StartPagePreferencesDialog(this);

	ContentsDialog *dialog(new ContentsDialog(ThemesManager::getIcon(QLatin1String("configure")), m_preferencesDialog->windowTitle(), QString(), QString(), QDialogButtonBox::NoButton, m_preferencesDialog, this));

	connect(m_preferencesDialog, SIGNAL(finished(int)), dialog, SLOT(close()));

	m_window->getContentsWidget()->showDialog(dialog, false);
}

void StartPageWidget::addTile()
{
	OpenAddressDialog dialog(this);
	dialog.setWindowTitle(tr("Add Tile"));

	connect(&dialog, SIGNAL(requestedLoadUrl(QUrl,WindowsManager::OpenHints)), this, SLOT(addTile(QUrl)));

	m_ignoreEnter = true;

	dialog.exec();
}

void StartPageWidget::addTile(const QUrl &url)
{
	BookmarksItem *bookmark(BookmarksManager::getModel()->addBookmark(BookmarksModel::UrlBookmark, 0, url, QString(), BookmarksManager::getModel()->getItem(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString())));

	if (bookmark)
	{
		m_model->reloadTile(bookmark->index(), true);
	}
}

void StartPageWidget::openTile()
{
	if (m_currentIndex.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("add"))
	{
		addTile();

		return;
	}

	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(m_currentIndex.data(BookmarksModel::TypeRole).toInt()));
	WindowsManager::OpenHints hints(WindowsManager::CurrentTabOpen);

	if (m_window->isPrivate())
	{
		hints |= WindowsManager::PrivateOpen;
	}

	if (type == BookmarksModel::FolderBookmark)
	{
		BookmarksItem *bookmark(BookmarksManager::getModel()->getBookmark(m_currentIndex));

		if (bookmark && bookmark->rowCount() > 0)
		{
			m_urlOpenTime = QTime::currentTime();

			ActionsManager::triggerAction(ActionsManager::OpenBookmarkAction, parent(), {{QLatin1String("bookmark"), bookmark->data(BookmarksModel::IdentifierRole)}, {QLatin1String("hints"), QVariant(hints)}});
		}

		return;
	}

	if (!m_currentIndex.isValid() || type != BookmarksModel::UrlBookmark)
	{
		return;
	}

	const QUrl url(m_currentIndex.data(BookmarksModel::UrlRole).toUrl());

	if (url.isValid())
	{
		m_urlOpenTime = QTime::currentTime();

		ActionsManager::triggerAction(ActionsManager::OpenUrlAction, parent(), {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(hints)}});
	}
}

void StartPageWidget::editTile()
{
	BookmarksItem *bookmark(BookmarksManager::getModel()->getBookmark(m_currentIndex));

	if (bookmark)
	{
		BookmarkPropertiesDialog dialog(bookmark, this);
		dialog.exec();

		m_model->reloadModel();
	}
}

void StartPageWidget::reloadTile()
{
	m_listView->openPersistentEditor(m_currentIndex);

	m_model->reloadTile(m_currentIndex);
}

void StartPageWidget::removeTile()
{
	BookmarksItem *bookmark(BookmarksManager::getModel()->getBookmark(m_currentIndex));

	if (bookmark)
	{
		const QString path(SessionsManager::getWritableDataPath(QLatin1String("thumbnails/")) + QString::number(bookmark->data(BookmarksModel::IdentifierRole).toULongLong()) + QLatin1String(".png"));

		if (QFile::exists(path))
		{
			QFile::remove(path);
		}

		bookmark->remove();
	}
}

void StartPageWidget::markForDeletion()
{
	if (m_deleteTimer == 0)
	{
		m_deleteTimer = startTimer(250);
	}
}

void StartPageWidget::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::StartPage_BackgroundColorOption:
		case SettingsManager::StartPage_BackgroundModeOption:
		case SettingsManager::StartPage_BackgroundPathOption:
			{
				const QString backgroundMode(SettingsManager::getOption(SettingsManager::StartPage_BackgroundModeOption).toString());

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

				update();
			}

			break;
		case SettingsManager::StartPage_ShowSearchFieldOption:
			{
				QGridLayout *layout(nullptr);
				const bool needsInitialization(m_contentsWidget->layout() == nullptr);

				if (needsInitialization)
				{
					layout = new QGridLayout(m_contentsWidget);
					layout->setContentsMargins(0, 0, 0, 0);
					layout->setSpacing(0);
				}
				else if ((m_searchWidget && !value.toBool()) || (!m_searchWidget && value.toBool()))
				{
					layout = qobject_cast<QGridLayout*>(m_contentsWidget->layout());

					for (int i = (layout->count() - 1); i >=0; --i)
					{
						QLayoutItem *item(layout->takeAt(i));

						if (item)
						{
							if (item->widget())
							{
								item->widget()->setParent(m_contentsWidget);
							}

							delete item;
						}
					}
				}

				if (value.toBool() && (needsInitialization || !m_searchWidget))
				{
					if (!m_searchWidget)
					{
						m_searchWidget = new SearchWidget(m_window, this);
						m_searchWidget->setFixedWidth(300);
					}

					layout->addItem(new QSpacerItem(1, 50, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 1);
					layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding), 1, 0);
					layout->addWidget(m_searchWidget, 1, 1, 1, 1, Qt::AlignCenter);
					layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding), 1, 2);
					layout->addItem(new QSpacerItem(1, 50, QSizePolicy::Fixed, QSizePolicy::Fixed), 2, 1);
					layout->addWidget(m_listView, 3, 0, 1, 3, Qt::AlignCenter);
					layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), 4, 1);
				}
				else if (!value.toBool() && (needsInitialization || m_searchWidget))
				{
					if (m_searchWidget)
					{
						m_searchWidget->deleteLater();
						m_searchWidget = nullptr;
					}

					layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), 0, 1);
					layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 0);
					layout->addWidget(m_listView, 1, 1, 1, 1, Qt::AlignCenter);
					layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 2);
					layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), 2, 1);
				}
			}

			break;
		case SettingsManager::StartPage_TilesPerRowOption:
		case SettingsManager::StartPage_TileHeightOption:
		case SettingsManager::StartPage_TileWidthOption:
		case SettingsManager::StartPage_ZoomLevelOption:
			updateSize();

			break;
		default:
			break;
	}
}

void StartPageWidget::updateTile(const QModelIndex &index)
{
	m_listView->update(index);
	m_listView->closePersistentEditor(index);

	m_thumbnail = QPixmap();
}

void StartPageWidget::updateSize()
{
	const qreal zoom(SettingsManager::getOption(SettingsManager::StartPage_ZoomLevelOption).toInt() / qreal(100));
	const int tileHeight(SettingsManager::getOption(SettingsManager::StartPage_TileHeightOption).toInt() * zoom);
	const int tileWidth(SettingsManager::getOption(SettingsManager::StartPage_TileWidthOption).toInt() * zoom);
	const int amount(m_model->rowCount());
	const int columns(getTilesPerRow());
	const int rows(qCeil(amount / qreal(columns)));

	m_listView->setGridSize(QSize(tileWidth, tileHeight));
	m_listView->setFixedSize(((qMin(amount, columns) * tileWidth) + 2), ((rows * tileHeight) + 20));

	m_thumbnail = QPixmap();
}

void StartPageWidget::updateTiles()
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		if (m_model->index(i, 0).data(StartPageModel::IsReloadingRole).toBool())
		{
			m_listView->openPersistentEditor(m_model->index(i, 0));
		}
	}

	updateSize();
}

void StartPageWidget::showContextMenu(const QPoint &position)
{
	QPoint hitPosition(position);

	if (hitPosition.isNull())
	{
		hitPosition = ((m_listView->hasFocus() && m_currentIndex.isValid()) ? m_listView->mapToGlobal(m_listView->visualRect(m_currentIndex).center()) : QCursor::pos());
	}

	const QModelIndex index(m_listView->indexAt(m_listView->mapFromGlobal(hitPosition)));
	QMenu menu;

	if (index.isValid() && index.data(Qt::AccessibleDescriptionRole).toString() != QLatin1String("add"))
	{
		menu.addAction(tr("Open"), this, SLOT(openTile()));
		menu.addSeparator();
		menu.addAction(tr("Edit…"), this, SLOT(editTile()));

		if (SettingsManager::getOption(SettingsManager::StartPage_TileBackgroundModeOption) == QLatin1String("thumbnail"))
		{
			menu.addAction(tr("Reload"), this, SLOT(reloadTile()))->setEnabled(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark);
		}

		menu.addSeparator();
		menu.addAction(tr("Delete"), this, SLOT(removeTile()));
	}
	else
	{
		menu.addAction(tr("Configure…"), this, SLOT(configure()));
		menu.addAction(tr("Add Tile…"), this, SLOT(addTile()));
	}

	menu.exec(hitPosition);
}

QPixmap StartPageWidget::getThumbnail()
{
	if (m_thumbnail.isNull())
	{
		QPixmap pixmap(widget()->size());
		pixmap.setDevicePixelRatio(devicePixelRatio());

		QPainter widgetPainter(&pixmap);

		widget()->render(&widgetPainter);

		widgetPainter.end();

		pixmap = pixmap.scaled(260, 170, Qt::KeepAspectRatio);

		QPixmap thumbnail(QSize(260, 170));
		thumbnail.setDevicePixelRatio(devicePixelRatio());
		thumbnail.fill(Qt::white);

		QPainter thumbnailPainter(&thumbnail);
		thumbnailPainter.drawPixmap(QPoint(((pixmap.width() < 260) ? ((260 - pixmap.width()) / 2) : 0), 0), pixmap);

		m_thumbnail = thumbnail;
	}

	return m_thumbnail;
}

int StartPageWidget::getTilesPerRow() const
{
	const int tilesPerRow(SettingsManager::getOption(SettingsManager::StartPage_TilesPerRowOption).toInt());

	if (tilesPerRow > 0)
	{
		return tilesPerRow;
	}

	return qMax(1, int((width() - 50) / (SettingsManager::getOption(SettingsManager::StartPage_TileWidthOption).toInt() * (SettingsManager::getOption(SettingsManager::StartPage_ZoomLevelOption).toInt() / qreal(100)))));
}

bool StartPageWidget::event(QEvent *event)
{
	if (!GesturesManager::isTracking() && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::Wheel))
	{
		GesturesManager::startGesture(this, event, QList<GesturesManager::GesturesContext>({GesturesManager::GenericGesturesContext}));
	}

	return QScrollArea::event(event);
}

bool StartPageWidget::eventFilter(QObject *object, QEvent *event)
{
	if ((object == m_contentsWidget || object == m_listView || object == m_listView->viewport()) && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::Wheel))
	{
		if (event->type() == QEvent::Wheel)
		{
			QWheelEvent *wheelEvent(static_cast<QWheelEvent*>(event));

			if (wheelEvent)
			{
				m_currentIndex = m_listView->indexAt(m_listView->mapFromGlobal(wheelEvent->globalPos()));
			}
		}
		else
		{
			QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

			if (mouseEvent)
			{
				m_currentIndex = m_listView->indexAt(m_listView->mapFromGlobal(mouseEvent->globalPos()));
			}
		}

		QList<GesturesManager::GesturesContext> contexts;

		if (m_currentIndex.isValid())
		{
			contexts.append(GesturesManager::LinkGesturesContext);
		}

		contexts.append(GesturesManager::GenericGesturesContext);

		if (GesturesManager::startGesture(object, event, contexts))
		{
			return true;
		}
	}

	if (object == m_listView && event->type() == QEvent::KeyRelease)
	{
		QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent)
		{
			if (m_ignoreEnter)
			{
				m_ignoreEnter = false;

				if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
				{
					return true;
				}
			}

			m_currentIndex = m_listView->currentIndex();

			if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
			{
				const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(m_currentIndex.data(BookmarksModel::TypeRole).toInt()));

				if (type == BookmarksModel::FolderBookmark)
				{
					BookmarksItem *bookmark(BookmarksManager::getModel()->getBookmark(m_currentIndex));

					if (bookmark && bookmark->rowCount() > 0)
					{
						m_ignoreEnter = true;

						Menu menu(Menu::BookmarksMenuRole, this);
						menu.menuAction()->setData(bookmark->index());
						menu.exec(m_listView->mapToGlobal(m_listView->visualRect(m_currentIndex).center()));
					}
				}
				else if (type == BookmarksModel::UrlBookmark)
				{
					const QUrl url(m_currentIndex.data(BookmarksModel::UrlRole).toUrl());

					if (keyEvent->modifiers() != Qt::NoModifier)
					{
						m_urlOpenTime = QTime::currentTime();

						ActionsManager::triggerAction(ActionsManager::OpenUrlAction, parent(), {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(WindowsManager::calculateOpenHints((m_window->isPrivate() ? WindowsManager::PrivateOpen : WindowsManager::DefaultOpen), Qt::LeftButton, keyEvent->modifiers()))}});
					}
					else
					{
						m_urlOpenTime = QTime::currentTime();

						m_window->setUrl(url);
					}
				}
				else if (m_currentIndex.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("add"))
				{
					addTile();
				}

				return true;
			}
		}
	}
	else if (object == m_listView->viewport() && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent)
		{
			m_currentIndex = m_listView->indexAt(mouseEvent->pos());
		}
	}
	else if (object == m_listView->viewport() && event->type() == QEvent::MouseMove)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent && mouseEvent->buttons().testFlag(Qt::LeftButton) && ((m_urlOpenTime.isValid() && m_urlOpenTime.msecsTo(QTime::currentTime()) < 1000) || m_window->getLoadingState() != WindowsManager::FinishedLoadingState))
		{
			return true;
		}
	}
	else if (object == m_listView->viewport() && event->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (m_ignoreEnter)
		{
			m_ignoreEnter = false;
		}

		if (mouseEvent && m_listView->indexAt(mouseEvent->pos()) == m_currentIndex)
		{
			m_currentIndex = m_listView->indexAt(mouseEvent->pos());

			if (mouseEvent->button() == Qt::LeftButton || mouseEvent->button() == Qt::MiddleButton)
			{
				const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(m_currentIndex.data(BookmarksModel::TypeRole).toInt()));

				if (type == BookmarksModel::FolderBookmark)
				{
					BookmarksItem *bookmark(BookmarksManager::getModel()->getBookmark(m_currentIndex));

					if (bookmark && bookmark->rowCount() > 0)
					{
						m_ignoreEnter = true;

						Menu menu(Menu::BookmarksMenuRole, this);
						menu.menuAction()->setData(bookmark->index());
						menu.exec(mouseEvent->globalPos());
					}

					return true;
				}

				if (!m_currentIndex.isValid() || type != BookmarksModel::UrlBookmark)
				{
					if (m_currentIndex.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("add"))
					{
						addTile();

						return true;
					}

					mouseEvent->ignore();

					return true;
				}

				const QUrl url(m_currentIndex.data(BookmarksModel::UrlRole).toUrl());

				if (url.isValid())
				{
					if (mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier)
					{
						m_urlOpenTime = QTime::currentTime();

						ActionsManager::triggerAction(ActionsManager::OpenUrlAction, parent(), {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(WindowsManager::calculateOpenHints((m_window->isPrivate() ? WindowsManager::PrivateOpen : WindowsManager::DefaultOpen), mouseEvent->button(), mouseEvent->modifiers()))}});
					}
					else if (mouseEvent->button() != Qt::MiddleButton)
					{
						WindowsManager::OpenHints hints(WindowsManager::CurrentTabOpen);

						if (m_window->isPrivate())
						{
							hints |= WindowsManager::PrivateOpen;
						}

						m_urlOpenTime = QTime::currentTime();

						ActionsManager::triggerAction(ActionsManager::OpenUrlAction, parent(), {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(hints)}});
					}
				}
			}
			else
			{
				mouseEvent->ignore();
			}

			return true;
		}
	}

	return QObject::eventFilter(object, event);
}

}
