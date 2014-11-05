/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_QTWEBKITWEBPAGE_H
#define OTTER_QTWEBKITWEBPAGE_H

#include "../../../../core/ActionsManager.h"
#include "../../../../core/WindowsManager.h"

#include <QtWebKitWidgets/QWebPage>

namespace Otter
{

class QtWebKitWebWidget;
class WebWidget;

class QtWebKitWebPage : public QWebPage
{
	Q_OBJECT

public:
	explicit QtWebKitWebPage(QtWebKitWebWidget *parent);

	void triggerAction(WebAction action, bool checked = false);
	void setParent(QtWebKitWebWidget *parent);
	void setUserAgent(const QString &identifier, const QString &value, bool manual = true);
	QPair<QString, QString> getUserAgent() const;
	bool extension(Extension extension, const ExtensionOption *option = NULL, ExtensionReturn *output = NULL);
	bool shouldInterruptJavaScript();
	bool supportsExtension(Extension extension) const;

public slots:
	void updatePageStyleSheets();

protected:
	void updateBlockedPageElements(const QStringList domainList, const bool isException);
	void javaScriptAlert(QWebFrame *frame, const QString &message);
	void javaScriptConsoleMessage(const QString &note, int line, const QString &source);
	QWebPage* createWindow(WebWindowType type);
	QString userAgentForUrl(const QUrl &url) const;
	bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);
	bool javaScriptConfirm(QWebFrame *frame, const QString &message);
	bool javaScriptPrompt(QWebFrame *frame, const QString &message, const QString &defaultValue, QString *result);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void clearIgnoreJavaScriptPopups();
	void pageLoadFinished();

private:
	QtWebKitWebWidget *m_webWidget;
	QString m_userAgentIdentifier;
	QString m_userAgentValue;
	QString m_userAgentParsed;
	bool m_ignoreJavaScriptPopups;
	bool m_isGlobalUserAgent;

	static QString m_defaultUserAgent;
	static QHash<QString, QString> m_userAgentComponents;

signals:
	void requestedNewWindow(WebWidget *widget, OpenHints hints);
};

}

#endif
