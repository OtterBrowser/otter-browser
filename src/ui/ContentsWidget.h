/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_CONTENTSWIDGET_H
#define OTTER_CONTENTSWIDGET_H

#include "WebWidget.h"

#include <QtCore/QPointer>
#include <QtWidgets/QWidget>

namespace Otter
{

class Action;
class ContentsDialog;
class Window;

class ContentsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ContentsWidget(const QVariantMap &parameters, Window *window);

	virtual void setParent(Window *window);
	virtual ContentsWidget* clone(bool cloneHistory = true) const;
	virtual Action* createAction(int identifier, const QVariantMap parameters = {}, bool followState = true);
	virtual WebWidget* getWebWidget() const;
	virtual QString parseQuery(const QString &query) const;
	virtual QString getTitle() const = 0;
	virtual QString getVersion() const;
	virtual QString getActiveStyleSheet() const;
	virtual QString getStatusMessage() const;
	virtual QLatin1String getType() const = 0;
	virtual QVariant getOption(int identifier) const;
	virtual QVariant getPageInformation(WebWidget::PageInformation key) const;
	virtual QUrl getUrl() const = 0;
	virtual QIcon getIcon() const = 0;
	virtual QPixmap getThumbnail();
	virtual ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const;
	virtual WindowHistoryInformation getHistory() const;
	virtual QStringList getStyleSheets() const;
	virtual QVector<WebWidget::LinkUrl> getFeeds() const;
	virtual QVector<WebWidget::LinkUrl> getSearchEngines() const;
	virtual QVector<NetworkManager::ResourceInformation> getBlockedRequests() const;
	virtual WebWidget::ContentStates getContentState() const;
	virtual WebWidget::LoadingState getLoadingState() const;
	int getSidebar() const;
	virtual int getZoom() const;
	virtual bool canClone() const;
	virtual bool canZoom() const;
	virtual bool isPrivate() const;
	bool isSidebarPanel() const;

public slots:
	virtual void triggerAction(int identifier, const QVariantMap &parameters = {});
	virtual void print(QPrinter *printer) = 0;
	virtual void goToHistoryIndex(int index);
	virtual void removeHistoryIndex(int index, bool purge = false);
	void showDialog(ContentsDialog *dialog, bool lockEventLoop = true);
	virtual void setOption(int identifier, const QVariant &value);
	virtual void setActiveStyleSheet(const QString &styleSheet);
	virtual void setHistory(const WindowHistoryInformation &history);
	virtual void setZoom(int zoom);
	virtual void setUrl(const QUrl &url, bool isTyped = true);

protected:
	void timerEvent(QTimerEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

protected slots:
	void triggerAction();
	void handleAboutToClose();
	void handleDialogFinished();

private:
	Window *m_window;
	QWidget *m_layer;
	QVector<QPointer<ContentsDialog> > m_dialogs;
	int m_layerTimer;
	int m_sidebar;

signals:
	void aboutToNavigate();
	void needsAttention();
	void requestedOpenUrl(const QUrl &url, SessionsManager::OpenHints hints);
	void requestedNewWindow(ContentsWidget *widget, SessionsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &search, SessionsManager::OpenHints hints);
	void requestedGeometryChange(const QRect &geometry);
	void webWidgetChanged();
	void statusMessageChanged(const QString &message);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void requestBlocked(const NetworkManager::ResourceInformation &request);
	void actionsStateChanged(ActionsManager::ActionDefinition::ActionCategories categories);
	void contentStateChanged(WebWidget::ContentStates state);
	void loadingStateChanged(WebWidget::LoadingState state);
	void pageInformationChanged(WebWidget::PageInformation, const QVariant &value);
	void optionChanged(int identifier, const QVariant &value);
	void zoomChanged(int zoom);
	void canZoomChanged(bool isAllowed);
};

}

#endif
