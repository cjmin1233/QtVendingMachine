#include "vendingmachine.h"
#include "ui_vendingmachine.h"

VendingMachine::VendingMachine(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::VendingMachine)
{
    ui->setupUi(this);

    // load price db
    // update price info to product buttons

    // connect button clicked signals to Calculating slots


}

VendingMachine::~VendingMachine()
{
    delete ui;
}
