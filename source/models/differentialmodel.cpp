#include "differentialmodel.h"
#include "../memrep/modelmgr.h"

DifferentialModel::DifferentialModel(QObject *parent, const std::string& name) :
    NumericModelBase(parent, name)
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
    const std::string model_step = std::to_string( ModelMgr::Instance()->ModelStep() );
    return temp + " = " + key + " + "
            + model_step + "*(" + value + ")";
}
