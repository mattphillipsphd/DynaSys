#include "jacobiangui.h"
#include "ui_jacobiangui.h"

JacobianGui::JacobianGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::JacobianGui), _modelMgr(ModelMgr::Instance())
{
    ui->setupUi(this);
}

JacobianGui::~JacobianGui()
{
    delete ui;
}

bool JacobianGui::IsFull() const
{
    bool is_full = true;
    const int size = ui->tblJacobian->model()->rowCount();
    for (int i=0; i<size; ++i)
        for (int j=0; j<size; ++j)
        {
            QModelIndex idx = ui->tblJacobian->indexAt(QPoint(i,j));
            if ( ui->tblJacobian->model()->data(idx).toString().isEmpty() )
            {
                is_full = false;
                break;
            }
        }
    return is_full;
}

QTableView* JacobianGui::Table()
{
    return ui->tblJacobian;
}

void JacobianGui::on_btnCancel_clicked()
{
    close();
}
void JacobianGui::on_btnValidate_clicked()
{
    close();
}
