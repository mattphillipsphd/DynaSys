#include "tpvtablemodel.h"

TPVTableModel::TPVTableModel(const VecStr& names, QObject* parent)
    : QAbstractTableModel(parent), _headers(VecStr(columnCount()))
{
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::TPVTableModel", std::this_thread::get_id());
#endif
    for (const auto& it : names)
        _data.push_back( RowRecord(it, false, 0.0) );
    if (!_data.empty())
        _data[0].is_enabled = true;
}

VecStr TPVTableModel::IsEnabled() const
{
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::IsEnabled", std::this_thread::get_id());
#endif
    VecStr vs;
    for (int i=0; i<rowCount(); ++i)
        if (IsEnabled(i)) vs.push_back( Name(i) );
    return vs;
}
bool TPVTableModel::IsEnabled(int idx) const
{
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::IsEnabled", std::this_thread::get_id());
#endif
    QModelIndex index = createIndex(idx, 0);
    return data(index,Qt::CheckStateRole).toBool();
}
double TPVTableModel::LogScale(int idx) const
{
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::LogScale", std::this_thread::get_id());
#endif
    QModelIndex index = createIndex(idx, 1);
    return data(index,Qt::EditRole).toDouble();
}
std::string TPVTableModel::Name(int idx) const
{
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::Name", std::this_thread::get_id());
#endif
    return headerData(idx, Qt::Vertical, Qt::DisplayRole).toString().toStdString();
}


int TPVTableModel::columnCount() const
{
    return columnCount( QModelIndex() );
}
int TPVTableModel::columnCount(const QModelIndex&) const
{
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::columnCount", std::this_thread::get_id());
#endif
    return 2;
}
QVariant TPVTableModel::data(const QModelIndex &index, int role) const
{
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::data", std::this_thread::get_id());
#endif
    QVariant value;
    switch (role)
    {
        case Qt::CheckStateRole:
            if (index.column()!=0) break;
            value = _data.at(index.row()).is_enabled;
            break;
        case Qt::EditRole:
        case Qt::DisplayRole:
            switch (index.column())
            {
                case 0:
//                    value = _data.at(index.row()).is_enabled;
                    break;
                case 1:
                    value = _data.at(index.row()).log_scale;
                    break;
            }
            break;
        default:
            break;
    }
    return value;
}
Qt::ItemFlags TPVTableModel::flags(const QModelIndex &index) const
{
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::flags", std::this_thread::get_id());
#endif
    switch (index.column())
    {
        case 0:
            return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
        case 1:
            return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
        default:
            throw std::runtime_error("TPVTableModel::flags, bad column index");
    }
}
QVariant TPVTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::headerData", std::this_thread::get_id());
#endif
    if (role!=Qt::DisplayRole) return QVariant();
    QVariant header;
    switch (orientation)
    {
        case Qt::Horizontal:
            header = _headers.at(section).c_str();
            break;
        case Qt::Vertical:
            if (section>(int)_data.size())
                throw std::runtime_error("TPVTableModel::headerData: Bad parameter index.");
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
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::rowCount", std::this_thread::get_id());
#endif
    return (int)_data.size();
}

bool TPVTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::setData", std::this_thread::get_id());
#endif
    switch (role)
    {
        case Qt::CheckStateRole:
            if (index.column()!=0) break;
        case Qt::EditRole:
        {
            if (index.row()>=rowCount())
                throw std::runtime_error("TPVTableModel::setData: Index out of bounds");
            switch (index.column())
            {
                case 0:
                    _data[ index.row() ].is_enabled = value.toBool();
                    break;
                case 1:
                    _data[ index.row() ].log_scale = value.toDouble();
                    break;
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

bool TPVTableModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
#ifdef DEBUG_TP_FUNC
    ScopeTracker st("TPVTableModel::setHeaderData", std::this_thread::get_id());
#endif
    if (orientation==Qt::Horizontal)
    {
        if (role==Qt::DisplayRole)
        {
            _headers[section] = value.toString().toStdString();
            emit headerDataChanged(orientation, section, section);
        }
    }
    else
        throw std::runtime_error("TPVTableModel::setHeaderData: Can't change parameter name.");
    return true;
}
