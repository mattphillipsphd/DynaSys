#include "aboutgui.h"
#include "ui_aboutgui.h"

AboutGui::AboutGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AboutGui)
{
    ui->setupUi(this);

    setWindowTitle("About DynaSys");

    ui->txtAbout->setReadOnly(true);
    ui->txtAbout->setText(std::string(
                "DynaSys " + ds::VERSION_STR + "\n"
                "Build date: " + std::string(__DATE__) + "\n\n"
                "Matt Phillips, mattphillipsphd@gmail.com\n\n"
                "Created with Qt, Qwt, and Ingo Berg's muParser.\n\n"
                "DynaSys is licensed under the terms of the GPLv3, http://www.gnu.org/licenses/licenses.html#GPL."
                ).c_str());
}

AboutGui::~AboutGui()
{
    delete ui;
}
