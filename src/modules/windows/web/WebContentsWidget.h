/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_WEBCONTENTSWIDGET_H
#define OTTER_WEBCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"

namespace Otter
{

namespace Ui
{
	class WebContentsWidget;
}

class ProgressBarWidget;
class WebWidget;

class WebContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	explicit WebContentsWidget(bool isPrivate, WebWidget *widget, Window *window);
	~WebContentsWidget();

	void search(const QString &search, const QString &query);
	void print(QPrinter *printer);
	WebContentsWidget* clone(bool cloneHistory = true);
	QAction* getAction(ActionIdentifier action);
	QUndoStack* getUndoStack();
	QString getDefaultTextEncoding() const;
	QString getTitle() const;
	QString getStatusMessage() const;
	QString getQuickFindValue() const;
	QLatin1String getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail() const;
	WindowHistoryInformation getHistory() const;
	QPair<QString, QString> getUserAgent() const;
	int getZoom() const;
	bool canClone() const;
	bool canZoom() const;
	bool isLoading() const;
	bool isPrivate() const;
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void goToHistoryIndex(int index);
	void triggerAction(ActionIdentifier action, bool checked = false);
	void setUserAgent(const QString &identifier, const QString &value);
	void setDefaultTextEncoding(const QString &encoding);
	void setHistory(const WindowHistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url, bool typed = true);

protected:
	void timerEvent(QTimerEvent *event);
	void changeEvent(QEvent *event);
	void focusInEvent(QFocusEvent *event);
	void resizeEvent(QResizeEvent *event);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void scheduleGeometryUpdate();
	void notifyRequestedOpenUrl(const QUrl &url, OpenHints hints);
	void notifyRequestedNewWindow(WebWidget *widget, OpenHints hints);
	void updateFind(bool backwards = false);
	void updateFindHighlight();
	void setLoading(bool loading);

private:
	WebWidget *m_webWidget;
	ProgressBarWidget *m_progressBarWidget;
	int m_progressBarTimer;
	bool m_showProgressBar;
	Ui::WebContentsWidget *m_ui;

	static QString m_quickFindQuery;
};

}

#endif
