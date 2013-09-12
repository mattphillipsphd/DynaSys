#include "conditionmodel.h"

ConditionModel::ConditionModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

void ConditionModel::AddCondition(const std::string& condition, const VecStr& exprns)
{
    int row = _conditions.size();
    _conditions.push_back( std::make_pair(condition, exprns) );
    emit dataChanged( createIndex(row,0), createIndex(row,0) );
}

const std::string& ConditionModel::Condition(int i) const
{
    return _conditions.at(i).first;
}
const VecStr& ConditionModel::Expressions(int i) const
{
    return _conditions.at(i).second;
}

int ConditionModel::columnCount() const
{
    return columnCount( QModelIndex() );
}
int ConditionModel::columnCount(const QModelIndex&) const
{
    return 1;
}
QVariant ConditionModel::data(const QModelIndex &index, int role) const
{
    QVariant value;
    switch (role)
    {
        case Qt::EditRole:
        case Qt::DisplayRole:
        {
            QModelIndex parent = index.parent();
            if (parent.isValid())
            {
                auto it = _conditions.cbegin() + parent.row();
                value = it->second.at( index.row() ).c_str();
            }
            else
            {
                auto it = _conditions.cbegin() + index.row();
                value = it->first.c_str();
            }
            break;
        }
        default:
            break;
    }
    return value;
}
Qt::ItemFlags ConditionModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}
QVariant ConditionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole) return QVariant();
    QVariant header;
/*    switch (orientation)
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
*/    return header;
}
QModelIndex ConditionModel::index(int row, int column,
                          const QModelIndex &parent) const
{
    return QModelIndex();
}

bool ConditionModel::insertRows(int row, int count, const QModelIndex& parent)
{
    beginInsertRows(parent, row, row+count-1);
    if (parent.isValid())
    {
        auto vec = (_conditions.begin() + parent.row())->second;
        std::vector<std::string> new_rows(count);
        vec.insert(vec.begin()+row, new_rows.begin(), new_rows.end());
    }
    else
    {
        std::vector< std::pair<std::string, std::vector<std::string> > > new_rows;
        _conditions.insert(_conditions.begin()+row, new_rows.begin(), new_rows.end());
    }
    endInsertRows();
    return true;
}
QModelIndex ConditionModel::parent(const QModelIndex& child) const
{
    return QModelIndex(); //child.model();
}
bool ConditionModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row+count-1);
    if (parent.isValid())
    {
        auto vec = (_conditions.begin() + parent.row())->second;
        vec.erase(vec.begin()+row, vec.begin()+row+count);
    }
    else
        _conditions.erase(_conditions.begin()+row, _conditions.begin()+row+count);
    endRemoveRows();
    return true;
}

int ConditionModel::rowCount() const
{
    return rowCount(QModelIndex());
}
int ConditionModel::rowCount(const QModelIndex& index) const
{
    if (index.isValid())
    {
        auto it = _conditions.cbegin() + index.row();
        return it->second.size();
    }
    return _conditions.size();
}

bool ConditionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    switch (role)
    {
        case Qt::EditRole:
        {
            QModelIndex parent = index.parent();
            if (parent.isValid())
            {
                auto it = _conditions.begin() + parent.row();
                it->second[ index.row() ] = value.toString().toStdString();
            }
            else
            {
                auto it = _conditions.begin() + index.row();
                it->first = value.toString().toStdString();
            }
            break;
        }
        default:
            return false;
            break;
    }
    emit dataChanged(index, index);
    return true;
}

bool ConditionModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    throw "ConditionModel::setHeaderData: Can't change header data.";
}
