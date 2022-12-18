#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QWidget* MainWindow::getCentralWidget()
{
    return ui->centralwidget;
}


QPushButton* MainWindow::getPushButtonForURL()
{
    return ui->pushButton_URL;
}


QComboBox* MainWindow::getURL()
{
    return ui->comboBox_URL;
}


QComboBox* MainWindow::getType()
{
    return ui->comboBox_Types;
}
