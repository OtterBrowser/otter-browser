#ifndef QOPER_MAINWINDOW_H
#define QOPER_MAINWINDOW_H

#include <QMainWindow>

namespace Otter
{

namespace Ui
{
    class MainWindow;
}

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
