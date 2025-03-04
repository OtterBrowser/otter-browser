/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_SOURCEVIEWERWEBWIDGET_H
#define OTTER_SOURCEVIEWERWEBWIDGET_H

#include "WebWidget.h"

#include <QtNetwork/QNetworkReply>

namespace Otter
{

class NetworkManager;
class SourceEditWidget;

class SourceViewerWebWidget final : public WebWidget
{
	Q_OBJECT

public:
	explicit SourceViewerWebWidget(bool isPrivate, ContentsWidget *parent = nullptr);

	void print(QPrinter *printer) override;
	WebWidget* clone(bool cloneHistory = true, bool isPrivate = false, const QStringList &excludedOptions = {}) const override;
	QString getTitle() const override;
	QString getSelectedText() const override;
	QUrl getUrl() const override;
	QIcon getIcon() const override;
	QPoint getScrollPosition() const override;
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	Session::Window::History getHistory() const override;
	HitTestResult getHitTestResult(const QPoint &position) override;
	WebWidget::LoadingState getLoadingState() const override;
	int getZoom() const override;
	bool canRedo() const override;
	bool canUndo() const override;
	bool hasSelection() const override;
	bool isPrivate() const override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;
	void findInPage(const QString &text, FindFlags flags = NoFlagsFind) override;
	void setOption(int identifier, const QVariant &value) override;
	void setScrollPosition(const QPoint &position) override;
	void setHistory(const Session::Window::History &history) override;
	void setZoom(int zoom) override;
	void setUrl(const QUrl &url, bool isTypedIn = true) override;
	void setContents(const QByteArray &contents, const QString &contentType);

protected:
	void setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions = {}) override;
	bool canViewSource() const override;

protected slots:
	void handleViewSourceReplyFinished();
	void handleZoomChanged();
	void notifyEditingActionsStateChanged();
	void showContextMenu(const QPoint &position = {-1, -1}) override;

private:
	SourceEditWidget *m_sourceEditWidget;
	NetworkManager *m_networkManager;
	QNetworkReply *m_viewSourceReply;
	QUrl m_url;
	bool m_isLoading;
	bool m_isPrivate;
};

}

#endif
