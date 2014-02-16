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
    std::getline(_in, line);

    const int num_models = std::stoi(line.c_str());
    for (int i=0; i<num_models; ++i)
    {
        std::getline(_in, line);
        int tab = line.find_first_of('\t');
        std::string name = line.substr(0,tab),
                num = line.substr(tab+1);
        const int num_pars = std::stoi(num.c_str());
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
            int tab = line.find_first_of('\t');
            std::string key = line.substr(0,tab),
                    value = line.substr(tab+1);
            model->AddParameter(key, value);
        }

        std::getline(_in, line);
    }

    _conditions->Read(_in);

    _in.close();
}
