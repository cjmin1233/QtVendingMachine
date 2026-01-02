#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_VendingMachine(new VendingMachine(this))
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
