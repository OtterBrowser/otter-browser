/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2021 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_HEADERWIDGET_H
#define OTTER_HEADERWIDGET_H

#include <QtWidgets/QLabel>

namespace Otter
{

class HeaderWidget final : public QLabel
{
public:
	explicit HeaderWidget(const QString &text, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
	explicit HeaderWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

protected:
	void changeEvent(QEvent *event) override;
	void updateFont();
};

}

#endif
