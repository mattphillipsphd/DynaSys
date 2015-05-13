#include "sysfileout.h"

SysFileOut::SysFileOut(const std::string& name)
    : _name(name)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileOut::SysFileOut", std::this_thread::get_id());
#endif
}

void SysFileOut::Save() const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileOut::Save", std::this_thread::get_id());
#endif
    _out.open(_name);

    ModelMgr* model_mgr = ModelMgr::Instance();

    SaveHeader();
    _out << ds::NUM_MODELS << std::endl;
    for (int i=0; i<ds::NUM_MODELS; ++i)
    {
        const ParamModelBase* model = model_mgr->Model((ds::PMODEL)i);
        const int num_pars = (int)model->NumPars();
        _out << model->Name() << "\t" << num_pars << std::endl;
        for (int i=0; i<num_pars; ++i)
        {
            _out << model->Key(i) << "\t" << model->Value(i);
            if (const NumericModelBase* nmodel = dynamic_cast<const NumericModelBase*>(model))
                 _out << "\t" << nmodel->Minimum(i) << "\t" << nmodel->Maximum(i);
            _out << std::endl;
        }
        _out << std::endl;
    }

    model_mgr->GetNotes()->Write(_out);

    _out.close();
}
void SysFileOut::Save(const VecStr& vmodels,
                      const Notes* notes) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileOut::Save", std::this_thread::get_id());
#endif
    _out.open(_name);

    SaveHeader();
    _out << vmodels.size() << std::endl;
    for (auto it : vmodels)
        _out << it;
    notes->Write(_out);

    _out.close();
}

void SysFileOut::SaveHeader() const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileOut::SaveHeader", std::this_thread::get_id());
#endif
    double model_step = ModelMgr::Instance()->ModelStep();
    _out << "DynaSys " << ds::VERSION_STR << std::endl;
    _out << "Model step: " << model_step << std::endl;
}
