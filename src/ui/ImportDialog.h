/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_IMPORTDIALOG_H
#define OTTER_IMPORTDIALOG_H

#include "../core/Importer.h"

#include <QtCore/QString>
#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class ImportDialog;
}

class ImportDialog : public QDialog
{
    Q_OBJECT

public:
	~ImportDialog();

	static void createDialog(const QString &importerName, QWidget *parent = NULL);

protected:
	ImportDialog(Importer *importer, QWidget *parent);

protected slots:
	void import();
	void setPath();

private:
	Importer *m_importer;
	QString m_path;
	Ui::ImportDialog *m_ui;
};

}

#endif
