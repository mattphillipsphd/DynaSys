#include "jacobianmodel.h"

const std::string JacobianModel::DELIM = ",";

JacobianModel::JacobianModel(QObject *parent, const std::string& name) :
    ParamModelBase(parent, name)
{

}

JacobianModel::~JacobianModel()
{

}

void JacobianModel::ProcessParamFileLine(const std::string& key, std::string rem)
{
    AddParameter(key, rem);
}
std::string JacobianModel::String() const
{
    std::string str;
    str += "#" + ds::Model( Id() ) + "\n";
    const size_t num_pars = NumPars();
    for (size_t i=0; i<num_pars; ++i)
        str += Key(i) + "\t" + Value(i) + "\n";
    str += "\n";
    return str;
}

int JacobianModel::columnCount() const
{
    return columnCount(QModelIndex());
}
int JacobianModel::columnCount(const QModelIndex&) const
{
    return rowCount();
}
QVariant JacobianModel::data(const QModelIndex &index, int role) const
{
    QVariant value;
    switch (role)
    {
        case Qt::EditRole:
        case Qt::DisplayRole:
        {
            if (index.row()>=rowCount() || index.row()>=columnCount())
                throw std::runtime_error("NumericModelBase::data: Index out of bounds");

            VecStr gradvec = ds::Split( Parameter(index.row())->value, DELIM );
            value = gradvec.at(index.column()).c_str();
            break;
        }
        default:
            break;
    }
    return value;
}
bool JacobianModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    std::string val = value.toString().toStdString();
    switch (role)
    {
        case Qt::EditRole:
        {
            if (index.row()>=rowCount() || index.row()>=columnCount())
                throw std::runtime_error("NumericModelBase::setData: Index out of bounds");

            VecStr gradvec = ds::Split( Parameter(index.row())->value, DELIM );
            gradvec[index.column()] = val;
            Parameter(index.row())->value = ds::Join(gradvec, DELIM);
            break;
        }
        default:
            return false;
            break;
    }
    emit dataChanged(index, index);
    return true;
}
