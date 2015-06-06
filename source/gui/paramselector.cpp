#include "paramselector.h"
#include "ui_paramselector.h"

ParamSelector::ParamSelector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParamSelector), _log(Log::Instance()), _modelMgr(ModelMgr::Instance()), _doNotUpdate(false)
{
    ui->setupUi(this);
}

ParamSelector::~ParamSelector()
{
    delete ui;
}

void ParamSelector::LoadData(int index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::LoadData", std::this_thread::get_id());
#endif
    const size_t num_pvs = _modelMgr->NumParVariants();
    if (num_pvs==0)
    {
        ui->cmbSelect->clear();
        ui->tblPars->setRowCount(0);
        ui->tblInputFiles->setRowCount(0);
        ui->txtNotes->clear();
        return;
    }
    _doNotUpdate = true;
    ui->cmbSelect->clear();
    for (size_t i=0; i<num_pvs; ++i)
        ui->cmbSelect->addItem( _modelMgr->GetParVariant(i)->title.c_str() );
    ui->cmbSelect->setCurrentIndex(index);
    _doNotUpdate = false;
    UpdateParVariant(index);
}

void ParamSelector::on_btnDelete_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::on_btnDelete_clicked", std::this_thread::get_id());
#endif
    int idx = ui->cmbSelect->currentIndex();
    if (idx==-1) return;
    _modelMgr->DeleteParVariant(idx);
    LoadData( std::min(idx, _modelMgr->NumParVariants()-1) );
}
void ParamSelector::on_btnNew_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::on_btnNew_clicked", std::this_thread::get_id());
#endif
    int idx = ui->cmbSelect->currentIndex();

    bool ok;
    std::string title = QInputDialog::getText(this, "Title of parameter variant",
                                         "Enter title (empty to select file):", QLineEdit::Normal,
                                         "", &ok).toStdString();
    if (!ok) return;
    ModelMgr::ParVariant* pv;
    if (title.empty())
    {
        std::string file_name = QFileDialog::getOpenFileName(nullptr,
                                                              "Select parameter variant file",
                                                              "",
                                                              "Parameter variant files (*.txt *.pvar)"
                                                             ).toStdString();
        if (file_name.empty()) return;
        pv = ReadPVFromFile(file_name);
    }
    else
        pv = ReadPVFromGui(title);

    _modelMgr->InsertParVariant(idx+1, pv);
    LoadData(idx+1);
}
void ParamSelector::on_btnSave_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::on_btnSave_clicked", std::this_thread::get_id());
#endif
    emit SaveParVariant();
}
void ParamSelector::on_btnSet_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::on_btnSet_clicked", std::this_thread::get_id());
#endif
    const int num_pars = ui->tblPars->rowCount();
    for (int i=0; i<num_pars; ++i)
    {
        std::string key = ui->tblPars->verticalHeaderItem(i)->text().toStdString();
        int idx = _modelMgr->Model(ds::INP)->KeyIndex(key);
        if (idx==-1)
        {
            _log->AddMesg("Warning: parameter " + key + " does not exist, not updated.");
            continue;
        }
        std::string value = ui->tblPars->item(i,0)->text().toStdString();
        _modelMgr->SetValue(ds::INP, idx, value);
    }

    const int num_input_files = ui->tblInputFiles->rowCount();
    for (int i=0; i<num_input_files; ++i)
    {
        std::string key = ui->tblInputFiles->verticalHeaderItem(i)->text().toStdString();
        int idx = _modelMgr->Model(ds::VAR)->KeyIndex(key);
        if (idx==-1)
        {
            _log->AddMesg("Warning: input file parameter " + key + " does not exist, not updated.");
            continue;
        }
        std::string value = ui->tblInputFiles->item(i,0)->text().toStdString();
        _modelMgr->SetValue(ds::VAR, idx, value);
    }
}

void ParamSelector::on_cmbSelect_currentIndexChanged(int index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::on_cmbSelect_currentIndexChanged", std::this_thread::get_id());
#endif
    if (index==-1 || _doNotUpdate) return;
    UpdateParVariant(index);
    LoadData(index);
}

ModelMgr::ParVariant* ParamSelector::ReadPVFromFile(const std::string& file_name) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::ReadPVFromFile", std::this_thread::get_id());
#endif
    std::ifstream in;
    in.open(file_name);

    ModelMgr::ParVariant* pv = SysFileIn::ReadParVariant(in);

    in.close();

    return pv;
}

ModelMgr::ParVariant* ParamSelector::ReadPVFromGui(const std::string& title) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::ReadPVFromGui", std::this_thread::get_id());
#endif
    ModelMgr::ParVariant* pv = new ModelMgr::ParVariant(title);

    const size_t num_pars = _modelMgr->Model(ds::INP)->NumPars();
    for (size_t j=0; j<num_pars; ++j)
    {
        std::string key = _modelMgr->Model(ds::INP)->ShortKey(j),
                val = _modelMgr->Model(ds::INP)->Value(j);
        pv->pars.push_back( PairStr(key, val) );
    }

    const size_t num_input_files = _modelMgr->Model(ds::VAR)->NumPars();
    for (size_t j=0; j<num_input_files; ++j)
    {
        std::string key = _modelMgr->Model(ds::VAR)->ShortKey(j),
                val = _modelMgr->Model(ds::VAR)->Value(j);
        if (val.at(0) == '"')
            pv->input_files.push_back( PairStr(key, val) );
    }

    pv->notes =  QDateTime::currentDateTime().toString().toStdString();

    return pv;
}

void ParamSelector::UpdateParVariant(int i)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::UpdateParVariant", std::this_thread::get_id());
#endif
    ui->tblPars->clear();
    ui->tblInputFiles->clear();

    const ModelMgr::ParVariant* pv = _modelMgr->GetParVariant(i);
    const size_t num_pars = pv->pars.size();
    ui->tblPars->setRowCount(num_pars);
    ui->tblPars->setColumnCount(1);
    ui->tblPars->setHorizontalHeaderItem(0, new QTableWidgetItem("Parameter Value"));
    ui->tblPars->horizontalHeader()->setStretchLastSection(true);
    for (size_t i=0; i<num_pars; ++i)
    {
        ui->tblPars->setVerticalHeaderItem(i,
                     new QTableWidgetItem( QString(pv->pars.at(i).first.c_str()) ));
        ui->tblPars->setItem(i, 0,
                     new QTableWidgetItem( QString(pv->pars.at(i).second.c_str()) ));
    }

    const size_t num_input_files = pv->input_files.size();
    ui->tblInputFiles->setRowCount(num_input_files);
    ui->tblInputFiles->setColumnCount(1);
    ui->tblInputFiles->setHorizontalHeaderItem(0, new QTableWidgetItem("Input Files"));
    ui->tblInputFiles->horizontalHeader()->setStretchLastSection(true);
    for (size_t i=0; i<num_input_files; ++i)
    {
        ui->tblInputFiles->setVerticalHeaderItem(i,
                           new QTableWidgetItem( QString(pv->input_files.at(i).first.c_str()) ));
        ui->tblInputFiles->setItem(i, 0,
                           new QTableWidgetItem( QString(pv->input_files.at(i).second.c_str()) ));
    }

    ui->txtNotes->setPlainText( pv->notes.c_str() );
}

void ParamSelector::on_tblPars_cellChanged(int row, int column)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::on_tblPars_cellChanged", std::this_thread::get_id());
#endif
    const size_t index = ui->cmbSelect->currentIndex();
    _modelMgr->SetPVPar(index, row, ui->tblPars->item(row,column)->text().toStdString() );
}

void ParamSelector::on_tblInputFiles_cellChanged(int row, int column)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::on_tblInputFiles_cellChanged", std::this_thread::get_id());
#endif
    const size_t index = ui->cmbSelect->currentIndex();
    _modelMgr->SetPVInputFile(index, row, ui->tblInputFiles->item(row,column)->text().toStdString() );
}

void ParamSelector::on_txtNotes_textChanged()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamSelector::on_txtNotes_textChanged", std::this_thread::get_id());
#endif
    const int index = ui->cmbSelect->currentIndex();
    if (index == -1) return;
    _modelMgr->SetPVNotes( index, ui->txtNotes->document()->toPlainText().toStdString() );
}
