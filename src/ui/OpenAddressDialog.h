/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_OPENADDRESSDIALOG_H
#define OTTER_OPENADDRESSDIALOG_H

#include "Dialog.h"
#include "../core/WindowsManager.h"

namespace Otter
{

namespace Ui
{
	class OpenAddressDialog;
}

class AddressWidget;
class InputInterpreter;

class OpenAddressDialog : public Dialog
{
	Q_OBJECT

public:
	explicit OpenAddressDialog(QWidget *parent = nullptr);
	~OpenAddressDialog();

	void setText(const QString &text);

protected:
	void changeEvent(QEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

protected slots:
	void handleUserInput();

private:
	AddressWidget *m_addressWidget;
	InputInterpreter *m_inputInterpreter;
	Ui::OpenAddressDialog *m_ui;

signals:
	void requestedOpenBookmark(BookmarksItem *bookmark, WindowsManager::OpenHints hints);
	void requestedLoadUrl(const QUrl &url, WindowsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &searchEngine, WindowsManager::OpenHints hints);
};

}

#endif
