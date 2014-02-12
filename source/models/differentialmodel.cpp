#include "differentialmodel.h"

DifferentialModel::DifferentialModel(QObject *parent, const std::string& name) :
    ParamModel(parent, name)
{
}

VecStr DifferentialModel::Expressions() const
{
    VecStr expressions;
    const size_t num_pars = NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        const std::string& key = ShortKey(i),
                & value = Value(i);
        expressions.push_back(key + " = " + key + " + " + value);
    }
    return expressions;
}

std::string DifferentialModel::ShortKey(size_t i) const
{
    return ParamModel::Key(i).substr( 0, Key(i).size()-1 );
}
