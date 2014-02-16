#include "differentialmodel.h"

DifferentialModel::DifferentialModel(QObject *parent, const std::string& name) :
    ParamModelBase(parent, name)
{
}

std::string DifferentialModel::Expression(size_t idx) const
{
    const std::string& key = ShortKey(idx),
            & value = Value(idx);
    return key + " = " + key + " + " + value;
}
VecStr DifferentialModel::Expressions() const
{
    VecStr expressions;
    const size_t num_pars = NumPars();
    for (size_t i=0; i<num_pars; ++i)
        expressions.push_back( Expression(i) );
    return expressions;
}

std::string DifferentialModel::ShortKey(size_t idx) const
{
    return ParamModelBase::Key(idx).substr( 0, Key(idx).size()-1 );
}
