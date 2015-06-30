#include "usernullclinegui.h"
#include "ui_usernullclinegui.h"

UserNullclineGui::UserNullclineGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserNullclineGui), _modelMgr(ModelMgr::Instance())
{
    ui->setupUi(this);
    ui->btnValidate->setEnabled(false);
}

UserNullclineGui::~UserNullclineGui()
{
    delete ui;
}

size_t UserNullclineGui::NumValidNCs() const
{
    const size_t num_ncs = _modelMgr->Model(ds::NC)->NumPars();
    size_t num_valid_ncs = 0;
    for (size_t i=0; i<num_ncs; ++i)
        num_valid_ncs += (size_t)( !_modelMgr->Model(ds::NC)->Value(i).empty() );
    return num_valid_ncs;
}

QTableView* UserNullclineGui::Table()
{
    return ui->tblNullclines;
}

void UserNullclineGui::on_btnCancel_clicked()
{
    close();
}
void UserNullclineGui::on_btnValidate_clicked()
{
    close();
}
