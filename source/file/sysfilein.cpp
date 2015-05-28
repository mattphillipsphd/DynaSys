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

    const int version = ReadVersion();
    if (!version>=202) //New format for writing conditions
        throw std::runtime_error("SysFileIn::Load: Obsolete parameter file!");
    const bool has_par_variants = version>=400;

    //Model step
    const double model_step = ReadModelStep();
    model_mgr->SetModelStep(model_step);

    //Number of models
    const int num_models = ReadNumModels();

    //Model data
    std::vector<ParamModelBase*> models = ReadModels(num_models);
    for (auto it : models)
    {
        ds::PMODEL mi = it->Id();
        model_mgr->SetModel(mi, it);
    }

    //Parameter variants, if present
    if (has_par_variants)
        model_mgr->SetParVariants( ReadParVariants() );

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
    const int num_models = ReadNumModels();

    std::vector<ParamModelBase*> models = ReadModels(num_models);
    for (auto it : models)
    {
        std::string model_str = it->String();
        vmodels.push_back(model_str);
    }

    for (auto it : models)
        delete it;

    _in.close();
}


std::vector<ParamModelBase*> SysFileIn::ReadModels(int num_models)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::ReadModels", std::this_thread::get_id());
#endif
    std::string line;
    std::vector<ParamModelBase*> models(num_models);
    for (int i=0; i<num_models; ++i)
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
    return num_models;
}
std::vector<ModelMgr::ParVariant*> SysFileIn::ReadParVariants()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::ReadParVariants", std::this_thread::get_id());
#endif
    std::string line;
    std::getline(_in, line);
    while (line.empty()) std::getline(_in, line);
    size_t tab = line.find_first_of('\t');
    const int num_variants = std::stoi( line.substr(tab+1) );

    std::vector<ModelMgr::ParVariant*> par_variants(num_variants);
    for (int i=0; i<num_variants; ++i)
    {
        //Title
        std::string title;
        std::getline(_in, title);
        while (title.empty()) std::getline(_in, title);
        ModelMgr::ParVariant* pv = new ModelMgr::ParVariant(title);


        //Inputs (parameters)
        std::getline(_in, line);
        tab = line.find_first_of('\t');
        const int num_pars = std::stoi( line.substr(tab+1) );
        for (int j=0; j<num_pars; ++j)
        {
            std::getline(_in, line);
            tab = line.find_first_of('\t');
            std::string key = line.substr(0, tab),
                    val = line.substr(tab+1);
            pv->pars.push_back( PairStr(key, val) );
        }

        //Input files
        std::getline(_in, line);
        tab = line.find_first_of('\t');
        const int num_input_files = std::stoi( line.substr(tab+1) );
        for (int j=0; j<num_input_files; ++j)
        {
            std::getline(_in, line);
            tab = line.find_first_of('\t');
            std::string key = line.substr(0, tab),
                    val = line.substr(tab+1);
            pv->input_files.push_back( PairStr(key, val) );
        }

        //Notes
        std::string paragraph;
        std::getline(_in, paragraph); //The "Notes" line
        std::getline(_in, paragraph);
        while (paragraph != ModelMgr::ParVariant::END_NOTES)
        {
            pv->notes += paragraph + "\n";
            std::getline(_in, paragraph);
        }

        par_variants[i] = pv;
    }

    return par_variants;
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
