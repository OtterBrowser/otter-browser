#ifndef QOPER_MAINWINDOW_H
#define QOPER_MAINWINDOW_H

#include <QMainWindow>

namespace Ui
{
    class MainWindow;
}

namespace Qoper
{

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = NULL);
    ~MainWindow();

protected:
    void changeEvent(QEvent *event);

private:
    Ui::MainWindow *m_ui;
};

}

#endif
