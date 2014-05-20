#include "initialcondmodel.h"

InitialCondModel::InitialCondModel(QObject *parent, const std::string& name) :
    ParamModelBase(parent, name)
{
}

VecStr InitialCondModel::Initializations() const
{
    VecStr initializations;
    const size_t num_pars = NumPars();
    for (size_t i=0; i<num_pars; ++i)
        initializations.push_back( Expression(i) );
    return initializations;
}
std::string InitialCondModel::ShortKey(size_t idx) const
{
    std::string key = Key(idx);
    return key.substr( 0, key.find('(') );
}

int InitialCondModel::ShortKeyIndex(const std::string& par_name) const
{
    std::string key = par_name + "{0}";
    return KeyIndex(key);
}
