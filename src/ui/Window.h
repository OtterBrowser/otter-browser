/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2016 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_WINDOW_H
#define OTTER_WINDOW_H

#include "ToolBarWidget.h"
#include "WebWidget.h"
#include "../core/SessionsManager.h"

#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

namespace Otter
{

class ContentsWidget;

class WindowToolBarWidget final : public ToolBarWidget
{
public:
	explicit WindowToolBarWidget(int identifier, Window *parent);

protected:
	void paintEvent(QPaintEvent *event) override;
};

class Window final : public QWidget, public ActionExecutor
{
	Q_OBJECT

public:
	explicit Window(const QVariantMap &parameters, ContentsWidget *widget, MainWindow *mainWindow);

	void clear();
	void setOption(int identifier, const QVariant &value);
	void setSession(const Session::Window &session, bool deferLoading = false);
	Window* clone(bool cloneHistory, MainWindow *mainWindow) const;
	MainWindow* getMainWindow() const;
	WindowToolBarWidget* getAddressBar() const;
	ContentsWidget* getContentsWidget();
	WebWidget* getWebWidget();
	QString getTitle() const;
	QString getIdentity() const;
	QLatin1String getType() const;
	QVariant getOption(int identifier) const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap createThumbnail() const;
	QDateTime getLastActivity() const;
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	Session::Window::History getHistory() const;
	Session::Window getSession() const;
	QSize sizeHint() const override;
	WebWidget::LoadingState getLoadingState() const;
	WebWidget::ContentStates getContentState() const;
	quint64 getIdentifier() const;
	int getZoom() const;
	bool canClone() const;
	bool canZoom() const;
	bool isAboutToClose() const override;
	bool isActive() const;
	bool isPinned() const;
	bool isPrivate() const;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;
	void requestClose();
	void search(const QString &query, const QString &searchEngine);
	void markAsActive(bool updateLastActivity = true);
	void setUrl(const QUrl &url, bool isTypedIn = true);
	void setZoom(int zoom);
	void setPinned(bool isPinned);

protected:
	void timerEvent(QTimerEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void updateFocus();
	void setContentsWidget(ContentsWidget *widget);

private:
	Q_DISABLE_COPY(Window)

	MainWindow *m_mainWindow;
	WindowToolBarWidget *m_addressBarWidget;
	QPointer<ContentsWidget> m_contentsWidget;
	QDateTime m_lastActivity;
	Session::Window m_session;
	QVariantMap m_parameters;
	quint64 m_identifier;
	int m_suspendTimer;
	bool m_isAboutToClose;
	bool m_isPinned;

	static quint64 m_identifierCounter;

signals:
	void activated();
	void aboutToClose();
	void aboutToNavigate();
	void needsAttention();
	void requestedSearch(const QString &query, const QString &searchEngine, SessionsManager::OpenHints hints = SessionsManager::DefaultOpen);
	void requestedNewWindow(ContentsWidget *widget, SessionsManager::OpenHints hints, const QVariantMap &parameters);
	void requestedCloseWindow(Window *window);
	void statusMessageChanged(const QString &message);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url, bool force);
	void iconChanged(const QIcon &icon);
	void requestBlocked(const NetworkManager::ResourceInformation &request);
	void actionsStateChanged();
	void arbitraryActionsStateChanged(const QVector<int> &identifiers);
	void categorizedActionsStateChanged(const QVector<int> &categories);
	void contentStateChanged(WebWidget::ContentStates state);
	void loadingStateChanged(WebWidget::LoadingState state);
	void pageInformationChanged(WebWidget::PageInformation, const QVariant &value);
	void optionChanged(int identifier, const QVariant &value);
	void zoomChanged(int zoom);
	void canZoomChanged(bool isAllowed);
	void isPinnedChanged(bool isPinned);
};

}

#endif
