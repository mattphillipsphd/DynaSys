#include "tpvtablemodel.h"

TPVTableModel::TPVTableModel(const VecStr& names, QObject* parent)
    : QAbstractTableModel(parent)
{
    for (const auto& it : names)
        _data.push_back( RowRecord(it, false) );
    if (!_data.empty())
        _data[0].is_enabled = true;
}

VecStr TPVTableModel::IsEnabled() const
{
    VecStr vs;
    for (int i=0; i<rowCount(); ++i)
        if (IsEnabled(i)) vs.push_back( Name(i) );
    return vs;
}
bool TPVTableModel::IsEnabled(int idx) const
{
    QModelIndex index = createIndex(idx, 0);
    return data(index,Qt::CheckStateRole).toBool();
}
std::string TPVTableModel::Name(int idx) const
{
    return headerData(idx, Qt::Vertical, Qt::DisplayRole).toString().toStdString();
}


int TPVTableModel::columnCount() const
{
    return columnCount( QModelIndex() );
}
int TPVTableModel::columnCount(const QModelIndex&) const
{
    return 1;
}
QVariant TPVTableModel::data(const QModelIndex &index, int role) const
{
    QVariant value;
    switch (role)
    {
        case Qt::CheckStateRole:
            if (index.column()!=0) break;
            value = _data.at(index.row()).is_enabled;
            break;
        case Qt::EditRole:
        case Qt::DisplayRole:
            if (index.column()==0) break;
            value = _data.at(index.row()).is_enabled;
            break;
        default:
            break;
    }
    return value;
}
Qt::ItemFlags TPVTableModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
//    return QAbstractTableModel::flags(index) |
//            (index.column()==0) ? Qt::ItemIsUserCheckable : Qt::ItemIsEditable;
}
QVariant TPVTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole) return QVariant();
    QVariant header;
    switch (orientation)
    {
        case Qt::Horizontal:
            header = "";
            break;
        case Qt::Vertical:
            if (section>(int)_data.size())
                throw "TPVTableModel::headerData: Bad parameter index.";
            header = _data.at(section).name.c_str();
            break;
    }
    return header;
}

int TPVTableModel::rowCount() const
{
    return rowCount(QModelIndex());
}
int TPVTableModel::rowCount(const QModelIndex&) const
{
    return (int)_data.size();
}

bool TPVTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    switch (role)
    {
        case Qt::CheckStateRole:
            if (index.column()!=0) break;
        case Qt::EditRole:
        {
            if (index.row()>=rowCount()) throw "TPVTableModel::setData: Index out of bounds";
            _data[ index.row() ].is_enabled = value.toBool();
            break;
        }
        default:
            return false;
            break;
    }
    emit dataChanged(index, index);
    return true;
}

bool TPVTableModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    throw "TPVTableModel::setHeaderData: Can't change parameter name.";
}
