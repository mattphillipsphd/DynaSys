#ifndef SYSFILEOUT_H
#define SYSFILEOUT_H

#include <string>
#include <fstream>

#include "../globals/scopetracker.h"
#include "../memrep/modelmgr.h"
#include "../memrep/notes.h"
#include "../models/parammodelbase.h"
#include "../models/conditionmodel.h"

class SysFileOut
{
    public:
        SysFileOut(const std::string& name);

        void Save() const;
        void Save(const VecStr& vmodels, const Notes* notes) const;

    private:
        void SaveHeader() const;

        const std::string _name;
        mutable std::ofstream _out;
};

#endif // SYSFILEOUT_H
