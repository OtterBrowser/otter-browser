/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2021 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ACTIONPARAMETERSDIALOG_H
#define OTTER_ACTIONPARAMETERSDIALOG_H

#include "Dialog.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class ActionParametersDialog;
}

class ActionParametersDialog final : public Dialog
{
	Q_OBJECT

public:
	explicit ActionParametersDialog(int action, const QVariantMap &parameters, QWidget *parent = nullptr);
	~ActionParametersDialog();

	QVariantMap getParameters() const;
	bool isModified() const;

protected:
	void changeEvent(QEvent *event) override;
	QStandardItem* addItem(const QString &key, const QVariant &value = {}, QStandardItem *parent = nullptr);
	QVariantMap getMap(const QModelIndex &parent = {}) const;

private:
	QStandardItemModel *m_model;
	Ui::ActionParametersDialog *m_ui;
};

}

#endif
