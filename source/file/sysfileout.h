#ifndef SYSFILEOUT_H
#define SYSFILEOUT_H

#include <string>
#include <fstream>

#include "../globals/scopetracker.h"
#include "../memrep/notes.h"
#include "../models/parammodelbase.h"
#include "../models/conditionmodel.h"

class SysFileOut
{
    public:
        SysFileOut(const std::string& name);

        void Save(const std::vector<const ParamModelBase*>& models,
                  double model_step,
                  const ConditionModel* conditions,
                  const Notes* notes) const;
        void Save(const VecStr& vmodels, double model_step, const Notes* notes) const;

    private:
        void SaveHeader(double model_step) const;

        const std::string _name;
        mutable std::ofstream _out;
};

#endif // SYSFILEOUT_H
