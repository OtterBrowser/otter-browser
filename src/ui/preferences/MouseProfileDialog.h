/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2017 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_MOUSEPROFILEDIALOG_H
#define OTTER_MOUSEPROFILEDIALOG_H

#include "../Dialog.h"
#include "../ItemDelegate.h"
#include "../../core/GesturesManager.h"

namespace Otter
{

namespace Ui
{
	class MouseProfileDialog;
}

class GestureActionDelegate final : public ItemDelegate
{
public:
	explicit GestureActionDelegate(QObject *parent);

	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class MouseProfileDialog final : public Dialog
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		ContextRole,
		NameRole,
		ParametersRole
	};

	explicit MouseProfileDialog(const QString &profile, const QHash<QString, MouseProfile> &profiles, QWidget *parent = nullptr);
	~MouseProfileDialog();

	MouseProfile getProfile() const;
	bool isModified() const;

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void addGesture();
	void updateGesturesActions();
	void updateStepsActions();

private:
	MouseProfile m_profile;
	Ui::MouseProfileDialog *m_ui;
};

}

#endif
