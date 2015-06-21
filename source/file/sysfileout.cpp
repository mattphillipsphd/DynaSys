#include "sysfileout.h"

SysFileOut::SysFileOut(const std::string& name)
    : _modelMgr(ModelMgr::Instance()), _name(name)
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

    SaveHeader();
    _out << ds::NUM_MODELS << std::endl;
    for (int i=0; i<ds::NUM_MODELS; ++i)
    {
        const ParamModelBase* model = _modelMgr->Model((ds::PMODEL)i);
        model->SaveString(_out);
    }

    SaveParVariants();

    _modelMgr->GetNotes()->Write(_out);

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
    SaveParVariants();
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

void SysFileOut::SaveParVariants() const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileOut::SaveParVariants", std::this_thread::get_id());
#endif
    const size_t num_pvs = _modelMgr->NumParVariants();
    _out << "ParameterVariants\t" << num_pvs << std::endl;

    for (size_t i=0; i<num_pvs; ++i)
    {
        const ModelMgr::ParVariant* const pv = _modelMgr->GetParVariant(i);
        _out << pv->title << std::endl;

        const size_t num_pars = pv->pars.size();
        _out << "Inputs\t" << num_pars << std::endl;
        for (size_t j=0; j<num_pars; ++j)
            _out << pv->pars.at(j).first << "\t" << pv->pars.at(j).second << std::endl;

        const size_t num_input_files = pv->input_files.size();
        _out << "InputFiles\t" << num_input_files << std::endl;
        for (size_t j=0; j<num_input_files; ++j)
            _out << pv->input_files.at(j).first << "\t" << pv->input_files.at(j).second << std::endl;

        _out << "Notes" << std::endl;
        _out << pv->notes;
        if (pv->notes.at( pv->notes.size()-1) != '\n') _out << "\n";
        _out << ModelMgr::ParVariant::END_NOTES << std::endl;
        _out << std::endl;
    }
}
