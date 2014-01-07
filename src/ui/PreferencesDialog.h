/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_PREFERENCESDIALOG_H
#define OTTER_PREFERENCESDIALOG_H

#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PreferencesDialog(const QLatin1String &section, QWidget *parent = NULL);
	~PreferencesDialog();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void browseDownloadsPath();
	void currentFontChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
	void fontChanged(QWidget *editor);
	void currentColorChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
	void colorChanged(QWidget *editor);
	void setupClearHistory();
	void addSearch();
	void editSearch();
	void updateSearchActions();
	void save();

private:
	QString m_defaultSearch;
	QStringList m_clearSettings;
	Ui::PreferencesDialog *m_ui;
};

}

#endif
