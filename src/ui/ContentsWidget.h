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

#ifndef OTTER_CONTENTSWIDGET_H
#define OTTER_CONTENTSWIDGET_H

#include "Window.h"

#include <QtCore/QPointer>

namespace Otter
{

enum SecurityLevel
{
	UnknownLevel = 0,
	LocalLevel = 1,
	SecureLevel = 2,
	InsecureLevel = 4,
	FraudLevel = 8
};

class ContentsDialog;

class ContentsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ContentsWidget(Window *window);

	virtual void print(QPrinter *printer) = 0;
	void setParent(Window *window);
	virtual ContentsWidget* clone(Window *window = NULL);
	virtual QAction* getAction(WindowAction action);
	virtual QUndoStack* getUndoStack();
	virtual QString getTitle() const = 0;
	virtual QString getVersion();
	virtual QLatin1String getType() const = 0;
	virtual QUrl getUrl() const = 0;
	virtual QIcon getIcon() const = 0;
	virtual QPixmap getThumbnail() const;
	virtual WindowHistoryInformation getHistory() const;
	virtual int getZoom() const;
	virtual bool canClone() const;
	virtual bool canZoom() const;
	virtual bool isLoading() const;
	virtual bool isPrivate() const;

public slots:
	void showDialog(ContentsDialog *dialog);
	void hideDialog(ContentsDialog *dialog);
	virtual void goToHistoryIndex(int index);
	virtual void triggerAction(WindowAction action, bool checked = false);
	virtual void setHistory(const WindowHistoryInformation &history);
	virtual void setZoom(int zoom);
	virtual void setUrl(const QUrl &url, bool typed = true);

protected:
	void showEvent(QShowEvent *event);
	void resizeEvent(QResizeEvent *event);

protected slots:
	void close();

private:
	QList<QPointer<ContentsDialog> > m_dialogs;
	QWidget *m_layer;

signals:
	void requestedOpenUrl(QUrl url, bool privateWindow = false, bool background = false, bool newWindow = false);
	void requestedAddBookmark(QUrl url, QString title);
	void requestedNewWindow(ContentsWidget *widget);
	void requestedSearch(QString query, QString search);
	void actionsChanged();
	void canZoomChanged(bool can);
	void statusMessageChanged(const QString &message, int timeout);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void loadingChanged(bool loading);
	void zoomChanged(int zoom);
};

}

#endif
