#include "CookiesDialog.h"

#include "ui_CookiesDialog.h"

namespace Qoper
{

CookiesDialog::CookiesDialog(QWidget *parent) : QDialog(parent),
    m_ui(new Ui::CookiesDialog)
{
    m_ui->setupUi(this);
}

CookiesDialog::~CookiesDialog()
{
    delete m_ui;
}

void CookiesDialog::changeEvent(QEvent *event)
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
