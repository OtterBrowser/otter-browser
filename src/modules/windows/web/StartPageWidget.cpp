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

#include "StartPageWidget.h"
#include "StartPageModel.h"
#include "../../../core/Application.h"
#include "../../../core/BookmarksModel.h"
#include "../../../core/GesturesManager.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../modules/widgets/search/SearchWidget.h"
#include "../../../ui/Animation.h"
#include "../../../ui/BookmarkPropertiesDialog.h"
#include "../../../ui/ContentsDialog.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/Menu.h"
#include "../../../ui/OpenAddressDialog.h"
#include "../../../ui/Window.h"

#include <QtCore/QFile>
#include <QtCore/QtMath>
#include <QtGui/QDrag>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPixmapCache>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QToolTip>
#ifdef OTTER_ENABLE_STARTPAGEBLUR
#include <QtWidgets/private/qpixmapfilter_p.h>
#endif

namespace Otter
{

StartPageModel* StartPageWidget::m_model(nullptr);
Animation* StartPageWidget::m_spinnerAnimation(nullptr);
QPointer<StartPagePreferencesDialog> StartPageWidget::m_preferencesDialog(nullptr);

TileDelegate::TileDelegate(QWidget *parent) : QStyledItemDelegate(parent),
	m_widget(parent),
	m_mode(NoBackground)
#ifdef OTTER_ENABLE_STARTPAGEBLUR
	, m_needsBlur(true)
#endif
{
	handleOptionChanged(SettingsManager::StartPage_BackgroundModeOption, SettingsManager::getOption(SettingsManager::StartPage_BackgroundModeOption));
	handleOptionChanged(SettingsManager::StartPage_TileBackgroundModeOption, SettingsManager::getOption(SettingsManager::StartPage_TileBackgroundModeOption));

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &TileDelegate::handleOptionChanged);
}

void TileDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	const QPalette::ColorGroup colorGroup(index.data(StartPageModel::IsEmptyRole).toBool() ? QPalette::Disabled : QPalette::Active);
	const int textHeight(qRound(option.fontMetrics.boundingRect(QLatin1String("X")).height() * 1.5));
	QRect rectangle(option.rect);
	rectangle.adjust(10, 10, -10, -10);

	const QRect tileRectangle(rectangle);

	if (m_mode != NoBackground)
	{
		rectangle.adjust(0, 0, 0, -textHeight);
	}

	QPainterPath path;
	path.addRoundedRect(tileRectangle, 5, 5);

	painter->setPen(QPen(QColor(26, 35, 126, 51), 1));
	painter->setRenderHint(QPainter::Antialiasing);

	if (index.data(StartPageModel::IsDraggedRole).toBool())
	{
		painter->drawPath(path);

		return;
	}

	const quint64 identifier(index.data(BookmarksModel::IdentifierRole).toULongLong());
	const bool isReloading(index.data(StartPageModel::IsReloadingRole).toBool());
	const QString key(createPixmapCacheKey(option.rect, identifier));
	QPixmap cachedPixmap;

	if (QPixmapCache::find(key, &cachedPixmap))
	{
		painter->drawPixmap(tileRectangle, cachedPixmap);

		if (isReloading)
		{
			drawAnimation(painter, rectangle);
		}

		drawFocusIndicator(painter, path, option, colorGroup);

		return;
	}

	cachedPixmap = QPixmap(tileRectangle.size());
	cachedPixmap.fill(Qt::transparent);

	QPainter pixmapPainter(&cachedPixmap);
	pixmapPainter.translate(-rectangle.topLeft());
	pixmapPainter.setPen(QPen(QColor(26, 35, 126, 51), 1));
	pixmapPainter.setRenderHint(QPainter::Antialiasing);

	const QRect textRectangle(rectangle.x(), (rectangle.y() + rectangle.height()), rectangle.width(), textHeight);
	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()));

	if (type == BookmarksModel::UnknownBookmark && index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("add"))
	{
		pixmapPainter.setBrush(QColor(179, 229, 252, 192));

#ifdef OTTER_ENABLE_STARTPAGEBLUR
		drawBlurBehind(&pixmapPainter, tileRectangle);
#endif

		pixmapPainter.drawPath(path);

		ThemesManager::createIcon(QLatin1String("list-add")).paint(&pixmapPainter, tileRectangle);

		painter->drawPixmap(tileRectangle, cachedPixmap);

		QPixmapCache::insert(key, cachedPixmap);

		drawFocusIndicator(painter, path, option, colorGroup);

		return;
	}

	pixmapPainter.setClipPath(path);

#ifdef OTTER_ENABLE_STARTPAGEBLUR
	if (type == BookmarksModel::FolderBookmark || m_mode != ThumbnailBackground)
	{
		drawBlurBehind(&pixmapPainter, tileRectangle);
	}
	else
	{
		drawBlurBehind(&pixmapPainter, textRectangle);
	}
#endif

	pixmapPainter.fillRect(tileRectangle, QColor(179, 229, 252, 128));

	if (type == BookmarksModel::FolderBookmark && m_mode != NoBackground)
	{
		ThemesManager::createIcon(QLatin1String("inode-directory")).paint(&pixmapPainter, rectangle, Qt::AlignCenter, (index.data(StartPageModel::IsEmptyRole).toBool() ? QIcon::Disabled : QIcon::Normal));
	}
	else
	{
		switch (m_mode)
		{
			case FaviconBackground:
				{
					const int faviconSize(((rectangle.height() > rectangle.width()) ? rectangle.width() : rectangle.height()) / 4);
					QRect faviconRectangle(0, 0, faviconSize, faviconSize);
					faviconRectangle.moveCenter(rectangle.center());

					HistoryManager::getIcon(index.data(BookmarksModel::UrlRole).toUrl()).paint(&pixmapPainter, faviconRectangle);
				}

				break;
			case ThumbnailBackground:
				pixmapPainter.save();
				pixmapPainter.setBrush(Qt::white);
				pixmapPainter.setPen(Qt::transparent);
				pixmapPainter.drawRect(rectangle);
				pixmapPainter.drawPixmap(rectangle, QPixmap(StartPageModel::getThumbnailPath(identifier)), rectangle.translated(-rectangle.topLeft()));
				pixmapPainter.restore();

				break;
			default:
				break;
		}
	}

	const QString text(option.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(), option.textElideMode, (rectangle.width() - 20)));
	QPalette palette(QGuiApplication::palette());
	palette.setColor(QPalette::Text, QColor(26, 35, 128));

	pixmapPainter.setClipping(false);
	pixmapPainter.setBrush(Qt::transparent);
	pixmapPainter.drawPath(path);
	pixmapPainter.setPen(palette.color(colorGroup, QPalette::Text));

	if (m_mode == NoBackground)
	{
		pixmapPainter.drawText(rectangle, Qt::AlignCenter, text);
	}
	else
	{
		pixmapPainter.drawText(textRectangle, Qt::AlignCenter, text);
	}

	painter->drawPixmap(tileRectangle, cachedPixmap);

	QPixmapCache::insert(key, cachedPixmap);

	if (isReloading)
	{
		drawAnimation(painter, rectangle);
	}

	drawFocusIndicator(painter, path, option, colorGroup);
}

void TileDelegate::drawAnimation(QPainter *painter, const QRect &rectangle) const
{
	const Animation *animation(StartPageWidget::getLoadingAnimation());

	if (animation)
	{
		const QPixmap pixmap(animation->getCurrentPixmap());
		QRect pixmapRectangle({0, 0}, pixmap.size());
		pixmapRectangle.moveCenter(rectangle.center());

		painter->drawPixmap(pixmapRectangle, pixmap);
	}
}

#ifdef OTTER_ENABLE_STARTPAGEBLUR
void TileDelegate::drawBlurBehind(QPainter *painter, const QRect &rectangle) const
{
	if (!m_needsBlur)
	{
		return;
	}

	QRect parentRectangle;
	parentRectangle.setTopLeft(m_widget->mapToParent(rectangle.topLeft()));
	parentRectangle.setBottomRight(m_widget->mapToParent(rectangle.bottomRight()));
	parentRectangle.adjust(-8, -8, 8, 8);

	QPixmap parentPixmap(parentRectangle.size());
	parentPixmap.fill(Qt::transparent);

	m_widget->parentWidget()->render(&parentPixmap, {}, QRegion(parentRectangle), (QWidget::DrawWindowBackground | QWidget::IgnoreMask));

	QPixmap blurredPixmap(parentRectangle.size());
	blurredPixmap.fill(Qt::transparent);

	QPainter blurredPainter(&blurredPixmap);
	QPixmapBlurFilter filter;
	filter.setBlurHints(QGraphicsBlurEffect::PerformanceHint);
	filter.setRadius(5);
	filter.draw(&blurredPainter, {}, parentPixmap);

	QRect blurredRectangle(rectangle);
	blurredRectangle.moveTo(8, 8);

	painter->drawPixmap(rectangle, blurredPixmap, blurredRectangle);
}
#endif

void TileDelegate::drawFocusIndicator(QPainter *painter, const QPainterPath &path, const QStyleOptionViewItem &option, QPalette::ColorGroup colorGroup) const
{
	if (!option.state.testFlag(QStyle::State_MouseOver) && !option.state.testFlag(QStyle::State_HasFocus))
	{
		return;
	}

	QColor highlightColor(QGuiApplication::palette().color(colorGroup, QPalette::Highlight));

	if (option.state.testFlag(QStyle::State_MouseOver))
	{
		highlightColor.setAlpha(150);
	}

	painter->setPen(QPen(highlightColor, 3));
	painter->setClipping(false);
	painter->setBrush(Qt::transparent);
	painter->drawPath(path);
}

void TileDelegate::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
#ifdef OTTER_ENABLE_STARTPAGEBLUR
		case SettingsManager::StartPage_BackgroundModeOption:
		case SettingsManager::StartPage_BackgroundPathOption:
			{
				const QString mode(SettingsManager::getOption(SettingsManager::StartPage_BackgroundModeOption).toString());

				m_needsBlur = (mode == QLatin1String("standard") || (mode != QLatin1String("none") && !SettingsManager::getOption(SettingsManager::StartPage_BackgroundPathOption).toString().isEmpty()));
			}

			break;
#endif
		case SettingsManager::StartPage_TileBackgroundModeOption:
			{
				const QString mode(value.toString());

				if (mode == QLatin1String("favicon"))
				{
					m_mode = FaviconBackground;
				}
				else if (mode == QLatin1String("thumbnail"))
				{
					m_mode = ThumbnailBackground;
				}
				else
				{
					m_mode = NoBackground;
				}
			}

			break;
		default:
			break;
	}
}

void TileDelegate::setPixmapCachePrefix(const QString &prefix)
{
	m_pixmapCachePrefix = prefix;
}

QString TileDelegate::createPixmapCacheKey(const QRect &rectangle, quint64 identifier) const
{
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);
	stream << rectangle;

	return m_pixmapCachePrefix + QLatin1String("-tile-") + QString::number(identifier) + QLatin1Char('-') + array.toBase64(QByteArray::OmitTrailingEquals);
}

QSize TileDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	const qreal zoom(SettingsManager::getOption(SettingsManager::StartPage_ZoomLevelOption).toInt() / static_cast<qreal>(100));

	return {qRound((SettingsManager::getOption(SettingsManager::StartPage_TileWidthOption).toInt() + 6) * zoom), qRound((SettingsManager::getOption(SettingsManager::StartPage_TileHeightOption).toInt() + 6) * zoom)};
}

StartPageContentsWidget::StartPageContentsWidget(QWidget *parent) : QWidget(parent),
	m_backgroundColor(Qt::transparent),
	m_backgroundMode(DefaultBackground)
{
	handleOptionChanged(SettingsManager::StartPage_BackgroundPathOption);
	setAutoFillBackground(true);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &StartPageContentsWidget::handleOptionChanged);
}

void StartPageContentsWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	painter.fillRect(contentsRect(), m_backgroundColor);

	if (m_backgroundMode == NoBackground || m_backgroundPath.isEmpty() || !QFile::exists(m_backgroundPath))
	{
		return;
	}

	const QString key(getPixmapCachePrefix() + QLatin1String("-background"));
	QPixmap cachedPixmap;

	if (QPixmapCache::find(key, &cachedPixmap))
	{
		painter.drawPixmap(contentsRect(), cachedPixmap);

		return;
	}

	const QPixmap pixmap(m_backgroundPath);

	if (pixmap.isNull())
	{
		return;
	}

	switch (m_backgroundMode)
	{
		case DefaultBackground:
		case BestFitBackground:
			cachedPixmap = pixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
			cachedPixmap = cachedPixmap.copy(contentsRect().translated(((cachedPixmap.width() - width()) / 2), ((cachedPixmap.height() - height()) / 2)));

			painter.drawPixmap(contentsRect(), cachedPixmap);

			break;
		case CenterBackground:
			painter.drawPixmap((contentsRect().center() - pixmap.rect().center()), pixmap);

			break;
		case StretchBackground:
			cachedPixmap = pixmap.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

			painter.drawPixmap(contentsRect(), cachedPixmap);

			break;
		case TileBackground:
			painter.drawTiledPixmap(contentsRect(), pixmap);

			break;
		default:
			break;
	}

	if (!cachedPixmap.isNull())
	{
		QPixmapCache::insert(key, cachedPixmap);
	}
}

void StartPageContentsWidget::handleOptionChanged(int identifier)
{
	if (identifier == SettingsManager::StartPage_BackgroundPathOption)
	{
		QPixmapCache::clear();

		update();
	}
}

void StartPageContentsWidget::setBackgroundMode(BackgroundMode mode)
{
	const QString color(SettingsManager::getOption(SettingsManager::StartPage_BackgroundColorOption).toString());

	switch (mode)
	{
		case DefaultBackground:
			m_backgroundPath = QLatin1String(":/style/start-page.svgz");
			m_backgroundColor = QColor(Qt::transparent);

			break;
		case NoBackground:
			m_backgroundPath = QString();
			m_backgroundColor = QColor(Qt::transparent);

			break;
		default:
			m_backgroundPath = SettingsManager::getOption(SettingsManager::StartPage_BackgroundPathOption).toString();
			m_backgroundColor = (color.isEmpty() ? QColor(Qt::transparent) : QColor(color));

			break;
	}

	m_backgroundMode = mode;

	update();
}

QString StartPageContentsWidget::getPixmapCachePrefix() const
{
	QString prefix;

	switch (m_backgroundMode)
	{
		case DefaultBackground:
			prefix = QLatin1String("start-page-standard");

			break;
		case BestFitBackground:
			prefix = QLatin1String("start-page-best-fit");

			break;
		case StretchBackground:
			prefix = QLatin1String("start-page-stretch");

			break;
		default:
			break;
	}

	return (prefix.isEmpty() ? prefix : prefix + QLatin1Char('-') + QString::number(width()) + QLatin1Char('-') + QString::number(height()));
}

StartPageWidget::StartPageWidget(Window *parent) : QScrollArea(parent),
	m_window(parent),
	m_contentsWidget(new StartPageContentsWidget(this)),
	m_listView(new QListView(this)),
	m_searchWidget(nullptr),
	m_tileDelegate(new TileDelegate(m_listView)),
	m_deleteTimer(0),
	m_isIgnoringEnter(false)
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
	m_listView->setItemDelegate(m_tileDelegate);
	m_listView->installEventFilter(this);
	m_listView->viewport()->setAttribute(Qt::WA_Hover);
	m_listView->viewport()->setMouseTracking(true);
	m_listView->viewport()->installEventFilter(this);

	setAcceptDrops(true);
	setWidget(m_contentsWidget);
	setWidgetResizable(true);
	setAlignment(Qt::AlignHCenter);
	updateSize();
	handleOptionChanged(SettingsManager::StartPage_BackgroundPathOption, SettingsManager::getOption(SettingsManager::StartPage_BackgroundPathOption));
	handleOptionChanged(SettingsManager::StartPage_ShowSearchFieldOption, SettingsManager::getOption(SettingsManager::StartPage_ShowSearchFieldOption));

	QPixmapCache::setCacheLimit(51200);

	if (!m_model->match(m_model->index(0, 0), StartPageModel::IsReloadingRole, true, 1, Qt::MatchExactly).isEmpty())
	{
		startReloadingAnimation();
	}

	connect(m_model, &StartPageModel::modelModified, this, &StartPageWidget::updateSize);
	connect(m_model, &StartPageModel::isReloadingTileChanged, this, &StartPageWidget::handleIsReloadingTileChanged);
	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &StartPageWidget::handleOptionChanged);
}

StartPageWidget::~StartPageWidget()
{
	QDrag::cancel();
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

void StartPageWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls() || event->mimeData()->hasText())
	{
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void StartPageWidget::dropEvent(QDropEvent *event)
{
	if (!event->mimeData()->hasUrls() && !event->mimeData()->hasText())
	{
		event->ignore();

		return;
	}

	event->accept();

	const QVector<QUrl> urls(Utils::extractUrls(event->mimeData()));

	if (urls.isEmpty())
	{
		const InputInterpreter::InterpreterResult result(InputInterpreter::interpret(event->mimeData()->text(), (InputInterpreter::NoBookmarkKeywordsFlag | InputInterpreter::NoSearchKeywordsFlag)));

		if (result.isValid())
		{
			switch (result.type)
			{
				case InputInterpreter::InterpreterResult::UrlType:
					Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), result.url}}, parentWidget());

					break;
				case InputInterpreter::InterpreterResult::SearchType:
					m_window->search(result.searchQuery, result.searchEngine);

					break;
				default:
					break;
			}
		}
	}
	else
	{
		for (int i = 0; i < urls.count(); ++i)
		{
			Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), urls.at(i)}}, parentWidget());
		}
	}
}

void StartPageWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	Q_UNUSED(parameters)

	SessionsManager::OpenHints hints(SessionsManager::DefaultOpen);

	switch (identifier)
	{
		case ActionsManager::OpenLinkAction:
		case ActionsManager::OpenLinkInCurrentTabAction:
			hints = SessionsManager::CurrentTabOpen;

			break;
		case ActionsManager::OpenLinkInNewTabAction:
			hints = SessionsManager::NewTabOpen;

			break;
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
			hints = (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen);

			break;
		case ActionsManager::OpenLinkInNewWindowAction:
			hints = SessionsManager::NewWindowOpen;

			break;
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
			hints = (SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen);

			break;
		case ActionsManager::OpenLinkInNewPrivateTabAction:
			hints = (SessionsManager::NewTabOpen | SessionsManager::PrivateOpen);

			break;
		case ActionsManager::OpenLinkInNewPrivateTabBackgroundAction:
			hints = (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen | SessionsManager::PrivateOpen);

			break;
		case ActionsManager::OpenLinkInNewPrivateWindowAction:
			hints = (SessionsManager::NewWindowOpen | SessionsManager::PrivateOpen);

			break;
		case ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction:
			hints = (SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen | SessionsManager::PrivateOpen);

			break;
		case ActionsManager::ContextMenuAction:
			showContextMenu();

			return;
		case ActionsManager::ReloadAction:
			{
				bool isReloading(false);

				for (int i = 0; i < m_model->rowCount(); ++i)
				{
					if (m_model->reloadTile(m_model->index(i, 0)))
					{
						isReloading = true;
					}
				}

				if (isReloading)
				{
					startReloadingAnimation();
				}
			}

			return;
		default:
			return;
	}

	const BookmarksModel::Bookmark *bookmark(StartPageModel::getBookmark(m_currentIndex));

	if (bookmark)
	{
		m_urlOpenTime = QTime::currentTime();

		Application::triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark->getIdentifier()}, {QLatin1String("hints"), QVariant(hints)}}, parentWidget(), trigger);
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

	ContentsDialog *dialog(new ContentsDialog(ThemesManager::createIcon(QLatin1String("configure")), m_preferencesDialog->windowTitle(), {}, {}, QDialogButtonBox::NoButton, m_preferencesDialog, this));

	connect(m_preferencesDialog, &StartPagePreferencesDialog::finished, dialog, &ContentsDialog::close);

	m_window->getContentsWidget()->showDialog(dialog, false);
}

void StartPageWidget::addTile()
{
	OpenAddressDialog dialog(ActionExecutor::Object(), this);
	dialog.setWindowTitle(tr("Add Tile"));

	m_isIgnoringEnter = true;

	const InputInterpreter::InterpreterResult result(dialog.getResult());

	if (dialog.exec() != QDialog::Accepted || !result.isValid())
	{
		return;
	}

	QModelIndex index;

	switch (result.type)
	{
		case InputInterpreter::InterpreterResult::BookmarkType:
			index = m_model->addTile(result.bookmark->getUrl());

			break;
		case InputInterpreter::InterpreterResult::UrlType:
			index = m_model->addTile(result.url);

			break;
		default:
			return;
	}

	if (index.isValid())
	{
		startReloadingAnimation();
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
	SessionsManager::OpenHints hints(SessionsManager::CurrentTabOpen);

	if (m_window->isPrivate())
	{
		hints |= SessionsManager::PrivateOpen;
	}

	if (type == BookmarksModel::FolderBookmark)
	{
		const BookmarksModel::Bookmark *bookmark(StartPageModel::getBookmark(m_currentIndex));

		if (bookmark && bookmark->hasChildren())
		{
			m_urlOpenTime = QTime::currentTime();

			Application::triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark->getIdentifier()}, {QLatin1String("hints"), QVariant(hints)}}, parentWidget());
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

		Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(hints)}}, parentWidget());
	}
}

void StartPageWidget::editTile()
{
	BookmarksModel::Bookmark *bookmark(StartPageModel::getBookmark(m_currentIndex));

	if (bookmark)
	{
		BookmarkPropertiesDialog dialog(bookmark, this);
		dialog.exec();

		m_model->reloadModel();
	}
}

void StartPageWidget::reloadTile()
{
	if (m_currentIndex.isValid() && m_model->reloadTile(m_currentIndex))
	{
		startReloadingAnimation();
	}
}

void StartPageWidget::removeTile()
{
	BookmarksModel::Bookmark *bookmark(StartPageModel::getBookmark(m_currentIndex));

	if (bookmark)
	{
		const QString path(StartPageModel::getThumbnailPath(bookmark->getIdentifier()));

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

void StartPageWidget::startReloadingAnimation()
{
	if (!m_spinnerAnimation)
	{
		m_spinnerAnimation = ThemesManager::createAnimation();
		m_spinnerAnimation->start();
	}

	connect(m_spinnerAnimation, &Animation::frameChanged, m_listView, static_cast<void(QListView::*)()>(&QListView::update), Qt::UniqueConnection);
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
				else if (backgroundMode == QLatin1String("none"))
				{
					m_contentsWidget->setBackgroundMode(StartPageContentsWidget::NoBackground);
				}
				else
				{
					m_contentsWidget->setBackgroundMode(StartPageContentsWidget::DefaultBackground);
				}

				m_tileDelegate->setPixmapCachePrefix(m_contentsWidget->getPixmapCachePrefix());

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

void StartPageWidget::handleIsReloadingTileChanged(const QModelIndex &index)
{
	QPixmapCache::remove(m_tileDelegate->createPixmapCacheKey(m_listView->visualRect(index), index.data(BookmarksModel::IdentifierRole).toULongLong()));

	m_listView->update(index);

	m_thumbnail = {};

	if (m_spinnerAnimation && m_model->match(m_model->index(0, 0), StartPageModel::IsReloadingRole, true, 1, Qt::MatchExactly).isEmpty())
	{
		m_spinnerAnimation->deleteLater();
		m_spinnerAnimation = nullptr;
	}
}

void StartPageWidget::updateSize()
{
	const qreal zoom(SettingsManager::getOption(SettingsManager::StartPage_ZoomLevelOption).toInt() / static_cast<qreal>(100));
	const int tileHeight(qRound((SettingsManager::getOption(SettingsManager::StartPage_TileHeightOption).toInt() + 10) * zoom));
	const int tileWidth(qRound((SettingsManager::getOption(SettingsManager::StartPage_TileWidthOption).toInt() + 10) * zoom));
	const int tilesPerRow(SettingsManager::getOption(SettingsManager::StartPage_TilesPerRowOption).toInt());
	const int amount(m_model->rowCount());
	const int columns((tilesPerRow > 0) ? tilesPerRow : qMax(1, ((width() - 50) / tileWidth)));
	const int rows(qCeil(amount / static_cast<qreal>(columns)));

	m_listView->setGridSize({tileWidth, tileHeight});
	m_listView->setFixedSize(((qMin(amount, columns) * tileWidth) + 2), ((rows * tileHeight) + 20));

	m_tileDelegate->setPixmapCachePrefix(m_contentsWidget->getPixmapCachePrefix());

	m_currentIndex = {};
	m_thumbnail = {};
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
		menu.addAction(ThemesManager::createIcon(QLatin1String("document-open")), QCoreApplication::translate("actions", "Open"), this, &StartPageWidget::openTile);
		menu.addSeparator();
		menu.addAction(tr("Edit…"), this, &StartPageWidget::editTile);

		if (SettingsManager::getOption(SettingsManager::StartPage_TileBackgroundModeOption) == QLatin1String("thumbnail"))
		{
			menu.addAction(tr("Reload"), this, &StartPageWidget::reloadTile)->setEnabled(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark);
		}

		menu.addSeparator();
		menu.addAction(ThemesManager::createIcon(QLatin1String("edit-delete")), tr("Remove"), this, &StartPageWidget::removeTile);
	}
	else
	{
		menu.addAction(tr("Configure…"), this, &StartPageWidget::configure);
		menu.addSeparator();
		menu.addAction(ThemesManager::createIcon(QLatin1String("list-add")), tr("Add Tile…"), this, &StartPageWidget::addTile);
	}

	menu.exec(hitPosition);
}

Animation* StartPageWidget::getLoadingAnimation()
{
	return m_spinnerAnimation;
}

QPixmap StartPageWidget::createThumbnail()
{
	if (m_thumbnail.isNull())
	{
		QPixmap pixmap(widget()->size());
		pixmap.setDevicePixelRatio(devicePixelRatio());

		QPainter painter(&pixmap);

		widget()->render(&painter);

		painter.end();

		m_thumbnail = pixmap.scaledToWidth(260, Qt::SmoothTransformation).copy(0, 0, 260, 170);
	}

	return m_thumbnail;
}

bool StartPageWidget::event(QEvent *event)
{
	if (!GesturesManager::isTracking())
	{
		switch (event->type())
		{
			case QEvent::MouseButtonDblClick:
			case QEvent::MouseButtonPress:
			case QEvent::Wheel:
				GesturesManager::startGesture(this, event, {GesturesManager::GenericContext});

				break;
			default:
				break;
		}
	}

	return QScrollArea::event(event);
}

bool StartPageWidget::eventFilter(QObject *object, QEvent *event)
{
	if ((object == m_contentsWidget || object == m_listView || object == m_listView->viewport()) && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::Wheel))
	{
		if (event->type() == QEvent::Wheel)
		{
			m_currentIndex = m_listView->indexAt(m_listView->mapFromGlobal(static_cast<QWheelEvent*>(event)->globalPosition().toPoint()));
		}
		else
		{
			m_currentIndex = m_listView->indexAt(m_listView->mapFromGlobal(static_cast<QMouseEvent*>(event)->globalPos()));
		}

		QVector<GesturesManager::GesturesContext> contexts({GesturesManager::GenericContext});

		if (m_currentIndex.isValid())
		{
			contexts.prepend(GesturesManager::LinkContext);
		}

		if (GesturesManager::startGesture(object, event, contexts))
		{
			return true;
		}
	}

	if (object == m_listView && event->type() == QEvent::KeyRelease)
	{
		const QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (m_isIgnoringEnter)
		{
			m_isIgnoringEnter = false;

			if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
			{
				return true;
			}
		}

		m_currentIndex = m_listView->currentIndex();

		if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
		{
			switch (static_cast<BookmarksModel::BookmarkType>(m_currentIndex.data(BookmarksModel::TypeRole).toInt()))
			{
				case BookmarksModel::FolderBookmark:
					{
						const BookmarksModel::Bookmark *bookmark(StartPageModel::getBookmark(m_currentIndex));

						if (bookmark && bookmark->hasChildren())
						{
							m_isIgnoringEnter = true;

							Menu menu(Menu::BookmarksMenu, this);
							menu.setMenuOptions({{QLatin1String("bookmark"), bookmark->getIdentifier()}});
							menu.exec(m_listView->mapToGlobal(m_listView->visualRect(m_currentIndex).center()));
						}
					}

					break;
				case BookmarksModel::UrlBookmark:
					{
						const QUrl url(m_currentIndex.data(BookmarksModel::UrlRole).toUrl());

						m_urlOpenTime = QTime::currentTime();

						if (keyEvent->modifiers() == Qt::NoModifier)
						{
							m_window->setUrl(url);
						}
						else
						{
							Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(SessionsManager::calculateOpenHints((m_window->isPrivate() ? SessionsManager::PrivateOpen : SessionsManager::DefaultOpen), Qt::LeftButton, keyEvent->modifiers()))}}, parentWidget());
						}
					}

					break;
				default:
					if (m_currentIndex.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("add"))
					{
						addTile();
					}

					break;
			}

			return true;
		}
	}
	else if (object == m_listView->viewport() && event->type() == QEvent::MouseButtonPress)
	{
		m_currentIndex = m_listView->indexAt(static_cast<QMouseEvent*>(event)->pos());
	}
	else if (object == m_listView->viewport() && event->type() == QEvent::MouseMove && static_cast<QMouseEvent*>(event)->buttons().testFlag(Qt::LeftButton) && ((m_urlOpenTime.isValid() && m_urlOpenTime.msecsTo(QTime::currentTime()) < 1000) || m_window->getLoadingState() != WebWidget::FinishedLoadingState))
	{
		return true;
	}
	else if (object == m_listView->viewport() && event->type() == QEvent::MouseButtonRelease)
	{
		const QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (m_isIgnoringEnter)
		{
			m_isIgnoringEnter = false;
		}

		if (m_listView->indexAt(mouseEvent->pos()) == m_currentIndex)
		{
			m_currentIndex = m_listView->indexAt(mouseEvent->pos());

			if (mouseEvent->button() == Qt::LeftButton || mouseEvent->button() == Qt::MiddleButton)
			{
				const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(m_currentIndex.data(BookmarksModel::TypeRole).toInt()));

				if (type == BookmarksModel::FolderBookmark)
				{
					const BookmarksModel::Bookmark *bookmark(StartPageModel::getBookmark(m_currentIndex));

					if (bookmark && bookmark->hasChildren())
					{
						m_isIgnoringEnter = true;

						Menu menu(Menu::BookmarksMenu, this);
						menu.setMenuOptions({{QLatin1String("bookmark"), bookmark->getIdentifier()}});
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

					event->ignore();

					return true;
				}

				const QUrl url(m_currentIndex.data(BookmarksModel::UrlRole).toUrl());

				if (url.isValid())
				{
					if (mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier)
					{
						m_urlOpenTime = QTime::currentTime();

						Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(SessionsManager::calculateOpenHints((m_window->isPrivate() ? SessionsManager::PrivateOpen : SessionsManager::DefaultOpen), mouseEvent->button(), mouseEvent->modifiers()))}}, parentWidget());
					}
					else if (mouseEvent->button() != Qt::MiddleButton)
					{
						SessionsManager::OpenHints hints(SessionsManager::CurrentTabOpen);

						if (m_window->isPrivate())
						{
							hints |= SessionsManager::PrivateOpen;
						}

						m_urlOpenTime = QTime::currentTime();

						Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}, {QLatin1String("hints"), QVariant(hints)}}, parentWidget());
					}
				}
			}
			else
			{
				event->ignore();
			}

			return true;
		}
	}
	else if (object == m_listView->viewport() && event->type() == QEvent::ToolTip)
	{
		const QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));
		const QModelIndex index(m_listView->indexAt(helpEvent->pos()));
		QString toolTip;

		if (index.isValid())
		{
			if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("add"))
			{
				toolTip = index.data(Qt::ToolTipRole).toString();
			}
			else
			{
				const BookmarksModel::Bookmark *bookmark(StartPageModel::getBookmark(index));

				if (bookmark)
				{
					const QKeySequence shortcut(ActionsManager::getActionShortcut(ActionsManager::OpenBookmarkAction, {{QLatin1String("startPageTile"), (index.row() + 1)}}));

					toolTip = Utils::appendShortcut(bookmark->getTitle(), shortcut);
				}
			}
		}

		if (toolTip.isEmpty())
		{
			QToolTip::hideText();
		}
		else
		{
			Utils::showToolTip(helpEvent->globalPos(), toolTip, m_listView, m_listView->visualRect(index));
		}

		return true;
	}

	return QWidget::eventFilter(object, event);
}

}
