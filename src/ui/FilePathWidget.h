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

#ifndef OTTER_FILEPATHWIDGET_H
#define OTTER_FILEPATHWIDGET_H

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLineEdit>

namespace Otter
{

class FilePathWidget : public QWidget
{
	Q_OBJECT

public:
	explicit FilePathWidget(QWidget *parent = NULL);

	void setSelectFile(bool mode);
	void setPath(const QString &path);
	QString getPath() const;

protected:
	void focusInEvent(QFocusEvent *event);

protected slots:
	void selectPath();
	void updateCompleter();

private:
	QLineEdit *m_lineEdit;
	QCompleter *m_completer;
	bool m_selectFile;
};

}

#endif
