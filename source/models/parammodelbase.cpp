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
const std::string& ParamModelBase::Key(size_t i) const
{
    return _parameters.at(i).first;
}
VecStr ParamModelBase::Keys() const
{
    VecStr vs;
    const size_t num_pars = _parameters.size();
    for (size_t i=0; i<num_pars; ++i)
        vs.push_back(Key(i));
    return vs;
}
std::string ParamModelBase::ShortKey(size_t i) const
{
    return Key(i);
}
const std::string& ParamModelBase::Value(const std::string& key) const
{
    return Value( Index(key) );
}
const std::string& ParamModelBase::Value(size_t i) const
{
    return _parameters.at(i).second;
}

void ParamModelBase::AddParameter(const std::string& key, const std::string& value)
{
    int row = _parameters.size();
    QModelIndex row_index = createIndex(row, 0);
    insertRows(row, 1, QModelIndex());
    _parameters[row] = StrPair(key, value);
    emit dataChanged(row_index, row_index);
    emit headerDataChanged(Qt::Vertical, row, row);
}
void ParamModelBase::SetPar(const std::string& key, const std::string& value)
{
    SetPar( Index(key), value );
}
void ParamModelBase::SetPar(int i, const std::string& value)
{
    setData( createIndex(i,0), value.c_str(), Qt::EditRole );
}

int ParamModelBase::columnCount() const
{
    return columnCount( QModelIndex() );
}
int ParamModelBase::columnCount(const QModelIndex&) const
{
    return 1;
}
QVariant ParamModelBase::data(const QModelIndex &index, int role) const
{
    QVariant value;
    switch (role)
    {
        case Qt::EditRole:
        case Qt::DisplayRole:
            value = _parameters.at( index.row() ).second.c_str();
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
            header = "Value";
            break;
        case Qt::Vertical:
            if (section>(int)_parameters.size())
                throw "ParamModelBase::headerData: Bad parameter index.";
            header = _parameters.at(section).first.c_str();
            break;
    }
    return header;
}

bool ParamModelBase::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row+count-1);
    std::vector<StrPair> new_rows(count);
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
    switch (role)
    {
        case Qt::EditRole:
        {
            if (index.row()>=rowCount()) throw "ParamModelBase::setData: Index out of bounds";
            _parameters[ index.row() ].second = value.toString().toStdString();
            break;
        }
        default:
            return false;
            break;
    }
    emit dataChanged(index, index);
    return true;
}

bool ParamModelBase::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    throw "ParamModelBase::setHeaderData: Can't change parameter name.";
}

int ParamModelBase::Index(const std::string& par_name) const
{
    auto it = std::find_if(_parameters.cbegin(), _parameters.cend(), [=](const StrPair& par)
    {
        return par_name == par.first;
    });
    if (it == _parameters.cend()) throw "ParamModelBase::Index";
    return it - _parameters.cbegin();
}
