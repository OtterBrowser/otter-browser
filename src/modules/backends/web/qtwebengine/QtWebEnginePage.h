/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_QQTWEBENGINEPAGE_H
#define OTTER_QQTWEBENGINEPAGE_H

#include "../../../../core/WindowsManager.h"

#include <QtWebEngineWidgets/QWebEnginePage>

namespace Otter
{

class QtWebEngineWebWidget;
class WebWidget;

class QtWebEnginePage : public QWebEnginePage
{
	Q_OBJECT

public:
	explicit QtWebEnginePage(bool isPrivate, QtWebEngineWebWidget *parent);

	bool isViewingMedia() const;

protected:
	void markAsPopup();
	void javaScriptAlert(const QUrl &url, const QString &message);
	void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &note, int line, const QString &source);
	QWebEnginePage* createWindow(WebWindowType type);
	QString createJavaScriptList(QStringList rules) const;
	QStringList chooseFiles(FileSelectionMode mode, const QStringList &oldFiles, const QStringList &acceptedMimeTypes);
	bool acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame);
	bool javaScriptConfirm(const QUrl &url, const QString &message);
	bool javaScriptPrompt(const QUrl &url, const QString &message, const QString &defaultValue, QString *result);

protected slots:
	void pageLoadFinished();
	void removePopup(const QUrl &url);

private:
	QtWebEngineWebWidget *m_widget;
	QList<QtWebEnginePage*> m_popups;
	QWebEnginePage::NavigationType m_previousNavigationType;
	bool m_ignoreJavaScriptPopups;
	bool m_isViewingMedia;
	bool m_isPopup;

signals:
	void requestedNewWindow(WebWidget *widget, WindowsManager::OpenHints hints);
	void requestedPopupWindow(const QUrl &parentUrl, const QUrl &popupUrl);
	void aboutToNavigate(const QUrl &url, QWebEnginePage::NavigationType navigationType);
	void viewingMediaChanged(bool viewingMedia);
};

}

#endif
