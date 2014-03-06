#include "aboutgui.h"
#include "ui_aboutgui.h"

AboutGui::AboutGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AboutGui)
{
    ui->setupUi(this);

    setWindowTitle("About DynaSys");

    ui->txtAbout->setText(std::string(
                "DynaSys " + ds::VERSION_STR + "\n\n"
                "Matt Phillips, mattphillipsphd@gmail.com\n\n"
                "Created with Qt, Qwt, and Ingo Berg's muParser."
                ).c_str());
}

AboutGui::~AboutGui()
{
    delete ui;
}
