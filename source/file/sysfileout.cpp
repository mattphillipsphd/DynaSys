#include "sysfileout.h"

SysFileOut::SysFileOut(const std::string& name,
                       const std::vector<const ParamModel*>& models, const ConditionModel* conditions)
    : _conditions(conditions), _models(models), _name(name)
{
}

void SysFileOut::Save()
{
    _out.open(_name);

    _out << "DynaSys " << ds::VERSION_STR << std::endl;
    _out << _models.size() << std::endl;
    for (auto it : _models)
    {
        const int num_pars = it->NumPars();
        _out << it->Name() << "\t" << num_pars << std::endl;
        for (int i=0; i<num_pars; ++i)
            _out << it->Key(i) << "\t" << it->Value(i) << std::endl;
        _out << std::endl;
    }

    _conditions->Write(_out);

    _out.close();
}
