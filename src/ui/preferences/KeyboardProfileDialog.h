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

#ifndef OTTER_KEYBOARDPROFILEDIALOG_H
#define OTTER_KEYBOARDPROFILEDIALOG_H

#include "../Dialog.h"
#include "../ItemDelegate.h"

#include <QtCore/QModelIndex>

namespace Otter
{

namespace Ui
{
	class KeyboardProfileDialog;
}

struct KeyboardProfile
{
	QString identifier;
	QString title;
	QString description;
	QString author;
	QString version;
	QHash<int, QVector<QKeySequence> > shortcuts;
	bool isModified = false;
};

class KeyboardShortcutDelegate final : public ItemDelegate
{
public:
	explicit KeyboardShortcutDelegate(QObject *parent);

	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class KeyboardProfileDialog final : public Dialog
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		ShortcutsRole
	};

	explicit KeyboardProfileDialog(const QString &profile, const QHash<QString, KeyboardProfile> &profiles, QWidget *parent = nullptr);
	~KeyboardProfileDialog();

	KeyboardProfile getProfile() const;

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void addShortcut();
	void removeShortcut();
	void updateActionsActions();
	void updateShortcutsActions();
	void saveShortcuts();

private:
	QString m_profile;
	QModelIndex m_currentAction;
	bool m_isModified;
	Ui::KeyboardProfileDialog *m_ui;
};

}

#endif
