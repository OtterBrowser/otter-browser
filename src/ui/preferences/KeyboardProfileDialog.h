/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../core/ActionsManager.h"

#include <QtCore/QModelIndex>
#include <QtWidgets/QKeySequenceEdit>
#include <QtWidgets/QToolButton>

namespace Otter
{

namespace Ui
{
	class KeyboardProfileDialog;
}

class KeyboardProfileDialog;

class ShortcutWidget final : public QKeySequenceEdit
{
	Q_OBJECT

public:
	explicit ShortcutWidget(const QKeySequence &shortcut, QWidget *parent = nullptr);

protected:
	void changeEvent(QEvent *event) override;

private:
	QToolButton *m_clearButton;

signals:
	void commitData(QWidget *editor);
};

class KeyboardActionDelegate final : public ItemDelegate
{
public:
	explicit KeyboardActionDelegate(QObject *parent);

	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class KeyboardShortcutDelegate final : public ItemDelegate
{
public:
	explicit KeyboardShortcutDelegate(KeyboardProfileDialog *parent);

	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

private:
	KeyboardProfileDialog *m_dialog;
};

class KeyboardProfileDialog final : public Dialog
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		NameRole,
		ParametersRole,
		StatusRole
	};

	enum ShortcutStatus
	{
		ErrorStatus = 0,
		WarningStatus,
		NormalStatus
	};

	struct ValidationResult final
	{
		QString text;
		QIcon icon;
		bool isError = false;
	};

	explicit KeyboardProfileDialog(const QString &profile, const QHash<QString, KeyboardProfile> &profiles, bool areSingleKeyShortcutsAllowed, QWidget *parent = nullptr);
	~KeyboardProfileDialog();

	KeyboardProfile getProfile() const;
	ValidationResult validateShortcut(const QKeySequence &shortcut, const QModelIndex &index) const;
	bool isModified() const;

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void addAction();
	void removeAction();
	void updateActions();

private:
	KeyboardProfile m_profile;
	bool m_areSingleKeyShortcutsAllowed;
	Ui::KeyboardProfileDialog *m_ui;
};

}

#endif
