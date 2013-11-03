#include "MainWindow.h"

#include "ui_MainWindow.h"

namespace Otter
{

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

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
