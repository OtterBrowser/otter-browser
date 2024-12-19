/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2016 - 2017 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_INPUTPREFERENCESPAGE_H
#define OTTER_INPUTPREFERENCESPAGE_H

#include "../../../core/ActionsManager.h"
#include "../../../ui/CategoriesTabWidget.h"
#include "../../../ui/ItemDelegate.h"

#include <QtGui/QStandardItemModel>
#include <QtWidgets/QKeySequenceEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>

namespace Otter
{
namespace Ui
{
	class InputPreferencesPage;
}

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

class ActionDelegate final : public ItemDelegate
{
public:
	explicit ActionDelegate(QObject *parent);

	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class ParametersDelegate final : public ItemDelegate
{
public:
	explicit ParametersDelegate(QObject *parent);

	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

class ShortcutDelegate final : public ItemDelegate
{
public:
	explicit ShortcutDelegate(QObject *parent);

	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

class InputPreferencesPage final : public CategoryPage
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		IsDisabledRole,
		NameRole,
		ParametersRole,
		StatusRole
	};

	enum EntryStatus
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

	explicit InputPreferencesPage(QWidget *parent);
	~InputPreferencesPage();

	void load() override;
	static QString createParamatersPreview(const QVariantMap &rawParameters, const QString &separator);
	QString getTitle() const override;
	static ValidationResult validateShortcut(const QKeySequence &shortcut, const QModelIndex &index);

public slots:
	void save() override;

protected:
	struct ShortcutsDefinition
	{
		QVariantMap parameters;
		QVector<QKeySequence> shortcuts;
		QVector<QKeySequence> disabledShortcuts;
	};

	void changeEvent(QEvent *event) override;
	void loadKeyboardDefinitions(const QString &identifier);
	void addKeyboardShortcuts(int identifier, const QString &name, const QString &text, const QIcon &icon, const QVariantMap &rawParameters, const QVector<QKeySequence> &shortcuts, bool areShortcutsDisabled);
	void addKeyboardShortcut(bool isDisabled);
	QString createProfileIdentifier(QStandardItemModel *model, const QString &base = {}) const;
	QHash<int, QVector<KeyboardProfile::Action> > getKeyboardDefinitions() const;

protected slots:
	void addKeyboardProfile();
	void editKeyboardProfile();
	void cloneKeyboardProfile();
	void removeKeyboardProfile();
	void updateKeyboardProfileActions();
	void editShortcutParameters();
	void updateKeyboardShortcutActions();

private:
	QStandardItemModel *m_keyboardShortcutsModel;
	QPushButton *m_advancedButton;
	QString m_activeKeyboardProfile;
	QStringList m_filesToRemove;
	QHash<QString, KeyboardProfile> m_keyboardProfiles;
	Ui::InputPreferencesPage *m_ui;

	static bool m_areSingleKeyShortcutsAllowed;
};

}

#endif
