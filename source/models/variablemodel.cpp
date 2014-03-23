#include "variablemodel.h"

VariableModel::VariableModel(QObject *parent, const std::string& name) :
    ParamModelBase(parent, name)
{
}

VecStr VariableModel::Expressions() const
{
    VecStr expressions;
    const size_t num_vars = NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        const std::string& key = Key(i),
                & value = Value(i);
        if (Input::Type(value)==Input::USER)
            expressions.push_back(key + " = " + value);
    }
    return expressions;
}
std::string VariableModel::TempExpression(size_t i) const
{
    return (Input::Type(Value(i))==Input::USER)
            ? TempKey(i) + " = " + Value(i)
            : Key(i) + " = " + Value(i);
}
VecStr VariableModel::TempExpressions() const
{
    VecStr expressions;
    const size_t num_vars = NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        const std::string& key = TempKey(i),
                & value = Value(i);
        if (Input::Type(value)==Input::USER)
            expressions.push_back(key + " = " + value);
    }
    return expressions;
}
