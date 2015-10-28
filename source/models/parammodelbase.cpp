#include "parammodelbase.h"
#include "../models/conditionmodel.h"
#include "../models/differentialmodel.h"
#include "../models/initialcondmodel.h"
#include "../models/jacobianmodel.h"
#include "../models/nullclinemodel.h"
#include "../models/parammodel.h"
#include "../models/statevarmodel.h"
#include "../models/variablemodel.h"

const std::string ParamModelBase::Param::DEFAULT_VAL = "0";

ParamModelBase* ParamModelBase::Create(ds::PMODEL mi)
{
    ParamModelBase* model = nullptr;
    switch (mi)
    {
        case ds::INP:
            model = new ParamModel(nullptr, ds::Model(mi));
            break;
        case ds::FUNC:
            model = new VariableModel(nullptr, ds::Model(mi));
            break;
        case ds::STATE:
            model = new StateVarModel(nullptr, ds::Model(mi));
            break;
        case ds::DIFF:
            model = new DifferentialModel(nullptr, ds::Model(mi));
            break;
        case ds::INIT:
            model = new InitialCondModel(nullptr, ds::Model(mi));
            break;
        case ds::COND:
            model = new ConditionModel(nullptr, ds::Model(mi));
            break;
        case ds::NC:
            model = new NullclineModel(nullptr, ds::Model(mi));
            break;
        case ds::JAC:
            model = new JacobianModel(nullptr, ds::Model(mi));
            break;
        default:
            throw std::runtime_error("SysFileIn::Load: Bad model name");
    }
    return model;
}

ParamModelBase::ParamModelBase(QObject* parent, const std::string& name) :
    QAbstractTableModel(parent), _id(ds::Model(name))
{
}
ParamModelBase::~ParamModelBase()
{
    for (auto it : _parameters)
        delete it;
}

void ParamModelBase::AddParameter(const std::string& param, const std::string& value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    int row = (int)_parameters.size();
    QModelIndex row_index = createIndex(row, 0);
    insertRows(row, 1, QModelIndex());
    _parameters[row] = new Param(param, value);
    emit dataChanged(row_index, row_index);
    emit headerDataChanged(Qt::Vertical, row, row);
}
void ParamModelBase::SetFreeze(size_t i, bool is_freeze)
{
    setData( createIndex(i,FREEZE), is_freeze, Qt::CheckStateRole );
}
void ParamModelBase::SetValue(const std::string& param, const std::string& value)
{
    SetValue( KeyIndex(param), value );
}
void ParamModelBase::SetValue(size_t i, const std::string& value)
{
    setData( createIndex(i,VALUE), value.c_str(), Qt::EditRole );
}

std::string ParamModelBase::Expression(size_t i) const
{
    return ShortKey(i) + " = " + Value(i);
}
std::string ParamModelBase::ExpressionList() const
{
    std::string elist;
    const size_t num_pars_m1 = _parameters.size()-1;
    for (size_t i=0; i<num_pars_m1; ++i)
        elist += Expression(i) + ", ";
    elist += Expression(num_pars_m1);
    return elist;
}
VecStr ParamModelBase::Expressions() const
{
    const size_t num_pars = _parameters.size();
    VecStr vs(num_pars);
    for (size_t i=0; i<num_pars; ++i)
        vs[i] = Expression(i);
    return vs;
}
bool ParamModelBase::IsFreeze(size_t idx) const
{
    return data(createIndex((int)idx,FREEZE),Qt::CheckStateRole).toBool();
}
std::string ParamModelBase::Key(size_t i) const
{
    return headerData((int)i, Qt::Vertical, Qt::DisplayRole).toString().toStdString();
}
VecStr ParamModelBase::Keys() const
{
    VecStr vs;
    const size_t num_pars = _parameters.size();
    for (size_t i=0; i<num_pars; ++i)
        vs.push_back(Key(i));
    return vs;
}
std::string ParamModelBase::Name() const
{
    return ds::Model(_id);
}
std::string ParamModelBase::ParamString(size_t i) const
{
    return Key(i) + "\t" + Value(i) + "\n";
}
void ParamModelBase::SaveString(std::ofstream& out) const
{
    const size_t num_pars = rowCount();
    out << Name() << "\t" << num_pars << std::endl;
    for (size_t j=0; j<num_pars; ++j)
        out << ParamString(j);
    out << std::endl;
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
std::string ParamModelBase::TempExpression(size_t i) const
{
    return Expression(i);
}
std::string ParamModelBase::TempExprnForCFile(size_t i) const
{
    return TempExpression(i);
}
VecStr ParamModelBase::TempExpressions() const
{
    const size_t num_pars = _parameters.size();
    VecStr vs(num_pars);
    for (size_t i=0; i<num_pars; ++i)
        vs[i] = TempExpression(i);
    return vs;
}
std::string ParamModelBase::TempKey(size_t i) const
{
    return ShortKey(i) + "_temp_";
}
const std::string& ParamModelBase::Value(const std::string& key) const
{
    return Value( KeyIndex(key) );
}
const std::string& ParamModelBase::Value(size_t i) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _parameters.at(i)->value;
        //Don't go through data, for speed
//    return data(createIndex(i,0),Qt::DisplayRole).toString().toStdString();
}
VecStr ParamModelBase::Values() const
{
    const size_t num_pars = _parameters.size();
    VecStr vs(num_pars);
    for (size_t i=0; i<num_pars; ++i)
        vs[i] = Value(i);
    return vs;
}

int ParamModelBase::columnCount() const
{
    return columnCount( QModelIndex() );
}
int ParamModelBase::columnCount(const QModelIndex&) const
{
    return NUM_BASE_COLUMNS;
}
QVariant ParamModelBase::data(const QModelIndex &index, int role) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    QVariant value;
    switch (role)
    {
        case Qt::CheckStateRole:
            if (index.column()!=FREEZE) break;
            value = _parameters.at( index.row() )->is_freeze;
            break;
        case Qt::EditRole:
        case Qt::DisplayRole:
            switch (index.column())
            {
                case FREEZE:
//                    value = _parameters.at( index.row() ).is_freeze;
                    break;
                case VALUE:
                    value = _parameters.at( index.row() )->value.c_str();
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
    switch (index.column())
    {
        case FREEZE:
            return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
        default:
            return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    }
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
                case FREEZE:
                    header = "";
                    break;
                case VALUE:
                    header = "Value";
                    break;
            }
            break;
        case Qt::Vertical:
            if (section>(int)_parameters.size())
                throw std::runtime_error("ParamModelBase::headerData: Bad parameter index.");
            header = _parameters.at(section)->key.c_str();
            break;
    }
    return header;
}

bool ParamModelBase::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row+count-1);
    std::vector<Param*> new_rows(count);
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
    return (int)_parameters.size();
}

bool ParamModelBase::setData(const QModelIndex &index, const QVariant &value, int role)
{
    _mutex.lock();
    std::string val = value.toString().toStdString();
    switch (role)
    {
        case Qt::CheckStateRole:
            if (index.column()!=FREEZE)
                throw std::runtime_error("ParamModelBase::setData: Bad role");
        case Qt::EditRole:
        {
            if (index.row()>=rowCount())
                throw std::runtime_error("ParamModelBase::setData: Index out of bounds");
            switch (index.column())
            {
                case FREEZE:
                    _parameters[ index.row() ]->is_freeze = value.toBool();
                    break;
                case VALUE:
                    _parameters[ index.row() ]->value = val;
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

bool ParamModelBase::setHeaderData(int, Qt::Orientation, const QVariant&, int)
{
    throw std::runtime_error("ParamModelBase::setHeaderData: Can't change parameter name.");
}

int ParamModelBase::KeyIndex(const std::string& par_name) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = std::find_if(_parameters.cbegin(), _parameters.cend(), [=](const Param* par)
    {
        return par_name == par->key;
    });
    if (it == _parameters.cend()) return -1;
    return it - _parameters.cbegin();
}
int ParamModelBase::ShortKeyIndex(const std::string& par_name) const
{
    return KeyIndex(par_name);
}
