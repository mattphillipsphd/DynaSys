#include "sysfilein.h"

SysFileIn::SysFileIn(const std::string& name,
                     std::vector<ParamModelBase*>& models, ConditionModel* conditions)
    : _conditions(conditions), _models(models), _name(name)
{
}

void SysFileIn::Load()
{
    _in.open(_name);
    std::string line;

    std::getline(_in, line);
    int version = ds::VersionNum(line);
    bool has_minmax = (version>=4); //I.e. >= 0.0.4

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
        if (name=="Parameters")
            model = new ParamModel(nullptr, name);
        else if (name=="Variables")
            model = new VariableModel(nullptr, name);
        else if (name=="Differentials")
            model = new DifferentialModel(nullptr, name);
        else if (name=="InitialConds")
            model = new InitialCondModel(nullptr, name);
        _models.push_back(model);

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

    _conditions->Read(_in);

    _in.close();
}
