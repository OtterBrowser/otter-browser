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
	explicit WebContentsWidget(bool privateWindow, WebWidget *widget, Window *window);
	~WebContentsWidget();

	void search(const QString &search, const QString &query);
	void print(QPrinter *printer);
	WebContentsWidget* clone(Window *parent = NULL);
	QAction* getAction(WindowAction action);
	QUndoStack* getUndoStack();
	QString getDefaultTextEncoding() const;
	QString getTitle() const;
	QString getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail() const;
	WindowHistoryInformation getHistory() const;
	int getZoom() const;
	bool canZoom() const;
	bool isClonable() const;
	bool isLoading() const;
	bool isPrivate() const;

public slots:
	void triggerAction(WindowAction action, bool checked = false);
	void setDefaultTextEncoding(const QString &encoding);
	void setHistory(const WindowHistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url);

protected:
	void changeEvent(QEvent *event);
	void resizeEvent(QResizeEvent *event);

protected slots:
	void notifyRequestedOpenUrl(const QUrl &url, bool background, bool newWindow);
	void notifyRequestedNewWindow(WebWidget *widget);
	void updateFind(bool backwards = false);
	void updateFindHighlight();
	void updateProgressBarWidget();
	void setLoading(bool loading);

private:
	WebWidget *m_webWidget;
	ProgressBarWidget *m_progressBarWidget;
	Ui::WebContentsWidget *m_ui;
};

}

#endif
