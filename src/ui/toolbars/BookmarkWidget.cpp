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

#include "BookmarkWidget.h"
#include "../MainWindow.h"
#include "../Menu.h"
#include "../ToolBarWidget.h"
#include "../../core/BookmarksModel.h"
#include "../../core/Utils.h"
#include "../../core/WindowsManager.h"

#include <QtGui/QMouseEvent>

namespace Otter
{

BookmarkWidget::BookmarkWidget(BookmarksItem *bookmark, QWidget *parent) : QToolButton(parent),
	m_bookmark(bookmark)
{
	updateBookmark();

	ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parent);

	if (toolBar)
	{
		setIconSize(toolBar->iconSize());
		setToolButtonStyle(toolBar->toolButtonStyle());

		connect(toolBar, SIGNAL(iconSizeChanged(QSize)), this, SLOT(setIconSize(QSize)));
		connect(toolBar, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)), this, SLOT(setToolButtonStyle(Qt::ToolButtonStyle)));
	}
}

BookmarkWidget::BookmarkWidget(const QString &path, QWidget *parent) : QToolButton(parent),
	m_bookmark(BookmarksManager::getModel()->getItem(path))
{
	updateBookmark();
}

void BookmarkWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QToolButton::mouseReleaseEvent(event);

	if (event->button() == Qt::LeftButton && m_bookmark)
	{
		MainWindow *window = MainWindow::findMainWindow(parentWidget());

		if (window)
		{
			window->getWindowsManager()->open(m_bookmark);
		}
	}
}

void BookmarkWidget::updateBookmark()
{
	if (m_bookmark)
	{
		const bool isFolder = (static_cast<BookmarksItem::BookmarkType>(m_bookmark->data(BookmarksModel::TypeRole).toInt()) == BookmarksItem::FolderBookmark);

		if (isFolder)
		{
			Menu *menu = new Menu(this);
			menu->setRole(BookmarksMenuRole);
			menu->menuAction()->setData(m_bookmark->index());

			setPopupMode(QToolButton::InstantPopup);
			setToolTip(m_bookmark->data(BookmarksModel::TitleRole).toString());
			setMenu(menu);
		}
		else
		{
			QStringList toolTip;
			toolTip.append(tr("Title: %1").arg(m_bookmark->data(BookmarksModel::TitleRole).toString()));
			toolTip.append(tr("Address: %1").arg(m_bookmark->data(BookmarksModel::UrlRole).toString()));

			if (m_bookmark->data(BookmarksModel::DescriptionRole).isValid())
			{
				toolTip.append(tr("Description: %1").arg(m_bookmark->data(BookmarksModel::DescriptionRole).toString()));
			}

			if (!m_bookmark->data(BookmarksModel::TimeAddedRole).toDateTime().isNull())
			{
				toolTip.append(tr("Created: %1").arg(m_bookmark->data(BookmarksModel::TimeAddedRole).toDateTime().toString()));
			}

			if (!m_bookmark->data(BookmarksModel::TimeVisitedRole).toDateTime().isNull())
			{
				toolTip.append(tr("Visited: %1").arg(m_bookmark->data(BookmarksModel::TimeVisitedRole).toDateTime().toString()));
			}

			setToolTip(QLatin1String("<div style=\"white-space:pre;\">") + toolTip.join(QLatin1Char('\n')) + QLatin1String("</div>"));
			setMenu(NULL);
		}

		setText(m_bookmark->data(BookmarksModel::TitleRole).toString());
		setIcon(m_bookmark->data(Qt::DecorationRole).value<QIcon>());
	}
	else
	{
		setText(QString());
		setToolTip(QString());
		setIcon(Utils::getIcon(QLatin1String("text-html")));
		setMenu(NULL);
	}
}

}

