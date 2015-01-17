/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_QTWEBENGINEWEBWIDGET_H
#define OTTER_QTWEBENGINEWEBWIDGET_H

#include "../../../../ui/WebWidget.h"

#include <QtWebEngineWidgets/QWebEngineView>

namespace Otter
{

class ContentsDialog;

class QtWebEngineWebWidget : public WebWidget
{
	Q_OBJECT

public:
	void print(QPrinter *printer);
	WebWidget* clone(bool cloneHistory = true);
	Action* getAction(int identifier);
	QString getTitle() const;
	QString getSelectedText() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail();
	QRect getProgressBarGeometry() const;
	WindowHistoryInformation getHistory() const;
	QHash<QByteArray, QByteArray> getHeaders() const;
	QVariantHash getStatistics() const;
	int getZoom() const;
	bool isLoading() const;
	bool isPrivate() const;
	bool find(const QString &text, FindFlags flags = HighlightAllFind);

public slots:
	void clearOptions();
	void showDialog(ContentsDialog *dialog);
	void hideDialog(ContentsDialog *dialog);
	void goToHistoryIndex(int index);
	void triggerAction(int identifier, bool checked = false);
	void setHistory(const WindowHistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url, bool typed = true);

protected:
	explicit QtWebEngineWebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent = NULL);

	void updateOptions(const QUrl &url);
	void setOptions(const QVariantHash &options);
	QWebEnginePage* getPage();

protected slots:
	void pageLoadStarted();
	void pageLoadFinished();
	void linkHovered(const QString &link);
	void notifyTitleChanged();
	void notifyUrlChanged(const QUrl &url);
	void notifyIconChanged();
	void updatePageActions(const QUrl &url);
	void updateNavigationActions();

private:
	QWebEngineView *m_webView;
	QIcon m_icon;
	QHash<int, Action*> m_actions;
	bool m_isLoading;
	bool m_isTyped;

signals:
	void aboutToReload();

friend class QtWebEnginePage;
friend class QtWebEngineWebBackend;
};

}

#endif
