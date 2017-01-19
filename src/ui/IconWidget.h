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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_ICONWIDGET_H
#define OTTER_ICONWIDGET_H

#include <QtWidgets/QToolButton>

namespace Otter
{

class IconWidget : public QToolButton
{
	Q_OBJECT

public:
	explicit IconWidget(QWidget *parent = nullptr);

	void setIcon(const QString &data);
	void setIcon(const QIcon &icon);
	void setPlaceholderIcon(const QIcon &icon);
	QString getIcon() const;
	int heightForWidth(int width) const;
	bool hasHeightForWidth() const;

protected:
	void resizeEvent(QResizeEvent *event);

protected slots:
	void clear();
	void selectFromFile();
	void selectFromTheme();

private:
	QString m_icon;
	QIcon m_placeholderIcon;

signals:
	void iconChanged(const QIcon &icon);
};

}

#endif
