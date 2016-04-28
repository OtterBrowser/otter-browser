/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_PREFERENCESADVANCEDPAGEWIDGET_H
#define OTTER_PREFERENCESADVANCEDPAGEWIDGET_H

#include <QtCore/QVariantMap>
#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class PreferencesAdvancedPageWidget;
}

struct MouseProfile;
struct KeyboardProfile;
class ItemViewWidget;

class PreferencesAdvancedPageWidget : public QWidget
{
	Q_OBJECT

public:
	explicit PreferencesAdvancedPageWidget(QWidget *parent = NULL);
	~PreferencesAdvancedPageWidget();

protected:
	void changeEvent(QEvent *event);
	void updateReaddKeyboardProfileMenu();
	void updateReaddMouseProfileMenu();
	QString createProfileIdentifier(ItemViewWidget *view, const QString &base = QString()) const;
	QStringList getSelectedUpdateChannels() const;
	KeyboardProfile loadKeyboardProfile(const QString &identifier, bool loadShortcuts) const;
	MouseProfile loadMouseProfile(const QString &identifier, bool loadGestures) const;

protected slots:
	void playNotificationSound();
	void updateNotificationsActions();
	void updateNotificationsOptions();
	void addDownloadsMimeType();
	void removeDownloadsMimeType();
	void updateDownloadsActions();
	void updateDownloadsOptions();
	void updateDownloadsMode();
	void manageUserAgents();
	void proxyModeChanged(int index);
	void addProxyException();
	void editProxyException();
	void removeProxyException();
	void updateProxyExceptionsActions();
	void addCipher(QAction *action);
	void removeCipher();
	void updateCiphersActions();
	void updateUpdateChannelsActions();
	void addKeyboardProfile();
	void readdKeyboardProfile(QAction *action);
	void editKeyboardProfile();
	void cloneKeyboardProfile();
	void removeKeyboardProfile();
	void updateKeyboardProfileActions();
	void addMouseProfile();
	void readdMouseProfile(QAction *action);
	void editMouseProfile();
	void cloneMouseProfile();
	void removeMouseProfile();
	void updateMouseProfileActions();
	void updateJavaScriptOptions();
	void changeCurrentPage();
	void save();

private:
	QStringList m_filesToRemove;
	QVariantMap m_javaScriptOptions;
	QHash<QString, KeyboardProfile> m_keyboardProfiles;
	QHash<QString, MouseProfile> m_mouseProfiles;
	bool m_userAgentsModified;
	Ui::PreferencesAdvancedPageWidget *m_ui;

signals:
	void settingsModified();
};

}

#endif
