#ifndef SYSFILEIN_H
#define SYSFILEIN_H

#include <string>
#include <fstream>

#include "conditionmodel.h"
#include "parammodel.h"

class SysFileIn
{
    public:
        SysFileIn(const std::string& name,
                  std::vector<ParamModel*>& models, ConditionModel* conditions);

        void Load();

    private:
        ConditionModel* _conditions;
        std::vector<ParamModel*>& _models;
        const std::string _name;
        std::ifstream _in;
};


#endif // SYSFILEIN_H
