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

#ifndef OTTER_OPENBOOKMARKDIALOG_H
#define OTTER_OPENBOOKMARKDIALOG_H

#include "Dialog.h"
#include "../core/ActionExecutor.h"

#include <QtWidgets/QCompleter>

namespace Otter
{

namespace Ui
{
	class OpenBookmarkDialog;
}

class OpenBookmarkDialog final : public Dialog
{
	Q_OBJECT

public:
	explicit OpenBookmarkDialog(ActionExecutor::Object executor, QWidget *parent = nullptr);
	~OpenBookmarkDialog();

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void openBookmark();
	void setCompletion(const QString &text);

private:
	QCompleter *m_completer;
	ActionExecutor::Object m_executor;
	Ui::OpenBookmarkDialog *m_ui;
};

}

#endif
