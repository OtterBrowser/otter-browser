/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2017 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_WEBCONTENTSWIDGET_H
#define OTTER_WEBCONTENTSWIDGET_H

#include "../../../core/PasswordsManager.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/SplitterWidget.h"

#include <QtWidgets/QVBoxLayout>

namespace Otter
{

class PasswordBarWidget;
class PermissionBarWidget;
class PopupsBarWidget;
class ProgressToolBarWidget;
class SearchBarWidget;
class StartPageWidget;
class WebsiteInformationDialog;

class WebContentsWidget final : public ContentsWidget
{
	Q_OBJECT

public:
	explicit WebContentsWidget(const QVariantMap &parameters, const QHash<int, QVariant> &options, WebWidget *widget, Window *window, QWidget *parent);

	void search(const QString &search, const QString &query);
	void print(QPrinter *printer) override;
	void setParent(Window *window) override;
	WebContentsWidget* clone(bool cloneHistory = true) const override;
	WebWidget* getWebWidget() const override;
	QString parseQuery(const QString &query) const override;
	QString getTitle() const override;
	QString getDescription() const override;
	QLatin1String getType() const override;
	QVariant getOption(int identifier) const override;
	QUrl getUrl() const override;
	QIcon getIcon() const override;
	QPixmap createThumbnail() override;
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	Session::Window::History getHistory() const override;
	QHash<int, QVariant> getOptions() const;
	WebWidget::ContentStates getContentState() const override;
	WebWidget::LoadingState getLoadingState() const override;
	int getZoom() const override;
	bool canClone() const override;
	bool canZoom() const override;
	bool isPrivate() const override;
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;
	void setOption(int identifier, const QVariant &value) override;
	void setHistory(const Session::Window::History &history) override;
	void setZoom(int zoom) override;
	void setUrl(const QUrl &url, bool isTypedIn = true) override;

protected:
	enum ScrollMode
	{
		NoScroll = 0,
		MoveScroll,
		DragScroll
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

	void timerEvent(QTimerEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void addInformationBar(QWidget *widget);
	void scrollContents(const QPoint &delta);
	void setScrollMode(ScrollMode mode);
	void setWidget(WebWidget *widget, const QVariantMap &parameters, const QHash<int, QVariant> &options);

protected slots:
	void findInPage(WebWidget::FindFlags flags);
	void closePasswordBar();
	void closePopupsBar();
	void handleOptionChanged(int identifier, const QVariant &value);
	void handleUrlChange(const QUrl &url);
	void handleSavePasswordRequest(const PasswordsManager::PasswordInformation &password, bool isUpdate);
	void handlePopupWindowRequest(const QUrl &parentUrl, const QUrl &popupUrl);
	void handlePermissionRequest(WebWidget::FeaturePermission feature, const QUrl &url, bool isCancellation);
	void handleInspectorVisibilityChangeRequest(bool isVisible);
	void handleLoadingStateChange(WebWidget::LoadingState state);
	void handleFindInPageQueryChanged();

private:
	QPointer<WebsiteInformationDialog> m_websiteInformationDialog;
	QVBoxLayout *m_layout;
	SplitterWidget *m_splitter;
	WebWidget *m_webWidget;
	Window *m_window;
	StartPageWidget *m_startPageWidget;
	SearchBarWidget *m_searchBarWidget;
	ProgressToolBarWidget *m_progressBarWidget;
	PasswordBarWidget *m_passwordBarWidget;
	PopupsBarWidget *m_popupsBarWidget;
	QString m_quickFindQuery;
	QPoint m_beginCursorPosition;
	QPoint m_lastCursorPosition;
	QVector<PermissionBarWidget*> m_permissionBarWidgets;
	ScrollMode m_scrollMode;
	int m_createStartPageTimer;
	int m_quickFindTimer;
	int m_scrollTimer;
	bool m_isTabPreferencesMenuVisible;
	bool m_isStartPageEnabled;
	bool m_isIgnoringMouseRelease;

	static QString m_sharedQuickFindQuery;
	static QMap<ScrollDirections, QCursor> m_scrollCursors;
};

}

#endif
