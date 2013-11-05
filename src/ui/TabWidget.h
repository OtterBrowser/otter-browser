#ifndef OTTER_TABWIDGET_H
#define OTTER_TABWIDGET_H

#include <QWidget>

namespace Otter {

namespace Ui {
class TabWidget;
}

class TabWidget : public QWidget
{
	Q_OBJECT

public:
	explicit TabWidget(QWidget *parent = 0);
	~TabWidget();

protected:
	void changeEvent(QEvent *e);

private:
	Ui::TabWidget *ui;
};


} // namespace Otter
#endif // OTTER_TABWIDGET_H
