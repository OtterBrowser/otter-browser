/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

class IconWidget final : public QToolButton
{
	Q_OBJECT

public:
	explicit IconWidget(QWidget *parent = nullptr);

	void setIcon(const QString &icon);
	void setIcon(const QIcon &icon);
	void setDefaultIcon(const QString &icon);
	QString getIcon() const;
	int heightForWidth(int width) const override;
	bool hasHeightForWidth() const override;

protected:
	void changeEvent(QEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

protected slots:
	void clear();
	void reset();
	void selectFromFile();
	void selectFromTheme();
	void populateMenu();

private:
	QString m_icon;
	QString m_defaultIcon;

signals:
	void iconChanged(const QString &icon);
};

}

#endif
