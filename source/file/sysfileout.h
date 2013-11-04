#ifndef SYSFILEOUT_H
#define SYSFILEOUT_H

#include <string>
#include <fstream>

#include "../models/parammodel.h"
#include "../models/conditionmodel.h"

class SysFileOut
{
    public:
        SysFileOut(const std::string& name,
                   const std::vector<const ParamModel*>& models, const ConditionModel* conditions);

        void Save();

    private:
        const ConditionModel* _conditions;
        const std::vector<const ParamModel*>& _models;
        const std::string _name;
        std::ofstream _out;
};

#endif // SYSFILEOUT_H
