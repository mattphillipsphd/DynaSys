#include "nullclinemodel.h"

const std::string NullclineModel::PREFIX = "_NC_";

NullclineModel::NullclineModel(QObject *parent, const std::string& name) :
    NumericModelBase(parent, name)
{
}

void NullclineModel::AddParameter(const std::string& param, const std::string& value)
{
    ParamModelBase::AddParameter(PREFIX + param, value);
}

std::string NullclineModel::TempExpression(size_t i) const
{
    return TempKey(i) + " = " + Value(i);
}
