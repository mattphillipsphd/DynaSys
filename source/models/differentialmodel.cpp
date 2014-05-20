#include "differentialmodel.h"

DifferentialModel::DifferentialModel(QObject *parent, const std::string& name) :
    ParamModelBase(parent, name), _modelStep(std::to_string(ds::DEFAULT_MODEL_STEP))
{
}

std::string DifferentialModel::ShortKey(size_t idx) const
{
    return ParamModelBase::Key(idx).substr( 0, Key(idx).size()-1 );
}

int DifferentialModel::ShortKeyIndex(const std::string& par_name) const
{
    std::string key = par_name + "'";
    return KeyIndex(key);
}

std::string DifferentialModel::TempExpression(size_t idx) const
{
    const std::string& temp = TempKey(idx),
            & key = ShortKey(idx),
            & value = Value(idx);
    return temp + " = " + key + " + "
            + _modelStep + "*(" + value + ")";
}
