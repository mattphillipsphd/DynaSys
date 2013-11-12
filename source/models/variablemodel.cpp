#include "variablemodel.h"

VariableModel::VariableModel(QObject *parent, const std::string& name) :
    ParamModel(parent, name)
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
