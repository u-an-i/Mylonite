#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QComboBox>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QWidget* getCentralWidget();
    QPushButton* getPushButtonForURL();
    QComboBox* getURL();
    QComboBox* getType();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
