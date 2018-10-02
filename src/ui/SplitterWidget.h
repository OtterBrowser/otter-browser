/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_SPLITTERWIDGET_H
#define OTTER_SPLITTERWIDGET_H

#include <QtWidgets/QSplitter>

namespace Otter
{

class MainWindow;

class SplitterWidget final : public QSplitter
{
public:
	explicit SplitterWidget(QWidget *parent = nullptr);
	SplitterWidget(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
	void showEvent(QShowEvent *event) override;

private:
	MainWindow *m_mainWindow;
	bool m_isInitialized;
};

}

#endif
