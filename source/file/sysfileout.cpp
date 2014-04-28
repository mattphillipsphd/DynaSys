#include "sysfileout.h"

SysFileOut::SysFileOut(const std::string& name)
    : _name(name)
{
}

void SysFileOut::Save(const std::vector<const ParamModelBase*>& models,
                      double model_step,
                      const ConditionModel* conditions,
                      const Notes* notes) const
{
    _out.open(_name);

    SaveHeader(model_step);
    _out << models.size() << std::endl;
    for (auto it : models)
    {
        const int num_pars = it->NumPars();
        _out << it->Name() << "\t" << num_pars << std::endl;
        for (int i=0; i<num_pars; ++i)
            _out << it->Key(i) << "\t" << it->Value(i) << "\t"
                 << it->Minimum(i) << "\t" << it->Maximum(i) << std::endl;
        _out << std::endl;
    }

    conditions->Write(_out);
    _out << std::endl;
    notes->Write(_out);

    _out.close();
}
void SysFileOut::Save(const VecStr& vmodels,
                      double model_step,
                      const Notes* notes) const
{
    _out.open(_name);

    SaveHeader(model_step);
    _out << vmodels.size() - 1 << std::endl; //Don't count conditions here
    for (auto it : vmodels)
        _out << it;
    notes->Write(_out);

    _out.close();
}

void SysFileOut::SaveHeader(double model_step) const
{
    _out << "DynaSys " << ds::VERSION_STR << std::endl;
    _out << "Model step: " << model_step << std::endl;
}
