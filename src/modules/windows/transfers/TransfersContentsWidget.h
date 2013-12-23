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
	QAction* getAction(WindowAction action);
	QString getTitle() const;
	QString getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;

public slots:
	void triggerAction(WindowAction action, bool checked = false);

protected:
	void changeEvent(QEvent *event);
	TransferInformation* getTransfer(const QModelIndex &index);
	int findTransfer(TransferInformation *transfer) const;

protected slots:
	void triggerAction();
	void addTransfer(TransferInformation *transfer);
	void removeTransfer(TransferInformation *transfer);
	void removeTransfer();
	void updateTransfer(TransferInformation *transfer);
	void openTransfer(const QModelIndex &index = QModelIndex());
	void openTransferFolder(const QModelIndex &index = QModelIndex());
	void copyTransferInformation();
	void stopResumeTransfer();
	void startQuickTransfer();
	void clearFinishedTransfers();
	void showContextMenu(const QPoint &point);
	void updateActions();

private:
	QStandardItemModel *m_model;
	QHash<WindowAction, QAction*> m_actions;
	QHash<TransferInformation*, QQueue<qint64> > m_speeds;
	Ui::TransfersContentsWidget *m_ui;
};

}

#endif
