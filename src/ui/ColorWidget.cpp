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

#include <QtWidgets/QColorDialog>

namespace Otter
{

ColorWidget::ColorWidget(QWidget *parent) : QPushButton(parent)
{
	setText(tr("Invalid"));
	setToolTip(tr("Invalid"));

	connect(this, SIGNAL(clicked(bool)), this, SLOT(selectColor()));
}

void ColorWidget::selectColor()
{
	QColorDialog dialog(this);
	dialog.setCurrentColor(palette().color(QPalette::Button));

	if (dialog.exec() == QDialog::Accepted)
	{
		setColor(dialog.currentColor());

		emit colorChanged(dialog.currentColor());
	}
}

void ColorWidget::setColor(const QColor &color)
{
	const QString text(color.isValid() ? color.name().toUpper() : tr("Invalid"));
	QPalette palette = this->palette();
	palette.setColor(QPalette::Button, color);

	setPalette(palette);
	setText(text);
	setToolTip(text);
}

QColor ColorWidget::getColor() const
{
	return palette().color(QPalette::Button);
}

}
