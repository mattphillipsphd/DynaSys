#include "initialcondmodel.h"

InitialCondModel::InitialCondModel(QObject *parent, const std::string& name) :
    ParamModel(parent, name)
{
}

VecStr InitialCondModel::Initializations() const
{
    VecStr initializations;
    const size_t num_pars = NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        std::string key = Key(i);
        const std::string& value = Value(i);
        int pos = key.find('(');
        key = key.substr(0,pos);
        initializations.push_back(key + " = " + value);
    }
    return initializations;
}
