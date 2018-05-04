/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

namespace Otter
{

class ContentsDialog;
class Window;

class ContentsWidget : public QWidget, public ActionExecutor
{
	Q_OBJECT

public:
	explicit ContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent);

	virtual void setParent(Window *window);
	virtual ContentsWidget* clone(bool cloneHistory = true) const;
	virtual WebWidget* getWebWidget() const;
	Window* getWindow() const;
	virtual QString parseQuery(const QString &query) const;
	virtual QString getTitle() const = 0;
	virtual QString getDescription() const;
	virtual QString getVersion() const;
	virtual QString getStatusMessage() const;
	virtual QLatin1String getType() const = 0;
	virtual QVariant getOption(int identifier) const;
	virtual QUrl getUrl() const = 0;
	virtual QIcon getIcon() const = 0;
	virtual QPixmap createThumbnail();
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	virtual WindowHistoryInformation getHistory() const;
	virtual WebWidget::ContentStates getContentState() const;
	virtual WebWidget::LoadingState getLoadingState() const;
	int getSidebar() const;
	virtual int getZoom() const;
	virtual bool canClone() const;
	virtual bool canZoom() const;
	virtual bool isPrivate() const;
	bool isSidebarPanel() const;

public slots:
	virtual void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;
	virtual void print(QPrinter *printer);
	void showDialog(ContentsDialog *dialog, bool lockEventLoop = true);
	virtual void setOption(int identifier, const QVariant &value);
	virtual void setHistory(const WindowHistoryInformation &history);
	virtual void setZoom(int zoom);
	virtual void setUrl(const QUrl &url, bool isTyped = true);

protected:
	void timerEvent(QTimerEvent *event) override;
	void changeEvent(QEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

protected slots:
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
	void requestedNewWindow(ContentsWidget *widget, SessionsManager::OpenHints hints, const QVariantMap &parameters);
	void requestedSearch(const QString &query, const QString &search, SessionsManager::OpenHints hints);
	void requestedGeometryChange(const QRect &geometry);
	void webWidgetChanged();
	void statusMessageChanged(const QString &message);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void requestBlocked(const NetworkManager::ResourceInformation &request);
	void arbitraryActionsStateChanged(const QVector<int> &identifiers);
	void categorizedActionsStateChanged(const QVector<int> &categories);
	void contentStateChanged(WebWidget::ContentStates state);
	void loadingStateChanged(WebWidget::LoadingState state);
	void pageInformationChanged(WebWidget::PageInformation, const QVariant &value);
	void optionChanged(int identifier, const QVariant &value);
	void zoomChanged(int zoom);
	void canZoomChanged(bool isAllowed);
};

}

#endif
