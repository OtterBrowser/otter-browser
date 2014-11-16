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

#ifndef OTTER_OPENADDRESSDIALOG_H
#define OTTER_OPENADDRESSDIALOG_H

#include "../core/WindowsManager.h"

#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class OpenAddressDialog;
}

class AddressWidget;

class OpenAddressDialog : public QDialog
{
	Q_OBJECT

public:
	explicit OpenAddressDialog(QWidget *parent = NULL);
	~OpenAddressDialog();

protected slots:
	void handleInput();
	void openUrl(const QUrl &url);
	void openSearch(const QString &query, const QString &engine);

protected:
	void changeEvent(QEvent *event);

private:
	AddressWidget *m_addressWidget;
	Ui::OpenAddressDialog *m_ui;

signals:
	void requestedLoadUrl(QUrl url, OpenHints hints);
	void requestedSearch(QString query, QString engine);
};

}

#endif
