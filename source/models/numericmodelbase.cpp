#include "numericmodelbase.h"

const std::string NumericModelBase::NumParam::DEFAULT_MAX = "100";
const std::string NumericModelBase::NumParam::DEFAULT_MIN = "-100";

NumericModelBase::NumericModelBase(QObject* parent, const std::string& name)
    : ParamModelBase(parent, name)
{
}
NumericModelBase::~NumericModelBase()
{
}

void NumericModelBase::ProcessParamFileLine(const std::string& key, std::string rem)
{
    std::string pmin(NumParam::DEFAULT_MIN), pmax(NumParam::DEFAULT_MAX), value(rem);
    size_t tab = rem.find_first_of('\t');
    if (tab!=std::string::npos)
    {
        value = rem.substr(0,tab);
        rem.erase(0,tab+1);
        tab = rem.find_first_of('\t');
        pmin = rem.substr(0,tab);
        pmax = rem.substr(tab+1);
    }

    AddParameter(key, value);
    const size_t j = KeyIndex(key);
    SetMinimum(j, std::stod(pmin));
    SetMaximum(j, std::stod(pmax));
}

void NumericModelBase::AddParameter(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(_nmutex);
    int row = (int)NumPars();
    QModelIndex row_index = createIndex(row, 0);
    insertRows(row, 1, QModelIndex());
    SetParameter(row, new NumParam(key, value));
    emit dataChanged(row_index, row_index);
    emit headerDataChanged(Qt::Vertical, row, row);
}
void NumericModelBase::SetMaximum(size_t idx, double val)
{
    setData(createIndex((int)idx,MAX),val,Qt::EditRole);
}
void NumericModelBase::SetMinimum(size_t idx, double val)
{
    setData(createIndex((int)idx,MIN),val,Qt::EditRole);
}
void NumericModelBase::SetRange(size_t idx, double min, double max)
{
    SetMinimum(idx, min);
    SetMaximum(idx, max);
}

double NumericModelBase::Maximum(size_t idx) const
{
    return data(createIndex((int)idx,MAX),Qt::DisplayRole).toDouble();
}
double NumericModelBase::Minimum(size_t idx) const
{
    return data(createIndex((int)idx,MIN),Qt::DisplayRole).toDouble();
}
std::string NumericModelBase::String() const
{
    std::string str;
    str += "#" + ds::Model( Id() ) + "\n";
    const size_t num_pars = NumPars();
    for (size_t i=0; i<num_pars; ++i)
        str += Key(i) + "\t" + Value(i) + "\t"
                + std::to_string(Minimum(i)) + "\t" + std::to_string(Maximum(i)) + "\n";
    str += "\n";
    return str;
}

int NumericModelBase::columnCount(const QModelIndex&) const
{
    return NUM_NCOLUMNS;
}
QVariant NumericModelBase::data(const QModelIndex &index, int role) const
{
    std::lock_guard<std::mutex> lock(_nmutex);
    QVariant value;
    switch (role)
    {
        case Qt::CheckStateRole:
            value = ParamModelBase::data(index, role);
            break;
        case Qt::EditRole:
        case Qt::DisplayRole:
            switch (index.column())
            {
                case FREEZE:
                case VALUE:
                    value = ParamModelBase::data(index, role);
                    break;
                case MIN:
                    value = Parameter( index.row() )->min.c_str();
                    break;
                case MAX:
                    value = Parameter( index.row() )->max.c_str();
                    break;
            }
            break;
        default:
            break;
    }
    return value;
}
QVariant NumericModelBase::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole) return QVariant();
    QVariant header;
    switch (orientation)
    {
        case Qt::Horizontal:
            switch (section)
            {
                case FREEZE:
                case VALUE:
                    header = ParamModelBase::headerData(section, orientation, role);
                    break;
                case MIN:
                    header = "Min";
                    break;
                case MAX:
                    header = "Max";
                    break;
            }
            break;
        case Qt::Vertical:
            if (section>(int)NumPars())
                throw std::runtime_error("NumericModelBase::headerData: Bad parameter index.");
            header = Parameter(section)->key.c_str();
            break;
    }
    return header;
}

bool NumericModelBase::setData(const QModelIndex &index, const QVariant &value, int role)
{
    _nmutex.lock();
    std::string val = value.toString().toStdString();
    switch (role)
    {
        case Qt::CheckStateRole:
        case Qt::EditRole:
        {
            if (index.row()>=rowCount())
                throw std::runtime_error("NumericModelBase::setData: Index out of bounds");
            switch (index.column())
            {
                case FREEZE:
                case VALUE:
                    ParamModelBase::setData(index, value, role);
                    _nmutex.unlock();
                    return true;
                    break;
                case MIN:
                    Parameter( index.row() )->min = val;
                    break;
                case MAX:
                    Parameter( index.row() )->max = val;
                    break;
            }
            break;
        }
        default:
            return false;
            break;
    }
    _nmutex.unlock();
    emit dataChanged(index, index);
    return true;
}
