/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ACTIONCOMBOBOXWIDGET_H
#define OTTER_ACTIONCOMBOBOXWIDGET_H

#include <QtCore/QTime>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QComboBox>

namespace Otter
{

class ItemViewWidget;

class ActionComboBoxWidget : public QComboBox
{
	Q_OBJECT

public:
	explicit ActionComboBoxWidget(QWidget *parent = NULL);

	void setActionIdentifier(int action);
	int getActionIdentifier() const;
	bool eventFilter(QObject *object, QEvent *event);

protected:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

private:
	ItemViewWidget *m_view;
	QLineEdit *m_filterLineEdit;
	QTime m_popupHideTime;
	bool m_wasPopupVisible;
};

}

#endif
