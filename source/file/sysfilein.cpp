#include "sysfilein.h"

SysFileIn::SysFileIn(const std::string& name)
    : _name(name)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::SysFileIn", std::this_thread::get_id());
#endif
}

void SysFileIn::Load()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::Load", std::this_thread::get_id());
#endif
    ModelMgr* model_mgr = ModelMgr::Instance();

    _in.open(_name);

    int version = ReadVersion();
    bool has_new_conds = (version>=202);
    if (!has_new_conds)
        throw std::runtime_error("SysFileIn::Load: Obsolete parameter file!");

    //Model step
    double model_step = ReadModelStep();
    model_mgr->SetModelStep(model_step);

    //Number of models
    const int num_models = ReadNumModels();

    //Model data
    std::vector<ParamModelBase*> models = ReadModels();
    for (int i=0; i<num_models; ++i)
        model_mgr->SetModel((ds::PMODEL)i, models[i]);

    //Notes
    model_mgr->SetNotes( ReadNotes() );

    _in.close();
}
void SysFileIn::Load(VecStr& vmodels)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::Load", std::this_thread::get_id());
#endif
    _in.open(_name);

    ReadVersion();
    ReadModelStep();
    ReadNumModels();

    std::vector<ParamModelBase*> models = ReadModels();
    for (auto it : models)
    {
        std::string model_str = it->String();
        vmodels.push_back(model_str);
    }

    for (auto it : models)
        delete it;

    _in.close();
}


std::vector<ParamModelBase*> SysFileIn::ReadModels()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::ReadModels", std::this_thread::get_id());
#endif
    std::string line;
    std::vector<ParamModelBase*> models(ds::NUM_MODELS);
    for (int i=0; i<ds::NUM_MODELS; ++i)
    {
        std::getline(_in, line);
        size_t tab = line.find_first_of('\t');
        std::string name = line.substr(0,tab),
                num = line.substr(tab+1);
        const int num_pars = std::stoi(num);
        ParamModelBase* model = ParamModelBase::Create( ds::Model(name) );

        for (int j=0; j<num_pars; ++j)
        {
            std::getline(_in, line);
            size_t tab = line.find_first_of('\t');
            std::string key = line.substr(0,tab),
                    rem = line.substr(tab+1);
            model->ProcessParamFileLine(key, rem);
        }
        models[i] = model;

        std::getline(_in, line);
    }
    return models;
}
double SysFileIn::ReadModelStep()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::ReadModelStep", std::this_thread::get_id());
#endif
    std::string line;
    std::getline(_in, line);
    size_t pos = line.find_last_of(':');
    std::string model_step = line.substr(pos+1);
    ds::RemoveWhitespace(model_step);
    return std::stod(model_step);
}
Notes* SysFileIn::ReadNotes()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::ReadNotes", std::this_thread::get_id());
#endif
    Notes* notes = new Notes();
    notes->Read(_in);
    return notes;
}
int SysFileIn::ReadNumModels()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::ReadNumModels", std::this_thread::get_id());
#endif
    std::string line;
    std::getline(_in, line);
    const int num_models = std::stoi(line);
    if (num_models != ds::NUM_MODELS)
        throw std::runtime_error("SysFileIn::Load: Wrong number of models");
    return num_models;
}
int SysFileIn::ReadVersion()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::ReadVersion", std::this_thread::get_id());
#endif
    std::string line;
    std::getline(_in, line);
    return ds::VersionNum(line);
}
