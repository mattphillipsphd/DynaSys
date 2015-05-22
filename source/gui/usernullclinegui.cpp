#include "usernullclinegui.h"
#include "ui_usernullclinegui.h"

UserNullclineGui::UserNullclineGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserNullclineGui), _modelMgr(ModelMgr::Instance())
{
    ui->setupUi(this);

    // #############
    ui->edV->setText( "k*(V - V_thresh)*(V - V_rest) - b*(V - V_rest) + I_bias" );
    ui->edU->setText( "b*(V - V_rest)" );
}

UserNullclineGui::~UserNullclineGui()
{
    delete ui;
}

size_t UserNullclineGui::NumNCs() const
{
    return _nullclines.size();
}

void UserNullclineGui::on_btnValidate_clicked()
{
    _nullclines.clear();
    _nullclines.push_back( ui->edV->text().toStdString() );
    _nullclines.push_back( ui->edU->text().toStdString() );

    _modelMgr->ClearParameters(ds::NC);
    size_t i=0;
    for (auto it : _nullclines)
    {
        std::string dvar = _modelMgr->Model(ds::DIFF)->ShortKey(i++);
        _modelMgr->AddParameter(ds::NC, dvar, it);
    }
    close();
}
