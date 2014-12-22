#include "paramselector.h"
#include "ui_paramselector.h"

ParamSelector::ParamSelector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParamSelector)
{
    ui->setupUi(this);
}

ParamSelector::~ParamSelector()
{
    delete ui;
}

void ParamSelector::on_btnDelete_clicked()
{

}
void ParamSelector::on_btnNew_clicked()
{

}
