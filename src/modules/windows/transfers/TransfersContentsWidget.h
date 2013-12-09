#ifndef OTTER_TRANSFERSCONTENTSWIDGET_H
#define OTTER_TRANSFERSCONTENTSWIDGET_H

#include "../../../core/TransfersManager.h"
#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class TransfersContentsWidget;
}

class Window;

class TransfersContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	explicit TransfersContentsWidget(Window *window);
	~TransfersContentsWidget();

	void print(QPrinter *printer);
	ContentsWidget* clone(Window *window = NULL);
	QAction* getAction(WindowAction action);
	QUndoStack* getUndoStack();
	QString getTitle() const;
	QString getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail() const;
	HistoryInformation getHistory() const;
	int getZoom() const;
	bool canZoom() const;
	bool isClonable() const;
	bool isLoading() const;
	bool isPrivate() const;

public slots:
	void triggerAction(WindowAction action, bool checked = false);
	void setHistory(const HistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url);

protected:
	void changeEvent(QEvent *event);
	TransferInformation* getTransfer(const QModelIndex &index);
	int findTransfer(TransferInformation *transfer) const;

protected slots:
	void addTransfer(TransferInformation *transfer);
	void removeTransfer(TransferInformation *transfer);
	void updateTransfer(TransferInformation *transfer);
	void stopResumeTransfer();
	void startQuickTransfer();
	void updateActions();

private:
	QStandardItemModel *m_model;
	QHash<WindowAction, QAction*> m_actions;
	Ui::TransfersContentsWidget *m_ui;

signals:
	void requestedOpenUrl(QUrl url, bool privateWindow, bool background, bool newWindow);
	void requestedAddBookmark(QUrl url);
	void requestedNewWindow(ContentsWidget *widget);
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
