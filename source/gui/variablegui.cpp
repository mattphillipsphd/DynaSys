#include "variablegui.h"
#include "ui_variablegui.h"

VariableGui::VariableGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VariableGui)
{
    ui->setupUi(this);
}

VariableGui::~VariableGui()
{
    delete ui;
}
