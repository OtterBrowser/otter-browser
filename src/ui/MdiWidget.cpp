#include "MdiWidget.h"

#include <QtCore/QEvent>

namespace Otter
{

MdiWidget::MdiWidget(QWidget *parent) : QMdiArea(parent)
{
}

bool MdiWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
	{
		 return QAbstractScrollArea::eventFilter(object, event);
	}

	return QMdiArea::eventFilter(object, event);
}

}
