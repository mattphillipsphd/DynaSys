#include "sysfilein.h"

ModelMgr::ParVariant* SysFileIn::ReadParVariant(std::ifstream& in)
{
    //Title
    std::string title;
    std::getline(in, title);
    while (title.empty() || std::isspace(title.at(0))) std::getline(in, title);
    ModelMgr::ParVariant* pv = new ModelMgr::ParVariant(title);

    //Inputs (parameters)
    std::string line;
    std::getline(in, line);
    size_t tab = line.find_first_of('\t');
    const int num_pars = std::stoi( line.substr(tab+1) );
    for (int j=0; j<num_pars; ++j)
    {
        std::getline(in, line);
        tab = line.find_first_of('\t');
        std::string key = line.substr(0, tab),
                val = line.substr(tab+1);
        pv->pars.push_back( PairStr(key, val) );
    }

    //Input files
    std::getline(in, line);
    tab = line.find_first_of('\t');
    const int numinput_files = std::stoi( line.substr(tab+1) );
    for (int j=0; j<numinput_files; ++j)
    {
        std::getline(in, line);
        tab = line.find_first_of('\t');
        std::string key = line.substr(0, tab),
                val = line.substr(tab+1);
        while (val.size()>0 && std::isspace(val.at(0))) val.erase(0,1);
        while (val.size()>0 && std::isspace(val.back())) val.erase(val.size()-1);
        pv->input_files.push_back( PairStr(key, val) );
    }

    //Notes
    std::string paragraph;
    std::getline(in, paragraph); //The "Notes" line
    std::getline(in, paragraph);
    while ( !strstr(paragraph.c_str(), ModelMgr::ParVariant::END_NOTES.c_str()) && !in.eof())
    {
        pv->notes += paragraph + "\n";
        std::getline(in, paragraph);
    }

    return pv;
}

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

    _fileVersion = ReadVersion();
    if (!_fileVersion>=202) //New format for writing conditions
        throw std::runtime_error("SysFileIn::Load: Obsolete parameter file!");
    const bool has_par_variants = _fileVersion>=400;

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
    _fileVersion = ds::VersionNum();

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
    const bool has_statevars = _fileVersion>=402;
    for (int i=0; i<num_models; ++i)
    {
        std::getline(_in, line);
        size_t tab = line.find_first_of('\t');
        std::string name = line.substr(0,tab),
                num = line.substr(tab+1);
        if (!has_statevars)
        {
            if (name=="Differentials") name = ds::Model(ds::STATE);
            else if (name=="Variables") name = ds::Model(ds::FUNC);
        }
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

    if (!has_statevars)
    {
        ParamModelBase* diffs = ParamModelBase::Create(ds::DIFF);
        auto it = std::find_if(models.cbegin(), models.cend(),
                               [](const ParamModelBase* model)
        {
            return model->Id()==ds::STATE;
        });
        const ParamModelBase* state_vars = *it;
        const size_t num_statevars = state_vars->NumPars();
        for (size_t i=0; i<num_statevars; ++i)
        {
            const std::string key = state_vars->ShortKey(i);
            diffs->AddParameter("_d" + key, ParamModelBase::Param::DEFAULT_VAL);
        }
        models.push_back(diffs);
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
    while (line.empty() || std::isspace(line.at(0))) std::getline(_in, line);
    size_t tab = line.find_first_of('\t');
    const int num_variants = std::stoi( line.substr(tab+1) );

    std::vector<ModelMgr::ParVariant*> par_variants(num_variants);
    for (int i=0; i<num_variants; ++i)
        par_variants[i] = ReadParVariant(_in);

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
