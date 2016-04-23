/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_PREFERENCESCONTENTPAGEWIDGET_H
#define OTTER_PREFERENCESCONTENTPAGEWIDGET_H

#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class PreferencesContentPageWidget;
}

class PreferencesContentPageWidget : public QWidget
{
	Q_OBJECT

public:
	explicit PreferencesContentPageWidget(QWidget *parent = NULL);
	~PreferencesContentPageWidget();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void currentFontChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
	void fontChanged(QWidget *editor);
	void currentColorChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
	void colorChanged(QWidget *editor);
	void save();

private:
	Ui::PreferencesContentPageWidget *m_ui;

signals:
	void settingsModified();
};

}

#endif
