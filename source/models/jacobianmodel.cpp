#include "jacobianmodel.h"

const std::string JacobianModel::DELIM = ",";

JacobianModel::JacobianModel(QObject *parent, const std::string& name) :
    ParamModelBase(parent, name)
{
}

JacobianModel::~JacobianModel()
{
}

std::string JacobianModel::Key(size_t i) const
{
    return Parameter((size_t)i)->key;
}
std::string JacobianModel::ParamString(size_t i) const
{
    const int num_pars = rowCount();
    std::string key = "grad("
            + ModelMgr::Instance()->Model(ds::DIFF)->ShortKey(i) + ")";
    std::string str;
    for (size_t j=0; j<num_pars-1; ++j)
        str += key + "\t" + Value(i*num_pars+j) + ",";
    str += Value((i+1)*num_pars-1) + "\n";
    return str;
}
void JacobianModel::ProcessParamFileLine(const std::string& key, std::string rem)
{
    VecStr gradvec = ds::Split(rem, DELIM);
    const size_t num_eqns = gradvec.size();
    size_t pos0 = key.find_first_of('('),
            pos1 = key.find_last_of(')');
    std::string diff = key.substr(pos0+1, (pos1-pos0)-1);
    for (size_t i=0; i<num_eqns; ++i)
    {
        std::string key_i = "_grad_" + diff + "_" + std::to_string(i);
        AddParameter(key_i, gradvec.at(i));
    }
}
std::string JacobianModel::String() const
{
    std::string str;
    str += "#" + ds::Model( Id() ) + "\n";
    const size_t num_pars = sqrt(NumPars());
    int ct = 0;
    for (size_t i=0; i<num_pars; ++i)
    {
        std::string key = "grad("
                + ModelMgr::Instance()->Model(ds::DIFF)->ShortKey(i) + ")";
        for (size_t j=0; j<num_pars-1; ++j)
            str += key + "\t" + Value(ct++) + ",";
        str += Value(ct) + "\n";
    }
    str += "\n";
    return str;
}
std::string JacobianModel::TempExpression(size_t i) const
{
    return Value(i).empty() ? "" : TempKey(i) + " = " + Value(i);
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
            value = Value(index.row()*columnCount() + index.column()).c_str();
            break;
        default:
            break;
    }
    return value;
}

QVariant JacobianModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole) return QVariant();
    QVariant header;
    switch (orientation)
    {
        case Qt::Horizontal:
            header = ("d" + ModelMgr::Instance()->Model(ds::DIFF)->ShortKey(section)).c_str();
            break;
        case Qt::Vertical:
            header = ("grad("
                      + ModelMgr::Instance()->Model(ds::DIFF)->ShortKey(section) + ")").c_str();
            break;
    }
    return header;
}

int JacobianModel::rowCount() const
{
    return rowCount( QModelIndex() );
}

int JacobianModel::rowCount(const QModelIndex&) const
{
    return sqrt( NumPars() );
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

            int idx = index.row()*columnCount() + index.column();
            Parameter(idx)->value = val;
            break;
        }
        default:
            return false;
            break;
    }
    emit dataChanged(index, index);
    return true;
}
