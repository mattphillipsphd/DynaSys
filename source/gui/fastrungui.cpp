#include "fastrungui.h"
#include "ui_fastrungui.h"

const int FastRunGui::DEFAULT_DURATION = 100;
const int FastRunGui::DEFAULT_MODN = 1000;

FastRunGui::FastRunGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FastRunGui), _fileName(ds::TEMP_DAT_FILE), _method(UNKNOWN)
{
    ui->setupUi(this);
    ui->edDuration->setText(QString("%1").arg(DEFAULT_DURATION));
    ui->edSaveModN->setText(QString("%1").arg(DEFAULT_MODN));
    ui->edFullFileName->setText(FullFileName().c_str());
    setWindowTitle("Run Simulation Offline");
}

FastRunGui::~FastRunGui()
{
    delete ui;
}

std::string FastRunGui::FullFileName() const
{
    return DDM::SaveDataDir().empty()
            ? _fileName
            : DDM::SaveDataDir() + "/" + _fileName;
}

void FastRunGui::UpdatePBar(int n)
{
    if (n==-1)
    {
        emit Finished();
        close();
    }
    else
        ui->prgSimulation->setValue(n);
}

void FastRunGui::closeEvent(QCloseEvent*)
{
    emit Finished();
}
void FastRunGui::showEvent(QShowEvent*)
{
    ui->btnStartFast->setText("Start");
    switch (_method)
    {
        case UNKNOWN:
            throw std::runtime_error("FastRunGui::showEvent: Bad Method");
        case FAST_RUN:
            ui->prgSimulation->show();
            ui->prgSimulation->setValue(0);
            if (_fileName==ds::TEMP_DAT_FILE)
            {
                _fileName = ds::TEMP_FILE;
                ui->edFullFileName->setText(FullFileName().c_str());
            }
            break;
        case COMPILED:
            ui->prgSimulation->hide();
            if (_fileName==ds::TEMP_FILE)
            {
                _fileName = ds::TEMP_DAT_FILE;
                ui->edFullFileName->setText(FullFileName().c_str());
            }
            break;
    }
}

void FastRunGui::on_btnFileName_clicked()
{
    std::string full_file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Save generated data",
                                                         DDM::SaveDataDir().c_str()).toStdString();
    if (full_file_name.empty()) return;
    size_t sep = full_file_name.find_last_of('/');
    std::string path = (sep==std::string::npos) ? "" : full_file_name.substr(0, sep);
    _fileName = ds::StripPath(full_file_name);
    DDM::SetSaveDataDir(path);
    size_t pos = _fileName.find_last_of('.');
    switch (_method)
    {
        case UNKNOWN:
            throw std::runtime_error("FastRunGui::on_btnFileName_clicked: Bad method");
        case FAST_RUN:
            if (pos==std::string::npos || _fileName.substr(pos) != ".txt")
                _fileName += ".txt";
            break;
        case COMPILED:
            if (pos==std::string::npos || _fileName.substr(pos) != ".dsdat")
                _fileName += ".dsdat";
            break;
    }
    ui->edFullFileName->setText(_fileName.c_str());
}
void FastRunGui::on_btnStartFast_clicked()
{
    if (ui->btnStartFast->text()=="Start")
    {
        const int duration = ui->edDuration->text().toInt(),
                save_mod_n = ui->edSaveModN->text().toInt();
        switch (_method)
        {
            case UNKNOWN:
                throw std::runtime_error("FastRunGui::on_btnStartFast_clicked: Bad Method");
            case FAST_RUN:
                ui->prgSimulation->setMinimum(0);
                ui->prgSimulation->setMaximum(duration);
                emit StartFastRun(duration, save_mod_n);
                ui->btnStartFast->setText("Cancel");
                break;
            case COMPILED:
                emit StartCompiled(duration, save_mod_n);
                close();
                break;
        }
    }
    else
    {
        emit Finished();
        close();
    }
}
void FastRunGui::on_edFullFileName_editingFinished()
{
    std::string full_file_name = ui->edFullFileName->text().toStdString();
    size_t sep = full_file_name.find_last_of('/');
    std::string path = (sep==std::string::npos) ? "" : full_file_name.substr(0, sep);

    QFile qf(path.c_str());
    if (!path.empty() && !qf.exists())
    {
        ui->edFullFileName->setText( FullFileName().c_str() );
        return;
    }
    _fileName = ds::StripPath(full_file_name);
    DDM::SetSaveDataDir(path);
}
