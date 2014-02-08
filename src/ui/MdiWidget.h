#ifndef OTTER_MDIWIDGET_H
#define OTTER_MDIWIDGET_H

#include <QtWidgets/QMdiArea>

namespace Otter
{

class MdiWidget : public QMdiArea
{
	Q_OBJECT

public:
	explicit MdiWidget(QWidget *parent = NULL);

	bool eventFilter(QObject *object, QEvent *event);
};

}

#endif
