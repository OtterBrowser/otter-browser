/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_QTWEBKITPAGE_H
#define OTTER_QTWEBKITPAGE_H

#include "../../../../core/SessionsManager.h"

#include <QtWebKit/QWebElement>
#include <QtWebKitWidgets/QWebPage>

namespace Otter
{

class QtWebKitNetworkManager;
class QtWebKitPage;
class QtWebKitWebWidget;
class WebWidget;

class QtWebKitFrame final : public QObject
{
	Q_OBJECT

public:
	explicit QtWebKitFrame(QWebFrame *frame, QtWebKitWebWidget *parent);

	void runUserScripts(const QUrl &url) const;
	bool isDisplayingErrorPage() const;

public slots:
	void handleIsDisplayingErrorPageChanged(QWebFrame *frame, bool isDisplayingErrorPage);

protected:
	void applyContentBlockingRules(const QStringList &rules, bool isHiding);

protected slots:
	void handleLoadFinished();

private:
	QWebFrame *m_frame;
	QtWebKitWebWidget *m_widget;
	bool m_isDisplayingErrorPage;
};

class QtWebKitPage final : public QWebPage
{
	Q_OBJECT

public:
	enum HistoryEntryData
	{
		IdentifierEntryData = 0,
		ZoomEntryData,
		PositionEntryData,
		VisitTimeEntryData
	};

	explicit QtWebKitPage(QtWebKitNetworkManager *networkManager, QtWebKitWebWidget *parent);
	~QtWebKitPage();

	void triggerAction(WebAction action, bool isChecked = false) override;
	QtWebKitFrame* getMainFrame() const;
	QtWebKitNetworkManager* getNetworkManager() const;
	QVariant runScript(const QString &path, QWebElement element = {});
	bool event(QEvent *event) override;
	bool extension(Extension extension, const ExtensionOption *option = nullptr, ExtensionReturn *output = nullptr) override;
	bool shouldInterruptJavaScript() override;
	bool supportsExtension(Extension extension) const override;
	bool isDisplayingErrorPage() const;
	bool isPopup() const;
	bool isViewingMedia() const;

public slots:
	void saveState(QWebFrame *frame, QWebHistoryItem *item);
	void restoreState(QWebFrame *frame);
	void markAsDisplayingErrorPage();
	void updateStyleSheets(const QUrl &url = {});

protected:
	explicit QtWebKitPage();
	explicit QtWebKitPage(const QUrl &url);

	void markAsPopup();
	void javaScriptAlert(QWebFrame *frame, const QString &message) override;
	QWebPage* createWindow(WebWindowType type) override;
	QtWebKitWebWidget* createWidget(SessionsManager::OpenHints hints);
	QString chooseFile(QWebFrame *frame, const QString &suggestedFile) override;
	QString userAgentForUrl(const QUrl &url) const override;
	QVariant getOption(int identifier) const;
	bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type) override;
	bool javaScriptConfirm(QWebFrame *frame, const QString &message) override;
	bool javaScriptPrompt(QWebFrame *frame, const QString &message, const QString &defaultValue, QString *result) override;

protected slots:
	void handleFrameCreation(QWebFrame *frame);
	void handleConsoleMessage(MessageSource category, MessageLevel level, const QString &message, int line, const QString &source);

private:
	QtWebKitWebWidget *m_widget;
	QtWebKitNetworkManager *m_networkManager;
	QtWebKitFrame *m_mainFrame;
	QVector<QtWebKitPage*> m_popups;
	bool m_isIgnoringJavaScriptPopups;
	bool m_isPopup;
	bool m_isViewingMedia;

signals:
	void requestedNewWindow(WebWidget *widget, SessionsManager::OpenHints hints, const QVariantMap &parameters);
	void requestedPopupWindow(const QUrl &parentUrl, const QUrl &popupUrl);
	void aboutToNavigate(const QUrl &url, QWebFrame *frame, NavigationType navigationType);
	void isDisplayingErrorPageChanged(QWebFrame *frame, bool isVisible);
	void viewingMediaChanged(bool isViewingMedia);

friend class QtWebKitWebPageThumbnailJob;
friend class QtWebKitWebBackend;
};

}

#endif
