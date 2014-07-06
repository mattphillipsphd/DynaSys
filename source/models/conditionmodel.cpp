#include "conditionmodel.h"

const std::string ConditionModel::DELIM = ",";

ConditionModel::ConditionModel(QObject *parent, const std::string& name) :
    ParamModelBase(parent, name)
{
}
ConditionModel::~ConditionModel()
{
}

void ConditionModel::AddCondition(const std::string& condition, const VecStr& exprns)
{
    AddParameter(condition, ds::Join(exprns, DELIM));
}
void ConditionModel::AddResult(int row, const std::string& result)
{
    VecStr result_vec = Results(row);
    std::string results = ds::Join(result_vec, DELIM);
    results += results.empty() ? result : DELIM + result;
    SetValue(row, results);
}

void ConditionModel::SetResults(int row, const VecStr& exprns)
{
    SetValue(row, ds::Join(exprns, DELIM));
}

std::string ConditionModel::Condition(int row) const
{
    return Key((size_t)row);
}
void ConditionModel::ProcessParamFileLine(const std::string& key, std::string rem)
{
    AddParameter(key, rem);
}

VecStr ConditionModel::Results(int row) const
{
    std::string delimc = DELIM;
    return ds::Split( Value((size_t)row), delimc );
}
std::string ConditionModel::String() const
{
    std::string str;
    str += "#" + ds::Model( Id() ) + "\n";
    const size_t num_pars = NumPars();
    for (size_t i=0; i<num_pars; ++i)
        str += Key(i) + "\t" + Value(i) + "\n";
    str += "\n";
    return str;
}

int ConditionModel::columnCount(const QModelIndex&) const
{
    return NUM_COND_COLUMNS;
}
QVariant ConditionModel::data(const QModelIndex &index, int role) const
{
    std::lock_guard<std::mutex> lock(_cmutex);
    QVariant value;
    switch (role)
    {
        case Qt::EditRole:
        case Qt::DisplayRole:
            switch (index.column())
            {
                case FREEZE:
                case VALUE:
                    value = ParamModelBase::data(index, role);
                    break;
                case TEST:
                    value = Parameter( index.row() )->key.c_str();
                    break;
            }
            break;
        default:
            break;
    }
    return value;
}
bool ConditionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    _cmutex.lock();
    std::string val = value.toString().toStdString();
    switch (role)
    {
         case Qt::EditRole:
        {
            if (index.row()>=rowCount())
                throw std::runtime_error("NumericModelBase::setData: Index out of bounds");
            switch (index.column())
            {
                case FREEZE:
                case VALUE:
                    _cmutex.unlock();
                    ParamModelBase::setData(index, value, role);
                    return true;
                    break;
                case TEST:
                    Parameter(index.row())->key = val;
                    break;
            }
            break;
        }
        default:
            return false;
            break;
    }
    _cmutex.unlock();
    emit dataChanged(index, index);
    return true;
}
