#include "parammodelbase.h"

ParamModelBase::ParamModelBase(QObject* parent, const std::string& name) :
    QAbstractTableModel(parent), _name(name)
{
}
ParamModelBase::~ParamModelBase()
{
}

std::string ParamModelBase::Expression(size_t i) const
{
    return ShortKey(i) + " = " + Value(i);
}
VecStr ParamModelBase::Expressions() const
{
    const size_t num_pars = _parameters.size();
    VecStr vs(num_pars);
    for (size_t i=0; i<num_pars; ++i)
        vs[i] = Expression(i);
    return vs;
}
std::string ParamModelBase::Key(size_t i) const
{
    return headerData(i, Qt::Vertical, Qt::DisplayRole).toString().toStdString();
}
VecStr ParamModelBase::Keys() const
{
    VecStr vs;
    const size_t num_pars = _parameters.size();
    for (size_t i=0; i<num_pars; ++i)
        vs.push_back(Key(i));
    return vs;
}
double ParamModelBase::Maximum(size_t idx) const
{
    return data(createIndex(idx,2),Qt::DisplayRole).toDouble();
}
double ParamModelBase::Minimum(size_t idx) const
{
    return data(createIndex(idx,1),Qt::DisplayRole).toDouble();
}
std::string ParamModelBase::ShortKey(size_t i) const
{
    return Key(i);
}
VecStr ParamModelBase::ShortKeys() const
{
    VecStr vs;
    const size_t num_pars = _parameters.size();
    for (size_t i=0; i<num_pars; ++i)
        vs.push_back(ShortKey(i));
    return vs;
}
const std::string& ParamModelBase::Value(const std::string& key) const
{
    return Value( KeyIndex(key) );
}
const std::string& ParamModelBase::Value(size_t i) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _parameters.at(i).value;
        //Don't go through data, for speed
//    return data(createIndex(i,0),Qt::DisplayRole).toString().toStdString();
}

void ParamModelBase::AddParameter(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    int row = (int)_parameters.size();
    QModelIndex row_index = createIndex(row, 0);
    insertRows(row, 1, QModelIndex());
    _parameters[row] = Param(key, value);
    emit dataChanged(row_index, row_index);
    emit headerDataChanged(Qt::Vertical, row, row);
}
void ParamModelBase::SetMaximum(size_t idx, double val)
{
    setData(createIndex(idx,2),val,Qt::EditRole);
}
void ParamModelBase::SetMinimum(size_t idx, double val)
{
    setData(createIndex(idx,1),val,Qt::EditRole);
}
void ParamModelBase::SetPar(const std::string& key, const std::string& value)
{
    SetPar( KeyIndex(key), value );
}
void ParamModelBase::SetPar(int i, const std::string& value)
{
    setData( createIndex(i,0), value.c_str(), Qt::EditRole );
}
void ParamModelBase::SetPar(int i, double value)
{
    setData( createIndex(i,0), value, Qt::EditRole );
}

int ParamModelBase::columnCount() const
{
    return columnCount( QModelIndex() );
}
int ParamModelBase::columnCount(const QModelIndex&) const
{
    return 3;
}
QVariant ParamModelBase::data(const QModelIndex &index, int role) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    QVariant value;
    switch (role)
    {
        case Qt::EditRole:
        case Qt::DisplayRole:
            switch (index.column())
            {
                case 0:
                    value = _parameters.at( index.row() ).value.c_str();
                    break;
                case 1:
                    value = _parameters.at( index.row() ).min.c_str();
                    break;
                case 2:
                    value = _parameters.at( index.row() ).max.c_str();
                    break;
            }
            break;
        default:
            break;
    }
    return value;
}
Qt::ItemFlags ParamModelBase::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}
QVariant ParamModelBase::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole) return QVariant();
    QVariant header;
    switch (orientation)
    {
        case Qt::Horizontal:
            switch (section)
            {
            case 0:
                header = "Value";
                break;
            case 1:
                header = "Min";
                break;
            case 2:
                header = "Max";
                break;
            }
            break;
        case Qt::Vertical:
            if (section>(int)_parameters.size())
                throw "ParamModelBase::headerData: Bad parameter index.";
            header = _parameters.at(section).key.c_str();
            break;
    }
    return header;
}

bool ParamModelBase::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row+count-1);
    std::vector<Param> new_rows(count);
    _parameters.insert(_parameters.begin()+row, new_rows.begin(), new_rows.end());
    endInsertRows();
    return true;
}
bool ParamModelBase::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row+count-1);
    _parameters.erase(_parameters.begin()+row, _parameters.begin()+row+count);
    endRemoveRows();
    return true;
}

int ParamModelBase::rowCount() const
{
    return rowCount(QModelIndex());
}
int ParamModelBase::rowCount(const QModelIndex&) const
{
    return _parameters.size();
}

bool ParamModelBase::setData(const QModelIndex &index, const QVariant &value, int role)
{
    _mutex.lock();
    std::string val = value.toString().toStdString();
    switch (role)
    {
        case Qt::EditRole:
        {
            if (index.row()>=rowCount()) throw "ParamModelBase::setData: Index out of bounds";
            switch (index.column())
            {
                case 0:
                    _parameters[ index.row() ].value = val;
                    break;
                case 1:
                    _parameters[ index.row() ].min = val;
                    break;
                case 2:
                    _parameters[ index.row() ].max = val;
                    break;
            }
            break;
        }
        default:
            return false;
            break;
    }
    _mutex.unlock();
    emit dataChanged(index, index);
    return true;
}

bool ParamModelBase::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    throw "ParamModelBase::setHeaderData: Can't change parameter name.";
}

int ParamModelBase::KeyIndex(const std::string& par_name) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = std::find_if(_parameters.cbegin(), _parameters.cend(), [=](const Param& par)
    {
        return par_name == par.key;
    });
    if (it == _parameters.cend()) return -1;
    return it - _parameters.cbegin();
}
int ParamModelBase::ShortKeyIndex(const std::string& par_name) const
{
    return KeyIndex(par_name);
}
