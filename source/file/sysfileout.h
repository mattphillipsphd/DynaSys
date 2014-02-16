#ifndef SYSFILEOUT_H
#define SYSFILEOUT_H

#include <string>
#include <fstream>

#include "../globals/globals.h"
#include "../models/parammodelbase.h"
#include "../models/conditionmodel.h"

class SysFileOut
{
    public:
        SysFileOut(const std::string& name,
                   const std::vector<const ParamModelBase*>& models, const ConditionModel* conditions);

        void Save();

    private:
        const ConditionModel* _conditions;
        const std::vector<const ParamModelBase*>& _models;
        const std::string _name;
        std::ofstream _out;
};

#endif // SYSFILEOUT_H
