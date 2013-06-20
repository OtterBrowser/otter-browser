#include "CokiesDialog.h"

#include "ui_CokiesDialog.h"

namespace Qoper
{

CokiesDialog::CokiesDialog(QWidget *parent) : QDialog(parent),
    m_ui(new Ui::CokiesDialog)
{
    m_ui->setupUi(this);
}

CokiesDialog::~CokiesDialog()
{
    delete m_ui;
}

void CokiesDialog::changeEvent(QEvent *event)
{
    QDialog::changeEvent(event);

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
