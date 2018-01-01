/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_PREFERENCESADVANCEDPAGEWIDGET_H
#define OTTER_PREFERENCESADVANCEDPAGEWIDGET_H

#include "../../core/NetworkManagerFactory.h"

#include <QtGui/QStandardItem>
#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class PreferencesAdvancedPageWidget;
}

class KeyboardProfile;
class MouseProfile;

class PreferencesAdvancedPageWidget final : public QWidget
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		SoundPathRole,
		ShouldShowAlertRole,
		ShouldShowNotificationRole,
		TransferModeRole,
		DownloadsPathRole,
		OpenCommandRole
	};

	explicit PreferencesAdvancedPageWidget(QWidget *parent = nullptr);
	~PreferencesAdvancedPageWidget();

public slots:
	void save();

protected:
	void changeEvent(QEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void updateReaddKeyboardProfileMenu();
	void updateReaddMouseProfileMenu();
	QString createProfileIdentifier(QStandardItemModel *model, const QString &base = {}) const;
	QStringList getSelectedUpdateChannels() const;

protected slots:
	void playNotificationSound();
	void updateNotificationsActions();
	void updateNotificationsOptions();
	void addOverride();
	void editOverride();
	void removeOverride();
	void updateOverridesActions();
	void addDownloadsMimeType();
	void removeDownloadsMimeType();
	void updateDownloadsActions();
	void updateDownloadsOptions();
	void updateDownloadsMode();
	void addUserAgent(QAction *action);
	void editUserAgent();
	void updateUserAgentsActions();
	void saveUsuerAgents(QJsonArray *userAgents, const QStandardItem *parent);
	void addProxy(QAction *action);
	void editProxy();
	void updateProxiesActions();
	void saveProxies(QJsonArray *proxies, const QStandardItem *parent);
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
	void updatePageSwitcher();

private:
	QStringList m_filesToRemove;
	QHash<QString, KeyboardProfile> m_keyboardProfiles;
	QHash<QString, MouseProfile> m_mouseProfiles;
	QHash<QString, ProxyDefinition> m_proxies;
	QHash<int, QVariant> m_javaScriptOptions;
	Ui::PreferencesAdvancedPageWidget *m_ui;

signals:
	void settingsModified();
};

}

#endif
