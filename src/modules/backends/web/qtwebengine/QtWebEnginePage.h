/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_QTWEBENGINEPAGE_H
#define OTTER_QTWEBENGINEPAGE_H

#include "../../../../core/SessionsManager.h"
#include "../../../../ui/WebWidget.h"

#include <QtWebEngineWidgets/QWebEngineCertificateError>
#include <QtWebEngineWidgets/QWebEnginePage>

namespace Otter
{

class QtWebEngineWebWidget;
class WebWidget;

class QtWebEnginePage final : public QWebEnginePage
{
	Q_OBJECT

public:
	struct HistoryEntryInformation
	{
		QIcon icon;
		QDateTime timeVisited;
		QPoint position;
		quint64 identifier = 0;
		int zoom = -1;
		bool isValid = false;
	};

	explicit QtWebEnginePage(bool isPrivate, QtWebEngineWebWidget *parent);

	void setHistory(const Session::Window::History &history);
	QtWebEngineWebWidget* getWebWidget() const;
	QString createScriptSource(const QString &path, const QStringList &parameters = {}) const;
	QVariant runScriptSource(const QString &script);
	QVariant runScriptFile(const QString &path, const QStringList &parameters = {});
	WebWidget::SslInformation getSslInformation() const;
	Session::Window::History getHistory() const;
	bool isPopup() const;
	bool isViewingMedia() const;

protected:
	void markAsPopup();
	void javaScriptAlert(const QUrl &url, const QString &message) override;
	void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &note, int line, const QString &source) override;
	QWebEnginePage* createWindow(WebWindowType type) override;
	QtWebEngineWebWidget* createWidget(SessionsManager::OpenHints hints);
	QString createJavaScriptList(const QStringList &rules) const;
	QStringList chooseFiles(FileSelectionMode mode, const QStringList &oldFiles, const QStringList &acceptedMimeTypes) override;
	bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
	bool certificateError(const QWebEngineCertificateError &error) override;
	bool javaScriptConfirm(const QUrl &url, const QString &message) override;
	bool javaScriptPrompt(const QUrl &url, const QString &message, const QString &defaultValue, QString *result) override;

protected slots:
	void handleLoadFinished();

private:
	QtWebEngineWebWidget *m_widget;
	WebWidget::SslInformation m_sslInformation;
	QVector<QtWebEnginePage*> m_popups;
	QVector<HistoryEntryInformation> m_history;
	NavigationType m_previousNavigationType;
	bool m_isIgnoringJavaScriptPopups;
	bool m_isViewingMedia;
	bool m_isPopup;

signals:
	void requestedNewWindow(WebWidget *widget, SessionsManager::OpenHints hints, const QVariantMap &parameters);
	void requestedPopupWindow(const QUrl &parentUrl, const QUrl &popupUrl);
	void aboutToNavigate(const QUrl &url, NavigationType navigationType);
	void viewingMediaChanged(bool isViewingMedia);
};

}

#endif
