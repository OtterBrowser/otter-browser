/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ADVANCEDPREFERENCESPAGE_H
#define OTTER_ADVANCEDPREFERENCESPAGE_H

#include "../../../core/NetworkManagerFactory.h"
#include "../../../ui/CategoriesTabWidget.h"

#include <QtGui/QStandardItem>

namespace Otter
{

namespace Ui
{
	class AdvancedPreferencesPage;
}

class MouseProfile;

class AdvancedPreferencesPage final : public CategoryPage
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

	explicit AdvancedPreferencesPage(QWidget *parent);
	~AdvancedPreferencesPage();

	void load() override;
	QString getTitle() const override;

public slots:
	void save() override;

protected:
	void changeEvent(QEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void updateReaddMouseProfileMenu();
	void updateStyle();
	QString createProfileIdentifier(QStandardItemModel *model, const QString &base = {}) const;

protected slots:
	void playNotificationSound();
	void updateNotificationsActions();
	void updateNotificationsOptions();
	void addDownloadsMimeType();
	void removeDownloadsMimeType();
	void updateDownloadsActions();
	void updateDownloadsOptions();
	void updateDownloadsMode();
	void addUserAgent(QAction *action);
	void editUserAgent();
	void updateUserAgentsActions();
	void saveUserAgents(QJsonArray *userAgents, const QStandardItem *parent);
	void addProxy(QAction *action);
	void editProxy();
	void updateProxiesActions();
	void saveProxies(QJsonArray *proxies, const QStandardItem *parent);
	void updateCiphersActions();
	void updateUpdateChannelsActions();
	void addMouseProfile();
	void readdMouseProfile(QAction *action);
	void editMouseProfile();
	void cloneMouseProfile();
	void removeMouseProfile();
	void updateMouseProfileActions();
	void updatePageSwitcher();

private:
	QStringList m_filesToRemove;
	QHash<QString, MouseProfile> m_mouseProfiles;
	QHash<QString, ProxyDefinition> m_proxies;
	Ui::AdvancedPreferencesPage *m_ui;
};

}

#endif
