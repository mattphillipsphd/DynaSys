#include "nullclinemodel.h"

NullclineModel::NullclineModel(QObject *parent, const std::string& name) :
    NumericModelBase(parent, name)
{
}

std::string NullclineModel::TempExpression(size_t i) const
{
    return Value(i).empty() ? "" : TempKey(i) + " = " + Value(i);
}
