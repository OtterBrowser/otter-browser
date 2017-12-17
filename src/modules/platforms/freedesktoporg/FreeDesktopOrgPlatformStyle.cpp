/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "FreeDesktopOrgPlatformStyle.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"

#include <QtCore/QProcess>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOption>

namespace Otter
{

FreeDesktopOrgPlatformStyle::FreeDesktopOrgPlatformStyle(const QString &name) : Style(name),
	m_isGtkStyle(false),
	m_isGtkAmbianceTheme(false)
{
	checkForAmbianceTheme();

	connect(ThemesManager::getInstance(), &ThemesManager::widgetStyleChanged, this, &FreeDesktopOrgPlatformStyle::checkForAmbianceTheme);
}

void FreeDesktopOrgPlatformStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	if (m_isGtkAmbianceTheme && element == CE_ToolBar)
	{
		const QStyleOptionToolBar *toolBarOption(qstyleoption_cast<const QStyleOptionToolBar*>(option));
		const ToolBarWidget *toolBar(qobject_cast<const ToolBarWidget*>(widget));
		const bool isAddressBar(toolBar && toolBar->getIdentifier() == ToolBarsManager::AddressBar);

		if (!toolBarOption || !toolBar || (toolBarOption->toolBarArea != Qt::TopToolBarArea && !isAddressBar))
		{
			return;
		}

		bool shouldFillBackground(!isAddressBar && (toolBarOption->positionOfLine == QStyleOptionToolBar::Beginning || toolBarOption->positionOfLine == QStyleOptionToolBar::OnlyOne));
		bool hasVisibleTabBar(false);

		if (!shouldFillBackground)
		{
			if (toolBar->getIdentifier() == ToolBarsManager::TabBar)
			{
				shouldFillBackground = true;
			}
			else
			{
				const MainWindow *mainWindow(MainWindow::findMainWindow(widget->parentWidget()));

				if (mainWindow)
				{
					const QVector<ToolBarWidget*> toolBars(mainWindow->getToolBars(Qt::TopToolBarArea));

					for (int i = (toolBars.count() - 1); i >= 0; --i)
					{
						if (toolBars.at(i)->getIdentifier() == ToolBarsManager::TabBar && toolBars.at(i)->isVisible())
						{
							hasVisibleTabBar = true;

							break;
						}

						if (toolBars.at(i) == toolBar)
						{
							shouldFillBackground = true;

							if (!isAddressBar && !hasVisibleTabBar)
							{
								break;
							}
						}
					}
				}
			}
		}

		if (!hasVisibleTabBar && isAddressBar)
		{
			shouldFillBackground = true;
		}

		if (shouldFillBackground)
		{
			painter->fillRect(option->rect, QColor(60, 59, 55));

			drawToolBarEdge(option, painter);

			return;
		}
	}

	Style::drawControl(element, option, painter, widget);
}

void FreeDesktopOrgPlatformStyle::checkForAmbianceTheme()
{
	const QString styleName(getName());

	m_isGtkStyle = false;
	m_isGtkAmbianceTheme = false;

	if (styleName == QLatin1String("gtk") || styleName == QLatin1String("gtk+") || styleName == QLatin1String("gtk2"))
	{
		m_isGtkStyle = true;
	}
	else
	{
		return;
	}

	QProcess process;
	process.setProgram(QLatin1String("gsettings"));
	process.setArguments({QLatin1String("get"), QLatin1String("org.gnome.desktop.interface"), QLatin1String("gtk-theme")});
	process.start(QIODevice::ReadOnly);
	process.waitForFinished();

	m_isGtkAmbianceTheme = (QString(process.readAll()).simplified().remove(QLatin1Char('\'')).remove(QLatin1Char('"')) == QLatin1String("Ambiance"));
}

int FreeDesktopOrgPlatformStyle::getExtraStyleHint(Style::ExtraStyleHint hint) const
{
	if (hint == CanAlignTabBarLabelHint && m_isGtkStyle)
	{
		return true;
	}

	return Style::getExtraStyleHint(hint);
}

}
