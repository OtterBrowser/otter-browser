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

#include "TileDelegate.h"
#include "../../../core/BookmarksModel.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/Utils.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QMovie>
#include <QtGui/QPainter>
#include <QtWidgets/QLabel>

namespace Otter
{

TileDelegate::TileDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void TileDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	const int textHeight = (option.fontMetrics.boundingRect(QLatin1String("X")).height() * 1.5);
	const QString tileBackgroundMode = SettingsManager::getValue(QLatin1String("StartPage/TileBackgroundMode")).toString();
	QRect rectangle(option.rect);
	rectangle.adjust(3, 3, -3, -3);

	QPainterPath path;
	path.addRoundedRect(rectangle, 5, 5);

	painter->setRenderHint(QPainter::HighQualityAntialiasing);

	if (index.data(BookmarksModel::UserRole).toBool() || index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("add"))
	{
		const bool isAddTile = (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("add"));

		if (isAddTile && (option.state.testFlag(QStyle::State_MouseOver) || option.state.testFlag(QStyle::State_HasFocus)))
		{
			painter->setPen(QPen(QGuiApplication::palette().color(QPalette::Highlight), 3));
		}
		else
		{
			painter->setPen(QPen(QColor(200, 200, 200), 1));
		}

		if (isAddTile)
		{
			painter->setBrush(QColor(200, 200, 200, 100));
		}
		else
		{
			painter->setBrush(Qt::transparent);
		}

		painter->drawPath(path);

		if (isAddTile)
		{
			Utils::getIcon(QLatin1String("list-add")).paint(painter, rectangle);
		}

		return;
	}

	painter->setClipPath(path);
	painter->fillRect(rectangle, QGuiApplication::palette().color(QPalette::Window));

	if (tileBackgroundMode != QLatin1String("none"))
	{
		rectangle.adjust(0, 0, 0, -textHeight);
	}

	const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt());

	if (type == BookmarksModel::FolderBookmark && tileBackgroundMode != QLatin1String("none"))
	{
		Utils::getIcon(QLatin1String("inode-directory")).paint(painter, rectangle);
	}
	else if (tileBackgroundMode == QLatin1String("thumbnail"))
	{
		painter->setBrush(Qt::white);
		painter->setPen(Qt::transparent);
		painter->drawRect(rectangle);
		painter->drawPixmap(rectangle, QPixmap(SessionsManager::getWritableDataPath(QLatin1String("thumbnails/")) + QString::number(index.data(BookmarksModel::IdentifierRole).toULongLong()) + QLatin1String(".png")));
	}
	else if (tileBackgroundMode == QLatin1String("favicon"))
	{
		HistoryManager::getIcon(index.data(BookmarksModel::UrlRole).toUrl()).paint(painter, rectangle);
	}

	painter->setClipping(false);
	painter->setPen(QGuiApplication::palette().color(QPalette::Text));

	if (tileBackgroundMode == QLatin1String("none"))
	{
		painter->drawText(rectangle, Qt::AlignCenter, option.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(), option.textElideMode, (rectangle.width() - 20)));
	}
	else
	{
		painter->drawText(QRect(rectangle.x(), (rectangle.y() + rectangle.height()), rectangle.width(), textHeight), Qt::AlignCenter, option.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(), option.textElideMode, (rectangle.width() - 20)));
	}

	if (option.state.testFlag(QStyle::State_MouseOver) || option.state.testFlag(QStyle::State_HasFocus))
	{
		painter->setPen(QPen(QGuiApplication::palette().color(QPalette::Highlight), 3));
	}
	else
	{
		painter->setPen(QPen(QColor(200, 200, 200), 1));
	}

	painter->setBrush(Qt::transparent);
	painter->drawPath(path);
}

void TileDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(index)

	QRect rectangle(option.rect);
	rectangle.adjust(3, 3, -3, -(3 + (option.fontMetrics.boundingRect(QLatin1String("X")).height() * 1.5)));

	editor->setGeometry(rectangle);
}

QWidget* TileDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	QLabel *editor = new QLabel(parent);

	editor->setAlignment(Qt::AlignCenter);

	QMovie *movie = new QMovie(QLatin1String(":/icons/loading.gif"), QByteArray(), editor);
	movie->start();

	editor->setMovie(movie);

	return editor;
}

QSize TileDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	const qreal zoom = (SettingsManager::getValue(QLatin1String("StartPage/ZoomLevel")).toInt() / qreal(100));

	return QSize((SettingsManager::getValue(QLatin1String("StartPage/TileWidth")).toInt() * zoom), (SettingsManager::getValue(QLatin1String("StartPage/TileHeight")).toInt() * zoom));
}

}
