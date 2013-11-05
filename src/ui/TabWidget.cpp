#include "TabWidget.h"
#include "ui_TabWidget.h"

namespace Otter {

TabWidget::TabWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::TabWidget)
{
	ui->setupUi(this);
}

TabWidget::~TabWidget()
{
	delete ui;
}

void TabWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

} // namespace Otter
