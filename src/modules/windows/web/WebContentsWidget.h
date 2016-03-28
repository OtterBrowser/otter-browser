/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "../../../core/PasswordsManager.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/WebWidget.h"

#include <QtWidgets/QVBoxLayout>

namespace Otter
{

class PasswordBarWidget;
class PermissionBarWidget;
class PopupsBarWidget;
class ProgressBarWidget;
class SearchBarWidget;
class StartPageWidget;
class WebsiteInformationDialog;
class WebWidget;

class WebContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	enum ScrollMode
	{
		NoScroll = 0,
		MoveScroll = 1,
		DragScroll = 2
	};

	enum ScrollDirection
	{
		NoDirection = 0,
		TopDirection = 1,
		BottomDirection = 2,
		RightDirection = 4,
		LeftDirection = 8
	};

	Q_DECLARE_FLAGS(ScrollDirections, ScrollDirection)

	explicit WebContentsWidget(bool isPrivate, WebWidget *widget, Window *window);

	void search(const QString &search, const QString &query);
	void print(QPrinter *printer);
	void setOptions(const QVariantHash &options);
	void setParent(Window *window);
	WebContentsWidget* clone(bool cloneHistory = true);
	Action* getAction(int identifier);
	WebWidget* getWebWidget();
	QString getTitle() const;
	QString getActiveStyleSheet() const;
	QString getStatusMessage() const;
	QLatin1String getType() const;
	QVariant getOption(const QString &key) const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail() const;
	WindowHistoryInformation getHistory() const;
	QStringList getStyleSheets() const;
	QList<LinkUrl> getFeeds() const;
	QList<LinkUrl> getSearchEngines() const;
	QVariantHash getOptions() const;
	WindowsManager::ContentStates getContentState() const;
	WindowsManager::LoadingState getLoadingState() const;
	int getZoom() const;
	bool canClone() const;
	bool canZoom() const;
	bool isPrivate() const;
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void goToHistoryIndex(int index);
	void removeHistoryIndex(int index, bool purge = false);
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap());
	void setOption(const QString &key, const QVariant &value);
	void setActiveStyleSheet(const QString &styleSheet);
	void setHistory(const WindowHistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url, bool typed = true);

protected:
	void timerEvent(QTimerEvent *event);
	void focusInEvent(QFocusEvent *event);
	void resizeEvent(QResizeEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void scrollContents(const QPoint &delta);
	void setScrollMode(ScrollMode mode);
	void setWidget(WebWidget *widget, bool isPrivate);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void findInPage(WebWidget::FindFlags flags = WebWidget::NoFlagsFind);
	void closePasswordBar();
	void closePopupsBar();
	void handleUrlChange(const QUrl &url);
	void handleAddPasswordRequest(const PasswordsManager::PasswordInformation &password);
	void handlePopupWindowRequest(const QUrl &parentUrl, const QUrl &popupUrl);
	void handlePermissionRequest(const QString &option, QUrl url, bool cancel);
	void handleLoadingStateChange(WindowsManager::LoadingState state);
	void notifyPermissionChanged(WebWidget::PermissionPolicies policies);
	void notifyRequestedOpenUrl(const QUrl &url, WindowsManager::OpenHints hints);
	void notifyRequestedNewWindow(WebWidget *widget, WindowsManager::OpenHints hints);
	void updateFindHighlight(WebWidget::FindFlags flags);

private:
	QPointer<WebsiteInformationDialog> m_websiteInformationDialog;
	QVBoxLayout *m_layout;
	WebWidget *m_webWidget;
	StartPageWidget *m_startPageWidget;
	SearchBarWidget *m_searchBarWidget;
	ProgressBarWidget *m_progressBarWidget;
	PasswordBarWidget *m_passwordBarWidget;
	PopupsBarWidget *m_popupsBarWidget;
	QString m_quickFindQuery;
	QPoint m_beginCursorPosition;
	QPoint m_lastCursorPosition;
	QList<PermissionBarWidget*> m_permissionBarWidgets;
	ScrollMode m_scrollMode;
	int m_quickFindTimer;
	int m_scrollTimer;
	int m_startPageTimer;
	bool m_isTabPreferencesMenuVisible;
	bool m_showStartPage;
	bool m_ignoreRelease;

	static QString m_sharedQuickFindQuery;
	static QMap<int, QPixmap> m_scrollCursors;
};

}

#endif
