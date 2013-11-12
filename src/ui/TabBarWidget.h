#ifndef OTTER_TABBARWIDGET_H
#define OTTER_TABBARWIDGET_H

#include <QtWidgets/QTabBar>

namespace Otter
{

class TabBarWidget : public QTabBar
{
	Q_OBJECT

public:
	explicit TabBarWidget(QWidget *parent = NULL);

protected:
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void tabInserted(int index);
	void tabRemoved(int index);

protected slots:
	void closeOther();
	void tabWidthChanged();

private:
	int m_clickedTab;

signals:
	void requestedClose(int tab);
	void requestedCloseOther(int tab);
};

}

#endif
