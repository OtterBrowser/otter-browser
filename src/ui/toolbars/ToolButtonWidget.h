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

#ifndef OTTER_TOOLBUTTONWIDGET_H
#define OTTER_TOOLBUTTONWIDGET_H

#include <QtCore/QVariantMap>
#include <QtWidgets/QToolButton>

namespace Otter
{

class ToolButtonWidget : public QToolButton
{
	Q_OBJECT

public:
	explicit ToolButtonWidget(QWidget *parent = NULL);

	QVariantMap getOptions() const;

public slots:
	void setOptions(const QVariantMap &options);

protected:
	void paintEvent(QPaintEvent *event);
	bool event(QEvent *event);

protected slots:
	void setMaximumButtonSize(int size);

private:
	QVariantMap m_options;
	bool m_isCustomized;
};

}

#endif
