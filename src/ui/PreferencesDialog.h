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

#ifndef OTTER_PREFERENCESDIALOG_H
#define OTTER_PREFERENCESDIALOG_H

#include <QtWidgets/QDialog>

namespace Otter
{

class TableViewWidget;

namespace Ui
{
	class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PreferencesDialog(const QLatin1String &section, QWidget *parent = NULL);
	~PreferencesDialog();

protected:
	void changeEvent(QEvent *event);
	void loadProfiles(const QString &type, const QString &key, TableViewWidget *view);
	QString createProfileIdentifier(TableViewWidget *view, QString identifier = QString());
	QString getProfilePath(const QString &type, const QString &identifier);
	QHash<QString, QString> getProfileInformation(const QString &path) const;
	QHash<QString, QVariantHash> getProfileData(const QString &path) const;
	QHash<QString, QList<QKeySequence> > getShortcuts() const;

protected slots:
	void useCurrentAsStartPage();
	void restoreStartPage();
	void currentFontChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
	void fontChanged(QWidget *editor);
	void currentColorChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
	void colorChanged(QWidget *editor);
	void setupClearHistory();
	void addSearch();
	void editSearch();
	void updateSearchActions();
	void manageUserAgents();
	void proxyModeChanged(int index);
	void browseAutomaticProxyPath();
	void addKeyboardProfile();
	void editKeyboardProfile();
	void cloneKeyboardProfile();
	void removeKeyboardProfile();
	void updateKeyboardProfleActions();
	void addMacrosProfile();
	void editMacrosProfile();
	void cloneMacrosProfile();
	void removeMacrosProfile();
	void updateMacrosProfleActions();
	void markModified();
	void save();

private:
	QString m_defaultSearch;
	QStringList m_clearSettings;
	QStringList m_removedProfiles;
	QHash<QString, QHash<QString, QString> > m_keyboardProfilesInformation;
	QHash<QString, QHash<QString, QString> > m_macrosProfilesInformation;
	QHash<QString, QHash<QString, QVariantHash> > m_keyboardProfilesData;
	QHash<QString, QHash<QString, QVariantHash> > m_macrosProfilesData;
	Ui::PreferencesDialog *m_ui;
};

}

#endif
