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

#include "StartPageModel.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/BookmarksModel.h"
#include "../../../core/SettingsManager.h"

namespace Otter
{

StartPageModel::StartPageModel(QObject *parent) : QStandardItemModel(parent)
{
	reload();

	connect(BookmarksManager::getModel(), SIGNAL(bookmarkAdded(BookmarksItem*)), this, SLOT(reload()));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkMoved(BookmarksItem*,BookmarksItem*,int)), this, SLOT(reload()));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkTrashed(BookmarksItem*)), this, SLOT(reload()));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkRestored(BookmarksItem*)), this, SLOT(reload()));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkRemoved(BookmarksItem*)), this, SLOT(reload()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

void StartPageModel::optionChanged(const QString &option)
{
	if (option == QLatin1String("StartPage/BookmarksFolder"))
	{
		reload();
	}
}

void StartPageModel::reload()
{
	const QString path = SettingsManager::getValue(QLatin1String("StartPage/BookmarksFolder")).toString();
	BookmarksItem *bookmark = BookmarksManager::getModel()->getItem(path);

	if (!bookmark)
	{
		const QStringList directories = path.split(QLatin1Char('/'), QString::SkipEmptyParts);

		bookmark = BookmarksManager::getModel()->getRootItem();

		for (int i = 0; i < directories.count(); ++i)
		{
			bool found = false;

			for (int j = 0; j < bookmark->rowCount(); ++j)
			{
				if (bookmark->child(j) && bookmark->child(j)->data(Qt::DisplayRole) == directories.at(i))
				{
					bookmark = dynamic_cast<BookmarksItem*>(bookmark->child(j));

					found = true;

					break;
				}
			}

			if (!found)
			{
				bookmark = BookmarksManager::getModel()->addBookmark(BookmarksModel::FolderBookmark, 0, QUrl(), directories.at(i), bookmark);
			}
		}
	}

	clear();

	if (bookmark)
	{
		for (int i = 0; i < bookmark->rowCount(); ++i)
		{
			if (bookmark->child(i) && static_cast<BookmarksModel::BookmarkType>(bookmark->child(i)->data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark)
			{
				appendRow(bookmark->child(i)->clone());
			}
		}
	}

	emit reloaded();
}

}
