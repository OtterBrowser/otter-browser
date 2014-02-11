#ifndef OTTER_MDIWIDGET_H
#define OTTER_MDIWIDGET_H

#include <QtCore/QPointer>
#include <QtWidgets/QMdiArea>

namespace Otter
{

class Window;

class MdiWidget : public QWidget
{
	Q_OBJECT

public:
	explicit MdiWidget(QWidget *parent = NULL);

	void addWindow(Window *window);
	void setActiveWindow(Window *window);
	Window* getActiveWindow();

protected:
	void resizeEvent(QResizeEvent *event);

private:
	QPointer<Window> m_activeWindow;
};

}

#endif
