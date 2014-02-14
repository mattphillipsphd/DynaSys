#include "aboutgui.h"
#include "ui_aboutgui.h"

AboutGui::AboutGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AboutGui)
{
    ui->setupUi(this);

    setWindowTitle("About DynaSys");

    ui->txtAbout->setText(
                "DynaSys 0.0.1\n\n"
                "Matt Phillips, mattphillipsphd@gmail.com\n\n"
                "Created with Qt and Ingo Borg's muParser."
                );
}

AboutGui::~AboutGui()
{
    delete ui;
}
