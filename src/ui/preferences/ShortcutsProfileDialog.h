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

#ifndef OTTER_SHORTCUTSPROFILEDIALOG_H
#define OTTER_SHORTCUTSPROFILEDIALOG_H

#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class ShortcutsProfileDialog;
}

class ShortcutsProfileDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ShortcutsProfileDialog(const QHash<QString, QString> &information, const QHash<QString, QVariantHash> &data, const QHash<QString, QList<QKeySequence> > &shortcuts, bool macrosMode, QWidget *parent = NULL);
	~ShortcutsProfileDialog();

	QHash<QString, QString> getInformation() const;
	QHash<QString, QVariantHash> getData() const;

protected:
	void changeEvent(QEvent *event);

protected slots:
	void addMacro();
	void addShortcut();
	void updateMacrosActions();
	void updateShortcutsActions();

private:
	QHash<QString, QList<QKeySequence> > m_shortcuts;
	bool m_macrosMode;
	Ui::ShortcutsProfileDialog *m_ui;
};

}

#endif
