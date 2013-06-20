#ifndef QOPER_COKIESDIALOG_H
#define QOPER_COKIESDIALOG_H

#include <QDialog>

namespace Qoper
{

namespace Ui
{
    class CokiesDialog;
}

class CokiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CokiesDialog(QWidget *parent = NULL);
    ~CokiesDialog();

protected:
    void changeEvent(QEvent *event);

private:
    Ui::CokiesDialog *m_ui;
};

}

#endif
