/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_WEBCONTENTSWIDGET_H
#define OTTER_WEBCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"
#include "../../../ui/WebWidget.h"

#include <QtWidgets/QVBoxLayout>

namespace Otter
{

class PermissionBarWidget;
class ProgressBarWidget;
class SearchBarWidget;
class StartPageWidget;
class WebWidget;

class WebContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	explicit WebContentsWidget(bool isPrivate, WebWidget *widget, Window *window);

	void search(const QString &search, const QString &query);
	void print(QPrinter *printer);
	void setParent(Window *window);
	WebContentsWidget* clone(bool cloneHistory = true);
	Action* getAction(int identifier);
	WebWidget* getWebWidget();
	QString getTitle() const;
	QString getStatusMessage() const;
	QLatin1String getType() const;
	QVariant getOption(const QString &key) const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail() const;
	WindowHistoryInformation getHistory() const;
	QList<FeedUrl> getFeeds() const;
	int getZoom() const;
	bool canClone() const;
	bool canZoom() const;
	bool isLoading() const;
	bool isPrivate() const;

public slots:
	void goToHistoryIndex(int index);
	void triggerAction(int identifier, bool checked = false);
	void setOption(const QString &key, const QVariant &value);
	void setHistory(const WindowHistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url, bool typed = true);

protected:
	void timerEvent(QTimerEvent *event);
	void focusInEvent(QFocusEvent *event);
	void resizeEvent(QResizeEvent *event);
	void keyPressEvent(QKeyEvent *event);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void findInPage(WebWidget::FindFlags flags = WebWidget::NoFlagsFind);
	void handleUrlChange(const QUrl &url);
	void handlePermissionRequest(const QString &option, QUrl url, bool cancel);
	void notifyPermissionChanged(WebWidget::PermissionPolicies policies);
	void notifyRequestedOpenUrl(const QUrl &url, OpenHints hints);
	void notifyRequestedNewWindow(WebWidget *widget, OpenHints hints);
	void updateFindHighlight(WebWidget::FindFlags flags);
	void setLoading(bool loading);

private:
	QVBoxLayout *m_layout;
	WebWidget *m_webWidget;
	StartPageWidget *m_startPageWidget;
	SearchBarWidget *m_searchBarWidget;
	ProgressBarWidget *m_progressBarWidget;
	QString m_quickFindQuery;
	QList<PermissionBarWidget*> m_permissionBarWidgets;
	int m_quickFindTimer;
	int m_startPageTimer;
	bool m_isTabPreferencesMenuVisible;
	bool m_showStartPage;

	static QString m_sharedQuickFindQuery;
};

}

#endif
