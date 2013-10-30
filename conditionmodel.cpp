#include "conditionmodel.h"

ConditionModel::ConditionModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

void ConditionModel::AddCondition(const std::string& condition, const VecStr& exprns)
{
    QVector<int> roles;
    roles << Qt::DisplayRole << Qt::EditRole;
    int row = _conditions.size();
//    _conditions.push_back( std::make_pair(condition, exprns) );
    QModelIndex parent = createIndex(row,0);
    insertRows(row, 1, QModelIndex());
    _conditions[row] = std::make_pair(condition, VecStr());
    emit dataChanged(parent, parent, roles);

    insertRows(0,exprns.size(),parent);
    for (size_t i=0; i<exprns.size(); ++i)
        _conditions[row].second[i] = exprns.at(i);
//    _conditions[row].second = exprns;
    QModelIndex exprn_begin = createIndex(0, 0, row),
            exprn_end = createIndex(exprns.size()-1, 0, row);
    emit dataChanged(exprn_begin, exprn_end, roles);
}

const std::string& ConditionModel::Condition(int i) const
{
    return _conditions.at(i).first;
}
const VecStr& ConditionModel::Expressions(int i) const
{
    return _conditions.at(i).second;
}
void ConditionModel::Read(std::ifstream& in)
{
    _conditions.clear();
    std::string line;
    while (line.empty())
        std::getline(in, line);
    int tab = line.find_first_of('\t');
    int num_conds = std::stoi( line.substr(tab+1) );
    for (int i=0; i<num_conds; ++i)
    {
        std::getline(in, line);
        tab = line.find_first_of('\t');
        std::string cond = line.substr(0,tab);
        int num_results = std::stoi( line.substr(tab+1) );
        VecStr results(num_results);
        for (int j=0; j<num_results; ++j)
        {
            std::getline(in, line);
            results[j] = line.substr(1); //The first character is a tab
        }
        _conditions.push_back( std::make_pair(cond, results) );
    }
}
void ConditionModel::Write(std::ofstream& out) const
{
    out << "Conditions\t" << _conditions.size() << std::endl;
    for (const auto& it : _conditions)
    {
        out << it.first << "\t" << it.second.size() << std::endl;
        for (const auto& itr : it.second)
            out << "\t" << itr << std::endl;
    }
    out << std::endl;
}

int ConditionModel::columnCount() const
{
    return columnCount( QModelIndex() );
}
int ConditionModel::columnCount(const QModelIndex& index) const
{
//    if (index.isValid()) return 0;
    return 1;
}
QVariant ConditionModel::data(const QModelIndex& index, int role) const
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
/*QVariant ConditionModel::headerData(int section, Qt::Orientation orientation, int role) const
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
}*/
QModelIndex ConditionModel::index(int row, int column,
                          const QModelIndex& parent) const
{
    if (column!=0) throw("ConditionModel::index: Bad Index");
    if (parent.isValid())
    {
        int parent_row = parent.row();
        return createIndex(row, column, parent_row);
    }
    return createIndex(row, column, -1);
}

bool ConditionModel::insertRows(int row, int count, const QModelIndex& parent)
{
    beginInsertRows(parent, row, row+count-1);
    if (parent.isValid())
    {
        auto& vec = (_conditions.begin() + parent.row())->second;
        VecStr new_rows(count);
        vec.insert(vec.begin()+row, new_rows.begin(), new_rows.end());
    }
    else
    {
        std::vector< std::pair<std::string, VecStr > > new_rows(count);
        _conditions.insert(_conditions.begin()+row, new_rows.begin(), new_rows.end());
    }
    endInsertRows();
    return true;
}
QModelIndex ConditionModel::parent(const QModelIndex& child) const
{
    if (child.internalId() == -1) return QModelIndex();
    return createIndex(child.internalId(), 0);
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
//        return 0;
        auto it = _conditions.cbegin() + index.row();
        return it->second.size();
    }
    return _conditions.size();
}

bool ConditionModel::setData(const QModelIndex& index, const QVariant& value, int role)
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

/*bool ConditionModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    throw "ConditionModel::setHeaderData: Can't change header data.";
}
*/
