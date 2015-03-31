/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include <QtCore/QVariantMap>
#include <QtWidgets/QDialog>

namespace Otter
{

struct SearchInformation;
struct ShortcutsProfile;
class ItemViewWidget;

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
	void updateReaddSearchMenu();
	void updateReaddKeyboardProfileMenu();
	QString createProfileIdentifier(ItemViewWidget *view, const QString &base = QString()) const;
	ShortcutsProfile loadKeyboardProfile(const QString &identifier, bool loadShortcuts) const;

protected slots:
	void currentTabChanged(int tab);
	void useCurrentAsHomePage();
	void useBookmarkAsHomePage(QAction *action);
	void restoreHomePage();
	void currentFontChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
	void fontChanged(QWidget *editor);
	void currentColorChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
	void colorChanged(QWidget *editor);
	void setupClearHistory();
	void manageAcceptLanguage();
	void addSearchEngine();
	void readdSearchEngine(QAction *action);
	void editSearchEngine();
	void removeSearchEngine();
	void updateSearchActions();
	void playNotificationSound();
	void updateNotificationsActions();
	void updateNotificationsOptions();
	void manageUserAgents();
	void proxyModeChanged(int index);
	void addCipher(QAction *action);
	void removeCipher();
	void updateCiphersActions();
	void addKeyboardProfile();
	void readdKeyboardProfile(QAction *action);
	void editKeyboardProfile();
	void cloneKeyboardProfile();
	void removeKeyboardProfile();
	void updateKeyboardProfileActions();
	void updateJavaScriptOptions();
	void openConfigurationManager();
	void markModified();
	void save();

private:
	QString m_defaultSearchEngine;
	QStringList m_filesToRemove;
	QStringList m_clearHisorySettings;
	QVector<bool> m_loadedTabs;
	QVariantMap m_javaScriptOptions;
	QHash<QString, ShortcutsProfile> m_shortcutsProfiles;
	QHash<QString, QPair<bool, SearchInformation> > m_searchEngines;
	bool m_userAgentsModified;
	Ui::PreferencesDialog *m_ui;
};

}

#endif
