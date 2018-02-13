/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PrivateWindowIndicatorWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/MainWindow.h"

#include <QtGui/QMouseEvent>

namespace Otter
{

PrivateWindowIndicatorWidget::PrivateWindowIndicatorWidget(const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_isHidden(false)
{
	setIcon(ThemesManager::createIcon(QLatin1String("window-private"), false));
	setText(definition.options.value(QLatin1String("text"), tr("Private Window")).toString());
	setButtonStyle(Qt::ToolButtonTextBesideIcon);
	setCheckable(true);
	setChecked(true);

	const MainWindow *mainWindow(MainWindow::findMainWindow(parent));

	m_isHidden = (mainWindow && !mainWindow->isPrivate());
}

void PrivateWindowIndicatorWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		setText(getOptions().value(QLatin1String("text"), tr("Private Window")).toString());
	}
}

void PrivateWindowIndicatorWidget::mousePressEvent(QMouseEvent *event)
{
	event->ignore();
}

void PrivateWindowIndicatorWidget::mouseReleaseEvent(QMouseEvent *event)
{
	event->ignore();
}

QSize PrivateWindowIndicatorWidget::minimumSizeHint() const
{
	if (m_isHidden)
	{
		return QSize(0, 0);
	}

	return ToolButtonWidget::minimumSizeHint();
}

QSize PrivateWindowIndicatorWidget::sizeHint() const
{
	if (m_isHidden)
	{
		return QSize(0, 0);
	}

	return ToolButtonWidget::sizeHint();
}

}
