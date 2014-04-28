#ifndef SYSFILEIN_H
#define SYSFILEIN_H

#include <string>
#include <fstream>

#include "../memrep/notes.h"
#include "../models/conditionmodel.h"
#include "../models/differentialmodel.h"
#include "../models/initialcondmodel.h"
#include "../models/parammodel.h"
#include "../models/variablemodel.h"

class SysFileIn
{
    public:
        SysFileIn(const std::string& name);

        void Load(std::vector<ParamModelBase*>& models,
                  std::string& model_step,
                  ConditionModel* conditions,
                  Notes* notes);
        void Load(VecStr& vmodels);

    private:
        const std::string _name;
        std::ifstream _in;
};


#endif // SYSFILEIN_H
