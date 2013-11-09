#include "TabWidget.h"
#include "ui_TabWidget.h"

namespace Otter
{

TabWidget::TabWidget(QWidget *parent) : QWidget(parent),
	m_ui(new Ui::TabWidget)
{
	m_ui->setupUi(this);
}

TabWidget::~TabWidget()
{
	delete m_ui;
}

void TabWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	switch (event->type())
	{
	case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

}
