#include "sysfilein.h"

SysFileIn::SysFileIn(const std::string& name)
    : _name(name)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::SysFileIn", std::this_thread::get_id());
#endif
}

void SysFileIn::Load(std::vector<ParamModelBase*>& models,
                     std::string& model_step,
                     ConditionModel* conditions,
                     Notes* notes)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::Load", std::this_thread::get_id());
#endif
    _in.open(_name);
    std::string line;

    std::getline(_in, line);
    int version = ds::VersionNum(line);
    bool has_minmax = (version>=4), //I.e. >= 0.0.4
            has_model_step = (version>=103),
            old_par_name = (version<104);

    if (has_model_step)
    {
        std::getline(_in, line);
        size_t pos = line.find_last_of(':');
        model_step = line.substr(pos+1);
        model_step.erase( std::remove_if(
                        model_step.begin(), model_step.end(), ::isspace ),
                          model_step.end() );
    }
    else
        model_step = "1.0";

    std::getline(_in, line);
    const int num_models = std::stoi(line);

    for (int i=0; i<num_models; ++i)
    {
        std::getline(_in, line);
        size_t tab = line.find_first_of('\t');
        std::string name = line.substr(0,tab),
                num = line.substr(tab+1);
        const int num_pars = std::stoi(num);
        ParamModelBase* model;
        if (old_par_name && name=="Parameters") name = "Inputs";
        switch (ds::Model(name))
        {
            case ds::INPUTS:
                model = new ParamModel(nullptr, name);
                break;
            case ds::VARIABLES:
                model = new VariableModel(nullptr, name);
                break;
            case ds::DIFFERENTIALS:
                model = new DifferentialModel(nullptr, name);
                break;
            case ds::INIT_CONDS:
                model = new InitialCondModel(nullptr, name);
                break;
            default:
                throw std::runtime_error("SysFileIn::Load: Bad model name");
        }
            // ### Should create a proper factory
        models.push_back(model);

        for (int j=0; j<num_pars; ++j)
        {
            std::getline(_in, line);
            size_t tab = line.find_first_of('\t');
            std::string key = line.substr(0,tab),
                    value = line.substr(tab+1);
            std::string pmin, pmax;
            if (has_minmax)
            {
                size_t tab = value.find_first_of('\t');
                std::string rest = value.substr(tab+1);
                value = value.substr(0,tab);
                tab = rest.find_first_of('\t');
                pmin = rest.substr(0,tab);
                pmax = rest.substr(tab+1);
            }
            model->AddParameter(key, value);
            if (has_minmax)
            {
                model->SetMinimum(j, std::stod(pmin));
                model->SetMaximum(j, std::stod(pmax));
            }
        }

        std::getline(_in, line);
    }

    conditions->Read(_in);

    if (notes) notes->Read(_in);

    _in.close();
}
void SysFileIn::Load(VecStr& vmodels)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("SysFileIn::Load", std::this_thread::get_id());
#endif
    std::vector<ParamModelBase*> models;
    std::string model_step;
    ConditionModel* conditions = new ConditionModel();
    Notes* notes = new Notes();

    Load(models, model_step, conditions, notes);

    for (auto it : models)
    {
        std::string model_str = it->String();
        vmodels.push_back(model_str);
    }
    vmodels.push_back( conditions->EdString() );

    for (auto it : models)
        delete it;
    delete conditions;
    delete notes;
}
