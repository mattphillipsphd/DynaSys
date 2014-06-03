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
VecStr VariableModel::Initializations() const
{
    VecStr initializations;
    const size_t num_vars = NumPars();
    for (size_t i=0; i<num_vars; ++i)
        initializations.push_back(Key(i) + " = 0");
    return initializations;
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
int VariableModel::TypeCount(Input::TYPE type) const
{
    const int num_vars = NumPars();
    int count = 0;
    for (size_t i=0; i<num_vars; ++i)
    {
        const std::string& value = Value(i);
        count += (int)( Input::Type(value)==type );
    }
    return count;
}
