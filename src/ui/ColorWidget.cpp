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

#include "ColorWidget.h"
#include "../core/ThemesManager.h"

#include <QtGui/QClipboard>
#include <QtWidgets/QApplication>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QMenu>

namespace Otter
{

ColorWidget::ColorWidget(QWidget *parent) : QPushButton(parent)
{
	QMenu *menu = new QMenu(this);
	menu->addAction(tr("Select Color…"), this, SLOT(selectColor()));
	menu->addAction(tr("Copy Color"), this, SLOT(copyColor()));
	menu->addSeparator();
	menu->addAction(ThemesManager::getIcon(QLatin1String("edit-clear")), tr("Clear"), this, SLOT(clear()));

	setMenu(menu);
	setText(tr("Invalid"));
	setToolTip(tr("Invalid"));
}

void ColorWidget::clear()
{
	setColor(QColor());
}

void ColorWidget::copyColor()
{
	QApplication::clipboard()->setText(m_color.name().toUpper());
}

void ColorWidget::selectColor()
{
	QColorDialog dialog(this);
	dialog.setOption(QColorDialog::ShowAlphaChannel);
	dialog.setCurrentColor(m_color);

	if (dialog.exec() == QDialog::Accepted)
	{
		setColor(dialog.currentColor());

		emit colorChanged(dialog.currentColor());
	}
}

void ColorWidget::setColor(const QString &color)
{
	setColor(color.isEmpty() ? QColor() : QColor(color));
}

void ColorWidget::setColor(const QColor &color)
{
	m_color = color;

	const QString text(color.isValid() ? color.name().toUpper() : tr("Invalid"));
	QPalette palette = this->palette();
	palette.setColor(QPalette::Button, (color.isValid() ? color : QApplication::palette(this).color(QPalette::Button)));

	setPalette(palette);
	setText(text);
	setToolTip(text);
}

QColor ColorWidget::getColor() const
{
	return m_color;
}

}
