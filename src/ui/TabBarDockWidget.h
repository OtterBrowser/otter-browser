#ifndef OTTER_TABBARDOCKWIDGET_H
#define OTTER_TABBARDOCKWIDGET_H

#include <QtWidgets/QDockWidget>
#include <QtWidgets/QToolButton>

namespace Otter
{

class TabBarWidget;

class TabBarDockWidget : public QDockWidget
{
	Q_OBJECT

public:
	explicit TabBarDockWidget(QWidget *parent = NULL);

	void setup(QAction *closedWindowsAction);
	TabBarWidget* getTabBar();

protected slots:
	void moveNewTabButton(int position);

private:
	TabBarWidget *m_tabBar;
	QToolButton *m_newTabButton;
};

}

#endif
