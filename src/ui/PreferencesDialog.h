/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_PREFERENCESDIALOG_H
#define OTTER_PREFERENCESDIALOG_H

#include "Dialog.h"

namespace Otter
{

namespace Ui
{
	class PreferencesDialog;
}

class PreferencesDialog final : public Dialog
{
	Q_OBJECT

public:
	explicit PreferencesDialog(const QString &section, QWidget *parent = nullptr);
	~PreferencesDialog();

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void currentTabChanged(int tab);
	void openConfigurationManager();
	void markAsModified();
	void save();

private:
	QVector<bool> m_loadedTabs;
	Ui::PreferencesDialog *m_ui;

signals:
	void requestedSave();
};

}

#endif
