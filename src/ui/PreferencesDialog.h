#ifndef QOPER_PREFERENCESDIALOG_H
#define QOPER_PREFERENCESDIALOG_H

#include <QDialog>

namespace Qoper
{

namespace Ui
{
    class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = NULL);
    ~PreferencesDialog();

protected:
    void changeEvent(QEvent *event);

private:
    Ui::PreferencesDialog *m_ui;
};

}

#endif
