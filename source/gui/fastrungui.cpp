#include "fastrungui.h"
#include "ui_fastrungui.h"

const int FastRunGui::DEFAULT_DURATION = 1000;
const int FastRunGui::DEFAULT_MODN = 1000;

FastRunGui::FastRunGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FastRunGui)
{
    ui->setupUi(this);
    ui->edDuration->setText(QString("%1").arg(DEFAULT_DURATION));
    ui->edSaveModN->setText(QString("%1").arg(DEFAULT_MODN));
    setWindowTitle("Run Simulation Offline");
}

FastRunGui::~FastRunGui()
{
    delete ui;
}

void FastRunGui::UpdatePBar(int n)
{
    if (n==-1)
    {
        emit Finished(true);
        close();
    }
    else
        ui->prgSimulation->setValue(n);
}

void FastRunGui::closeEvent(QCloseEvent*)
{
    emit Finished(false);
}
void FastRunGui::showEvent(QShowEvent*)
{
    ui->btnStartFast->setText("Start");
    ui->prgSimulation->setValue(0);
}

void FastRunGui::on_btnStartFast_clicked()
{
    if (ui->btnStartFast->text()=="Start")
    {
        const int duration = ui->edDuration->text().toInt(),
                save_mod_n = ui->edSaveModN->text().toInt();
        ui->prgSimulation->setMinimum(0);
        ui->prgSimulation->setMaximum(duration);
        emit Start(duration, save_mod_n);
        ui->btnStartFast->setText("Cancel");
    }
    else
    {
        emit Finished(false);
        close();
    }
}
