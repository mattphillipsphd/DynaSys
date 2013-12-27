#include "parammodel.h"

ParamModel::ParamModel(QObject* parent, const std::string& name) :
    QAbstractTableModel(parent), _name(name)
{
}
ParamModel::~ParamModel()
{
}

const std::string& ParamModel::Key(size_t i) const
{
    return _parameters.at(i).first;
}
std::string ParamModel::ShortKey(size_t i) const
{
    return Key(i);
}
const std::string& ParamModel::Value(const std::string& key) const
{
    return Value( Index(key) );
}
const std::string& ParamModel::Value(size_t i) const
{
    return _parameters.at(i).second;
}

void ParamModel::AddParameter(const std::string& key, const std::string& value)
{
    int row = _parameters.size();
    QModelIndex row_index = createIndex(row, 0);
    insertRows(row, 1, QModelIndex());
    _parameters[row] = StrPair(key, value);
    emit dataChanged(row_index, row_index);
    emit headerDataChanged(Qt::Vertical, row, row);
}
void ParamModel::SetPar(const std::string& key, const std::string& value)
{
    SetPar( Index(key), value );
}
void ParamModel::SetPar(int i, const std::string& value)
{
    setData( createIndex(i,0), value.c_str(), Qt::EditRole );
}

int ParamModel::columnCount() const
{
    return columnCount( QModelIndex() );
}
int ParamModel::columnCount(const QModelIndex&) const
{
    return 1;
}
QVariant ParamModel::data(const QModelIndex &index, int role) const
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
Qt::ItemFlags ParamModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}
QVariant ParamModel::headerData(int section, Qt::Orientation orientation, int role) const
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
                throw "ParamModel::headerData: Bad parameter index.";
            header = _parameters.at(section).first.c_str();
            break;
    }
    return header;
}

bool ParamModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row+count-1);
    std::vector<StrPair> new_rows(count);
    _parameters.insert(_parameters.begin()+row, new_rows.begin(), new_rows.end());
    endInsertRows();
    return true;
}
bool ParamModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row+count-1);
    _parameters.erase(_parameters.begin()+row, _parameters.begin()+row+count);
    endRemoveRows();
    return true;
}

int ParamModel::rowCount() const
{
    return rowCount(QModelIndex());
}
int ParamModel::rowCount(const QModelIndex&) const
{
    return _parameters.size();
}

bool ParamModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    switch (role)
    {
        case Qt::EditRole:
        {
            if (index.row()>=rowCount()) throw "ParamModel::setData: Index out of bounds";
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

bool ParamModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    throw "ParamModel::setHeaderData: Can't change parameter name.";
}

int ParamModel::Index(const std::string& par_name) const
{
    auto it = std::find_if(_parameters.cbegin(), _parameters.cend(), [=](const StrPair& par)
    {
        return par_name == par.first;
    });
    if (it == _parameters.cend()) throw "ParamModel::Index";
    return it - _parameters.cbegin();
}
