/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_COOKIESCONTENTSWIDGET_H
#define OTTER_COOKIESCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>
#include <QtNetwork/QNetworkCookie>

namespace Otter
{

namespace Ui
{
	class CookiesContentsWidget;
}

class Window;

class CookiesContentsWidget final : public ContentsWidget
{
	Q_OBJECT

public:
	enum DataRole
	{
		CookieRole = Qt::UserRole
	};

	explicit CookiesContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent);
	~CookiesContentsWidget();

	void print(QPrinter *printer) override;
	QString getTitle() const override;
	QLatin1String getType() const override;
	QUrl getUrl() const override;
	QIcon getIcon() const override;
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	WebWidget::LoadingState getLoadingState() const override;
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;

protected:
	void changeEvent(QEvent *event) override;
	QStandardItem* findDomainItem(const QString &domain);
	QString getCookieDomain(const QNetworkCookie &cookie) const;
	QNetworkCookie getCookie(const QVariant &data) const;

protected slots:
	void populateCookies();
	void addCookie();
	void removeCookies();
	void removeDomainCookies();
	void removeAllCookies();
	void cookieProperties();
	void handleCookieAdded(const QNetworkCookie &cookie);
	void handleCookieRemoved(const QNetworkCookie &cookie);
	void showContextMenu(const QPoint &position);
	void updateActions();

private:
	QStandardItemModel *m_model;
	CookieJar *m_cookieJar;
	bool m_isLoading;
	Ui::CookiesContentsWidget *m_ui;
};

}

#endif
