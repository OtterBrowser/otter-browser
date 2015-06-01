/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_CONTENTSWIDGET_H
#define OTTER_CONTENTSWIDGET_H

#include "Window.h"

#include <QtCore/QPointer>

namespace Otter
{

enum SecurityLevel
{
	UnknownSecurityLevel = 0,
	LocalSecurityLevel = 1,
	SecureSecurityLevel = 2,
	InsecureSecurityLevel = 4,
	FraudSecurityLevel = 8
};

class ContentsDialog;

class ContentsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ContentsWidget(Window *window);

	virtual void setParent(Window *window);
	virtual ContentsWidget* clone(bool cloneHistory = true);
	virtual Action* getAction(int identifier);
	Window* getParent();
	virtual QString getTitle() const = 0;
	virtual QString getVersion() const;
	virtual QString getStatusMessage() const;
	virtual QLatin1String getType() const = 0;
	virtual QUrl getUrl() const = 0;
	virtual QIcon getIcon() const = 0;
	virtual QPixmap getThumbnail() const;
	virtual WindowHistoryInformation getHistory() const;
	virtual QList<LinkUrl> getFeeds() const;
	virtual QList<LinkUrl> getSearchEngines() const;
	virtual int getZoom() const;
	virtual bool canClone() const;
	virtual bool canZoom() const;
	virtual bool isLoading() const;
	virtual bool isPrivate() const;

public slots:
	virtual void triggerAction(int identifier, bool checked = false);
	virtual void print(QPrinter *printer) = 0;
	virtual void goToHistoryIndex(int index);
	void showDialog(ContentsDialog *dialog);
	virtual void setHistory(const WindowHistoryInformation &history);
	virtual void setZoom(int zoom);
	virtual void setUrl(const QUrl &url, bool typed = true);

protected:
	void timerEvent(QTimerEvent *event);
	void showEvent(QShowEvent *event);
	void resizeEvent(QResizeEvent *event);

protected slots:
	void triggerAction();
	void close();
	void cleanupDialog();

private:
	QWidget *m_layer;
	QList<QPointer<ContentsDialog> > m_dialogs;
	int m_layerTimer;

signals:
	void requestedOpenUrl(QUrl url, OpenHints hints);
	void requestedAddBookmark(QUrl url, QString title, QString description);
	void requestedEditBookmark(QUrl url);
	void requestedNewWindow(ContentsWidget *widget, OpenHints hints);
	void requestedSearch(QString query, QString search, OpenHints hints);
	void requestedGeometryChange(const QRect &geometry);
	void webWidgetChanged();
	void canZoomChanged(bool can);
	void statusMessageChanged(const QString &message);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void loadingChanged(bool loading);
	void zoomChanged(int zoom);
};

}

#endif
